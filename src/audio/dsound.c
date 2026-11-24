/*
 * dsound.c -- the title's DirectSound calls (src/audio/dsound_hooks.c, written by tools/take_dsound.py) to the host
 * mixer (src/audio/dsound_hle.cpp). NGB_AUDIO_ENGINE=apu leaves every call to the recompiled library instead, which
 * drives the emulated sound chip (xboxrecomp/src/apu) as before 0.9.30.
 *
 * A taken call returns as the library's own would: eax its result, the stack past its return address and the
 * arguments it pops (tools/dsound_symbols.txt).
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include "dsound_funcs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The mixer takes host locks shared with the audio device's thread: a guest thread must not be parked by the
 * runtime's scheduler while it holds one (xboxrecomp win32_compat.h: a host section). */
#include "platform/vcpu.h"
#if !XBOX_HAS_VCPU
static inline void compat_host_enter(void) {}
static inline void compat_host_leave(void) {}
#else
void compat_host_enter(void);
void compat_host_leave(void);
#endif

int dsound_hle_call(int fn, uint32_t va, const uint32_t *args, uint32_t *result);   /* src/audio/dsound_hle.cpp */

static int s_engine = -1;   /* 1 the host mixer, 0 the recompiled library */

int dsound_host_mixer(void)
{
    if (s_engine < 0) {
        const char *e = getenv("NGB_AUDIO_ENGINE");
        s_engine = !(e && !strcmp(e, "apu"));
    }
    return s_engine;
}

int dsound_hook(int fn)
{
    uint32_t args[8], result = 0;
    int n = ds_funcs[fn].stack / 4;

    if (!dsound_host_mixer())
        return 0;
    for (int i = 0; i < n && i < 8; i++)
        args[i] = MEM32(esp + 4 + 4 * i);
    compat_host_enter();
    int took = dsound_hle_call(fn, ds_funcs[fn].va, args, &result);
    compat_host_leave();
    if (!took)
        return 0;
    eax = result;
    esp += 4 + (uint32_t)ds_funcs[fn].stack;
    return 1;
}

void mcpx_apu_set_hrtf(int mode);              /* xboxrecomp/src/apu */
void dsound_hle_set_positional(int mode);      /* src/audio/dsound_hle.cpp */

/* Positional Audio (0 off, 1 the HRTF filters, 2 panning, 3 the two mixed; #30, #86, #126), for whichever engine plays
 * (the old one has no mix: 3 is its filters) */
void ngb_audio_set_positional(int mode)
{
    static const char *const name[] = { "Off", "Console", "Clear", "Mixed" };
    fprintf(stderr, "[AUDIO] Positional Audio: %s\n", mode >= 0 && mode <= 3 ? name[mode] : "?");
    mcpx_apu_set_hrtf(mode);
    dsound_hle_set_positional(mode);
}

void dsound_hle_set_output(int choice);         /* src/audio/dsound_hle.cpp */

/* Audio Output (0 auto, 1 stereo, 2 5.1): for the mixer, when it starts (the title's DirectSoundCreate), and for
 * the arcade's programs the game hands over to (NGB_AUDIO_OUTPUT, src/trilogy/main.c). */
void ngb_audio_set_output(int choice)
{
    char v[4];
    dsound_hle_set_output(choice);
    snprintf(v, sizeof v, "%d", choice < 0 ? 0 : choice > 2 ? 2 : choice);
#if defined(_WIN32)
    _putenv_s("NGB_AUDIO_OUTPUT", v);
#else
    setenv("NGB_AUDIO_OUTPUT", v, 1);
#endif
}
