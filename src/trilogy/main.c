/*
 * NinjaTrilogy -- the NES Ninja Gaiden that the arcade cabinet in Han's Bar and the main menu's
 * ARCADE NINJA GAIDEN run (#18). On the Xbox that is a second program on the disc
 * (NinjaTrilogy.xbe): the game hands over to it with XLaunchNewImage, the console reboots into it,
 * and it hands back to default.xbe the same way, passing launch data both ways. Here it is
 * recompiled on its own (tools/pipeline_trilogy.sh) and this is its host: the boot of src/main.c
 * without what belongs to Ninja Gaiden Black (its Direct3D renderer, menus, settings). It draws
 * with the GPU model's own NV2A renderer: its Direct3D runs as it was built.
 *
 * Started by the game (src/launch.c) with NGB_GAME_DIR, NGB_SAVE_DIR and the launch data page in
 * RECOMP_LAUNCH_PAGE; or by hand: NinjaTrilogy <game dir> [<save dir>].
 */
#if defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#include <unistd.h>
#include <pthread.h>
#include "platform/vcpu.h"
#else
#include <limits.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include "platform/win32_compat.h"
#include "platform/vcpu.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <xbox/xboxrecomp.h>
#include "usb/ohci.h"
#include "usb/usb_gamepad.h"
#include "nv2a_vk_runtime.h"
#include "nv2a_window.h"
#include "ngb_launch.h"

#ifndef TRILOGY_XBE   /* NinjaTomoe.xbe for the Japanese build (CMakeLists.txt) */
#define TRILOGY_XBE "NinjaTrilogy.xbe"
#endif

/* RECOMP_TLS: the runtime's guest registers are thread-local (as in src/main.c) */
extern ptrdiff_t g_xbox_mem_offset;
extern void xbox_InputInit(void);
extern void xbe_entry_point(void);
extern int recomp_dispatch_init(void);
extern void xbox_kernel_init(void);
extern void xbox_kernel_bridge_init(void);
typedef struct MCPXAPUState MCPXAPUState;
extern MCPXAPUState *mcpx_apu_init_standalone(uint8_t *ram_ptr);
int dsound_host_mixer(void);              /* src/audio/dsound.c */
void dsound_hle_set_output(int choice);   /* src/audio/dsound_hle.cpp */
extern MCPXAPUState *g_apu_state;
extern bool apu_hook_handle_mmio(PCONTEXT ctx, uintptr_t fault_addr, uint32_t fault_xbox_va, int is_write);
#define APU_TRAP_BASE 0xFE800000u
#define APU_TRAP_END  0xFE880000u

static char s_game_dir[4096], s_save_dir[4096];

static void set_default(const char *name, const char *value)
{
    if (getenv(name))
        return;
#if defined(_WIN32)
    _putenv_s(name, value);
#else
    setenv(name, value, 0);
#endif
}

static bool load_file(const char *path, void **out_data, size_t *out_size)
{
    FILE *f = fopen(path, "rb");
    long size;
    void *data;

    if (!f)
        return false;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0 || !(data = malloc((size_t)size))) {
        fclose(f);
        return false;
    }
    if (fread(data, 1, (size_t)size, f) != (size_t)size) {
        free(data);
        fclose(f);
        return false;
    }
    fclose(f);
    *out_data = data;
    *out_size = (size_t)size;
    return true;
}

/* The sound chip's and the controller ports' registers trap here to be serviced (as in src/main.c);
 * any other access violation is reported. */
static LONG CALLBACK veh_handler(PEXCEPTION_POINTERS ep)
{
#if defined(_WIN32) && XBOX_HAS_VCPU
    vcpu_win_unredirect(ep);
#endif
    if (ep->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;
    uintptr_t fault_addr = ep->ExceptionRecord->ExceptionInformation[1];
    int is_write = ep->ExceptionRecord->ExceptionInformation[0] ? 1 : 0;
    uint32_t xbox_va = (uint32_t)(fault_addr - (uintptr_t)g_xbox_mem_offset);
    if (g_apu_state && xbox_va >= APU_TRAP_BASE && xbox_va < APU_TRAP_END &&
        apu_hook_handle_mmio(ep->ContextRecord, fault_addr, xbox_va, is_write))
        return EXCEPTION_CONTINUE_EXECUTION;
    if (xbox_OhciOwnsAddress(xbox_va) && xbox_OhciHandleMmio(ep->ContextRecord, xbox_va))
        return EXCEPTION_CONTINUE_EXECUTION;
    fprintf(stderr, "[CRASH] access violation at RIP=0x%llX, address 0x%llX (%s), guest VA 0x%08X\n"
                    "  guest eax=%08X ecx=%08X edx=%08X ebx=%08X esi=%08X edi=%08X esp=%08X\n",
            (unsigned long long)ep->ContextRecord->Rip, (unsigned long long)fault_addr, is_write ? "write" : "read",
            xbox_va, g_eax, g_ecx, g_edx, g_ebx, g_esi, g_edi, g_esp);
    fflush(stderr);
    return EXCEPTION_CONTINUE_SEARCH;
}

/* NGB_TRILOGY_TABLE_DUMP=<file>: 3 s in, the CPU core's opcode table (0x3210A0, 65536 handlers, filled at
 * run time) as its distinct targets, one a line -- seeds for tools/pipeline_trilogy.sh. */
static void *table_dump(void *path)
{
    sleep(3);
    static uint32_t seen[8192];
    int n = 0;
    for (uint32_t i = 0; i < 65536; i++) {
        uint32_t t = *(uint32_t *)((uintptr_t)g_xbox_mem_offset + 0x3210A0u + i * 4);
        int k = 0;
        while (k < n && seen[k] != t) k++;
        if (k == n && n < 8192) seen[n++] = t;
    }
    FILE *f = fopen((const char *)path, "w");
    for (int k = 0; f && k < n; k++) fprintf(f, "0x%08X\n", seen[k]);
    if (f) fclose(f);
    fprintf(stderr, "[TABLE] %d distinct handlers written to %s\n", n, (const char *)path);
    return NULL;
}

static void *guest_main(void *unused)
{
    char xbe_path[4200];
    void *xbe_data = NULL;
    size_t xbe_size = 0;

    (void)unused;
#if XBOX_HAS_VCPU
    vcpu_attach(VCPU_PRIO_NORMAL);
#endif
    snprintf(xbe_path, sizeof xbe_path, "%s/" TRILOGY_XBE, s_game_dir);
    if (!load_file(xbe_path, &xbe_data, &xbe_size)) {
        fprintf(stderr, "Cannot read %s\n", xbe_path);
        exit(1);
    }
    printf("XBE loaded: %zu bytes\n", xbe_size);
    if (!xbox_MemoryLayoutInit(xbe_data, xbe_size)) {
        fprintf(stderr, "Failed to initialize the Xbox memory layout.\n");
        exit(1);
    }
    g_xbox_mem_offset = xbox_GetMemoryOffset();

    Nv2aVkOptions o = { .headless = getenv("NGB_RENDERER") && !strcmp(getenv("NGB_RENDERER"), "null") };
    if (xbox_Nv2aVkStart(&o) != 0) {
        fprintf(stderr, "Cannot start the GPU.\n");
        exit(1);
    }
    if (getenv("RECOMP_AC97_READY") && !dsound_host_mixer()) {
        g_apu_state = mcpx_apu_init_standalone((uint8_t *)xbox_GetMemoryBase());
        fprintf(stderr, "[BOOT] emulated APU %s\n", g_apu_state ? "up" : "FAILED to initialise");
    }
    xbox_kernel_init();
    {
        extern void xbox_path_init(const char *game_dir, const char *save_dir);
        printf("Save directory: %s\n", s_save_dir);
        xbox_path_init(s_game_dir, s_save_dir);
    }
    xbox_kernel_bridge_init();
    xbox_OhciInit();
    g_esp = XBOX_STACK_TOP;
    if (!recomp_dispatch_init())
        fprintf(stderr, "[BOOT] flat dispatch unavailable\n");
    if (getenv("NGB_TRILOGY_TABLE_DUMP")) {
        pthread_t th;
        if (pthread_create(&th, NULL, table_dump, (void *)getenv("NGB_TRILOGY_TABLE_DUMP")) == 0)
            pthread_detach(th);
    }
    {   /* Inert unless RECOMP_WATCHDOG_SECS is set; must run on the guest thread (as src/main.c). */
        extern void xbox_WatchdogStart(void);
        xbox_WatchdogStart();
    }
    printf("Starting " TRILOGY_XBE "...\n");
    xbe_entry_point();
    printf("Program returned.\n");
    exit(0);
    return NULL;
}

#if defined(_WIN32)
#include <tlhelp32.h>
#include <psapi.h>
/* NGB_HANG_DUMP=<seconds> (debugging, Windows): each thread's call stack into the log, as in src/main.c. */
static DWORD WINAPI hang_dump_thread(LPVOID arg)
{
    static HMODULE mods[256];
    static char names[256][64];
    static char out[1 << 16];
    MODULEINFO mi[256];
    DWORD need = 0, pid = GetCurrentProcessId(), me = GetCurrentThreadId();
    int nmods, len = 0;
    HANDLE snap;
    THREADENTRY32 te;

    Sleep((DWORD)(uintptr_t)arg * 1000);
    for (int round = 0; round < 3; round++, Sleep(3000)) {
    len = 0;
    K32EnumProcessModules(GetCurrentProcess(), mods, sizeof mods, &need);
    nmods = (int)(need / sizeof(HMODULE));
    if (nmods > 256) nmods = 256;
    for (int i = 0; i < nmods; i++) {
        char path[MAX_PATH], *b;
        K32GetModuleInformation(GetCurrentProcess(), mods[i], &mi[i], sizeof mi[i]);
        K32GetModuleFileNameExA(GetCurrentProcess(), mods[i], path, sizeof path);
        b = strrchr(path, '\\');
        snprintf(names[i], sizeof names[i], "%s", b ? b + 1 : path);
    }
    snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    te.dwSize = sizeof te;
    for (BOOL ok = Thread32First(snap, &te); ok && len < (int)sizeof out - 4096; ok = Thread32Next(snap, &te)) {
        HANDLE h;
        CONTEXT c;
        if (te.th32OwnerProcessID != pid || te.th32ThreadID == me)
            continue;
        h = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE,
                       te.th32ThreadID);
        if (!h)
            continue;
        (SuspendThread)(h);
        memset(&c, 0, sizeof c);
        c.ContextFlags = CONTEXT_FULL;
        if ((GetThreadContext)(h, &c)) {
            len += snprintf(out + len, sizeof out - len, "[HANG] thread %lu:", te.th32ThreadID);
            for (int f = 0; f < 20 && c.Rip; f++) {
                DWORD64 base = 0;
                PRUNTIME_FUNCTION fn = RtlLookupFunctionEntry(c.Rip, &base, NULL);
                int m = -1;
                for (int i = 0; i < nmods; i++)
                    if (c.Rip >= (DWORD64)mi[i].lpBaseOfDll &&
                        c.Rip < (DWORD64)mi[i].lpBaseOfDll + mi[i].SizeOfImage)
                        m = i;
                if (m >= 0)
                    len += snprintf(out + len, sizeof out - len, " %s+%llx", names[m],
                                    (unsigned long long)(c.Rip - (DWORD64)mi[m].lpBaseOfDll));
                else
                    len += snprintf(out + len, sizeof out - len, " %llx", (unsigned long long)c.Rip);
                if (fn) {
                    PVOID hd;
                    DWORD64 ef;
                    RtlVirtualUnwind(UNW_FLAG_NHANDLER, base, c.Rip, fn, &c, &hd, &ef, NULL);
                } else {
                    c.Rip = *(DWORD64 *)c.Rsp;
                    c.Rsp += 8;
                }
            }
            len += snprintf(out + len, sizeof out - len, "\n");
        }
        (ResumeThread)(h);
        CloseHandle(h);
    }
    CloseHandle(snap);
    {
        extern void nv2a_debug_state(char *buf, int n);
        extern void xbox_kernel_debug_counts(char *buf, int n, uint32_t watch_va);
        char gpu[512];
        nv2a_debug_state(gpu, sizeof gpu);
        len += snprintf(out + len, sizeof out - len, "%s\n", gpu);
        xbox_kernel_debug_counts(gpu, sizeof gpu, 0x3D83C8);   /* D3D's device pointer */
        len += snprintf(out + len, sizeof out - len, "%s\n", gpu);
        vcpu_debug_state(out + len, (int)sizeof out - len);   /* who holds the CPU, who waits */
        len += (int)strlen(out + len);
        /* NGB_HANG_MEM=va:bytes[,va:bytes...] (hex): guest memory, as words */
        if (getenv("NGB_HANG_MEM")) {
            char spec[512], *tok, *save = NULL;
            snprintf(spec, sizeof spec, "%s", getenv("NGB_HANG_MEM"));
            for (tok = strtok_r(spec, ",", &save); tok; tok = strtok_r(NULL, ",", &save)) {
                uint32_t va = (uint32_t)strtoul(tok, NULL, 16), n = 64;
                char *c = strchr(tok, ':');
                if (c) n = (uint32_t)strtoul(c + 1, NULL, 16);
                for (uint32_t i = 0; i < n && len < (int)sizeof out - 256; i += 32) {
                    len += snprintf(out + len, sizeof out - len, "[MEM] %08X:", va + i);
                    for (uint32_t j = i; j < i + 32 && j < n; j += 4)
                        len += snprintf(out + len, sizeof out - len, " %08X",
                                        *(uint32_t *)((uint8_t *)g_xbox_mem_offset + va + j));
                    len += snprintf(out + len, sizeof out - len, "\n");
                }
            }
        }
    }
    fprintf(stderr, "[HANG] sample %d\n", round + 1);
    fputs(out, stderr);
    fflush(stderr);
    }
    return 0;
}
#endif

int main(int argc, char **argv)
{
    const char *g = getenv("NGB_GAME_DIR"), *s = getenv("NGB_SAVE_DIR");
    snprintf(s_game_dir, sizeof s_game_dir, "%s", argc > 1 ? argv[1] : g ? g : "game");
    if (argc > 2 || s)
        snprintf(s_save_dir, sizeof s_save_dir, "%s", argc > 2 ? argv[2] : s);
    else
        ngb_default_save_dir(s_save_dir, sizeof s_save_dir);
    ngb_launch_log_to_file(s_save_dir, "log-trilogy.txt");
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
#if defined(_WIN32)
    timeBeginPeriod(1);
    if (getenv("NGB_HANG_DUMP"))
        CloseHandle(CreateThread(NULL, 0, hang_dump_thread, (LPVOID)(uintptr_t)atoi(getenv("NGB_HANG_DUMP")), 0, NULL));
#endif
    printf("=== Ninja Gaiden (NES) from Ninja Gaiden Black - Static Recompilation ===\n");
    printf("Game directory: %s\n", s_game_dir);
    AddVectoredExceptionHandler(1, veh_handler);

    /* Inherited from Ninja Gaiden Black, which draws with its own Direct3D renderer and has the GPU
     * model only run the pushbuffer: here the GPU model draws. */
#if defined(_WIN32)
    _putenv_s("NV2A_NO_DRAWS", "");
    _putenv_s("NV2A_NO_PREWARM", "");
#else
    unsetenv("NV2A_NO_DRAWS");
    unsetenv("NV2A_NO_PREWARM");
#endif
    if (getenv("NGB_RENDERER") && strcmp(getenv("NGB_RENDERER"), "null"))
#if defined(_WIN32)
        _putenv_s("NGB_RENDERER", "");
#else
        unsetenv("NGB_RENDERER");
#endif
    /* DirectSound on the host mixer (src/audio, as the game's; NGB_AUDIO_ENGINE=apu the emulated sound chip):
     * the chip's registers stay plain memory. Audio Output comes from the game (NGB_AUDIO_OUTPUT). */
    if (!dsound_host_mixer())
        set_default("RECOMP_AC97_READY", "1");
    if (getenv("NGB_AUDIO_OUTPUT"))
        dsound_hle_set_output(atoi(getenv("NGB_AUDIO_OUTPUT")));
    set_default("RECOMP_USB", "1");
    set_default("NV2A_TEX_FILTER", "original");
    ngb_launch_init(s_game_dir, s_save_dir, ngb_launch_window_w(), ngb_launch_window_h(), ngb_launch_fullscreen());

    int headless = getenv("NGB_RENDERER") && !strcmp(getenv("NGB_RENDERER"), "null");
    if (!headless && nv2a_window_create("Ninja Gaiden", ngb_launch_window_w(), ngb_launch_window_h(),
                                        ngb_launch_fullscreen(), 4.0 / 3.0, 1) != 0) {
        fprintf(stderr, "Cannot open the window.\n");
        return 1;
    }
    if (!headless)
        xbox_InputInit();

#if defined(_WIN32)
    CloseHandle(CreateThread(NULL, 64u << 20, (LPTHREAD_START_ROUTINE)guest_main, NULL, 0, NULL));
#else
    {
        pthread_attr_t attr;
        pthread_t th;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 64u << 20);
        if (pthread_create(&th, &attr, guest_main, NULL) != 0) {
            fprintf(stderr, "Cannot start the guest thread.\n");
            return 1;
        }
        pthread_attr_destroy(&attr);
    }
#endif
    if (!headless) {
        while (nv2a_window_pump())
            usleep(2000);
        printf("Window closed.\n");
        _exit(0);
    }
    for (;;)
#if defined(_WIN32)
        Sleep(INFINITE);
#else
        pause();
#endif
}

#ifdef NGB_TRILOGY_ABI_CHECK
/* -DNGB_TRILOGY_ABI_CHECK=ON: a call that returned with ebx, esi or edi changed or its stack not unwound
 * (the generated code's RECOMP_ABI_CALL), once per callee: where a recompiled function breaks its caller. */
void recomp_abi_violation_log(uint32_t va, uint32_t ebx0, uint32_t esi0, uint32_t edi0, uint32_t esp0)
{
    static uint32_t seen[64];
    static int count;
    for (int i = 0; i < count; i++)
        if (seen[i] == va)
            return;
    if (count == 64)
        return;
    seen[count++] = va;
    fprintf(stderr, "[ABI] sub_%08X:%s%s%s%s  ebx %08X->%08X esi %08X->%08X edi %08X->%08X esp %08X->%08X\n", va,
            g_ebx != ebx0 ? " ebx" : "", g_esi != esi0 ? " esi" : "", g_edi != edi0 ? " edi" : "",
            g_esp != esp0 + 4 ? " esp" : "", ebx0, g_ebx, esi0, g_esi, edi0, g_edi, esp0, g_esp);
}
#endif
