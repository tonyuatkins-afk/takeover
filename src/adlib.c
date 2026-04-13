/*
 * adlib.c - AdLib/OPL2 FM synthesis audio driver
 *
 * Programs the OPL2 chip via ports 388h (address) and 389h (data).
 * Instruments defined as 11-byte patches. Background tracks are
 * arrays of note events advanced by the BIOS tick counter.
 *
 * All patch and track data is static const __far (zero DGROUP cost).
 * Only ~12 bytes of playback state live in DGROUP.
 */

#include "adlib.h"
#include <conio.h>
#include <dos.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* OPL2 register write with required delays                            */
/* ------------------------------------------------------------------ */

static void opl_write(unsigned char reg, unsigned char val)
{
    int i;
    outp(0x388, reg);
    for (i = 0; i < 6; i++) inp(0x388);    /* 3.3us delay */
    outp(0x389, val);
    for (i = 0; i < 35; i++) inp(0x388);   /* 23us delay */
}

/* ------------------------------------------------------------------ */
/* OPL2 operator offset table (channel 0-8 -> register offset)         */
/* ------------------------------------------------------------------ */

static const unsigned char __far op_off[9] = {
    0x00, 0x01, 0x02, 0x08, 0x09, 0x0A, 0x10, 0x11, 0x12
};

/* ------------------------------------------------------------------ */
/* Instrument patch format (11 bytes per patch)                        */
/* ------------------------------------------------------------------ */

typedef struct {
    unsigned char mod_char;    /* reg 0x20: AM/VIB/EG/KSR/MULTI */
    unsigned char car_char;    /* reg 0x23 */
    unsigned char mod_level;   /* reg 0x40: KSL/TOTAL_LEVEL */
    unsigned char car_level;   /* reg 0x43 */
    unsigned char mod_ad;      /* reg 0x60: ATTACK/DECAY */
    unsigned char car_ad;      /* reg 0x63 */
    unsigned char mod_sr;      /* reg 0x80: SUSTAIN/RELEASE */
    unsigned char car_sr;      /* reg 0x83 */
    unsigned char mod_wave;    /* reg 0xE0: WAVEFORM */
    unsigned char car_wave;    /* reg 0xE3 */
    unsigned char feedback;    /* reg 0xC0: FEEDBACK/CONNECTION */
} adlib_patch_t;

/* ------------------------------------------------------------------ */
/* Instrument definitions (const __far, zero DGROUP)                   */
/* ------------------------------------------------------------------ */

/* Deep sustained drone/pad */
static const adlib_patch_t __far patch_drone = {
    0x01, 0x01,   /* mod/car: sustained, multiply 1 */
    0x1F, 0x00,   /* mod quiet, car full volume */
    0xF0, 0xF0,   /* slow attack, no decay */
    0x00, 0x00,   /* full sustain, no release */
    0x00, 0x00,   /* sine wave both */
    0x06          /* feedback 3, additive */
};

/* Metallic bell accent */
static const adlib_patch_t __far patch_bell = {
    0x01, 0x11,   /* mod: multiply 1, car: multiply 1 + vibrato */
    0x0A, 0x00,   /* mod slightly attenuated, car full */
    0xF4, 0xF2,   /* fast attack, some decay */
    0x77, 0x75,   /* moderate sustain, moderate release */
    0x00, 0x00,   /* sine wave */
    0x02          /* feedback 1, FM */
};

/* Low bass tone */
static const adlib_patch_t __far patch_bass = {
    0x01, 0x01,   /* multiply 1 both */
    0x10, 0x00,   /* mod attenuated, car full */
    0xF0, 0xF0,   /* slow attack */
    0x11, 0x11,   /* high sustain, slow release */
    0x00, 0x00,   /* sine wave */
    0x04          /* feedback 2, FM */
};

/* Load a patch into a channel (0-8) */
static void load_patch(int ch, const adlib_patch_t __far *p)
{
    unsigned char off = op_off[ch];

    opl_write(0x20 + off, p->mod_char);
    opl_write(0x23 + off, p->car_char);
    opl_write(0x40 + off, p->mod_level);
    opl_write(0x43 + off, p->car_level);
    opl_write(0x60 + off, p->mod_ad);
    opl_write(0x63 + off, p->car_ad);
    opl_write(0x80 + off, p->mod_sr);
    opl_write(0x83 + off, p->car_sr);
    opl_write(0xE0 + off, p->mod_wave);
    opl_write(0xE3 + off, p->car_wave);
    opl_write(0xC0 + ch,  p->feedback);
}

/* ------------------------------------------------------------------ */
/* Note control                                                        */
/* ------------------------------------------------------------------ */

/* OPL2 F-number table for notes C through B (octave-independent).
 * F-number = frequency * 2^(20-octave) / 49716 */
static const unsigned int __far fnums[12] = {
    0x157, /* C  */
    0x16B, /* C# */
    0x181, /* D  */
    0x198, /* D# */
    0x1B0, /* E  */
    0x1CA, /* F  */
    0x1E5, /* F# */
    0x202, /* G  */
    0x220, /* G# */
    0x241, /* A  */
    0x263, /* A# */
    0x287  /* B  */
};

static void note_on(int ch, int note, int octave)
{
    unsigned int fn;
    if (note < 0 || note > 11) return;
    if (octave < 0) octave = 0;
    if (octave > 7) octave = 7;
    fn = fnums[note];
    opl_write(0xA0 + ch, (unsigned char)(fn & 0xFF));
    opl_write(0xB0 + ch, (unsigned char)(0x20 | ((octave & 7) << 2) |
              ((fn >> 8) & 3)));
}

static void note_off(int ch)
{
    opl_write(0xB0 + ch, 0x00);
}

/* ------------------------------------------------------------------ */
/* Track format and data                                               */
/* ------------------------------------------------------------------ */

/* A track event: what to do on a channel at a point in time */
typedef struct {
    unsigned char channel;    /* 0-8, or 0xFF = loop back to start */
    unsigned char note;       /* 0-11 (C-B), or 0xFF = note off */
    unsigned char octave;     /* 0-7 */
    unsigned char patch_id;   /* 0=drone, 1=bell, 2=bass */
    unsigned char duration;   /* in BIOS ticks (55ms), 0 = immediate */
} adlib_evt_t;

/* Axiom Regent ambient track: ominous drone with periodic accents */
static const adlib_evt_t __far track_axiom[] = {
    /* Ch  Note   Oct  Patch  Dur */
    {  0,    0,    2,    2,    0 },  /* bass C2 */
    {  1,    0,    3,    0,   36 },  /* drone C3 -- hold 2 sec */
    {  2,    4,    4,    1,   18 },  /* bell E4 accent -- 1 sec */
    {  2, 0xFF,    0,    1,   18 },  /* bell off -- 1 sec silence */
    {  2,    7,    4,    1,   18 },  /* bell G4 accent */
    {  2, 0xFF,    0,    1,   36 },  /* bell off -- 2 sec */
    {  1,    5,    3,    0,   36 },  /* drone F3 -- 2 sec */
    {  2,    4,    5,    1,   18 },  /* bell E5 high accent */
    {  2, 0xFF,    0,    1,   54 },  /* bell off -- 3 sec */
    {  1,    0,    3,    0,   36 },  /* drone back to C3 */
    {  2,    3,    4,    1,   18 },  /* bell D#4 */
    {  2, 0xFF,    0,    1,   36 },  /* bell off -- 2 sec */
    { 0xFF,  0,    0,    0,    0 },  /* loop */
};

/* Hushline: quiet, unsettling, sparse. Whispered edits. */
static const adlib_evt_t __far track_hushline[] = {
    {  0,    7,    2,    0,   54 },  /* drone G2 -- 3 sec */
    {  1,   11,    4,    1,    9 },  /* bell B4 -- brief */
    {  1, 0xFF,    0,    1,   72 },  /* silence -- 4 sec */
    {  0,    5,    2,    0,   54 },  /* drone F2 */
    {  1,    8,    4,    1,    9 },  /* bell G#4 -- brief */
    {  1, 0xFF,    0,    1,   90 },  /* long silence -- 5 sec */
    {  0,    7,    2,    0,   36 },  /* drone G2 */
    {  1,    3,    5,    1,    9 },  /* bell D#5 high -- brief */
    {  1, 0xFF,    0,    1,   54 },  /* silence -- 3 sec */
    { 0xFF,  0,    0,    0,    0 },
};

/* Kestrel-9: tense, escalating, alert-like. Staccato. */
static const adlib_evt_t __far track_kestrel[] = {
    {  0,    0,    2,    2,    0 },  /* bass C2 */
    {  1,    0,    3,    0,   18 },  /* drone C3 -- 1 sec */
    {  2,    0,    5,    1,    5 },  /* bell C5 -- staccato */
    {  2, 0xFF,    0,    1,    5 },
    {  2,    0,    5,    1,    5 },  /* repeat */
    {  2, 0xFF,    0,    1,   18 },
    {  1,    1,    3,    0,   18 },  /* drone C#3 -- tension rise */
    {  2,    4,    5,    1,    5 },  /* bell E5 */
    {  2, 0xFF,    0,    1,   18 },
    {  1,    3,    3,    0,   18 },  /* drone D#3 */
    {  2,    7,    5,    1,    5 },  /* bell G5 */
    {  2, 0xFF,    0,    1,    5 },
    {  2,    7,    5,    1,    5 },  /* double tap */
    {  2, 0xFF,    0,    1,   36 },
    {  1,    0,    3,    0,   18 },  /* drone back to C3 */
    { 0xFF,  0,    0,    0,    0 },
};

/* Orchard Clerk: warm, gentle, almost pleasant. Major key. */
static const adlib_evt_t __far track_orchard[] = {
    {  0,    0,    3,    0,   36 },  /* drone C3 -- warm pad */
    {  1,    4,    4,    1,   18 },  /* bell E4 -- major third */
    {  1,    7,    4,    1,   18 },  /* bell G4 -- fifth */
    {  1,    0,    5,    1,   18 },  /* bell C5 -- octave */
    {  1, 0xFF,    0,    1,   36 },  /* rest -- 2 sec */
    {  0,    5,    3,    0,   36 },  /* drone F3 */
    {  1,    9,    4,    1,   18 },  /* bell A4 */
    {  1,    0,    5,    1,   18 },  /* bell C5 */
    {  1, 0xFF,    0,    1,   36 },  /* rest */
    {  0,    7,    3,    0,   36 },  /* drone G3 */
    {  1,   11,    4,    1,   18 },  /* bell B4 */
    {  1, 0xFF,    0,    1,   36 },  /* rest */
    { 0xFF,  0,    0,    0,    0 },
};

/* Cinder Mirror: literary, contemplative, minor key arpeggios */
static const adlib_evt_t __far track_cinder[] = {
    {  0,    9,    2,    0,   36 },  /* drone A2 -- minor key base */
    {  1,    0,    4,    1,   18 },  /* bell C4 */
    {  1,    4,    4,    1,   18 },  /* bell E4 */
    {  1,    9,    4,    1,   18 },  /* bell A4 -- minor arpeggio */
    {  1, 0xFF,    0,    1,   18 },
    {  0,    5,    2,    0,   36 },  /* drone F2 */
    {  1,    9,    4,    1,   18 },  /* bell A4 */
    {  1,    0,    5,    1,   18 },  /* bell C5 */
    {  1, 0xFF,    0,    1,   36 },
    {  0,    4,    2,    0,   36 },  /* drone E2 */
    {  1,    7,    4,    1,   18 },  /* bell G4 */
    {  1,   11,    4,    1,   18 },  /* bell B4 */
    {  1, 0xFF,    0,    1,   36 },
    {  0,    9,    2,    0,   36 },  /* back to A2 */
    {  1,    4,    5,    1,    9 },  /* bell E5 -- high, brief */
    {  1, 0xFF,    0,    1,   54 },  /* long rest */
    { 0xFF,  0,    0,    0,    0 },
};

/* Track table for name lookup */
typedef struct {
    const char __far *name;
    const adlib_evt_t __far *events;
} adlib_track_t;

static const char __far tn_axiom[]   = "drone_axiom";
static const char __far tn_hushline[] = "drone_hushline";
static const char __far tn_kestrel[] = "drone_kestrel";
static const char __far tn_orchard[] = "drone_orchard";
static const char __far tn_cinder[]  = "drone_cinder";

static const adlib_track_t __far tracks[] = {
    { tn_axiom,   track_axiom },
    { tn_hushline, track_hushline },
    { tn_kestrel, track_kestrel },
    { tn_orchard, track_orchard },
    { tn_cinder,  track_cinder },
    { NULL, NULL }
};

/* ------------------------------------------------------------------ */
/* Playback state (DGROUP: ~12 bytes)                                  */
/* ------------------------------------------------------------------ */

static int playing = 0;
static int muted = 0;
static const adlib_evt_t __far *cur_events = NULL;
static int cur_idx = 0;
static unsigned long next_tick = 0;

static const adlib_patch_t __far *patch_table[3];

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

void adlib_init(void)
{
    int i;

    /* Reset all OPL2 registers */
    for (i = 1; i <= 0xF5; i++)
        opl_write((unsigned char)i, 0);

    /* Enable waveform select */
    opl_write(0x01, 0x20);

    patch_table[0] = &patch_drone;
    patch_table[1] = &patch_bell;
    patch_table[2] = &patch_bass;

    playing = 0;
    cur_events = NULL;
}

void adlib_shutdown(void)
{
    int i;
    playing = 0;
    /* Silence all channels */
    for (i = 0; i < 9; i++)
        note_off(i);
}

int adlib_play_track(const char *name)
{
    int i;
    for (i = 0; tracks[i].name != NULL; i++) {
        /* Compare near string to far string */
        const char __far *fn = tracks[i].name;
        const char *n = name;
        int match = 1;
        while (*n && *fn) {
            if (*n != *fn) { match = 0; break; }
            n++;
            fn++;
        }
        if (match && *n == '\0' && *fn == '\0') {
            unsigned long far *tick =
                (unsigned long far *)MK_FP(0x0040, 0x006C);
            cur_events = tracks[i].events;
            cur_idx = 0;
            next_tick = *tick;
            playing = 1;
            /* Load initial patches and start first event immediately */
            adlib_tick();
            return 0;
        }
    }
    return -1;
}

void adlib_stop_track(void)
{
    int i;
    playing = 0;
    for (i = 0; i < 9; i++)
        note_off(i);
}

void adlib_tick(void)
{
    unsigned long far *tick;
    unsigned long now;
    const adlib_evt_t __far *evt;

    if (!playing || !cur_events) return;

    tick = (unsigned long far *)MK_FP(0x0040, 0x006C);
    now = *tick;

    if (now < next_tick) return;

    /* Process current event */
    evt = &cur_events[cur_idx];

    if (evt->channel == 0xFF) {
        /* Loop marker: restart track */
        cur_idx = 0;
        next_tick = now;
        return;
    }

    /* When muted, advance timing but silence all notes */
    if (muted) {
        note_off(evt->channel);
        next_tick = now + (unsigned long)evt->duration;
        cur_idx++;
        return;
    }

    /* Load patch and play note */
    if (evt->patch_id < 3) {
        load_patch(evt->channel, patch_table[evt->patch_id]);
    }

    if (evt->note == 0xFF) {
        note_off(evt->channel);
    } else {
        note_on(evt->channel, evt->note, evt->octave);
    }

    /* Schedule next event */
    next_tick = now + (unsigned long)evt->duration;
    cur_idx++;
}

void adlib_set_mute(int m)
{
    int i;
    muted = m;
    if (m) {
        for (i = 0; i < 9; i++)
            note_off(i);
    }
}

int adlib_get_mute(void) { return muted; }
