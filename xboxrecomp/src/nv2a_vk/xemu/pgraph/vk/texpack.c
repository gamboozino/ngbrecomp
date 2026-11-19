/*
 * texpack.c -- dumping a title's textures and replacing them (xboxrecomp).
 *
 *   NV2A_TEX_DUMP=dir      write each texture the title uploads, once, as
 *                          dir/<id>.png (the top level, as the shader sees
 *                          it: RGBA after the format's swizzle)
 *   NV2A_TEX_DUMP_RAW=1    with NV2A_TEX_DUMP, also the texture's bytes as
 *                          they are in guest memory (<id>.raw), to find
 *                          where a texture comes from in the title's files
 *   (and pgraph_vk_texpack_register: a replacement made by the program)
 *   NV2A_TEX_REPLACE=dir   where dir/<id>.png exists, use it instead of the
 *                          title's texture: any size with the same aspect
 *                          ratio, mipmapped on upload. dir/<id>.dds the same,
 *                          compressed (BC1/BC2/BC3, DXT1/3/5) or plain 32-bit,
 *                          its own mipmaps kept; over a .png of the same id.
 *                          Subfolders are read too (a pack sorted by area).
 *
 * <id> is 16 hex digits: a hash of the texture's bytes in guest memory (and
 * its palette), its format and size. A pack is a folder of such files, made
 * from a dump: edit or upscale the PNGs, keep the names. Nothing of the
 * title's need ship with the program.
 */
#include "renderer.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <xxhash.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO_WRITE
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static char s_dump_dir[1024], s_repl_dir[1024];

/* Replacements given at run time (pgraph_vk_texpack_register): the texture
 * at a guest address, while its bytes are still the ones given. */
typedef struct {
    uint64_t addr, hash, id;
    uint32_t len;
    int w, h;
    uint8_t *rgba;
} MemRepl;
static MemRepl s_mem[96];
static int s_mem_n;
static GMutex s_mem_lock;
#define MEM_ID_BASE 0xFE00000000000000ull
static int s_init;
static GHashTable *s_have;      /* ids with a replacement file: its path (in s_repl_dir) */
static GHashTable *s_dumped;    /* ids written or queued this run */
static GAsyncQueue *s_queue;    /* dumps for the writer thread */

typedef struct {
    uint64_t id;
    int w, h;
    uint8_t *rgba;
} DumpJob;

static void *dump_writer(void *arg)
{
    (void)arg;
    for (;;) {
        DumpJob *j = g_async_queue_pop(s_queue);
        char path[1100];
        snprintf(path, sizeof path, "%s/%016" PRIx64 ".png", s_dump_dir, j->id);
        stbi_write_png(path, j->w, j->h, 4, j->rgba, j->w * 4);
        g_free(j->rgba);
        g_free(j);
    }
    return NULL;
}

static guint id_hash(gconstpointer k)
{
    uint64_t v = *(const uint64_t *)k;
    return (guint)(v ^ (v >> 32));
}

static gboolean id_equal(gconstpointer a, gconstpointer b)
{
    return *(const uint64_t *)a == *(const uint64_t *)b;
}

static int ext_is(const char *s, const char *ext)   /* (case aside) */
{
    for (; *s && *ext; s++, ext++)
        if ((*s | 0x20) != (*ext | 0x20))
            return 0;
    return !*s && !*ext;
}

static int is_dds(const char *path)
{
    size_t n = path ? strlen(path) : 0;
    return n > 4 && ext_is(path + n - 4, ".dds");
}

/* dir (and its subfolders, a few deep): every <id>.png and <id>.dds, a .dds
 * over a .png of the same id */
static void scan_dir(const char *path, int depth, int *n, int *dds)
{
    DIR *dir = opendir(path);
    struct dirent *de;
    while (dir && (de = readdir(dir))) {
        uint64_t id;
        char tail[8];
        char full[1400];
        if (de->d_name[0] == '.')
            continue;
        snprintf(full, sizeof full, "%s/%s", path, de->d_name);
        if (sscanf(de->d_name, "%16" SCNx64 "%7s", &id, tail) == 2 && strlen(de->d_name) == 20 &&
            (ext_is(tail, ".png") || ext_is(tail, ".dds"))) {
            int dds_file = ext_is(tail, ".dds");
            const char *had = g_hash_table_lookup(s_have, &id);
            if (had && !dds_file)
                continue;
            if (!had)
                (*n)++;
            if (dds_file && !is_dds(had))   /* (one counted once) */
                (*dds)++;
            uint64_t *k = g_new(uint64_t, 1);
            *k = id;
            g_hash_table_replace(s_have, k, g_strdup(full));
            continue;
        }
        struct stat st;
        if (depth < 4 && stat(full, &st) == 0 && S_ISDIR(st.st_mode))
            scan_dir(full, depth + 1, n, dds);
    }
    if (dir)
        closedir(dir);
}

static void texpack_init(void)
{
    const char *e;

    if (s_init)
        return;
    s_init = 1;
    s_have = g_hash_table_new_full(id_hash, id_equal, g_free, g_free);
    s_dumped = g_hash_table_new_full(id_hash, id_equal, g_free, NULL);

    if ((e = getenv("NV2A_TEX_REPLACE")) && *e) {
        int n = 0, dds = 0;
        snprintf(s_repl_dir, sizeof s_repl_dir, "%s", e);
        scan_dir(e, 0, &n, &dds);
        fprintf(stderr, "nv2a: %d replacement textures in %s (%d DDS)\n", n, e, dds);
    }
    if ((e = getenv("NV2A_TEX_DUMP")) && *e) {
        snprintf(s_dump_dir, sizeof s_dump_dir, "%s", e);
        g_mkdir_with_parents(s_dump_dir, 0755);
        s_queue = g_async_queue_new();
        g_thread_new("texdump", dump_writer, NULL);
        fprintf(stderr, "nv2a: dumping textures to %s\n", e);
    }
}

static pgraph_vk_texgen_fn s_gen;

/* A program's own maker of replacements: shown every texture the title
 * uses, as its contents change (the format is the NV2A's texture format
 * number, the palette that of an indexed one); it registers any it makes
 * with pgraph_vk_texpack_register. */
void pgraph_vk_texpack_set_generator(pgraph_vk_texgen_fn fn)
{
    __atomic_store_n(&s_gen, fn, __ATOMIC_RELEASE);
}

void pgraph_vk_texpack_generate(uint32_t addr, const void *bytes, uint32_t len,
                                unsigned format, int w, int h, const void *palette)
{
    pgraph_vk_texgen_fn fn = __atomic_load_n(&s_gen, __ATOMIC_ACQUIRE);
    if (fn)
        fn(addr, bytes, len, format, w, h, palette);
}

bool pgraph_vk_texpack_active(void)
{
    texpack_init();
    return s_repl_dir[0] || s_dump_dir[0] || __atomic_load_n(&s_mem_n, __ATOMIC_ACQUIRE) ||
           __atomic_load_n(&s_gen, __ATOMIC_ACQUIRE);
}

void memory_region_set_client_dirty(MemoryRegion *mr, hwaddr addr, hwaddr size,
                                    unsigned client);

/* Any thread: from now on the texture whose bytes are at guest physical
 * addr (len bytes, as they are now) is drawn from rgba (w x h, RGBA rows,
 * the same aspect ratio), until those bytes change. Again for the same
 * address replaces it. */
void pgraph_vk_texpack_register(uint32_t addr, const void *bytes, uint32_t len,
                                const uint8_t *rgba, int w, int h)
{
    uint64_t hash = XXH3_64bits(bytes, len);
    texpack_init();
    g_mutex_lock(&s_mem_lock);
    int i = 0;
    while (i < s_mem_n && s_mem[i].addr != addr)
        i++;
    if (i == (int)(sizeof s_mem / sizeof s_mem[0])) {   /* full: the oldest goes */
        static int next;
        i = next++ % (int)(sizeof s_mem / sizeof s_mem[0]);
    }
    {
        MemRepl *m = &s_mem[i];
        uint8_t *copy = g_malloc((size_t)w * h * 4);
        memcpy(copy, rgba, (size_t)w * h * 4);
        g_free(m->rgba);
        m->addr = addr;
        m->len = len;
        m->hash = hash;
        m->w = w;
        m->h = h;
        m->rgba = copy;
        m->id = MEM_ID_BASE | ((uint64_t)i << 48) | (hash & 0xFFFFFFFFFFFFull);
        if (i == s_mem_n)
            __atomic_store_n(&s_mem_n, i + 1, __ATOMIC_RELEASE);
    }
    g_mutex_unlock(&s_mem_lock);
    /* Seen again at its next use, even where page tracking says clean. */
    memory_region_set_client_dirty(NULL, addr, len, DIRTY_MEMORY_NV2A_TEX);
}

/* The replacement for the texture at addr with these bytes, or 0. */
uint64_t pgraph_vk_texpack_mem_id(uint64_t addr, const void *bytes, uint32_t len)
{
    uint64_t id = 0;
    int n = __atomic_load_n(&s_mem_n, __ATOMIC_ACQUIRE);
    for (int i = 0; i < n && !id; i++) {
        if (s_mem[i].addr != addr || s_mem[i].len != len)
            continue;
        g_mutex_lock(&s_mem_lock);
        if (XXH3_64bits(bytes, len) == s_mem[i].hash)
            id = s_mem[i].id;
        g_mutex_unlock(&s_mem_lock);
    }
    return id;
}

static MemRepl *mem_find(uint64_t id)
{
    if ((id & 0xFF00000000000000ull) != MEM_ID_BASE)
        return NULL;
    int i = (int)(id >> 48 & 0xFF);
    return i < __atomic_load_n(&s_mem_n, __ATOMIC_ACQUIRE) && s_mem[i].id == id
           ? &s_mem[i] : NULL;
}

uint64_t pgraph_vk_texpack_id(uint64_t content_hash, const TextureShape *s)
{
    uint64_t v[4] = { content_hash, s->color_format, s->width, s->height };
    return XXH3_64bits(v, sizeof v);
}

bool pgraph_vk_texpack_has(uint64_t id)
{
    texpack_init();
    if (mem_find(id))
        return true;
    return s_repl_dir[0] && g_hash_table_contains(s_have, &id);
}

uint8_t *pgraph_vk_texpack_load(uint64_t id, int *w, int *h)
{
    g_mutex_lock(&s_mem_lock);
    MemRepl *m = mem_find(id);
    if (m) {
        uint8_t *px = malloc((size_t)m->w * m->h * 4);
        memcpy(px, m->rgba, (size_t)m->w * m->h * 4);
        *w = m->w;
        *h = m->h;
        g_mutex_unlock(&s_mem_lock);
        return px;
    }
    g_mutex_unlock(&s_mem_lock);
    const char *path = s_repl_dir[0] ? g_hash_table_lookup(s_have, &id) : NULL;
    int n;
    if (!path || is_dds(path))
        return NULL;   /* (a .dds: pgraph_vk_texpack_read) */
    uint8_t *px = stbi_load(path, w, h, &n, 4);
    if (!px)
        fprintf(stderr, "nv2a: cannot read %s: %s\n", path, stbi_failure_reason());
    return px;
}

/* A replacement made at run time (pgraph_vk_texpack_register): in memory already */
bool pgraph_vk_texpack_is_mem(uint64_t id)
{
    return mem_find(id) != NULL;
}

/* A replacement's kind: 1 a picture (.png, or a run-time one: pgraph_vk_texpack_load),
 * 2 a DDS (pgraph_vk_texpack_read), 0 none. The table is only written at start: any thread. */
int pgraph_vk_texpack_kind(uint64_t id)
{
    texpack_init();
    if (mem_find(id))
        return 1;
    const char *path = s_repl_dir[0] ? g_hash_table_lookup(s_have, &id) : NULL;
    if (!path)
        return 0;
    return is_dds(path) ? 2 : 1;
}

/* A replacement file's bytes as they are (a .dds), g_free'd by the caller; NULL if unreadable. */
uint8_t *pgraph_vk_texpack_read(uint64_t id, size_t *len)
{
    const char *path = s_repl_dir[0] ? g_hash_table_lookup(s_have, &id) : NULL;
    gchar *data = NULL;
    gsize n = 0;
    if (!path || !g_file_get_contents(path, &data, &n, NULL)) {
        fprintf(stderr, "nv2a: cannot read the replacement %016" PRIx64 "\n", id);
        return NULL;
    }
    *len = n;
    return (uint8_t *)data;
}

void pgraph_vk_texpack_free(uint8_t *px)
{
    stbi_image_free(px);
}

/* One component of a texel through the view's swizzle. */
static uint8_t swz(VkComponentSwizzle s, int self, const uint8_t c[4])
{
    switch (s) {
    case VK_COMPONENT_SWIZZLE_ZERO: return 0;
    case VK_COMPONENT_SWIZZLE_ONE:  return 255;
    case VK_COMPONENT_SWIZZLE_R:    return c[0];
    case VK_COMPONENT_SWIZZLE_G:    return c[1];
    case VK_COMPONENT_SWIZZLE_B:    return c[2];
    case VK_COMPONENT_SWIZZLE_A:    return c[3];
    default:                        return c[self];
    }
}

static inline uint8_t x5(unsigned v) { return (uint8_t)((v << 3) | (v >> 2)); }
static inline uint8_t x6(unsigned v) { return (uint8_t)((v << 2) | (v >> 4)); }
static inline uint8_t x4(unsigned v) { return (uint8_t)(v * 17); }

void pgraph_vk_texpack_dump(uint64_t id, const void *data, int w, int h,
                            VkFormat fmt, VkComponentMapping map)
{
    texpack_init();
    if (!s_dump_dir[0] || w <= 0 || h <= 0 || g_hash_table_contains(s_dumped, &id))
        return;

    const uint8_t *p = data;
    uint8_t *out = g_malloc((size_t)w * h * 4);
    for (int i = 0; i < w * h; i++) {
        uint8_t c[4] = { 0, 0, 0, 255 };
        switch (fmt) {
        case VK_FORMAT_R8G8B8A8_UNORM:
            memcpy(c, p + i * 4, 4);
            break;
        case VK_FORMAT_B8G8R8A8_UNORM:
            c[0] = p[i * 4 + 2]; c[1] = p[i * 4 + 1]; c[2] = p[i * 4]; c[3] = p[i * 4 + 3];
            break;
        case VK_FORMAT_R5G6B5_UNORM_PACK16: {
            unsigned v = ((const uint16_t *)p)[i];
            c[0] = x5(v >> 11); c[1] = x6((v >> 5) & 63); c[2] = x5(v & 31);
            break;
        }
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16: {
            unsigned v = ((const uint16_t *)p)[i];
            c[0] = x5((v >> 10) & 31); c[1] = x5((v >> 5) & 31); c[2] = x5(v & 31);
            c[3] = (v & 0x8000) ? 255 : 0;
            break;
        }
        case VK_FORMAT_A4R4G4B4_UNORM_PACK16: {
            unsigned v = ((const uint16_t *)p)[i];
            c[0] = x4((v >> 8) & 15); c[1] = x4((v >> 4) & 15); c[2] = x4(v & 15);
            c[3] = x4(v >> 12);
            break;
        }
        case VK_FORMAT_R8_UNORM:
            c[0] = p[i];
            break;
        case VK_FORMAT_R8G8_UNORM:
            c[0] = p[i * 2]; c[1] = p[i * 2 + 1];
            break;
        default:
            g_free(out);
            return;
        }
        out[i * 4 + 0] = swz(map.r, 0, c);
        out[i * 4 + 1] = swz(map.g, 1, c);
        out[i * 4 + 2] = swz(map.b, 2, c);
        out[i * 4 + 3] = swz(map.a, 3, c);
    }

    uint64_t *k = g_new(uint64_t, 1);
    *k = id;
    g_hash_table_add(s_dumped, k);
    DumpJob *j = g_new(DumpJob, 1);
    j->id = id;
    j->w = w;
    j->h = h;
    j->rgba = out;
    g_async_queue_push(s_queue, j);
}

/* Formats a dump can be written from (as decoded for upload). */
bool pgraph_vk_texpack_format_ok(VkFormat fmt)
{
    switch (fmt) {
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_R5G6B5_UNORM_PACK16:
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
    case VK_FORMAT_A4R4G4B4_UNORM_PACK16:
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8G8_UNORM:
        return true;
    default:
        return false;
    }
}

void pgraph_vk_texpack_dump_raw(uint64_t id, const void *data, size_t len)
{
    static int on = -1;
    static GHashTable *done;
    char path[1100];
    FILE *f;

    if (on < 0) {
        texpack_init();
        on = s_dump_dir[0] && getenv("NV2A_TEX_DUMP_RAW") != NULL;
        done = g_hash_table_new_full(id_hash, id_equal, g_free, NULL);
    }
    if (!on || g_hash_table_contains(done, &id))
        return;
    uint64_t *k = g_new(uint64_t, 1);
    *k = id;
    g_hash_table_add(done, k);
    snprintf(path, sizeof path, "%s/%016" PRIx64 ".raw", s_dump_dir, id);
    if ((f = fopen(path, "wb"))) {
        fwrite(data, 1, len, f);
        fclose(f);
    }
}

void pgraph_vk_texpack_free_read(uint8_t *p)
{
    g_free(p);
}
