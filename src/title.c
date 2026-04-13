/*
 * title.c - Animated plasma title screen with TAKEOVER logo
 *
 * Renders a real-time plasma effect in VGA Mode 13h (320x200x256)
 * with the TAKEOVER logo overlaid. Uses a 256-byte sine lookup table
 * stored in a far segment (zero DGROUP cost).
 *
 * On 8088: renders at half resolution (160x100, doubled) for speed.
 * On 286+: renders at full 320x200.
 * CPU speed auto-detected via BIOS tick counter timing.
 */

#include "title.h"
#include "vga13h.h"
#include "engine.h"     /* engine_delay */
#include "screen.h"     /* scr_kbhit, scr_getkey */
#include <dos.h>
#include <conio.h>

/* ------------------------------------------------------------------ */
/* Sine lookup table (256 entries, maps to 0..255)                     */
/* Generated from: sin_tab[i] = 128 + 127 * sin(i * 2 * PI / 256)    */
/* Stored in far segment, zero DGROUP cost.                            */
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
/* Plasma palette: dark-to-green-to-cyan-to-white gradient             */
/* 256 entries * 3 bytes (R,G,B in 6-bit VGA DAC values 0-63)          */
/* Stored in far segment.                                              */
/* ------------------------------------------------------------------ */

static const unsigned char __far plasma_pal[768] = {
    /* 0-63: black to dark green */
     0, 0, 0,  0, 0, 0,  0, 1, 0,  0, 1, 0,  0, 2, 0,  0, 2, 0,
     0, 3, 0,  0, 3, 0,  0, 4, 0,  0, 4, 1,  0, 5, 1,  0, 5, 1,
     0, 6, 1,  0, 6, 1,  0, 7, 2,  0, 7, 2,  0, 8, 2,  0, 8, 2,
     0, 9, 2,  0, 9, 3,  0,10, 3,  0,10, 3,  0,11, 3,  0,11, 3,
     0,12, 4,  0,12, 4,  0,13, 4,  0,13, 4,  0,14, 4,  0,14, 5,
     0,15, 5,  0,15, 5,  1,16, 5,  1,16, 5,  1,17, 6,  1,17, 6,
     1,18, 6,  1,18, 6,  1,19, 6,  1,19, 7,  1,20, 7,  1,20, 7,
     1,21, 7,  1,21, 7,  1,22, 8,  1,22, 8,  1,23, 8,  1,23, 8,
     1,24, 8,  1,24, 9,  1,25, 9,  1,25, 9,  1,26, 9,  1,26, 9,
     2,27,10,  2,27,10,  2,28,10,  2,28,10,  2,29,10,  2,29,11,
     2,30,11,  2,30,11,  2,31,11,  2,31,11,
    /* 64-127: green to cyan */
     2,32,12,  2,33,13,  3,34,14,  3,35,15,  3,36,16,  3,37,17,
     3,38,18,  4,39,19,  4,40,20,  4,41,21,  4,42,22,  5,43,23,
     5,44,24,  5,45,25,  5,46,26,  6,47,27,  6,48,28,  6,49,29,
     6,50,30,  7,51,31,  7,52,32,  7,53,33,  8,54,34,  8,55,35,
     8,56,36,  9,57,37,  9,58,38,  9,59,39, 10,60,40, 10,61,41,
    10,62,42, 11,63,43, 11,63,44, 12,63,45, 12,63,46, 13,63,47,
    13,63,48, 14,63,49, 14,63,50, 15,63,51, 15,63,52, 16,63,53,
    16,63,54, 17,63,55, 17,63,56, 18,63,57, 18,63,58, 19,63,59,
    19,63,60, 20,63,61, 20,63,62, 21,63,63, 22,63,63, 23,63,63,
    24,63,63, 25,63,63, 26,63,63, 27,63,63, 28,63,63, 29,63,63,
    30,63,63, 31,63,63, 32,63,63, 33,63,63,
    /* 128-191: cyan to white */
    34,63,63, 35,63,63, 36,63,63, 37,63,63, 38,63,63, 39,63,63,
    40,63,63, 41,63,63, 42,63,63, 43,63,63, 44,63,63, 45,63,63,
    46,63,63, 47,63,63, 48,63,63, 49,63,63, 50,63,63, 51,63,63,
    52,63,63, 53,63,63, 54,63,63, 55,63,63, 56,63,63, 57,63,63,
    58,63,63, 59,63,63, 60,63,63, 61,63,63, 62,63,63, 63,63,63,
    63,63,63, 63,63,63, 63,63,63, 63,63,63, 63,63,63, 63,63,63,
    63,63,63, 63,63,63, 63,63,63, 63,63,63, 63,63,63, 63,63,63,
    63,63,63, 63,63,63, 63,63,63, 63,63,63, 63,63,63, 63,63,63,
    63,63,63, 63,63,63, 63,63,63, 63,63,63, 63,63,63, 63,63,63,
    63,63,63, 63,63,63, 63,63,63, 63,63,63, 63,63,63, 63,63,63,
    63,63,63, 63,63,63, 63,63,63, 63,63,63,
    /* 192-255: white back to dark green */
    63,63,63, 62,63,62, 60,62,60, 58,61,58, 56,60,56, 54,59,54,
    52,58,52, 50,57,50, 48,56,48, 46,55,46, 44,54,44, 42,53,42,
    40,52,40, 38,51,38, 36,50,36, 34,49,34, 32,48,32, 30,47,30,
    28,46,28, 26,45,26, 24,44,24, 22,43,22, 20,42,20, 18,41,18,
    16,40,16, 14,39,14, 12,38,12, 10,37,10,  8,36, 8,  6,35, 6,
     4,34, 4,  3,33, 3,  2,32, 2,  2,31, 2,  1,30, 1,  1,29, 1,
     1,28, 1,  1,27, 1,  0,26, 0,  0,25, 0,  0,24, 0,  0,23, 0,
     0,22, 0,  0,21, 0,  0,20, 0,  0,19, 0,  0,18, 0,  0,17, 0,
     0,16, 0,  0,15, 0,  0,14, 0,  0,13, 0,  0,12, 0,  0,11, 0,
     0,10, 0,  0, 9, 0,  0, 8, 0,  0, 7, 0,  0, 6, 0,  0, 5, 0,
     0, 4, 0,  0, 3, 0,  0, 2, 0,  0, 1, 0
};

/* ------------------------------------------------------------------ */
/* Block-letter TAKEOVER logo (8x8 pixel font, 8 chars)                */
/* Each byte = one row of pixels, MSB = leftmost.                      */
/* Stored in far segment.                                              */
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

/* ------------------------------------------------------------------ */
/* Title screen implementation                                         */
/* ------------------------------------------------------------------ */

/* Detect if CPU is fast enough for full-resolution plasma.
 * Times a tight loop against the BIOS tick counter. */
static int cpu_is_fast(void)
{
    unsigned long far *tick = (unsigned long far *)MK_FP(0x0040, 0x006C);
    unsigned long start;
    unsigned int i;
    volatile unsigned int dummy = 0;

    /* Wait for tick boundary */
    start = *tick;
    while (*tick == start) { }

    /* Count iterations in one tick (~55ms) */
    start = *tick;
    for (i = 0; i < 30000u; i++) {
        dummy += i;
    }
    /* On a 4.77 MHz 8088, 30000 iterations takes several ticks.
     * On a 286+, it finishes within one tick. */
    return (*tick == start) ? 1 : 0;
}

/* Draw the logo at a given position with color. Scale = pixel multiplier. */
static void draw_logo(int base_x, int base_y, int scale, unsigned char color)
{
    int ch, row, bit;
    unsigned char far *fb = (unsigned char far *)MK_FP(0xA000, 0x0000);

    for (ch = 0; ch < 8; ch++) {
        int cx = base_x + ch * 8 * scale + ch * scale; /* spacing */
        int cy;
        for (row = 0; row < 8; row++) {
            unsigned char bits = logo_font[ch][row];
            cy = base_y + row * scale;
            for (bit = 0; bit < 8; bit++) {
                if (bits & (0x80 >> bit)) {
                    int px = cx + bit * scale;
                    int sy, sx;
                    for (sy = 0; sy < scale; sy++) {
                        int yy = cy + sy;
                        if (yy >= 0 && yy < 200) {
                            unsigned int off = (unsigned int)yy * 320u;
                            for (sx = 0; sx < scale; sx++) {
                                int xx = px + sx;
                                if (xx >= 0 && xx < 320)
                                    fb[off + (unsigned int)xx] = color;
                            }
                        }
                    }
                }
            }
        }
    }
}

/* Draw tagline text using simple 1-pixel-wide font (CP437-ish) */
static void draw_tagline(int y, const char __far *text, unsigned char color)
{
    /* Simple: use vga_putpixel for a dotted underline effect instead
     * of trying to render a full font. The tagline is communicated
     * by the plasma's atmosphere, not by small text. */
    int x = 40;
    while (*text && x < 280) {
        /* Draw a small dot for each character position */
        vga_putpixel(x, y, color);
        vga_putpixel(x + 1, y, color);
        x += 4;
        text++;
    }
    (void)text;
}

void title_show(void)
{
    unsigned char far *fb = (unsigned char far *)MK_FP(0xA000, 0x0000);
    int fast;
    unsigned char p1 = 0, p2 = 0, p3 = 0, p4 = 0;
    int logo_x, logo_y, logo_scale;

    vga_enter_13h();

    /* Set palette to all black first (for fade-in) */
    {
        int i;
        outp(0x3C8, 0);
        for (i = 0; i < 768; i++)
            outp(0x3C9, 0);
    }

    fast = cpu_is_fast();

    /* Logo positioning */
    logo_scale = 2;
    logo_x = (320 - 8 * 8 * logo_scale - 7 * logo_scale) / 2;
    logo_y = 80;

    /* Fade in the palette over a few frames */
    vga_fade_in(plasma_pal, 16, 55);

    /* Main animation loop */
    while (!scr_kbhit()) {
        if (fast) {
            /* Full resolution: 320x200 */
            unsigned int x, y;
            unsigned int off = 0;
            for (y = 0; y < 200; y++) {
                for (x = 0; x < 320; x++) {
                    unsigned char v1 = sin_tab[(x + p1) & 0xFF];
                    unsigned char v2 = sin_tab[(y + p2) & 0xFF];
                    unsigned char v3 = sin_tab[((x + y + p3) >> 1) & 0xFF];
                    unsigned char v4 = sin_tab[((x - y + p4 + 256) >> 1) & 0xFF];
                    fb[off] = (unsigned char)((v1 + v2 + v3 + v4) >> 2);
                    off++;
                }
            }
        } else {
            /* Half resolution: 160x100, each pixel doubled */
            unsigned int x, y;
            for (y = 0; y < 100; y++) {
                unsigned int row1 = (unsigned int)(y * 2) * 320u;
                unsigned int row2 = row1 + 320u;
                for (x = 0; x < 160; x++) {
                    unsigned char v1 = sin_tab[((x << 1) + p1) & 0xFF];
                    unsigned char v2 = sin_tab[((y << 1) + p2) & 0xFF];
                    unsigned char v3 = sin_tab[(((x + y) + p3)) & 0xFF];
                    unsigned char v4 = sin_tab[(((x - y) + p4 + 256)) & 0xFF];
                    unsigned char c = (unsigned char)((v1 + v2 + v3 + v4) >> 2);
                    unsigned int px = (unsigned int)(x << 1);
                    fb[row1 + px] = c;
                    fb[row1 + px + 1] = c;
                    if (row2 < 64000u) {
                        fb[row2 + px] = c;
                        fb[row2 + px + 1] = c;
                    }
                }
            }
        }

        /* Draw logo on top of plasma */
        draw_logo(logo_x, logo_y, logo_scale, 255);

        /* Tagline dots below logo */
        draw_tagline(logo_y + 20, "CHOOSE AN AI. WATCH IT TAKE OVER.", 253);

        /* "Press any key" indicator */
        {
            int bx;
            for (bx = 130; bx < 190; bx++) {
                vga_putpixel(bx, 180, 240);
                vga_putpixel(bx, 181, 240);
            }
        }

        /* Advance phase */
        p1 += 1;
        p2 += 2;
        p3 += 3;
        p4 += 1;

        vga_wait_vsync();
    }

    /* Consume the keypress */
    scr_getkey();

    /* Fade out */
    vga_fade_out(24, 40);
}
