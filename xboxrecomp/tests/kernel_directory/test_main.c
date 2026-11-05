#include "kernel.h"
#include "xbox_memory_layout.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef void (*guest_fn)(void);
extern guest_fn recomp_lookup_kernel(uint32_t);
#include "recomp_regs.h"   /* the registers: g_eax... */
extern ptrdiff_t g_xbox_mem_offset;
void *recomp_lookup(ULONG a) { (void)a; abort(); }
void *recomp_lookup_manual(ULONG a) { (void)a; abort(); }
static uint8_t *mem;
static guest_fn query;
static int invoke(HANDLE h, const char *pattern, unsigned restart, unsigned klass) {
    uint32_t *sp=(uint32_t *)(mem+0x20000);
    memset(sp,0,64); sp[0]=0xBEEF0001;
    sp[1]=(uint32_t)(uintptr_t)h; sp[5]=0x30000; sp[6]=0x31000;
    sp[7]=512; sp[8]=klass; sp[9]=pattern?0x32000:0; sp[10]=restart;
    if(pattern) {
        *(uint16_t *)(mem+0x32000)=(uint16_t)strlen(pattern);
        *(uint16_t *)(mem+0x32002)=(uint16_t)strlen(pattern)+1;
        *(uint32_t *)(mem+0x32004)=0x32100;
        strcpy((char *)mem+0x32100,pattern);
    }
    memset(mem+0x30000,0xCC,8); memset(mem+0x31000,0xCC,512);
    g_esp=0x20000; query();
    if(g_esp!=0x2002C) { fprintf(stderr,"FAIL directory query stack: %08X expected 0002002C\n",g_esp); return 0; }
    if(*(uint32_t *)(mem+0x30000)!=g_eax) { puts("FAIL I/O status differs"); return 0; }
    return 1;
}
int main(void) {
    char root[MAX_PATH], path[MAX_PATH];
    GetTempPathA(MAX_PATH,root); sprintf(path,"%sxml1-dir-test-%lu",root,GetCurrentProcessId());
    if(!CreateDirectoryA(path,NULL)) return 10;
    char child[MAX_PATH]; sprintf(child,"%s\\SaveSlot",path); CreateDirectoryA(child,NULL);
    HANDLE h=CreateFileA(path,FILE_LIST_DIRECTORY,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);
    if(h==INVALID_HANDLE_VALUE||(uintptr_t)h>UINT32_MAX) return 11;
    mem=VirtualAlloc(NULL,16*1024*1024,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE); if(!mem) return 12;
    g_xbox_mem_offset=(ptrdiff_t)mem;
    *(uint32_t *)(mem+0x10000)=0x800000CF;
    xbox_kernel_set_thunk_address(0x10000,1); xbox_kernel_bridge_init();
    query=recomp_lookup_kernel(*(uint32_t *)(mem+0x10000));
    int ok=invoke(h,"Save*",1,1);
    XBOX_FILE_DIRECTORY_INFORMATION *e=(void *)(mem+0x31000);
    if(ok && (g_eax || e->FileNameLength!=8 || memcmp(e->FileName,"SaveSlot",8) || !(e->FileAttributes&16))) {
        fprintf(stderr,"FAIL filtered directory: status=%08X length=%lu\n",g_eax,e->FileNameLength); ok=0;
    }
    if(ok) ok=invoke(h,NULL,0,1) && g_eax==0x80000006u;
    if(ok) ok=invoke(h,"Save*",1,1) && g_eax==0;
    if(ok) ok=invoke(h,"*",1,1) && g_eax==0 && e->FileNameLength==8 && !memcmp(e->FileName,"SaveSlot",8);
    if(ok) ok=invoke(h,"*",1,2) && g_eax==0xC0000003u;
    CloseHandle(h); RemoveDirectoryA(child); RemoveDirectoryA(path); VirtualFree(mem,0,MEM_RELEASE);
    if(!ok) return 1;
    puts("PASS: Xbox directory ABI, 40-byte cleanup, filter, continuation, restart, dot exclusion, and class rejection"); return 0;
}

