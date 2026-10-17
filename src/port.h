/*
 * port.h -- the few host calls that differ between hosts, for the hand-written code.
 *
 * ngb_mkdir: make a directory (0 on success). ngb_exe_path: the running program's path, with '/'
 * separators (its length, or -1). ngb_home: the user's home ($HOME; on Windows %USERPROFILE%, else $HOME).
 * ngb_mem_write/ngb_mem_read: this process's memory, whatever its protection; 0 when the address is
 * not mapped (the game's read-only constants, before its image is). On Windows also setenv and
 * unsetenv, which its C runtime does not have.
 */
#ifndef NGB_PORT_H
#define NGB_PORT_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <unistd.h>   /* (mingw's: usleep, access) */
#include <windows.h>

static inline int ngb_mkdir(const char *path) { return _mkdir(path); }

static inline long ngb_exe_path(char *out, size_t n)
{
    DWORD len = GetModuleFileNameA(NULL, out, (DWORD)n);
    if (!len || len >= n)
        return -1;
    for (DWORD i = 0; i < len; i++)
        if (out[i] == '\\')
            out[i] = '/';
    return (long)len;
}

static inline const char *ngb_home(void)
{
    /* %USERPROFILE% first: a HOME there can be a Unix path (Proton passes Linux's through) */
    const char *h = getenv("USERPROFILE");
    return h && *h ? h : getenv("HOME");
}

static inline int ngb_mem_write(void *addr, const void *src, size_t len)
{
    SIZE_T done = 0;
    return WriteProcessMemory(GetCurrentProcess(), addr, src, len, &done) && done == len;
}

static inline int ngb_mem_read(const void *addr, void *dst, size_t len)
{
    SIZE_T done = 0;
    return ReadProcessMemory(GetCurrentProcess(), addr, dst, len, &done) && done == len;
}

#ifndef __cplusplus
static inline int setenv(const char *name, const char *value, int overwrite)
{
    if (!overwrite && getenv(name))
        return 0;
    return _putenv_s(name, value);
}

static inline int unsetenv(const char *name) { return _putenv_s(name, ""); }

#endif

#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach-o/dyld.h>
#include <sys/stat.h>
#include <unistd.h>
static inline int ngb_mkdir(const char *path) { return mkdir(path, 0755); }
static inline long ngb_exe_path(char *out, size_t n)
{
    uint32_t size = (uint32_t)n;
    return _NSGetExecutablePath(out, &size) == 0 ? (long)strlen(out) : -1;
}
static inline const char *ngb_home(void) { return getenv("HOME"); }

static inline void ngb_macos_dir(char *out, size_t n, const char *name, int make)
{
    const char *home = ngb_home();
    snprintf(out, n, "%s/Library/Application Support/NinjaGaidenBlack", home ? home : ".");
    if (make)
        ngb_mkdir(out);
    size_t len = strlen(out);
    snprintf(out + len, n - len, "/%s", name);
    if (make)
        ngb_mkdir(out);
}

static inline int ngb_mem_read(const void *addr, void *dst, size_t len)
{
    mach_vm_size_t done = 0;
    return mach_vm_read_overwrite(mach_task_self(), (mach_vm_address_t)addr, len,
                                  (mach_vm_address_t)dst, &done) == KERN_SUCCESS
        && done == len;
}
static inline int ngb_mem_write(void *addr, const void *src, size_t len)
{
    mach_vm_address_t region = (mach_vm_address_t)addr;
    mach_vm_size_t span = 0;
    vm_region_basic_info_data_64_t info;
    mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t object = MACH_PORT_NULL;
    if (mach_vm_region(mach_task_self(), &region, &span, VM_REGION_BASIC_INFO_64,
                       (vm_region_info_t)&info, &count, &object) != KERN_SUCCESS)
        return 0;
    if (object)
        mach_port_deallocate(mach_task_self(), object);
    if ((mach_vm_address_t)addr < region || (mach_vm_address_t)addr + len > region + span)
        return 0;
    if (!(info.protection & VM_PROT_WRITE) &&
        mach_vm_protect(mach_task_self(), (mach_vm_address_t)addr, len, FALSE,
                        info.protection | VM_PROT_WRITE | VM_PROT_COPY) != KERN_SUCCESS)
        return 0;
    int ok = mach_vm_write(mach_task_self(), (mach_vm_address_t)addr, (vm_offset_t)src,
                           (mach_msg_type_number_t)len) == KERN_SUCCESS;
    if (!(info.protection & VM_PROT_WRITE))
        mach_vm_protect(mach_task_self(), (mach_vm_address_t)addr, len, FALSE, info.protection);
    return ok;
}
#else /* Linux */
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static inline int ngb_mkdir(const char *path) { return mkdir(path, 0755); }

static inline long ngb_exe_path(char *out, size_t n)
{
    ssize_t len = readlink("/proc/self/exe", out, n - 1);
    if (len <= 0)
        return -1;
    out[len] = 0;
    return (long)len;
}

static inline const char *ngb_home(void) { return getenv("HOME"); }

static inline int ngb_mem_fd(void)
{
    static int fd = -2;
    if (fd == -2)
        fd = open("/proc/self/mem", O_RDWR);
    return fd;
}

static inline int ngb_mem_write(void *addr, const void *src, size_t len)
{
    return ngb_mem_fd() >= 0 && pwrite(ngb_mem_fd(), src, len, (off_t)(uintptr_t)addr) == (ssize_t)len;
}

static inline int ngb_mem_read(const void *addr, void *dst, size_t len)
{
    return ngb_mem_fd() >= 0 && pread(ngb_mem_fd(), dst, len, (off_t)(uintptr_t)addr) == (ssize_t)len;
}
#endif

/* The settings' folder, made if make: $XDG_CONFIG_HOME or ~/.config, on Windows %APPDATA%, then
 * NinjaGaidenBlack. */
static inline void ngb_config_dir(char *out, size_t n, int make)
{
    const char *xdg = getenv("XDG_CONFIG_HOME"), *own = getenv("NGB_CONFIG_DIR");
    if (own && *own) {   /* the folder itself (portable mode, src/main.c) */
        snprintf(out, n, "%s", own);
        if (make)
            ngb_mkdir(out);
        return;
    }
#ifdef _WIN32
    const char *appdata = getenv("APPDATA");
    if ((!xdg || !*xdg) && appdata && *appdata) {
        snprintf(out, n, "%s/NinjaGaidenBlack", appdata);
        if (make)
            ngb_mkdir(out);
        return;
    }
#endif
    if (xdg && *xdg) {
        snprintf(out, n, "%s/NinjaGaidenBlack", xdg);
    } else {
#ifdef __APPLE__
        ngb_macos_dir(out, n, "config", make);
#else
        const char *home = ngb_home();
        snprintf(out, n, "%s/.config", home ? home : ".");
        if (make)
            ngb_mkdir(out);
        snprintf(out, n, "%s/.config/NinjaGaidenBlack", home ? home : ".");
#endif
    }
    if (make)
        ngb_mkdir(out);
}

#endif
