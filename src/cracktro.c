/*
 * cracktro.c - Demoscene cracktro easter egg
 *
 * Classic 1990s cracktro layout in Mode 13h (160x100 half-res):
 *   - Raster bars behind TAKEOVER logo (top third)
 *   - Starfield background (middle)
 *   - DYCP sine-bouncing scrolltext (bottom half)
 *   - 9-channel OPL2 chiptune
 *
 * All data const __far. Zero DGROUP for read-only content.
 */

#include "cracktro.h"
#include "vga13h.h"
#include "adlib.h"
#include "engine.h"     /* engine_delay */
#include "hwdetect.h"
#include "screen.h"     /* scr_kbhit, scr_getkey, KEY_ESC */
#include <dos.h>
#include <conio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Sine table (shared with other modules)                              */
/* ------------------------------------------------------------------ */

static const unsigned char __far sin_tab[256] = {
    128,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,
    176,179,182,185,188,190,193,196,198,201,203,206,208,211,213,215,
    218,220,222,224,226,228,230,232,234,235,237,238,240,241,243,244,
    245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255,
    255,255,255,255,254,254,254,253,253,252,251,250,250,249,248,246,
    245,244,243,241,240,238,237,235,234,232,230,228,226,224,222,220,
    218,215,213,211,208,206,203,201,198,196,193,190,188,185,182,179,
    176,173,170,167,165,162,158,155,152,149,146,143,140,137,134,131,
    128,125,122,119,116,113,110,107,104,101, 98, 94, 91, 89, 86, 83,
     80, 77, 74, 71, 68, 66, 63, 60, 58, 55, 53, 50, 48, 45, 43, 41,
     38, 36, 34, 32, 30, 28, 26, 24, 22, 21, 19, 18, 16, 15, 13, 12,
     11, 10,  8,  7,  6,  6,  5,  4,  3,  3,  2,  2,  2,  1,  1,  1,
      1,  1,  1,  1,  2,  2,  2,  3,  3,  4,  5,  6,  6,  7,  8, 10,
     11, 12, 13, 15, 16, 18, 19, 21, 22, 24, 26, 28, 30, 32, 34, 36,
     38, 41, 43, 45, 48, 50, 53, 55, 58, 60, 63, 66, 68, 71, 74, 77,
     80, 83, 86, 89, 91, 94, 98,101,104,107,110,113,116,119,122,125
};

/* ------------------------------------------------------------------ */
/* Logo font (reuse from title.c format, 8x8 pixel)                   */
/* ------------------------------------------------------------------ */

static const unsigned char __far logo_font[8][8] = {
    /* T */ { 0xFE, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10 },
    /* A */ { 0x38, 0x44, 0x82, 0x82, 0xFE, 0x82, 0x82, 0x82 },
    /* K */ { 0x82, 0x84, 0x88, 0xF0, 0x88, 0x84, 0x82, 0x82 },
    /* E */ { 0xFE, 0x80, 0x80, 0xFC, 0x80, 0x80, 0x80, 0xFE },
    /* O */ { 0x7C, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x7C },
    /* V */ { 0x82, 0x82, 0x82, 0x44, 0x44, 0x28, 0x28, 0x10 },
    /* E */ { 0xFE, 0x80, 0x80, 0xFC, 0x80, 0x80, 0x80, 0xFE },
    /* R */ { 0xFC, 0x82, 0x82, 0xFC, 0x88, 0x84, 0x82, 0x82 }
};

/* Minimal 8x8 ASCII font for scrolltext (chars 32-95) */
static const unsigned char __far mini_font[64][8] = {
    /* 32 SPACE */ {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* 33 !     */ {0x10,0x10,0x10,0x10,0x10,0x00,0x10,0x00},
    /* 34 "     */ {0x28,0x28,0x00,0x00,0x00,0x00,0x00,0x00},
    /* 35 #     */ {0x28,0x7C,0x28,0x28,0x7C,0x28,0x00,0x00},
    /* 36 $     */ {0x10,0x3C,0x50,0x38,0x14,0x78,0x10,0x00},
    /* 37 %     */ {0x60,0x64,0x08,0x10,0x20,0x4C,0x0C,0x00},
    /* 38 &     */ {0x30,0x48,0x30,0x50,0x4C,0x44,0x3A,0x00},
    /* 39 '     */ {0x10,0x10,0x00,0x00,0x00,0x00,0x00,0x00},
    /* 40 (     */ {0x08,0x10,0x20,0x20,0x20,0x10,0x08,0x00},
    /* 41 )     */ {0x20,0x10,0x08,0x08,0x08,0x10,0x20,0x00},
    /* 42 *     */ {0x00,0x28,0x10,0x7C,0x10,0x28,0x00,0x00},
    /* 43 +     */ {0x00,0x10,0x10,0x7C,0x10,0x10,0x00,0x00},
    /* 44 ,     */ {0x00,0x00,0x00,0x00,0x00,0x10,0x10,0x20},
    /* 45 -     */ {0x00,0x00,0x00,0x7C,0x00,0x00,0x00,0x00},
    /* 46 .     */ {0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00},
    /* 47 /     */ {0x00,0x04,0x08,0x10,0x20,0x40,0x00,0x00},
    /* 48 0     */ {0x38,0x44,0x4C,0x54,0x64,0x44,0x38,0x00},
    /* 49 1     */ {0x10,0x30,0x10,0x10,0x10,0x10,0x38,0x00},
    /* 50 2     */ {0x38,0x44,0x04,0x08,0x10,0x20,0x7C,0x00},
    /* 51 3     */ {0x38,0x44,0x04,0x18,0x04,0x44,0x38,0x00},
    /* 52 4     */ {0x08,0x18,0x28,0x48,0x7C,0x08,0x08,0x00},
    /* 53 5     */ {0x7C,0x40,0x78,0x04,0x04,0x44,0x38,0x00},
    /* 54 6     */ {0x18,0x20,0x40,0x78,0x44,0x44,0x38,0x00},
    /* 55 7     */ {0x7C,0x04,0x08,0x10,0x20,0x20,0x20,0x00},
    /* 56 8     */ {0x38,0x44,0x44,0x38,0x44,0x44,0x38,0x00},
    /* 57 9     */ {0x38,0x44,0x44,0x3C,0x04,0x08,0x30,0x00},
    /* 58 :     */ {0x00,0x00,0x10,0x00,0x00,0x10,0x00,0x00},
    /* 59 ;     */ {0x00,0x00,0x10,0x00,0x00,0x10,0x10,0x20},
    /* 60 <     */ {0x04,0x08,0x10,0x20,0x10,0x08,0x04,0x00},
    /* 61 =     */ {0x00,0x00,0x7C,0x00,0x7C,0x00,0x00,0x00},
    /* 62 >     */ {0x20,0x10,0x08,0x04,0x08,0x10,0x20,0x00},
    /* 63 ?     */ {0x38,0x44,0x04,0x08,0x10,0x00,0x10,0x00},
    /* 64 @     */ {0x38,0x44,0x5C,0x54,0x5C,0x40,0x38,0x00},
    /* 65 A     */ {0x38,0x44,0x44,0x7C,0x44,0x44,0x44,0x00},
    /* 66 B     */ {0x78,0x44,0x44,0x78,0x44,0x44,0x78,0x00},
    /* 67 C     */ {0x38,0x44,0x40,0x40,0x40,0x44,0x38,0x00},
    /* 68 D     */ {0x78,0x44,0x44,0x44,0x44,0x44,0x78,0x00},
    /* 69 E     */ {0x7C,0x40,0x40,0x78,0x40,0x40,0x7C,0x00},
    /* 70 F     */ {0x7C,0x40,0x40,0x78,0x40,0x40,0x40,0x00},
    /* 71 G     */ {0x38,0x44,0x40,0x5C,0x44,0x44,0x3C,0x00},
    /* 72 H     */ {0x44,0x44,0x44,0x7C,0x44,0x44,0x44,0x00},
    /* 73 I     */ {0x38,0x10,0x10,0x10,0x10,0x10,0x38,0x00},
    /* 74 J     */ {0x04,0x04,0x04,0x04,0x04,0x44,0x38,0x00},
    /* 75 K     */ {0x44,0x48,0x50,0x60,0x50,0x48,0x44,0x00},
    /* 76 L     */ {0x40,0x40,0x40,0x40,0x40,0x40,0x7C,0x00},
    /* 77 M     */ {0x44,0x6C,0x54,0x44,0x44,0x44,0x44,0x00},
    /* 78 N     */ {0x44,0x64,0x54,0x4C,0x44,0x44,0x44,0x00},
    /* 79 O     */ {0x38,0x44,0x44,0x44,0x44,0x44,0x38,0x00},
    /* 80 P     */ {0x78,0x44,0x44,0x78,0x40,0x40,0x40,0x00},
    /* 81 Q     */ {0x38,0x44,0x44,0x44,0x54,0x48,0x34,0x00},
    /* 82 R     */ {0x78,0x44,0x44,0x78,0x50,0x48,0x44,0x00},
    /* 83 S     */ {0x38,0x44,0x40,0x38,0x04,0x44,0x38,0x00},
    /* 84 T     */ {0x7C,0x10,0x10,0x10,0x10,0x10,0x10,0x00},
    /* 85 U     */ {0x44,0x44,0x44,0x44,0x44,0x44,0x38,0x00},
    /* 86 V     */ {0x44,0x44,0x44,0x44,0x28,0x28,0x10,0x00},
    /* 87 W     */ {0x44,0x44,0x44,0x54,0x54,0x6C,0x44,0x00},
    /* 88 X     */ {0x44,0x44,0x28,0x10,0x28,0x44,0x44,0x00},
    /* 89 Y     */ {0x44,0x44,0x28,0x10,0x10,0x10,0x10,0x00},
    /* 90 Z     */ {0x7C,0x04,0x08,0x10,0x20,0x40,0x7C,0x00},
    /* 91 [     */ {0x38,0x20,0x20,0x20,0x20,0x20,0x38,0x00},
    /* 92 \     */ {0x00,0x40,0x20,0x10,0x08,0x04,0x00,0x00},
    /* 93 ]     */ {0x38,0x08,0x08,0x08,0x08,0x08,0x38,0x00},
    /* 94 ^     */ {0x10,0x28,0x44,0x00,0x00,0x00,0x00,0x00},
    /* 95 _     */ {0x00,0x00,0x00,0x00,0x00,0x00,0x7C,0x00}
};

/* ------------------------------------------------------------------ */
/* Scrolltext                                                          */
/* ------------------------------------------------------------------ */

static const char __far scrolltext[] =
    "TAKEOVER - CRACKED BY BARELY BOOTING / TONYUATKINS-AFK ...    "
    "GREETINGS FLY OUT TO: ALL DOS GAME DEVS STILL KEEPING THE "
    "FLAME ALIVE ... THE DEMOSCENE MASSIVE ... OPENWATCOM CREW ... "
    "DOSBOX-X TEAM ... VOGONS FORUM DWELLERS ... AND YOU FOR "
    "PLAYING ALL FIVE SCENARIOS! ...    "
    "CODED IN C89 FOR THE INTEL 8088 - NO PROTECTED MODE, NO "
    "PROBLEM ...    "
    "REMEMBER: THE REAL AI TAKEOVER WAS THE FRIENDS WE MADE ALONG "
    "THE WAY ...    "
    "           --- BARELY BOOTING 2026 --- PRESS ESC TO EXIT "
    "---              ";

/* ------------------------------------------------------------------ */
/* Starfield                                                           */
/* ------------------------------------------------------------------ */

#define NUM_STARS 40

typedef struct {
    int x, y;
    unsigned char speed;
} star_t;

static star_t stars[NUM_STARS];

static unsigned int lcg_state = 12345u;

static unsigned int lcg_next(void)
{
    lcg_state = lcg_state * 1103u + 13849u;
    return lcg_state;
}

static void init_stars(void)
{
    int i;
    for (i = 0; i < NUM_STARS; i++) {
        stars[i].x = (int)(lcg_next() % 160u);
        stars[i].y = 15 + (int)(lcg_next() % 60u); /* rows 15-74 */
        stars[i].speed = (unsigned char)(1 + lcg_next() % 3u);
    }
}

/* ------------------------------------------------------------------ */
/* Chiptune: 9-channel OPL2 music (C minor, 10 bars at 120 BPM)       */
/* ------------------------------------------------------------------ */

/* Event format: absolute tick, channel, note, octave, on/off */
typedef struct {
    unsigned int tick;
    unsigned char channel;
    unsigned char note;     /* 0-11 or 0xFF=note-off */
    unsigned char octave;
} tune_evt_t;

/* Patch assignments: ch0-1=lead, ch2=counter, ch3-4=pads, ch5-6=bass,
 * ch7=arpeggio, ch8=percussion.
 * We reuse the existing patches: 0=drone, 1=bell, 2=bass */

/* 120 BPM = 2 beats/sec. BIOS tick = 18.2 Hz.
 * Quarter note = 9 ticks. Eighth = 4-5 ticks. */
#define Q  9    /* quarter */
#define E  4    /* eighth */
#define H  18   /* half */
#define W  36   /* whole */

static const tune_evt_t __far chiptune[] = {
    /* Bars 1-2: Bass + percussion intro */
    {  0, 5,  0, 2 },  /* bass C2 */
    {  0, 8,  0, 5 },  /* perc C5 */
    {  E, 8,0xFF,0 },
    {  Q, 8,  0, 5 },
    { Q+E,8,0xFF,0 },
    { H,  5,  7, 2 },  /* bass G2 */
    { H,  8,  0, 5 },
    {H+E, 8,0xFF,0 },
    {H+Q, 8,  0, 5 },
    {H+Q+E,8,0xFF,0 },

    /* Bar 2 */
    { W,  5,  8, 2 },  /* bass Ab2 */
    { W,  8,  0, 5 },
    {W+E, 8,0xFF,0 },
    {W+Q, 8,  0, 5 },
    {W+Q+E,8,0xFF,0 },
    {W+H, 5,  7, 2 },  /* bass G2 */
    {W+H, 8,  0, 5 },
    {W+H+E,8,0xFF,0 },

    /* Bars 3-4: Add pads (Cm - Ab) */
    { W*2,   3,  0, 3 },  /* pad C3 */
    { W*2,   4,  3, 3 },  /* pad Eb3 */
    { W*2,   5,  0, 2 },  /* bass C2 */
    { W*2+H, 5,  7, 2 },
    { W*3,   3,  8, 3 },  /* pad Ab3 */
    { W*3,   4,  0, 4 },  /* pad C4 */
    { W*3,   5,  8, 2 },  /* bass Ab2 */

    /* Bars 5-6: Lead melody enters */
    { W*4,   0,  0, 5 },  /* lead C5 */
    { W*4,   5,  0, 2 },  /* bass C2 */
    { W*4,   3,  0, 3 },  /* pad C3 */
    { W*4,   4,  3, 3 },  /* pad Eb3 */
    { W*4+Q, 0,  0, 5 },  /* C5 */
    { W*4+H, 0,  3, 5 },  /* Eb5 */
    { W*4+H+Q,0, 7, 5 },  /* G5 */
    { W*5,   0,  8, 5 },  /* Ab5 */
    { W*5,   5,  8, 2 },  /* bass Ab2 */
    { W*5,   3,  8, 3 },  /* pad Ab3 */
    { W*5+H, 0,  7, 5 },  /* G5 */

    /* Bars 7-8: Full arrangement + arpeggio */
    { W*6,   0,  0, 5 },  /* C5 */
    { W*6,   5,  3, 2 },  /* bass Eb2 */
    { W*6,   3,  3, 3 },  /* pad Eb3 */
    { W*6,   4,  7, 3 },  /* pad G3 */
    { W*6,   7,  0, 4 },  /* arp C4 */
    { W*6+E, 7,  3, 4 },  /* arp Eb4 */
    { W*6+Q, 0,  2, 5 },  /* D5 */
    { W*6+Q, 7,  7, 4 },  /* arp G4 */
    { W*6+Q+E,7, 0, 5 },  /* arp C5 */
    { W*6+H, 0,  3, 5 },  /* Eb5 */
    { W*6+H, 7,  3, 4 },  /* arp Eb4 */
    { W*6+H+E,7, 7, 4 },
    { W*6+H+Q,0, 5, 5 },  /* F5 */
    { W*6+H+Q,7, 0, 5 },
    { W*7,   0,  7, 5 },  /* G5 - whole note resolve */
    { W*7,   5,  7, 2 },  /* bass G2 */
    { W*7,   3,  7, 3 },  /* pad G3 */
    { W*7,   4, 11, 3 },  /* pad B3 */
    { W*7,   7,  7, 4 },  /* arp G4 */
    { W*7+E, 7, 11, 4 },
    { W*7+Q, 7,  2, 5 },
    { W*7+Q+E,7, 7, 4 },
    { W*7+H, 7, 11, 4 },
    { W*7+H+E,7, 2, 5 },

    /* Bars 9-10: Repeat bars 5-6 with counter-melody */
    { W*8,   0,  0, 5 },  /* lead C5 */
    { W*8,   2,  7, 4 },  /* counter G4 */
    { W*8,   5,  0, 2 },
    { W*8+Q, 0,  0, 5 },
    { W*8+H, 0,  3, 5 },  /* Eb5 */
    { W*8+H, 2,  3, 4 },  /* counter Eb4 */
    { W*8+H+Q,0, 7, 5 },  /* G5 */
    { W*9,   0,  8, 5 },  /* Ab5 */
    { W*9,   2,  0, 5 },  /* counter C5 */
    { W*9,   5,  8, 2 },
    { W*9+H, 0,  7, 5 },  /* G5 */
    { W*9+H, 2,0xFF,0 },  /* counter off */

    /* End marker: loop back */
    { W*10,  0,0xFF,0 },  /* silence all */
    { W*10,  2,0xFF,0 },
    { W*10,  3,0xFF,0 },
    { W*10,  4,0xFF,0 },
    { W*10,  5,0xFF,0 },
    { W*10,  7,0xFF,0 },
    { W*10,  8,0xFF,0 },
    { 0xFFFF,0, 0,  0 }   /* sentinel: loop */
};

#define TUNE_LOOP_TICKS (W * 10)

/* ------------------------------------------------------------------ */
/* Palette                                                             */
/* ------------------------------------------------------------------ */

static unsigned char __far cracktro_pal[768];

static void setup_palette(void)
{
    int i;
    /* Index 0: black (background) */
    cracktro_pal[0] = 0; cracktro_pal[1] = 0; cracktro_pal[2] = 0;

    /* 1-32: raster bar gradient (blue -> cyan -> white -> cyan -> blue) */
    for (i = 1; i <= 16; i++) {
        unsigned char v = (unsigned char)((i - 1) * 4);
        cracktro_pal[i * 3 + 0] = (unsigned char)(v / 4);
        cracktro_pal[i * 3 + 1] = (unsigned char)(v / 2);
        cracktro_pal[i * 3 + 2] = v;
    }
    for (i = 17; i <= 32; i++) {
        unsigned char v = (unsigned char)((32 - i) * 4);
        cracktro_pal[i * 3 + 0] = (unsigned char)(v / 4);
        cracktro_pal[i * 3 + 1] = (unsigned char)(v / 2);
        cracktro_pal[i * 3 + 2] = v;
    }

    /* 33-40: star brightness */
    for (i = 33; i <= 40; i++) {
        unsigned char v = (unsigned char)((i - 33) * 8 + 7);
        cracktro_pal[i * 3 + 0] = v;
        cracktro_pal[i * 3 + 1] = v;
        cracktro_pal[i * 3 + 2] = v;
    }

    /* 240: logo color (bright green) */
    cracktro_pal[240 * 3 + 0] = 10;
    cracktro_pal[240 * 3 + 1] = 63;
    cracktro_pal[240 * 3 + 2] = 20;

    /* 250: scrolltext color (bright cyan) */
    cracktro_pal[250 * 3 + 0] = 20;
    cracktro_pal[250 * 3 + 1] = 63;
    cracktro_pal[250 * 3 + 2] = 63;

    /* 255: white */
    cracktro_pal[255 * 3 + 0] = 63;
    cracktro_pal[255 * 3 + 1] = 63;
    cracktro_pal[255 * 3 + 2] = 63;
}

/* ------------------------------------------------------------------ */
/* Rendering helpers                                                   */
/* ------------------------------------------------------------------ */

static void render_raster_bars(unsigned char far *fb, unsigned int frame)
{
    int y, x;
    /* Top 30 rows: horizontal color bands */
    for (y = 0; y < 30; y++) {
        unsigned int row = (unsigned int)(y * 2) * 320u;
        unsigned int row2 = row + 320u;
        unsigned char color = (unsigned char)(
            1 + (((unsigned int)y + frame) & 31u));
        for (x = 0; x < 160; x++) {
            unsigned int px = (unsigned int)(x << 1);
            fb[row + px] = color;
            fb[row + px + 1] = color;
            fb[row2 + px] = color;
            fb[row2 + px + 1] = color;
        }
    }
}

static void render_logo(unsigned char far *fb)
{
    /* Draw logo centered on top of raster bars */
    int ch, row, bit;
    int base_x = (160 - 8 * 9) / 2; /* 8 chars * (8+1 spacing) */
    int base_y = 10;

    for (ch = 0; ch < 8; ch++) {
        int cx = base_x + ch * 9;
        for (row = 0; row < 8; row++) {
            unsigned char bits = logo_font[ch][row];
            int cy = base_y + row;
            for (bit = 0; bit < 8; bit++) {
                if (bits & (0x80 >> bit)) {
                    int px = cx + bit;
                    if (px >= 0 && px < 160 && cy >= 0 && cy < 100) {
                        unsigned int off1 = (unsigned int)(cy * 2) * 320u
                                          + (unsigned int)(px * 2);
                        unsigned int off2 = off1 + 320u;
                        fb[off1] = 240;
                        fb[off1 + 1] = 240;
                        if (off2 < 64000u) {
                            fb[off2] = 240;
                            fb[off2 + 1] = 240;
                        }
                    }
                }
            }
        }
    }
}

static void render_starfield(unsigned char far *fb, int frame)
{
    int i;
    (void)frame;
    for (i = 0; i < NUM_STARS; i++) {
        star_t *s = &stars[i];
        unsigned char bright = (unsigned char)(33 + s->speed * 2);

        /* Clear old position (will be overwritten anyway by bars/bg) */
        /* Draw star */
        if (s->y >= 15 && s->y < 90 && s->x >= 0 && s->x < 160) {
            unsigned int off = (unsigned int)(s->y * 2) * 320u
                             + (unsigned int)(s->x * 2);
            fb[off] = bright;
            fb[off + 1] = bright;
        }

        /* Move star */
        s->x -= s->speed;
        if (s->x < 0) {
            s->x = 159;
            s->y = 15 + (int)(lcg_next() % 60u);
            s->speed = (unsigned char)(1 + lcg_next() % 3u);
        }
    }
}

static void render_dycp(unsigned char far *fb, unsigned int frame,
                        unsigned int scroll_offset)
{
    /* DYCP: each visible character gets a sine-displaced Y position */
    int col;
    int text_len;
    const char __far *p = scrolltext;

    /* Count text length */
    text_len = 0;
    while (p[text_len]) text_len++;

    for (col = 0; col < 18; col++) {
        int char_idx = (scroll_offset / 8 + col) % text_len;
        int sub_pixel = scroll_offset % 8;
        unsigned char ch;
        int base_y;
        int cy_offset;
        int draw_x = col * 9 - sub_pixel;
        int row, bit;

        ch = (unsigned char)scrolltext[char_idx];
        if (ch < 32 || ch > 95) ch = 32;

        /* Sine-based Y bounce: +/- 12 pixels around baseline 60 */
        cy_offset = ((int)sin_tab[(unsigned char)(col * 14 + frame * 3)]
                     - 128) * 12 / 128;
        base_y = 60 + cy_offset;

        for (row = 0; row < 8; row++) {
            unsigned char bits = mini_font[ch - 32][row];
            int cy = base_y + row;
            if (cy < 0 || cy >= 100) continue;

            for (bit = 0; bit < 8; bit++) {
                if (bits & (0x80 >> bit)) {
                    int px = draw_x + bit;
                    if (px >= 0 && px < 160) {
                        unsigned int off =
                            (unsigned int)(cy * 2) * 320u +
                            (unsigned int)(px * 2);
                        unsigned int off2 = off + 320u;
                        fb[off] = 250;
                        fb[off + 1] = 250;
                        if (off2 < 64000u) {
                            fb[off2] = 250;
                            fb[off2 + 1] = 250;
                        }
                    }
                }
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* Music playback                                                      */
/* ------------------------------------------------------------------ */

/* OPL2 register write helper (same as opl_write in adlib.c,
 * but accessible from this module) */
static void opl_write_ext(unsigned char reg, unsigned char val)
{
    int i;
    outp(0x388, reg);
    for (i = 0; i < 6; i++) inp(0x388);
    outp(0x389, val);
    for (i = 0; i < 35; i++) inp(0x388);
}

static int tune_idx;
static unsigned int tune_tick;

static void tune_init(void)
{
    tune_idx = 0;
    tune_tick = 0;
}

static void tune_advance(void)
{
    static const unsigned int __far fnums[12] = {
        0x157,0x16B,0x181,0x198,0x1B0,0x1CA,
        0x1E5,0x202,0x220,0x241,0x263,0x287
    };

    while (chiptune[tune_idx].tick != 0xFFFF &&
           chiptune[tune_idx].tick <= tune_tick) {
        const tune_evt_t __far *e = &chiptune[tune_idx];
        if (e->note == 0xFF) {
            opl_write_ext(0xB0 + e->channel, 0x00);
        } else {
            unsigned int fn = fnums[e->note];
            opl_write_ext(0xA0 + e->channel,
                          (unsigned char)(fn & 0xFF));
            opl_write_ext(0xB0 + e->channel,
                          (unsigned char)(0x20 |
                           ((e->octave & 7) << 2) |
                           ((fn >> 8) & 3)));
        }
        tune_idx++;
    }

    if (chiptune[tune_idx].tick == 0xFFFF) {
        tune_idx = 0;
        tune_tick = 0;
    } else {
        tune_tick++;
    }
}

/* Set up instrument patches for the chiptune */
static void tune_setup_patches(void)
{
    int i;
    /* Bell-like patch for lead/arp/perc (channels 0,1,2,7,8) */
    static const unsigned char __far bell_regs[] = {
        0x01, 0x11, 0x0A, 0x00, 0xF4, 0xF2, 0x77, 0x75,
        0x00, 0x00, 0x02
    };
    /* Drone patch for pads (channels 3,4) */
    static const unsigned char __far drone_regs[] = {
        0x01, 0x01, 0x1F, 0x00, 0xF0, 0xF0, 0x00, 0x00,
        0x00, 0x00, 0x06
    };
    /* Bass patch for bass (channels 5,6) */
    static const unsigned char __far bass_regs[] = {
        0x01, 0x01, 0x10, 0x00, 0xF0, 0xF0, 0x11, 0x11,
        0x00, 0x00, 0x04
    };

    /* Operator offset table */
    static const unsigned char __far op_off[9] = {
        0x00,0x01,0x02,0x08,0x09,0x0A,0x10,0x11,0x12
    };

    for (i = 0; i < 9; i++) {
        const unsigned char __far *p;
        unsigned char off = op_off[i];

        if (i <= 2 || i == 7 || i == 8)
            p = bell_regs;
        else if (i <= 4)
            p = drone_regs;
        else
            p = bass_regs;

        opl_write_ext(0x20 + off, p[0]);
        opl_write_ext(0x23 + off, p[1]);
        opl_write_ext(0x40 + off, p[2]);
        opl_write_ext(0x43 + off, p[3]);
        opl_write_ext(0x60 + off, p[4]);
        opl_write_ext(0x63 + off, p[5]);
        opl_write_ext(0x80 + off, p[6]);
        opl_write_ext(0x83 + off, p[7]);
        opl_write_ext(0xE0 + off, p[8]);
        opl_write_ext(0xE3 + off, p[9]);
        opl_write_ext(0xC0 + i,   p[10]);
    }
}

/* ------------------------------------------------------------------ */
/* Main cracktro loop                                                  */
/* ------------------------------------------------------------------ */

void cracktro_run(void)
{
    unsigned char far *fb = (unsigned char far *)MK_FP(0xA000, 0x0000);
    unsigned int frame = 0;
    unsigned int scroll_offset = 0;
    unsigned long far *bios_tick;
    unsigned long last_tick, now;

    vga_enter_13h();
    setup_palette();

    /* Start black, fade in */
    {
        int i;
        outp(0x3C8, 0);
        for (i = 0; i < 768; i++)
            outp(0x3C9, 0);
    }

    init_stars();

    /* Set up OPL2 chiptune if AdLib present */
    if (g_hw.adlib) {
        /* Stop any existing music first */
        adlib_stop_track();
        tune_setup_patches();
        tune_init();
    }

    vga_fade_in(cracktro_pal, 16, 40);

    bios_tick = (unsigned long far *)MK_FP(0x0040, 0x006C);
    last_tick = *bios_tick;

    while (1) {
        /* Clear framebuffer (starfield area only) */
        {
            unsigned int y;
            for (y = 30; y < 100; y++) {
                unsigned int row = (unsigned int)(y * 2) * 320u;
                unsigned int row2 = row + 320u;
                unsigned int x;
                for (x = 0; x < 320; x++) {
                    fb[row + x] = 0;
                    fb[row2 + x] = 0;
                }
            }
        }

        /* Render layers */
        render_raster_bars(fb, frame);
        render_logo(fb);
        render_starfield(fb, frame);
        render_dycp(fb, frame, scroll_offset);

        vga_wait_vsync();

        /* Advance scroll (1 pixel per frame) */
        scroll_offset++;

        /* Advance music on each BIOS tick */
        now = *bios_tick;
        while (last_tick != now) {
            if (g_hw.adlib)
                tune_advance();
            last_tick++;
        }

        frame++;

        /* Check for exit */
        if (scr_kbhit()) {
            int key = scr_getkey();
            if (key == KEY_ESC || (key & 0xFF) == 0x1B)
                break;
        }
    }

    /* Silence music */
    if (g_hw.adlib) {
        int i;
        for (i = 0; i < 9; i++)
            opl_write_ext(0xB0 + i, 0x00);
    }

    vga_fade_out(16, 40);
    vga_leave_13h();
    scr_init();
}
