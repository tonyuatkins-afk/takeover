/*
 * vga13h.c - VGA Mode 13h graphics primitives
 *
 * 320x200, 256 colors, linear framebuffer at A000:0000.
 * All data access via far pointers. Zero DGROUP cost.
 */

#include "vga13h.h"
#include "engine.h"     /* engine_delay */
#include <i86.h>
#include <conio.h>
#include <dos.h>
#include <string.h>

#define VGA_FB ((unsigned char far *)MK_FP(0xA000, 0x0000))

void vga_enter_13h(void)
{
    union REGS r;
    r.h.ah = 0x00;
    r.h.al = 0x13;
    int86(0x10, &r, &r);
}

void vga_leave_13h(void)
{
    union REGS r;
    r.h.ah = 0x00;
    r.h.al = 0x03;
    int86(0x10, &r, &r);
}

void vga_putpixel(int x, int y, unsigned char color)
{
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT)
        VGA_FB[(unsigned int)y * 320u + (unsigned int)x] = color;
}

void vga_clear(unsigned char color)
{
    unsigned char far *p = VGA_FB;
    unsigned int i;
    for (i = 0; i < 64000u; i++)
        *p++ = color;
}

void vga_set_palette_entry(unsigned char idx, unsigned char r,
                           unsigned char g, unsigned char b)
{
    outp(0x3C8, idx);
    outp(0x3C9, r);
    outp(0x3C9, g);
    outp(0x3C9, b);
}

void vga_set_palette(const unsigned char __far *pal)
{
    int i;
    outp(0x3C8, 0);
    for (i = 0; i < 768; i++)
        outp(0x3C9, pal[i]);
}

void vga_fade_out(int steps, int delay_ms)
{
    static unsigned char cur[768];
    int s, i;

    /* Read current palette */
    outp(0x3C7, 0);
    for (i = 0; i < 768; i++)
        cur[i] = (unsigned char)inp(0x3C9);

    for (s = steps - 1; s >= 0; s--) {
        vga_wait_vsync();
        outp(0x3C8, 0);
        for (i = 0; i < 768; i++) {
            unsigned char v = (unsigned char)((unsigned int)cur[i] * (unsigned int)s / (unsigned int)steps);
            outp(0x3C9, v);
        }
        engine_delay(delay_ms);
    }
}

void vga_fade_in(const unsigned char __far *target_pal,
                 int steps, int delay_ms)
{
    int s, i;

    for (s = 1; s <= steps; s++) {
        vga_wait_vsync();
        outp(0x3C8, 0);
        for (i = 0; i < 768; i++) {
            unsigned char v = (unsigned char)((unsigned int)target_pal[i] * (unsigned int)s / (unsigned int)steps);
            outp(0x3C9, v);
        }
        engine_delay(delay_ms);
    }
}

void vga_wait_vsync(void)
{
    while (inp(0x3DA) & 0x08) { }
    while (!(inp(0x3DA) & 0x08)) { }
}
