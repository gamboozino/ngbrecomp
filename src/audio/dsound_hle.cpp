// DirectSound (XDK 5849) on a host mixer: the title's sound buffers read from guest memory as they play, mixed into
// 48 kHz stereo or 5.1 for SDL's output. Modelled on SCDA-XBERecomp's xdk/dsound.cpp (see NOTICE): its buffers, 3D pass,
// mix bins and I3DL2 reverb, ported to this title's library (tools/dsound_symbols.txt: the calls it makes, buffers
// only, no streams) and given the sound chip's HRTF filters for Positional Audio's "Console" choice ("Clear": their
// levels alone).
//
// The emulated sound chip (xboxrecomp/src/apu) is not started with this; NGB_AUDIO_ENGINE=apu goes back to it.
// 5.1 by Audio Output, on Auto when the default output device has six channels or more (the title is told its
// speakers are surround, and mixes for them); NGB_AUDIO_CHANNELS=2 or 6 overrides it. NGB_DSOUND_LOG=1 logs the
// calls as they first come (2 every volume change too, 3 every Lock); NGB_DSOUND_WAV=<file> writes what plays
// (48 kHz, 16-bit); NGB_DSOUND_REVERB=0 leaves the reverb out, NGB_DSOUND_XTALK=1 adds the crosstalk effect (comparing).
#include <SDL.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "kernel/kernel.h"

extern "C" {
#include "dsound_funcs.h"
uint32_t xbox_HeapAlloc(uint32_t size, uint32_t align);
extern volatile int apu_host_paused;   // xboxrecomp/src/apu/apu_core.c: a menu holds the game
int xbox_guest_set_event(uint32_t handle);   // xboxrecomp kernel_bridge.c
#include "nv2a_vk/host/threadload.h"
}

using kernel::HostPtr;
using kernel::R16;
using kernel::R32;
using kernel::W32;

namespace {

constexpr uint32_t kRate = 48000;

// mix bins (DSMIXBIN), as the mixer's lanes: the speakers, then the 3D (crosstalk) submixes the HRTF feeds
constexpr uint32_t kBinFrontLeft = 0, kBinFrontRight = 1, kBinCentre = 2, kBinSub = 3, kBinBackLeft = 4,
                   kBinBackRight = 5;
constexpr uint32_t kBinCrosstalkFrontLeft = 6, kBinCrosstalkFrontRight = 7, kBinCrosstalkBackLeft = 8,
                   kBinCrosstalkBackRight = 9;
constexpr uint32_t kBinFirstSend = 10;  // effect sends from here up
constexpr uint32_t kBinI3dl2 = 10;      // dsstdfx's I3DL2 reverb reads this send
constexpr uint32_t kSends = 1u << kBinI3dl2;
constexpr uint32_t kLanes = kBinFirstSend;

// the library's full HRTF tables (DirectSoundUseFullHRTF; the game's are at 0x395430 and 0x383E70, its filter pick
// at 0x37D8A8, dsound_hle_set_hrtf_tables): a filter number per azimuth step by elevation step, and the filters --
// the ear away from the sound first, 31 sign and magnitude taps and its interaural delay in samples in the last
// byte, then the near ear's. A program without them (the arcade's, which has no 3D sound) plays 3D sounds centred.
uint32_t g_hrtf_index = 0, g_hrtf_filters = 0;
constexpr int kHrtfTaps = 31;
constexpr int kHrtfAzimuths = 61;    // 0 to 180 degrees in 3
constexpr int kHrtfElevations = 31;  // -90 to 90 degrees in 6
constexpr int kHrtfAzimuthStep = 3, kHrtfElevationStep = 6;
// the azimuth steps widen towards the poles: 3 up to 30 degrees of elevation, 6 up to 60, 12 beyond, none at 90
constexpr int kHrtfMidStep = 6, kHrtfPoleStep = 12, kHrtfLowBand = 30, kHrtfMidBand = 60, kHrtfPole = 90;
constexpr int kEarLength = 64;  // past the longest delay and the taps: 31 + 31

constexpr uint32_t kDsErrInvalidCall = 0x88780032u, kEFail = 0x80004005u;

std::atomic<int> g_positional{2};
// "Mixed" (Positional Audio 3): the filters' share of each ear, the rest "Clear"'s level -- the filters' cues for
// where a sound is (behind, to the side) with less of their dullness (#126). NGB_POS_MIX=0..1 sets it, for tuning.
float g_pos_mix = 0.5f;
// "Mixed"'s level half by the sound's direction (EarPair::mixpan): the far ear 6 dB down for a sound straight to one
// side. Clear's levels -- the filters' strengths, a few dB apart and least in the bass -- left the hooves and footsteps
// around Ryu hard to place (#126). NGB_MIX_PAN_DB=n sets the dB (0: Clear's levels), NGB_MIX_PAN_FILE=<file> holding n
// is read again each second (testing: strengths compared in one run).
std::atomic<float> g_mix_pan_db{6.0f};
static void MixPanTick(uint32_t frames) {
    static uint32_t since = 48000;
    static const char* file = std::getenv("NGB_MIX_PAN_FILE");
    static bool env_read;
    if (!env_read) {
        env_read = true;
        if (const char* e = std::getenv("NGB_MIX_PAN_DB")) g_mix_pan_db = float(std::atof(e));
    }
    static uint64_t played;
    played += frames;
    if (!file || (since += frames) < 48000) return;
    since = 0;
    if (FILE* f = std::fopen(file, "r")) {
        float v;
        if (std::fscanf(f, "%f", &v) == 1 && v != g_mix_pan_db.load()) {
            g_mix_pan_db = v;
            std::fprintf(stderr, "[MIXPAN] %.0f dB from sample %llu\n", v, (unsigned long long)played);
        }
        std::fclose(f);
    }
}
bool g_surround = false;
int g_output_choice = 0;  // Audio Output: 0 auto, 1 stereo, 2 5.1 (dsound_hle_set_output)  // the output is 5.1: FL FR C LFE BL BR, SDL's order and the mix bins'  // Positional Audio: 0 off, 1 console (the HRTF filters), 2 clear (their levels)
int g_log = -1;

bool Log() {
    if (g_log < 0) g_log = std::getenv("NGB_DSOUND_LOG") ? 1 : 0;
    return g_log > 0;
}

float F32(uint32_t v) {
    float f;
    std::memcpy(&f, &v, 4);
    return f;
}
float Db(int32_t hundredths) { return hundredths <= -10000 ? 0.0f : std::pow(10.0f, hundredths / 2000.0f); }

// one ear's filter: the taps, and the delay its ear hears the sound with
struct Ear {
    float tap[kHrtfTaps] = {};
    float delay = 0;
    int peak = 0;   // the strongest tap: where "Mixed"'s level lines up with the filter
};
struct EarPair {
    Ear e[2];  // left, right
    float pan[2] = {1, 1};  // "Clear": each ear's level from its filter's strength, at constant power
    float norm = 1;         // "Mixed": the filters brought to that same power, so neither half outweighs the other
    float mixpan[2] = {1, 1};   // "Mixed"'s levels: Clear's, or by the direction (NGB_MIX_PAN_DB, testing)
};

struct Voice {
    uint32_t obj = 0;
    int refs = 1;
    // format
    uint16_t tag = 1, channels = 1, bits = 16, align = 2;
    uint32_t rate = 48000;
    // the buffer
    uint32_t data = 0, bytes = 0, loop_start = 0, loop_len = 0;
    uint32_t owned = 0, owned_bytes = 0;  // memory made with the buffer (a size and no data)
    bool playing = false, looping = false;
    double pos = 0;  // frames
    uint32_t freq = 0;
    // levels
    float volume = 1, headroom = 1;
    uint8_t nbins = 0, bin[8] = {};
    int32_t bin_mb[32] = {};
    // 3D
    bool is3d = false;
    float p[3] = {}, min_dist = 1, max_dist = 1e9f, rolloff = 1;
    float lf = 1, hf = 1, room = 1;  // I3DL2 source (not set by this title: the defaults)
    // the filter (SetFilter, DLS2 mode): the sound chip's state variable low pass, per channel
    bool lpf = false;
    float fc = 1, q = 1, qn = 1;
    float sv_l[2] = {}, sv_b[2] = {};
    // the last block's levels, the next block ramps from them
    float gain[2][kLanes] = {};
    float send_gain[2] = {};
    bool has_gain = false;
    // a mono 3D voice: its recent samples and the filters it was last heard through
    float ear[kEarLength] = {};
    uint32_t ear_pos = 0;
    EarPair hrtf;
    bool has_hrtf = false;
    // SetNotificationPositions: frames, and the events set when play reaches them (DSBPN_OFFSETSTOP: at the stop)
    std::vector<std::pair<uint32_t, uint32_t>> notify;
    uint32_t notify_stop = 0;
    // the ADPCM block last decoded
    int64_t block = -1;
    int16_t decoded[65 * 2];
};

std::recursive_mutex g_lock;
std::map<uint32_t, Voice> g_voices;
std::vector<uint32_t> g_free_objects;  // released buffers' objects, for the next
// released buffers' memory by size, for the next buffers: the title creates and releases a buffer for many of the
// sounds it streams, and that memory was never given back, so the heap ran out after a few hours of play (#137).
// It is kept here rather than freed: the guest heap neither splits nor joins blocks, so a freed block went to the
// next small allocation and the next buffer took new memory all the same.
std::multimap<uint32_t, uint32_t> g_spare_memory;
std::vector<uint32_t> g_events;        // notifications reached, set at the next DirectSoundDoWork (the title's thread)
float g_listener[3] = {}, g_front[3] = {0, 0, 1}, g_top[3] = {0, 1, 0};
// IDirectSound_SetMixBinHeadroom: a step halves the bin; the library starts bins 0..30 at 1 and 31 at 0
uint8_t g_bin_headroom[32] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                              1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0};
uint32_t g_ds = 0;
bool g_started = false;
FILE* g_wav = nullptr;
uint32_t g_wav_frames = 0;

// Xbox ADPCM plays 64 frames per 36 byte block (the header sample and 63 nibbles, the last dropped)
constexpr uint32_t kAdpcmFrames = 64;

uint32_t FramesOf(const Voice& v, uint32_t bytes) {
    if (v.tag == 0x69) return bytes / (36u * v.channels) * kAdpcmFrames;
    return bytes / std::max<uint32_t>(v.align, 1);
}
uint32_t BytesOf(const Voice& v, uint32_t frames) {
    if (v.tag == 0x69) return frames / kAdpcmFrames * 36 * v.channels;
    return frames * v.align;
}

const int kIndex[16] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};
const int kStep[89] = {7,     8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,    23,
                       25,    28,    31,    34,    37,    41,    45,    50,    55,    60,    66,    73,    80,
                       88,    97,    107,   118,   130,   143,   157,   173,   190,   209,   230,   253,   279,
                       307,   337,   371,   408,   449,   494,   544,   598,   658,   724,   796,   876,   963,
                       1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,  3327,
                       3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845,  8630,  9493,  10442, 11487,
                       12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};

// one 36 byte block per channel of IMA ADPCM: 65 frames decoded, kAdpcmFrames played
void DecodeBlock(const uint8_t* b, int ch, int16_t* out) {
    for (int c = 0; c < ch; c++) {
        int pred = int16_t(b[c * 4] | b[c * 4 + 1] << 8), idx = std::min<int>(b[c * 4 + 2], 88);
        out[c] = int16_t(pred);
        const uint8_t* d = b + ch * 4;
        for (int k = 0; k < 8; k++)
            for (int j = 0; j < 8; j++) {
                int n = (d[(k * ch + c) * 4 + j / 2] >> ((j & 1) * 4)) & 15;
                int step = kStep[idx], diff = step >> 3;
                if (n & 1) diff += step >> 2;
                if (n & 2) diff += step >> 1;
                if (n & 4) diff += step;
                pred = std::clamp(n & 8 ? pred - diff : pred + diff, -32768, 32767);
                idx = std::clamp(idx + kIndex[n], 0, 88);
                out[(1 + k * 8 + j) * ch + c] = int16_t(pred);
            }
    }
}

// channel c at frame f
float Sample(Voice& v, uint32_t f, int c) {
    c = std::min<int>(c, v.channels - 1);
    if (v.tag == 0x69) {
        int64_t at = int64_t(v.data) + int64_t(f / kAdpcmFrames) * 36 * v.channels;
        if (at != v.block) {
            DecodeBlock(static_cast<const uint8_t*>(HostPtr(uint32_t(at))), std::min<int>(v.channels, 2), v.decoded);
            v.block = at;
        }
        return v.decoded[(f % kAdpcmFrames) * v.channels + c] / 32768.0f;
    }
    const uint8_t* p = static_cast<const uint8_t*>(HostPtr(v.data));
    if (v.bits == 8) return (p[f * v.align + c] - 128) / 128.0f;
    int16_t s;
    std::memcpy(&s, p + f * v.align + c * 2, 2);
    return s / 32768.0f;
}

// ── the I3DL2 reverb (dsstdfx's, as SCDA-XBERecomp has it from the effects image) ──

struct Reverb {
    struct Pole {
        float a = 0, b = 0, z = 0;
        float Run(float x) { return z = a * z + b * x + 1e-20f; }  // the offset keeps silence out of denormals
    };
    struct Line {
        std::vector<float> s;
        uint32_t w = 0;
        float At(uint32_t delay) const { return s[(w + s.size() - delay % s.size()) % s.size()]; }
        void Put(float x) {
            s[w] = x;
            w = (w + 1) % uint32_t(s.size());
        }
    };
    struct Params {
        uint32_t refl, tap[4], in[4], chain[8], comb[4], echo;
        float early[4], late, diff[8], rest, cross;
        Pole in_lp, chain_lp[3], loop_lp[4], echo_lp[2];
    };
    static constexpr int kLen[14] = {6000, 6000, 6000, 6000, 680, 612, 640, 720, 1392, 1684, 2044, 2476, 24000, 24000};
    Line line[14];
    Params now{}, next{};
    bool on = false, pending = false;
    float fade = 1, held[4] = {};
    uint32_t tick = 0;

    Reverb() {
        for (int i = 0; i < 14; i++) line[i].s.assign(kLen[i], 0.0f);
    }
    static float Lin(float mb) { return std::pow(10.0f, mb / 2000.0f); }
    // y = a * y1 + b * x: mb millibels at 0 Hz, hf more at ref Hz
    static Pole Design(int mb, int hf, float ref, float rate) {
        Pole p;
        if (mb <= -10000) return p;
        p.b = mb ? Lin(float(mb)) : -1;  // 1.0 is past the DSP's fractions and comes out as -1
        if (hf >= 0) return p;
        p.b = Lin(float(mb));
        float g = Lin(2.0f * hf), c = std::cos(6.2831853f * ref / rate);
        p.a = (1 - g * c - std::sqrt(2 * g * (1 - c) - g * g * (1 - c * c))) / (1 - g);
        p.b *= 1 - p.a;
        return p;
    }
    // DSI3DL2LISTENER as 12 floats, the library's own conversion
    void Set(const float* p) {
        static const float kGain[4] = {0.57f, 0.52f, 0.54f, 0.6f}, kTap[4] = {0.1052f, 0.0673f, 0.1737f, 0.2404f};
        static const float kComb[4] = {0.2469f, 0.2209f, 0.2311f, 0.2608f};
        static const float kCombBase[4] = {0.003651f, 0.003515f, 0.003673f, 0.003946f};
        static const float kChain[8] = {0.040023f, 0.288186f, 0.603764f, 0.851905f,
                                        0.021224f, 0.37542f,  0.623583f, 0.759229f};
        static const float kSign[8] = {1, -1, -1, 1, 1, 1, -1, -1};
        static const int kInput[4] = {0, 58, 134, 232};
        auto al = [](int n) { return (n + 1) & ~3; };
        auto tap = [](int n) { return uint32_t(std::max((n + 3) & ~3, 16)); };  // read in blocks of 4, 16 back at least
        float decay = std::max(p[3], 0.1f), ratio = std::max(p[4], 0.1f), d = std::clamp(p[9] * 0.01f, 0.0f, 1.0f);
        float sc = std::clamp((2 - p[10] * 0.01f) * (decay - 1) * 5, 1.0f, 60.0f);
        float f = sc >= 2.5f ? 1.0f : (sc - 0.8f) * 0.5882353f;
        // a loop of n samples loses 60 dB in the decay time, high frequencies sooner by the ratio
        auto loop = [&](int n) {
            float x = n / 24000.0f / decay * -6000.0f;
            int lf = int(x), hf = int(ratio < 1 ? x / ratio : x);
            return Design(lf, hf - lf, p[11], 24000);
        };
        Params q{};
        q.in_lp = Design(0, int(p[1]), p[11], 48000);
        int refl = al(int(p[6] * 24000)), rev = int(p[8] * 24000), echo = int(f * 24000), prev = 0;
        q.refl = tap(refl);
        q.late = Lin(p[7] + p[0]);
        for (int k = 0; k < 4; k++) {
            q.early[k] = Lin(p[5] + p[0]) * kGain[k];
            q.tap[k] = tap(refl + al(int(rev * kTap[k])));
            q.in[k] = tap(refl + al(rev) + kInput[k]);
            q.comb[k] = tap(std::min(al(int((p[8] * kComb[k] + kCombBase[k]) * 24000)), kLen[4 + k] - 1));
            q.loop_lp[k] = loop(kLen[8 + k]);
            // echo times count from the input, a stage of the chain holds 250 ms
            int a = al(int(f * kChain[k] * 24000)), b = al(int(f * kChain[4 + k] * 24000)), m = std::max(a, b) - prev;
            float r = m > 5999 ? 5999.0f / m : 0;
            q.chain[k * 2] = tap(r ? al(int((a - prev) * r)) : a - prev);
            q.chain[k * 2 + 1] = tap(r ? al(int((b - prev) * r)) : b - prev);
            prev = b;
        }
        q.echo = tap(std::min(al(echo), 23999));
        for (int k = 0; k < 3; k++) q.chain_lp[k] = loop(echo >> 2);
        q.echo_lp[0] = q.echo_lp[1] = loop(echo);
        for (int k = 0; k < 8; k++) q.diff[k] = d * kSign[k];
        q.rest = d > 0 ? 1 - d : -1;
        q.cross = 0.2f * d;
        // a change fades out, swaps, fades in
        (on ? next : now) = q;
        pending = on;
        on = true;
    }
    // one 24 kHz sample in, front left, front right, back left, back right out
    void Frame(float x, float o[4]) {
        static const float kFeed[4] = {-0.4f, 0.4f, -0.4f, -0.4f};
        Params& q = now;
        Line* l = line;
        float t = l[0].At(q.refl), e[4], in[4], c[8], u[4], sum = 0;
        for (int k = 0; k < 4; k++) {
            e[k] = t + l[0].At(q.tap[k]) + kFeed[k] * l[4 + k].At(q.comb[k]);
            in[k] = l[0].At(q.in[k]);
            c[k * 2] = l[k].At(q.chain[k * 2]);
            c[k * 2 + 1] = l[k].At(q.chain[k * 2 + 1]);
        }
        float sa = c[1] + c[2] + c[4] + c[7], sb = c[0] + c[3] + c[5] + c[6];
        float ea = q.rest * sa + l[12].At(q.echo), eb = q.rest * sb + l[13].At(q.echo);
        for (int j = 0; j < 4; j++)
            sum += u[j] = l[8 + j].At(0) + q.diff[j * 2] * in[j & 1 ? 2 : 0] + q.diff[j * 2 + 1] * in[j & 1 ? 3 : 1];
        for (int j = 0; j < 4; j++) {
            o[j] = q.early[j] * e[j] + q.late * u[j] + q.late * 0.5f * (j < 2 ? ea : eb);
            l[4 + j].Put(e[j]);
            l[8 + j].Put(q.loop_lp[j].Run(u[j] - 0.5f * sum + q.cross * (sa + sb)));
        }
        l[0].Put(x);
        for (int k = 0; k < 3; k++) l[k + 1].Put(q.chain_lp[k].Run(c[k * 2 + 1]));
        l[12].Put(q.echo_lp[0].Run(ea));
        l[13].Put(q.echo_lp[1].Run(eb));
    }
    // adds the reverb of the 48 kHz send to the crosstalk lanes, as the image's reverb writes them
    void Run(const float* send, float* out, uint32_t frames) {
        static const uint32_t kOut[4] = {kBinCrosstalkFrontLeft, kBinCrosstalkFrontRight, kBinCrosstalkBackLeft,
                                         kBinCrosstalkBackRight};
        for (uint32_t i = 0; on && i < frames; i++) {
            float x = now.in_lp.Run(send[i] * 0.5f), o[4];
            if (!(tick++ & 1)) {
                if (pending && (fade -= 1 / 128.0f) <= 0) {
                    now = next;
                    pending = false;
                    fade = 0;
                } else if (!pending) {
                    fade = std::min(fade + 1 / 128.0f, 1.0f);
                }
                Frame(x, o);
                for (int k = 0; k < 4; k++) held[k] = o[k] * fade;
            }
            for (int k = 0; k < 4; k++) out[i * kLanes + kOut[k]] += held[k];
        }
    }
} g_reverb;

// dsstdfx's crosstalk effect (the image's last: the four 3D submixes in, the front and back pairs out), as
// SCDA-XBERecomp has it from the image: each side of a pair takes the other side's output of 11 samples before
// through a one pole filter, -0.77 of it at low frequencies -- what keeps the HRTF's two ears apart on speakers
// (the filters give the far ear the more bass, this takes it back out of the near one). It runs on the reverb
// as well, which the image writes into the same bins. Off unless NGB_DSOUND_XTALK=1, and then for "Console" alone:
// 0.9.40 ran it, and its bass came out mostly out of phase between the speakers (#126); xemu running the image on
// its DSP sounds as "Console" does without it (level, width and tone within 0.3 dB over the attract demo), so the
// library leaves it switched off there or never reaches it.
struct CrossFeed {
    static constexpr uint32_t kDelay = 11;
    static constexpr float kPole = 0.924f, kFeed = -0.0585f;  // image coefficients 0x7645A1, 0xF88313
    float s[2] = {}, past[2][kDelay] = {};
    uint32_t at = 0;
    void Run(float& left, float& right) {
        s[0] = kPole * s[0] + kFeed * past[1][at] + 1e-20f;  // the offset keeps silence out of denormals
        s[1] = kPole * s[1] + kFeed * past[0][at] + 1e-20f;
        left += s[0];
        right += s[1];
        past[0][at] = left;
        past[1][at] = right;
        at = (at + 1) % kDelay;
    }
} g_front_feed, g_back_feed;

// ── 3D: where a voice is, as the library's 3D pass works it out ──

float Dot(const float a[3], const float b[3]) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

// degrees from straight ahead, plus to the right: the library's octant estimate, not atan2
float Azimuth(const float d[3], const float front[3], const float right[3]) {
    float ahead = Dot(d, front), side = Dot(d, right);
    float a = std::fabs(ahead), s = std::fabs(side), deg = 0;
    if (a > s) deg = 45 * s / a;
    else if (s > 0) deg = 90 - 45 * a / s;
    if (ahead < 0) deg = 180 - deg;
    return side < 0 ? -deg : deg;
}
// degrees above the horizontal, the same estimate
float Elevation(const float d[3], const float front[3], const float right[3], const float top[3]) {
    float flat = std::hypot(Dot(d, front), Dot(d, right)), up = Dot(d, top), u = std::fabs(up), deg = 0;
    if (flat > u) deg = 45 * u / flat;
    else if (u > 0) deg = 90 - 45 * flat / u;
    return up < 0 ? -deg : deg;
}

// the library's filter pick (0x37D8A8): the filter for a direction, by its table
uint32_t HrtfEntry(float azimuth, float elevation) {
    int half = kHrtfElevationStep / 2;
    int el = int(elevation < 0 ? elevation - half : elevation + half) / kHrtfElevationStep * kHrtfElevationStep;
    int step = kHrtfAzimuthStep;
    if (std::abs(el) > kHrtfLowBand) step = kHrtfMidStep;
    if (std::abs(el) > kHrtfMidBand) step = kHrtfPoleStep;
    int deg = int(std::fabs(azimuth) + step / 2.0f) / step * step;
    if (std::abs(el) == kHrtfPole) deg = 0;
    int a = deg / kHrtfAzimuthStep, e = (el + kHrtfPole) / kHrtfElevationStep;
    if (a < 0 || a >= kHrtfAzimuths) a = 0;
    if (e < 0 || e >= kHrtfElevations) e = 0;
    return static_cast<const uint16_t*>(HostPtr(g_hrtf_index))[a * kHrtfElevations + e];
}

// the two ears' filters for a direction: the table's far ear is the left for a sound on the right
EarPair Ears(float azimuth, float elevation) {
    EarPair p;
    if (!(std::fabs(azimuth) <= 180) || !(std::fabs(elevation) <= 90)) return p;
    const uint8_t* f = static_cast<const uint8_t*>(HostPtr(g_hrtf_filters)) + HrtfEntry(azimuth, elevation) * 32u;
    int far = azimuth < 0 ? 1 : 0;
    for (int k = 0; k < 2; k++) {
        const uint8_t* t = f + k * 32;
        Ear& ear = p.e[k == 0 ? far : 1 - far];
        for (int i = 0; i < kHrtfTaps; i++) ear.tap[i] = (t[i] & 0x80 ? -float(t[i] & 0x7F) : float(t[i])) / 128.0f;
        ear.delay = t[31];
        for (int i = 1; i < kHrtfTaps; i++)
            if (std::fabs(ear.tap[i]) > std::fabs(ear.tap[ear.peak])) ear.peak = i;
    }
    // "Clear": the filters' strengths as the ears' levels, a few dB apart, the pair scaled to constant power
    // (0 dB). 0.9.40 gave each ear its filter's RMS gain instead, +2 to +5 dB by direction, most where the
    // filters carry the most bass; it was heard as boomy and muffled. A pan by the direction with the table's
    // delay between the ears was also tried for 0.9.40 and heard as worse.
    float e[2] = {};
    for (int c = 0; c < 2; c++)
        for (int i = 0; i < kHrtfTaps; i++) e[c] += p.e[c].tap[i] * p.e[c].tap[i];
    for (int c = 0; c < 2; c++) p.pan[c] = e[0] + e[1] > 0 ? std::sqrt(2 * e[c] / (e[0] + e[1])) : 1.0f;
    p.norm = e[0] + e[1] > 0 ? std::sqrt(2 / (e[0] + e[1])) : 1.0f;
    // "Mixed"'s levels by the direction (g_mix_pan_db): the far ear that many dB down for a sound straight to one side,
    // less towards the front and back and above, the pair at constant power like Clear's
    const float mix_db = g_mix_pan_db.load(std::memory_order_relaxed);
    p.mixpan[0] = p.pan[0];
    p.mixpan[1] = p.pan[1];
    if (mix_db > 0) {
        float side = std::fabs(std::sin(azimuth * 3.14159265f / 180.0f)) * std::cos(elevation * 3.14159265f / 180.0f);
        float g = std::pow(10.0f, -mix_db * side / 20.0f), s = std::sqrt(2.0f / (1.0f + g * g));
        p.mixpan[far] = g * s;
        p.mixpan[1 - far] = s;
    }
    return p;
}

struct Place {
    bool on = false;
    float att = 1, azimuth = 0, elevation = 0;
    float front = 1, back = 0, centre = 1;  // the 3D submixes' front and back pairs, the centre bin
};

// a 3D voice's share for the pair behind, by power: none to 45 degrees, half at 90, all from 135, drawn towards half
// and half within half a unit of the head (the library's, after SCDA-XBERecomp)
float BackShare(float azimuth, float elevation, float dist) {
    float a = std::fabs(azimuth) / 90, e = std::fabs(elevation) / 90;
    float back = std::clamp(0.5f - (1 - a) * (1 - e), 0.0f, 1.0f);
    if (dist < 0.5f) back = ((back - 0.5f) * dist + 1) * 0.5f;
    return back;
}

// a voice whose fifth bin is the centre shares the front with it by whole degrees below 45: 0.7071 cos^2(2a)
void ShareCentre(Place& at) {
    int az = int(std::fabs(at.azimuth)), el = int(std::fabs(at.elevation));
    if (az >= 45 || el >= 45) {
        at.centre = 0;
        return;
    }
    float both = std::cos(2 * az * 0.017453293f) * std::cos(2 * el * 0.017453293f);
    at.centre *= 0.7071f * both;
    at.front *= 1 - 0.5f * both * both;
}

// distance attenuation (DS3D's: rolloff times the distance past the minimum, capped at the maximum) and direction
Place Locate(const Voice& v) {
    Place at;
    if (!v.is3d) return at;
    at.on = true;
    float d[3] = {v.p[0] - g_listener[0], v.p[1] - g_listener[1], v.p[2] - g_listener[2]};
    float dist = std::min(std::sqrt(Dot(d, d)), v.max_dist);
    if (dist > v.min_dist) at.att = v.min_dist / (v.min_dist + v.rolloff * (dist - v.min_dist));
    float right[3] = {g_top[1] * g_front[2] - g_top[2] * g_front[1], g_top[2] * g_front[0] - g_top[0] * g_front[2],
                      g_top[0] * g_front[1] - g_top[1] * g_front[0]};
    at.azimuth = Azimuth(d, g_front, right);
    at.elevation = Elevation(d, g_front, right, g_top);
    // 5.1 splits the 3D submixes between the front and back pairs; stereo plays the front pair alone
    if (g_surround) {
        float back = BackShare(at.azimuth, at.elevation, dist);
        at.front = std::sqrt(1 - back);
        at.back = std::sqrt(back);
    }
    at.centre = at.front;
    if (v.nbins >= 5 && v.bin[4] == kBinCentre) ShareCentre(at);
    return at;
}

// a 3D voice's level in a bin besides its distance: the 3D submixes' pairs and the centre take their shares
float BinScale(const Place& at, uint32_t bin) {
    if (!at.on) return 1.0f;
    if (bin == kBinCentre) return at.centre;
    if (bin == kBinCrosstalkFrontLeft || bin == kBinCrosstalkFrontRight) return at.front;
    if (bin == kBinCrosstalkBackLeft || bin == kBinCrosstalkBackRight) return at.back;
    return 1.0f;
}

// gain of the voice's channels in each lane, and their share of the reverb send: a voice's slots alternate between
// its channels.
void Levels(const Voice& v, const Place& at, float g[2][kLanes], float w[2]) {
    std::memset(g, 0, 2 * kLanes * sizeof(float));
    w[0] = w[1] = 0;
    static const uint8_t kFront[2] = {kBinFrontLeft, kBinFrontRight};
    const uint8_t* bin = v.nbins ? v.bin : kFront;
    uint32_t n = v.nbins ? v.nbins : 2, voices = (std::max<uint32_t>(v.channels, 1) - 1) / 2 + 1;
    uint32_t per = std::max<uint32_t>(n / voices, 2), step = v.channels == 1 ? 1 : 2;
    bool speakers = false;
    for (uint32_t s = 0; s < n; s++)
        speakers |= bin[s] < kBinFirstSend && bin[s] != kBinSub && v.bin_mb[bin[s]] > -10000;
    for (uint32_t c = 0; c < 2; c++)
        for (uint32_t s = c; c < v.channels && s < per && s < n; s += step) {
            uint32_t b = bin[s];
            // the bin's headroom (SetMixBinHeadroom; the library's default a step, half), as the chip mixes it: the
            // game sets its bins to none, the arcade leaves them
            float x = std::ldexp(Db(v.bin_mb[b]) * v.volume * v.headroom * at.att * BinScale(at, b),
                                 -g_bin_headroom[b]);
            if (kSends >> b & 1) {
                if (at.on) x *= v.room;
                w[c] += x;
                continue;
            }
            if (b >= kBinFirstSend) continue;  // other effect sends
            if (!speakers) {  // a voice heard through effect sends alone plays on the front pair
                g[c][kBinFrontLeft] = std::max(g[c][kBinFrontLeft], x);
                g[c][kBinFrontRight] = std::max(g[c][kBinFrontRight], x);
                continue;
            }
            // every slot adds into its bin, as the chip mixes them: a bin listed twice plays twice as loud (the
            // title's music lists the front pair twice)
            g[c][b] += x;
        }
    // a sound placed at NaN goes silent alone, where its NaN would silence the whole mix
    for (int c = 0; c < 2; c++) {
        for (uint32_t k = 0; k < kLanes; k++)
            if (!std::isfinite(g[c][k])) g[c][k] = 0;
        if (!std::isfinite(w[c])) w[c] = 0;
    }
}

// ── mixing ──

// a notification reached: its event set at the next DirectSoundDoWork, on the title's thread
void Reached(uint32_t ev) { g_events.push_back(ev); }

// the voice's frame at its position and the one after it, none once it stopped
struct Cursor {
    uint32_t f, next;
};
std::optional<Cursor> BufferFrame(Voice& v) {
    uint32_t f = uint32_t(v.pos), end = FramesOf(v, v.bytes), start = 0;
    if (v.looping && v.loop_len) end = FramesOf(v, v.loop_start + v.loop_len);
    if (v.loop_len) start = FramesOf(v, v.loop_start);
    if (f >= end) {
        if (!v.looping || !end || start >= end) {
            v.playing = false;
            v.pos = 0;
            return std::nullopt;
        }
        v.pos -= end - start;
        f = uint32_t(v.pos);
        for (auto& [at, ev] : v.notify)  // the notifications from the start of the loop to here (the end's came)
            if (at >= start && at <= f) Reached(ev);
        if (f >= end) f = start, v.pos = start;
    }
    if (!v.data) return std::nullopt;
    Cursor at = {f, f};
    if (f + 1 < end) at.next = f + 1;
    else if (v.looping) at.next = start;
    return at;
}

// the chip's state variable low pass (SetFilter's DLS2 mode), as the emulated chip runs it
float RunFilter(Voice& v, int c, float x) {
    x *= v.qn;
    v.sv_b[c] -= v.sv_b[c] * v.sv_b[c] * v.sv_b[c] * 0.001f;
    float h = x - v.sv_l[c] - v.q * v.sv_b[c];
    v.sv_b[c] += v.fc * h;
    v.sv_l[c] += v.fc * v.sv_b[c];
    return std::clamp(v.sv_l[c], -1.0f, 1.0f);
}

void ReadFrame(Voice& v, const Cursor& at, float x[2]) {
    float t = float(v.pos - std::floor(v.pos));
    for (int c = 0; c < 2; c++) {
        float now = Sample(v, at.f, c);
        x[c] = now + (Sample(v, at.next, c) - now) * t;
        if (v.lpf) x[c] = RunFilter(v, c, x[c]);
    }
}

// a mono 3D voice through one ear's filter (or its "Clear" level, or both: "Mixed"), from its recent samples
float EarOut(const Voice& v, const Ear& e, float pan, float norm, int mode) {
    auto at = [&](int back) { return v.ear[(v.ear_pos - 1 - back) % kEarLength]; };
    if (mode == 2) return pan * at(0);
    float acc = 0;
    int d = int(e.delay);
    for (int k = 0; k < kHrtfTaps; k++) acc += e.tap[k] * at(d + k);
    if (mode == 3)   // (the level delayed to the filter's strongest tap: out of line, the two would comb)
        return g_pos_mix * norm * acc + (1 - g_pos_mix) * pan * std::copysign(1.0f, e.tap[e.peak]) * at(d + e.peak);
    return acc;
}

// adds the voice to the lanes and its reverb share to the send
void MixVoice(Voice& v, float* out, float* send, uint32_t frames, int mode) {
    Place at = Locate(v);
    float g[2][kLanes], w[2];
    Levels(v, at, g, w);
    bool ears = at.on && v.channels == 1 && mode != 0 && g_hrtf_index;
    EarPair now, then;
    if (ears) {
        now = Ears(at.azimuth, at.elevation);
        then = v.has_hrtf ? v.hrtf : now;
    }
    if (!v.has_gain) {
        std::memcpy(v.gain, g, sizeof(g));
        v.send_gain[0] = w[0], v.send_gain[1] = w[1];
    }
    v.has_gain = true;
    float dg[2][kLanes], dw[2] = {(w[0] - v.send_gain[0]) / frames, (w[1] - v.send_gain[1]) / frames};
    for (int c = 0; c < 2; c++)
        for (uint32_t k = 0; k < kLanes; k++) dg[c][k] = (g[c][k] - v.gain[c][k]) / float(frames);
    double rate = double(v.freq ? v.freq : v.rate) / kRate;
    for (uint32_t i = 0; i < frames; i++) {
        std::optional<Cursor> cur = BufferFrame(v);
        if (!cur) break;
        float x[2];
        ReadFrame(v, *cur, x);
        v.send_gain[0] += dw[0], v.send_gain[1] += dw[1];
        send[i] += x[0] * v.send_gain[0] + x[1] * v.send_gain[1];  // the send leaves before the HRTF
        float* o = out + i * kLanes;
        if (ears) {
            // the filters change over the block: the old ones fade into the new
            v.ear[v.ear_pos % kEarLength] = x[0];
            v.ear_pos++;
            float t = float(i + 1) / frames, ear[2];
            for (int c = 0; c < 2; c++)
                ear[c] = EarOut(v, then.e[c], mode == 3 ? then.mixpan[c] : then.pan[c], then.norm, mode) * (1 - t) +
                         EarOut(v, now.e[c], mode == 3 ? now.mixpan[c] : now.pan[c], now.norm, mode) * t;
            for (uint32_t k = 0; k < kLanes; k++) {
                v.gain[0][k] += dg[0][k];
                bool left = k == kBinFrontLeft || k == kBinBackLeft || k == kBinCrosstalkFrontLeft ||
                            k == kBinCrosstalkBackLeft;
                bool right = k == kBinFrontRight || k == kBinBackRight || k == kBinCrosstalkFrontRight ||
                             k == kBinCrosstalkBackRight;
                o[k] += (left ? ear[0] : right ? ear[1] : x[0]) * v.gain[0][k];
            }
        } else {
            for (uint32_t k = 0; k < kLanes; k++) {
                v.gain[0][k] += dg[0][k];
                v.gain[1][k] += dg[1][k];
                o[k] += x[0] * v.gain[0][k] + x[1] * v.gain[1][k];
            }
        }
        uint32_t before = uint32_t(v.pos);
        v.pos += rate;
        uint32_t after = uint32_t(v.pos);
        if (after != before)
            for (auto& [at, ev] : v.notify)
                if (at > before && at <= after) Reached(ev);
    }
    if (!v.playing && v.notify_stop) Reached(v.notify_stop);
    std::memcpy(v.gain, g, sizeof(g));
    v.send_gain[0] = w[0], v.send_gain[1] = w[1];
    if (ears) {
        v.hrtf = now;
        v.has_hrtf = true;
    }
}

// 48 kHz: the voices, the reverb, the 3D submixes onto the speakers' pairs (the crosstalk effect first, if asked); stereo folds
// the centre and the back pair
// into the front (and drops the sub)
void Render(float* out, uint32_t frames) {
    MixPanTick(frames);
    static std::vector<float> lanes, send;
    lanes.assign(size_t(frames) * kLanes, 0.0f);
    send.assign(frames, 0.0f);
    {
        std::lock_guard<std::recursive_mutex> lock(g_lock);
        int mode = g_positional.load();
        for (auto& [obj, v] : g_voices)
            if (v.playing) MixVoice(v, lanes.data(), send.data(), frames, mode);
        static const bool reverb = !(std::getenv("NGB_DSOUND_REVERB") && *std::getenv("NGB_DSOUND_REVERB") == '0');
        if (reverb) g_reverb.Run(send.data(), lanes.data(), frames);
    }
    static const bool xtalk = std::getenv("NGB_DSOUND_XTALK") && *std::getenv("NGB_DSOUND_XTALK") == '1';
    for (uint32_t i = 0; xtalk && g_positional.load() == 1 && i < frames; i++) {
        float* m = &lanes[size_t(i) * kLanes];
        g_front_feed.Run(m[kBinCrosstalkFrontLeft], m[kBinCrosstalkFrontRight]);
        g_back_feed.Run(m[kBinCrosstalkBackLeft], m[kBinCrosstalkBackRight]);
    }
    if (g_surround) {
        for (uint32_t i = 0; i < frames; i++) {
            const float* m = &lanes[size_t(i) * kLanes];
            float* o = out + size_t(i) * 6;
            o[0] = m[kBinFrontLeft] + m[kBinCrosstalkFrontLeft];
            o[1] = m[kBinFrontRight] + m[kBinCrosstalkFrontRight];
            o[2] = m[kBinCentre];
            o[3] = m[kBinSub];
            o[4] = m[kBinBackLeft] + m[kBinCrosstalkBackLeft];
            o[5] = m[kBinBackRight] + m[kBinCrosstalkBackRight];
            for (int k = 0; k < 6; k++) o[k] = std::clamp(o[k], -1.0f, 1.0f);
        }
        return;
    }
    for (uint32_t i = 0; i < frames; i++) {
        const float* m = &lanes[size_t(i) * kLanes];
        float c = 0.70710678f * m[kBinCentre];
        float l = m[kBinFrontLeft] + m[kBinBackLeft] + m[kBinCrosstalkFrontLeft] + m[kBinCrosstalkBackLeft] + c;
        float r = m[kBinFrontRight] + m[kBinBackRight] + m[kBinCrosstalkFrontRight] + m[kBinCrosstalkBackRight] + c;
        out[i * 2] = std::clamp(l, -1.0f, 1.0f);
        out[i * 2 + 1] = std::clamp(r, -1.0f, 1.0f);
    }
}

int Channels() { return g_surround ? 6 : 2; }

void WriteWav(const float* frames, uint32_t n) {
    if (!g_wav) return;
    std::vector<int16_t> s(size_t(n) * Channels());
    for (size_t i = 0; i < s.size(); i++) s[i] = int16_t(std::lround(frames[i] * 32767.0f));
    std::fwrite(s.data(), 2, s.size(), g_wav);
    g_wav_frames += n;
    // the header's sizes kept current, so a killed run leaves a playable file
    uint32_t data = g_wav_frames * 2 * Channels(), riff = 36 + data;
    long at = std::ftell(g_wav);
    std::fseek(g_wav, 4, SEEK_SET);
    std::fwrite(&riff, 4, 1, g_wav);
    std::fseek(g_wav, 40, SEEK_SET);
    std::fwrite(&data, 4, 1, g_wav);
    std::fseek(g_wav, at, SEEK_SET);
}

// the device's pull; held by a menu, silence and the voices stay where they are
void Pull(float* out, uint32_t frames) {
    recomp_load_sample(RECOMP_LOAD_AUDIO);   // this thread's CPU time, for log.txt
    if (apu_host_paused) {
        std::memset(out, 0, size_t(frames) * Channels() * sizeof(float));
        return;
    }
    Render(out, frames);
    WriteWav(out, frames);
}

void SdlCallback(void*, Uint8* stream, int len) {
    Pull(reinterpret_cast<float*>(stream), uint32_t(len) / (4 * Channels()));
}

// Audio Output (setting 25): Stereo, 5.1, or Auto -- 5.1 when the default output device has six channels or more.
// NGB_AUDIO_CHANNELS=2|6 overrides it.
bool WantSurround() {
    if (const char* c = std::getenv("NGB_AUDIO_CHANNELS")) return std::atoi(c) >= 6;
    if (g_output_choice) return g_output_choice == 2;
    SDL_AudioSpec spec{};
    char* name = nullptr;
    if (SDL_GetDefaultAudioInfo(&name, &spec, 0) != 0) return false;
    if (name) SDL_free(name);
    return spec.channels >= 6;
}

void Start() {
    if (g_started) return;
    g_started = true;
    const char* e = std::getenv("NGB_AUDIO");
    bool device = !(e && *e == '0');
    if (device && !SDL_WasInit(SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        std::fprintf(stderr, "[DSOUND] SDL audio unavailable: %s\n", SDL_GetError());
        device = false;
    }
    SDL_AudioDeviceID dev = 0;
    if (device) {
        g_surround = WantSurround();
        SDL_AudioSpec want{}, have{};
        want.freq = kRate;
        want.format = AUDIO_F32SYS;
        want.channels = uint8_t(Channels());
        want.samples = 512;
        want.callback = SdlCallback;
        dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
        if (!dev && g_surround) {  // a device that takes no 5.1: stereo
            g_surround = false;
            want.channels = 2;
            dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
        }
        if (!dev) std::fprintf(stderr, "[DSOUND] cannot open an audio device: %s\n", SDL_GetError());
    } else if (const char* c = std::getenv("NGB_AUDIO_CHANNELS")) {
        g_surround = std::atoi(c) >= 6;
    }
    if (const char* w = std::getenv("NGB_DSOUND_WAV")) {
        g_wav = std::fopen(w, "wb");
        if (g_wav) {
            uint16_t ch = uint16_t(Channels()), block = uint16_t(ch * 2);
            uint32_t rate = kRate, bytes = kRate * block, fmt = 16, zero = 0;
            uint16_t pcm = 1, bits = 16;
            std::fwrite("RIFF", 1, 4, g_wav);
            std::fwrite(&zero, 4, 1, g_wav);
            std::fwrite("WAVEfmt ", 1, 8, g_wav);
            std::fwrite(&fmt, 4, 1, g_wav);
            std::fwrite(&pcm, 2, 1, g_wav);
            std::fwrite(&ch, 2, 1, g_wav);
            std::fwrite(&rate, 4, 1, g_wav);
            std::fwrite(&bytes, 4, 1, g_wav);
            std::fwrite(&block, 2, 1, g_wav);
            std::fwrite(&bits, 2, 1, g_wav);
            std::fwrite("data", 1, 4, g_wav);
            std::fwrite(&zero, 4, 1, g_wav);
        }
    }
    if (dev) {
        SDL_PauseAudioDevice(dev, 0);
        std::fprintf(stderr, "[DSOUND] host mixer: %s, 48 kHz %s\n", SDL_GetCurrentAudioDriver(),
                     g_surround ? "5.1" : "stereo");
        return;
    }
    // no device: the voices still play out in time, for the title's code that waits on them
    std::fprintf(stderr, "[DSOUND] host mixer without a device (sound off)\n");
    std::thread([] {
        std::vector<float> buf(480 * 6);
        auto next = std::chrono::steady_clock::now();
        for (;;) {
            next += std::chrono::milliseconds(10);
            std::this_thread::sleep_until(next);
            Pull(buf.data(), 480);
        }
    }).detach();
}

// ── the calls ──

void ReadFormat(Voice& v, uint32_t wfx) {
    if (!wfx) return;
    v.tag = R16(wfx);
    v.channels = std::clamp<uint16_t>(R16(wfx + 2), 1, 2);
    v.rate = R32(wfx + 4);
    v.align = std::max<uint16_t>(R16(wfx + 12), 1);
    v.bits = R16(wfx + 14);
    if (v.tag != 1 && v.tag != 0x69) std::fprintf(stderr, "[DSOUND] format %X not supported\n", v.tag);
    if (R16(wfx + 2) > 2) std::fprintf(stderr, "[DSOUND] %u channels: the first two play\n", R16(wfx + 2));
}

// DSMIXBINS: the voice's bins and their volumes
void SetBins(Voice& v, uint32_t mixbins) {
    v.nbins = 0;  // none listed: front left and right
    if (!mixbins) return;
    uint32_t n = std::min<uint32_t>(R32(mixbins), 8), pairs = R32(mixbins + 4);
    for (uint32_t i = 0; i < n; i++) {
        v.bin[i] = R32(pairs + i * 8) & 31;
        v.bin_mb[v.bin[i]] = int32_t(R32(pairs + i * 8 + 4));
    }
    v.nbins = uint8_t(n);
}
void SetBinVolumes(Voice& v, uint32_t mixbins) {
    if (!mixbins) return;
    uint32_t n = std::min<uint32_t>(R32(mixbins), 32), pairs = R32(mixbins + 4);
    for (uint32_t i = 0; i < n; i++) v.bin_mb[R32(pairs + i * 8) & 31] = int32_t(R32(pairs + i * 8 + 4));
}
// 600 mB unless 3D or a mix-in or effect-in buffer
float DefaultHeadroom(uint32_t flags) { return flags & 0x382010 ? 1.0f : Db(-600); }

Voice* Find(uint32_t obj) {
    auto it = g_voices.find(obj);
    return it == g_voices.end() ? nullptr : &it->second;
}

uint32_t NewObject() {
    if (!g_free_objects.empty()) {
        uint32_t o = g_free_objects.back();
        g_free_objects.pop_back();
        return o;
    }
    return xbox_HeapAlloc(0x40, 16);
}

// a buffer's own memory, zeroed: the smallest released block it fits in, if there is one
uint32_t OwnedMemory(Voice& v, uint32_t bytes) {
    auto it = g_spare_memory.lower_bound(bytes);
    uint32_t va = 0, size = bytes;
    if (it != g_spare_memory.end()) {
        size = it->first;
        va = it->second;
        g_spare_memory.erase(it);
    } else {
        va = xbox_HeapAlloc(bytes, 16);
    }
    if (va) std::memset(HostPtr(va), 0, bytes);
    v.owned = va;
    v.owned_bytes = va ? size : 0;
    return va;
}

void SpareOwned(Voice& v) {
    if (v.owned) g_spare_memory.emplace(v.owned_bytes, v.owned);
    v.owned = v.owned_bytes = 0;
}

uint32_t NewBuffer(uint32_t desc, uint32_t out) {
    Start();
    uint32_t obj = NewObject();
    if (!obj) return 0x8007000Eu;  // E_OUTOFMEMORY
    std::memset(HostPtr(obj), 0, 0x40);
    Voice& v = g_voices[obj];
    v = Voice{};
    v.obj = obj;
    uint32_t flags = R32(desc + 4), bytes = R32(desc + 8);
    ReadFormat(v, R32(desc + 12));
    SetBins(v, R32(desc + 16));
    v.is3d = flags & 0x10;
    v.headroom = DefaultHeadroom(flags);
    if (bytes) {
        v.data = OwnedMemory(v, bytes);
        v.bytes = v.loop_len = bytes;
    }
    W32(out, obj);
    if (Log())
        std::fprintf(stderr, "[DSOUND] buffer %08X: flags %X, %u bytes, format %X %u ch %u Hz, %u bins\n", obj, flags,
                     bytes, v.tag, v.channels, v.rate, v.nbins);
    return 0;
}

// the effects image the title loads (dsstdfx.bin): its effects' descriptor, each segment at zeroed memory. The
// effects do not run; the I3DL2 reverb is the mixer's own (SetI3DL2Listener).
uint32_t EffectsImage(uint32_t image, uint32_t size, uint32_t out) {
    auto u32 = [&](size_t o) { return o + 4 <= size ? R32(image + uint32_t(o)) : 0u; };
    size_t t = 0x818 + size_t(u32(0x804) + u32(0x80C)) * 4;
    uint32_t count = std::min<uint32_t>(u32(t), 64);
    uint32_t desc = xbox_HeapAlloc(8 + std::max(count, 1u) * 32, 16);
    if (!desc) return 0x8007000Eu;
    W32(desc, count);
    W32(desc + 4, u32(t + 4));
    for (uint32_t i = 0; i < count; i++) {
        size_t m = t + 8 + i * 32;
        for (int k = 0; k < 4; k++) {
            uint32_t sz = std::max(u32(m + k * 8 + 4), 0x40u), seg = xbox_HeapAlloc(sz, 16);
            if (seg) std::memset(HostPtr(seg), 0, sz);
            W32(desc + 8 + i * 32 + k * 8, seg);
            W32(desc + 8 + i * 32 + k * 8 + 4, u32(m + k * 8 + 4));
        }
    }
    if (out) W32(out, desc);
    std::fprintf(stderr, "[DSOUND] effects image: %u effects (the reverb is the mixer's)\n", count);
    return 0;
}

void LogFirst(int fn, const uint32_t* a) {
    static bool seen[DSF_COUNT];
    if (!Log() || seen[fn]) return;
    seen[fn] = true;
    std::fprintf(stderr, "[DSOUND] first %s(%08X %08X %08X %08X)\n", ds_funcs[fn].name, a[0], a[1], a[2], a[3]);
}

}  // namespace

extern "C" void dsound_hle_set_positional(int mode) {
    static const bool once = [] {   // (read once: the audio thread reads g_pos_mix while the menus set the mode)
        float m = std::getenv("NGB_POS_MIX") ? float(std::atof(std::getenv("NGB_POS_MIX"))) : g_pos_mix;
        g_pos_mix = m >= 0 ? std::min(m, 1.0f) : 0.0f;   // (a NaN to 0, not into every voice)
        return true;
    }();
    (void)once;
    g_positional = std::clamp(mode, 0, 3);
}
// before the title's DirectSoundCreate: Audio Output (0 auto, 1 stereo, 2 5.1)
extern "C" void dsound_hle_set_output(int choice) { g_output_choice = std::clamp(choice, 0, 2); }
// the program's HRTF tables (DirectSoundUseFullHRTF's), for Positional Audio
extern "C" void dsound_hle_set_hrtf_tables(uint32_t index, uint32_t filters) {
    g_hrtf_index = index;
    g_hrtf_filters = filters;
}

extern "C" int dsound_hle_call(int fn, uint32_t va, const uint32_t* a, uint32_t* r) {
    (void)va;
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    LogFirst(fn, a);
    *r = 0;
    switch (fn) {
    // the device
    case DSF_DirectSoundCreate:
        Start();
        if (!g_ds) {
            g_ds = xbox_HeapAlloc(0x40, 16);
            std::memset(HostPtr(g_ds), 0, 0x40);
        }
        W32(a[1], g_ds);
        return 1;
    case DSF_DirectSoundDoWork: {
        // the notifications reached since the last: set here, on the title's thread, as the events are its own
        std::vector<uint32_t> ev;
        ev.swap(g_events);
        for (uint32_t e : ev) xbox_guest_set_event(e);
        return 1;
    }
    case DSF_IDirectSound_Release:
    case DSF_DirectSoundUseFullHRTF:
    case DSF_IDirectSound_CommitDeferredSettings:  // the settings apply as they come
    case DSF_IDirectSound_SetVelocity:             // no Doppler
        return 1;
    case DSF_IDirectSound_GetSpeakerConfig:
        // DSSPEAKER_SURROUND with DSSPEAKER_ENABLE_AC3 (the dashboard's Dolby Digital) on 5.1, else DSSPEAKER_STEREO
        W32(a[1], g_surround ? 0x2u | 0x10000u : 0u);
        return 1;
    case DSF_IDirectSound_DownloadEffectsImage:
        *r = EffectsImage(a[1], a[2], a[4]);
        return 1;
    case DSF_IDirectSound_SetMixBinHeadroom:
        if (a[1] < 32) g_bin_headroom[a[1]] = a[2] & 7;
        if (Log()) std::fprintf(stderr, "[DSOUND] mix bin %u headroom %u\n", a[1], a[2]);
        return 1;
    case DSF_IDirectSound_SetPosition:
        for (int i = 0; i < 3; i++) g_listener[i] = F32(a[1 + i]);
        return 1;
    case DSF_IDirectSound_SetI3DL2Listener: {
        if (!a[1]) return 1;
        float p[12];
        for (int i = 0; i < 12; i++) {
            uint32_t x = R32(a[1] + i * 4);
            p[i] = i < 2 || i == 5 || i == 7 ? float(int32_t(x)) : F32(x);
        }
        if (Log())
            std::fprintf(stderr, "[DSOUND] reverb: room %.0f, reflections %.0f after %.3f s, reverb %.0f after %.3f s, "
                         "decay %.2f s\n", p[0], p[5], p[6], p[7], p[8], p[3]);
        g_reverb.Set(p);
        return 1;
    }
    case DSF_IDirectSound_CreateSoundBuffer:
        *r = NewBuffer(a[1], a[2]);
        return 1;
    default:
        break;
    }

    // the buffer's calls
    Voice* vp = Find(a[0]);
    if (!vp) {
        *r = kEFail;
        return 1;
    }
    Voice& v = *vp;
    switch (fn) {
    case DSF_IDirectSoundBuffer_Release:
        if (--v.refs > 0) {
            *r = uint32_t(v.refs);
            return 1;
        }
        SpareOwned(v);
        g_free_objects.push_back(v.obj);
        g_voices.erase(v.obj);
        return 1;
    case DSF_IDirectSoundBuffer_Unlock:
        return 1;
    case DSF_IDirectSoundBuffer_SetBufferData:
        if (a[1] == v.data && a[2] == v.bytes) return 1;
        if (v.owned && a[1] != v.owned) SpareOwned(v);  // the title's own memory, or a new size: ours is unused
        v.data = a[1];
        if (!a[1] && a[2]) v.data = OwnedMemory(v, a[2]);  // a size and no data
        v.bytes = v.loop_len = a[2];
        v.loop_start = 0;
        v.pos = 0;
        v.block = -1;
        return 1;
    case DSF_IDirectSoundBuffer_Play:
        v.playing = true;
        v.looping = a[3] & 1;    // DSBPLAY_LOOPING
        if (a[3] & 2) v.pos = 0;  // DSBPLAY_FROMSTART
        return 1;
    case DSF_IDirectSoundBuffer_Stop:
        if (v.playing && v.notify_stop) g_events.push_back(v.notify_stop);
        v.playing = false;
        return 1;
    case DSF_IDirectSoundBuffer_SetNotificationPositions: {
        // DSBPOSITIONNOTIFY: dwOffset, hEventNotify
        v.notify.clear();
        v.notify_stop = 0;
        for (uint32_t i = 0; i < std::min<uint32_t>(a[1], 64); i++) {
            uint32_t off = R32(a[2] + i * 8), ev = R32(a[2] + i * 8 + 4);
            if (off == 0xFFFFFFFFu) v.notify_stop = ev;
            else v.notify.push_back({FramesOf(v, off), ev});
        }
        if (Log()) std::fprintf(stderr, "[DSOUND] %08X: %u notifications\n", v.obj, a[1]);
        return 1;
    }
    case DSF_IDirectSoundBuffer_StopEx:
        // a[1..2] the time to stop at (now, here). DSBSTOPEX_RELEASEWAVEFORM (2): out of the loop, on to the end;
        // otherwise at once (DSBSTOPEX_ENVELOPE's release has no envelope to play here)
        if (a[3] & 2) v.looping = false;
        else v.playing = false;
        return 1;
    case DSF_IDirectSoundBuffer_SetLoopRegion:
        if (a[1] > v.bytes || a[2] > v.bytes - a[1]) {
            *r = kDsErrInvalidCall;
            return 1;
        }
        v.loop_start = a[1];
        v.loop_len = a[2] ? a[2] : v.bytes - a[1];
        return 1;
    case DSF_IDirectSoundBuffer_GetStatus:
        W32(a[1], (v.playing ? 1u : 0u) | (v.playing && v.looping ? 4u : 0u));
        return 1;
    case DSF_IDirectSoundBuffer_GetCurrentPosition: {
        uint32_t p = BytesOf(v, uint32_t(v.pos));
        if (a[1]) W32(a[1], p);
        if (a[2]) W32(a[2], v.bytes ? (p + BytesOf(v, kAdpcmFrames * 4)) % v.bytes : 0);
        return 1;
    }
    case DSF_IDirectSoundBuffer_SetCurrentPosition:
        v.pos = FramesOf(v, a[1]);
        return 1;
    case DSF_IDirectSoundBuffer_Lock: {
        uint32_t off = a[1], n = a[2];
        if (Log() && std::getenv("NGB_DSOUND_LOG")[0] == '3') {
            using namespace std::chrono;
            static auto t0 = steady_clock::now();
            std::fprintf(stderr, "[DSOUND] %08X lock %u+%u (flags %X), playing at %u, %.3f s\n", v.obj, off, n, a[7],
                         BytesOf(v, uint32_t(v.pos)), duration<double>(steady_clock::now() - t0).count());
        }
        if (a[7] & 2) n = v.bytes;  // DSBLOCK_ENTIREBUFFER
        if (a[7] & 1) off = BytesOf(v, uint32_t(v.pos));  // DSBLOCK_FROMWRITECURSOR
        if (!v.bytes) {
            *r = kEFail;
            return 1;
        }
        off %= v.bytes;
        n = std::min(n, v.bytes);
        uint32_t first = std::min(n, v.bytes - off);
        W32(a[3], v.data + off);
        W32(a[4], first);
        if (a[5]) W32(a[5], n > first ? v.data : 0);
        if (a[6]) W32(a[6], n > first ? n - first : 0);
        return 1;
    }
    case DSF_IDirectSoundBuffer_SetVolume:
        v.volume = Db(int32_t(a[1]));
        if (Log() > 0 && std::getenv("NGB_DSOUND_LOG")[0] == '2')
            std::fprintf(stderr, "[DSOUND] %08X volume %d\n", v.obj, int32_t(a[1]));
        return 1;
    case DSF_IDirectSoundBuffer_SetFrequency:
        v.freq = a[1];
        return 1;
    case DSF_IDirectSoundBuffer_SetFilter: {
        // DSFILTERDESC: dwMode, dwQCoefficient, adwCoefficients[4]. DLS2 mode (1): the low pass's cutoff and
        // resonance in the chip's own units, as the library writes them to the voice.
        uint32_t d = a[1];
        uint32_t mode = d ? R32(d) : 0;
        v.lpf = mode == 1;
        if (v.lpf) {
            int16_t fc = int16_t(R32(d + 8));
            uint16_t q = uint16_t(R32(d + 12));
            v.fc = std::clamp(std::pow(2.0f, fc / 4096.0f), 0.003906f, 1.0f);
            v.q = std::clamp(q / 32768.0f, 0.079407f, 1.0f);
            v.qn = std::sqrt(v.q / 2.0f + 0.01f);
        }
        if (Log()) std::fprintf(stderr, "[DSOUND] %08X filter mode %u: %08X %08X\n", v.obj, mode,
                                d ? R32(d + 8) : 0, d ? R32(d + 12) : 0);
        return 1;
    }
    case DSF_IDirectSoundBuffer_SetHeadroom:
        v.headroom = Db(-int32_t(a[1]));
        return 1;
    case DSF_IDirectSoundBuffer_SetMixBinVolumes:
        SetBinVolumes(v, a[1]);
        return 1;
    case DSF_IDirectSoundBuffer_SetMixBins:
        SetBins(v, a[1]);
        if (Log()) {
            std::fprintf(stderr, "[DSOUND] %08X bins:", v.obj);
            for (int i = 0; i < v.nbins; i++) std::fprintf(stderr, " %u@%d", v.bin[i], v.bin_mb[v.bin[i]]);
            std::fprintf(stderr, "\n");
        }
        return 1;
    case DSF_IDirectSoundBuffer_SetMaxDistance:
        v.max_dist = F32(a[1]);
        return 1;
    case DSF_IDirectSoundBuffer_SetMinDistance:
        v.min_dist = std::max(F32(a[1]), 1e-6f);
        return 1;
    case DSF_IDirectSoundBuffer_SetPosition:
        for (int i = 0; i < 3; i++) v.p[i] = F32(a[1 + i]);
        return 1;
    case DSF_IDirectSoundBuffer_SetRolloffFactor:
        v.rolloff = F32(a[1]);
        return 1;
    case DSF_IDirectSoundBuffer_SetVelocity:       // no Doppler
    case DSF_IDirectSoundBuffer_SetDistanceFactor:  // the units: Doppler only
        return 1;
    default:
        return 0;
    }
}
