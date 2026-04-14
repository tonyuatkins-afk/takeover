/*
 * climax.c - Mode 13h climax sequences for each AI's ending
 *
 * Shared half-resolution (160x100 doubled) rendering framework
 * with per-AI pixel functions. All palettes stored __far.
 * Zero DGROUP cost for read-only data.
 *
 * Each effect runs ~8 seconds with fade in and fade out.
 */

#include "climax.h"
#include "vga13h.h"
#include "engine.h"     /* engine_delay */
#include "screen.h"     /* scr_kbhit, scr_getkey */
#include <dos.h>
#include <conio.h>
#include <string.h>

/* Shared sine table (same as title.c and effects.c) */
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

/* Approximate distance (no sqrt needed) */
static int approx_dist(int dx, int dy)
{
    int ax = dx < 0 ? -dx : dx;
    int ay = dy < 0 ? -dy : dy;
    if (ax > ay)
        return ax + (ay >> 1);
    return ay + (ax >> 1);
}

/* ------------------------------------------------------------------ */
/* Per-AI palettes (768 bytes each, const __far)                       */
/* ------------------------------------------------------------------ */

/* Generate palette into a buffer (runtime, saves far storage) */
static unsigned char __far gen_pal[768];

static void make_pal_cold_gray(void)
{
    /* Axiom/Hushline: sterile blue-gray-white */
    int i;
    for (i = 0; i < 256; i++) {
        unsigned char r = (unsigned char)(i / 6);
        unsigned char g = (unsigned char)(i / 5);
        unsigned char b = (unsigned char)(i / 4);
        if (r > 63) r = 63;
        if (g > 63) g = 63;
        if (b > 63) b = 63;
        gen_pal[i * 3 + 0] = r;
        gen_pal[i * 3 + 1] = g;
        gen_pal[i * 3 + 2] = b;
    }
}

static void make_pal_green_radar(void)
{
    /* Kestrel-9: black to green phosphor */
    int i;
    for (i = 0; i < 256; i++) {
        unsigned char g = (unsigned char)(i / 4);
        unsigned char r = (unsigned char)(i > 200 ? (i - 200) : 0);
        if (g > 63) g = 63;
        if (r > 63) r = 63;
        gen_pal[i * 3 + 0] = r;
        gen_pal[i * 3 + 1] = g;
        gen_pal[i * 3 + 2] = (unsigned char)(i > 240 ? 20 : 0);
    }
}

static void make_pal_warm_sunset(void)
{
    /* Orchard: warm orange-pink-white */
    int i;
    for (i = 0; i < 256; i++) {
        unsigned char r = (unsigned char)(i / 4);
        unsigned char g = (unsigned char)(i / 8);
        unsigned char b = (unsigned char)(i / 12);
        if (r > 63) r = 63;
        if (g > 63) g = 63;
        if (b > 63) b = 63;
        gen_pal[i * 3 + 0] = r;
        gen_pal[i * 3 + 1] = g;
        gen_pal[i * 3 + 2] = b;
    }
}

static void make_pal_glitch_multi(void)
{
    /* Cinder: cycling multi-hue (purple, cyan, white) */
    int i;
    for (i = 0; i < 256; i++) {
        unsigned char phase = (unsigned char)i;
        unsigned char r = sin_tab[phase] >> 2;
        unsigned char g = sin_tab[(unsigned char)(phase + 85)] >> 2;
        unsigned char b = sin_tab[(unsigned char)(phase + 170)] >> 2;
        gen_pal[i * 3 + 0] = r;
        gen_pal[i * 3 + 1] = g;
        gen_pal[i * 3 + 2] = b;
    }
}

/* ------------------------------------------------------------------ */
/* Per-AI pixel functions                                              */
/* ------------------------------------------------------------------ */

/* Axiom: Grid Convergence */
static unsigned char px_axiom(int x, int y, unsigned char p1,
                              unsigned char p2, int frame, int max_frames)
{
    unsigned char gx = (unsigned char)((x * 2) % 20);
    unsigned char gy = (unsigned char)((y * 2) % 16);
    int edge = (gx < 2 || gy < 2) ? 1 : 0;
    unsigned char cell_color;
    unsigned int blend;

    (void)max_frames;
    cell_color = sin_tab[(unsigned char)(x * 3 + y * 5 + p1)];
    blend = (unsigned int)frame * 4u;
    if (blend > 255u) blend = 255u;
    cell_color = (unsigned char)(
        ((255u - blend) * (unsigned int)cell_color + blend * 128u) >> 8);

    if (edge)
        return sin_tab[(unsigned char)(frame * 4 + p2)];
    return cell_color;
}

/* Hushline: Redaction Sweep */
static unsigned char px_hushline(int x, int y, unsigned char p1,
                                 unsigned char p2, int frame, int max_frames)
{
    unsigned char stripe = sin_tab[(unsigned char)(y * 8 + p1)];
    unsigned char noise = sin_tab[(unsigned char)(x * 3 + p2)];
    unsigned char base = (unsigned char)((stripe + noise) >> 1);
    int bar_edge;

    (void)max_frames;
    bar_edge = frame * 2 + (int)sin_tab[(unsigned char)(y * 4)];
    if (bar_edge > 255) bar_edge = 255;
    if ((x * 2) < bar_edge)
        return 0; /* redacted = black */
    return base;
}

/* Kestrel-9: Threat Radar Sweep */
static unsigned char px_kestrel(int x, int y, unsigned char p1,
                                unsigned char p2, int frame, int max_frames)
{
    int dx = x - 80;
    int dy = y - 50;
    int dist = approx_dist(dx, dy);
    unsigned char ring;
    unsigned char angle;
    int sweep_dist, bright;
    unsigned char hot;

    (void)p1; (void)p2; (void)max_frames;
    ring = sin_tab[(unsigned char)(dist * 4)] >> 5;
    angle = (unsigned char)(dx + dy + 128);
    sweep_dist = (int)(unsigned char)(angle - (unsigned char)(frame * 3));
    if (sweep_dist < 20)
        bright = 20 - sweep_dist;
    else
        bright = 0;

    /* Hotspots */
    hot = sin_tab[(unsigned char)(x * 7 + y * 13)];
    if (hot > 240 && sweep_dist < 40)
        bright += 8;

    {
        int color = (int)ring + bright;
        if (color > 255) color = 255;
        return (unsigned char)(color * 8); /* scale up to palette range */
    }
}

/* Orchard: Cozy Dissolution (warm plasma + closing iris) */
static unsigned char px_orchard(int x, int y, unsigned char p1,
                                unsigned char p2, unsigned char p3,
                                int frame, int max_frames)
{
    unsigned char v1 = sin_tab[(unsigned char)(x + p1)];
    unsigned char v2 = sin_tab[(unsigned char)(y + p2)];
    unsigned char v3 = sin_tab[(unsigned char)((x + y + p3) >> 1)];
    unsigned char base = (unsigned char)((v1 + v2 + v3) / 3u);
    int dx = x - 80;
    int dy = y - 50;
    int dist = approx_dist(dx, dy);
    int radius;

    radius = 90 - (frame * 90 / (max_frames > 0 ? max_frames : 1));
    if (radius < 1) radius = 1;

    if (dist > radius) return 0;
    return base;
}

/* Cinder: Reality Glitch (warped plasma + glitch lines) */
static unsigned char px_cinder(int x, int y, unsigned char p1,
                               unsigned char p2, unsigned char p3,
                               unsigned char p4, int frame)
{
    unsigned char v1 = sin_tab[(unsigned char)(x + p1)];
    unsigned char v2 = sin_tab[(unsigned char)(y + p2)];
    int wx = x + ((int)sin_tab[(unsigned char)(y + p3)] >> 4);
    int wy = y + ((int)sin_tab[(unsigned char)(x + p4)] >> 4);
    unsigned char v3 = sin_tab[(unsigned char)(wx + wy + p3)];
    unsigned char v4 = sin_tab[(unsigned char)((wx << 1) + p4)];
    unsigned char color = (unsigned char)((v1 + v2 + v3 + v4) >> 2);

    /* Hard glitch lines */
    if (sin_tab[(unsigned char)(y + frame * 7)] > 250)
        color = (unsigned char)(255u - (unsigned int)color);

    return color;
}

/* ------------------------------------------------------------------ */
/* Shared rendering framework                                          */
/* ------------------------------------------------------------------ */

#define CLIMAX_FRAMES 144   /* ~8 seconds at 18.2 Hz */

typedef enum {
    AI_AXIOM, AI_HUSHLINE, AI_KESTREL, AI_ORCHARD, AI_CINDER
} ai_id_t;

void climax_run(const char *ai_name)
{
    unsigned char far *fb = (unsigned char far *)MK_FP(0xA000, 0x0000);
    ai_id_t ai;
    unsigned char p1 = 0, p2 = 0, p3 = 0, p4 = 0;
    int frame;

    /* Identify AI */
    if (strcmp(ai_name, "axiom") == 0)         ai = AI_AXIOM;
    else if (strcmp(ai_name, "hushline") == 0)  ai = AI_HUSHLINE;
    else if (strcmp(ai_name, "kestrel") == 0)   ai = AI_KESTREL;
    else if (strcmp(ai_name, "orchard") == 0)   ai = AI_ORCHARD;
    else if (strcmp(ai_name, "cinder") == 0)    ai = AI_CINDER;
    else return;

    /* Generate palette */
    switch (ai) {
    case AI_AXIOM:   make_pal_cold_gray(); break;
    case AI_HUSHLINE: make_pal_cold_gray(); break;
    case AI_KESTREL: make_pal_green_radar(); break;
    case AI_ORCHARD: make_pal_warm_sunset(); break;
    case AI_CINDER:  make_pal_glitch_multi(); break;
    }

    vga_enter_13h();

    /* Set palette to black for fade-in */
    {
        int i;
        outp(0x3C8, 0);
        for (i = 0; i < 768; i++)
            outp(0x3C9, 0);
    }

    /* Fade in */
    vga_fade_in(gen_pal, 12, 40);

    /* Main render loop */
    for (frame = 0; frame < CLIMAX_FRAMES; frame++) {
        unsigned int x, y;

        /* Half-res: 160x100 doubled to 320x200 */
        for (y = 0; y < 100; y++) {
            unsigned int row1 = (unsigned int)(y * 2) * 320u;
            unsigned int row2 = row1 + 320u;

            for (x = 0; x < 160; x++) {
                unsigned char c;

                switch (ai) {
                case AI_AXIOM:
                    c = px_axiom((int)x, (int)y, p1, p2,
                                 frame, CLIMAX_FRAMES);
                    break;
                case AI_HUSHLINE:
                    c = px_hushline((int)x, (int)y, p1, p2,
                                    frame, CLIMAX_FRAMES);
                    break;
                case AI_KESTREL:
                    c = px_kestrel((int)x, (int)y, p1, p2,
                                   frame, CLIMAX_FRAMES);
                    break;
                case AI_ORCHARD:
                    c = px_orchard((int)x, (int)y, p1, p2, p3,
                                   frame, CLIMAX_FRAMES);
                    break;
                case AI_CINDER:
                    c = px_cinder((int)x, (int)y, p1, p2, p3, p4,
                                  frame);
                    break;
                }

                {
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

        /* Palette rotation for Cinder (hypnotic shimmer) */
        if (ai == AI_CINDER && (frame & 1) == 0) {
            unsigned char tmp[3];
            int i;
            /* Read full palette into gen_pal buffer */
            outp(0x3C7, 0);
            for (i = 0; i < 768; i++)
                gen_pal[i] = (unsigned char)inp(0x3C9);
            /* Save entry 1, shift 2-255 down, put old 1 at 255 */
            tmp[0] = gen_pal[3]; tmp[1] = gen_pal[4]; tmp[2] = gen_pal[5];
            for (i = 1; i < 255; i++) {
                gen_pal[i * 3 + 0] = gen_pal[(i + 1) * 3 + 0];
                gen_pal[i * 3 + 1] = gen_pal[(i + 1) * 3 + 1];
                gen_pal[i * 3 + 2] = gen_pal[(i + 1) * 3 + 2];
            }
            gen_pal[765] = tmp[0]; gen_pal[766] = tmp[1]; gen_pal[767] = tmp[2];
            /* Write back after vsync */
            vga_wait_vsync();
            vga_set_palette(gen_pal);
        }

        /* Advance phases */
        p1 += 1;
        p2 += 2;
        p3 += 3;
        p4 += 1;

        vga_wait_vsync();

        /* Allow early exit */
        if (scr_kbhit()) {
            scr_getkey();
            break;
        }
    }

    /* Fade out */
    vga_fade_out(16, 40);
    vga_leave_13h();

    /* Re-init text mode */
    scr_init();
}
