/*
 * vga13h.h - VGA Mode 13h (320x200, 256 color) graphics primitives
 *
 * Framebuffer at A000:0000, accessed via far pointers.
 * Zero DGROUP cost.
 */

#ifndef VGA13H_H
#define VGA13H_H

#define VGA_WIDTH   320
#define VGA_HEIGHT  200

void vga_enter_13h(void);
void vga_leave_13h(void);
void vga_putpixel(int x, int y, unsigned char color);
void vga_clear(unsigned char color);
void vga_set_palette_entry(unsigned char idx, unsigned char r,
                           unsigned char g, unsigned char b);
void vga_set_palette(const unsigned char __far *pal);
void vga_fade_out(int steps, int delay_ms);
void vga_fade_in(const unsigned char __far *target_pal,
                 int steps, int delay_ms);
void vga_wait_vsync(void);

#endif /* VGA13H_H */
