/*
 * screen.c - Direct VGA/MDA text buffer rendering library
 *
 * Writes directly to the text buffer segment (auto-detected).
 * 80x25 text mode assumed.
 *
 * Adapted from the NetISA screen library.
 */

#include "screen.h"
#include <i86.h>
#include <string.h>

/* Text buffer segment: set by scr_init() via INT 11h */
unsigned short scr_segment = 0xB800;

/* Saved video state for shutdown restore */
static unsigned char saved_mode = 0x03;
static unsigned short saved_cursor = 0;

static cell_t far *scr_cell(int x, int y)
{
    return (cell_t far *)MK_FP(scr_segment, (y * SCR_WIDTH + x) * 2);
}

void scr_init(void)
{
    union REGS r;

    /* Detect display adapter via INT 11h equipment list.
     * Bits 5-4: initial video mode.
     *   11b (3) = 80x25 mono (MDA) -> segment 0xB000
     *   10b (2) = 80x25 color (CGA/EGA/VGA) -> segment 0xB800
     *   01b (1) = 40x25 color -> segment 0xB800
     *   00b (0) = unused / EGA+ -> segment 0xB800
     */
    int86(0x11, &r, &r);
    if (((r.w.ax >> 4) & 0x03) == 0x03)
        scr_segment = 0xB000;
    else
        scr_segment = 0xB800;

    /* Save current video mode */
    r.h.ah = 0x0F;
    int86(0x10, &r, &r);
    saved_mode = r.h.al;

    /* Save cursor shape */
    r.h.ah = 0x03;
    r.h.bh = 0;
    int86(0x10, &r, &r);
    saved_cursor = r.w.cx;

    /* Set 80x25 text mode to ensure clean state.
     * Mode 7 for MDA, mode 3 for color adapters. */
    r.h.ah = 0x00;
    r.h.al = (scr_segment == 0xB000) ? 0x07 : 0x03;
    int86(0x10, &r, &r);

    scr_cursor_hide();
    scr_clear(ATTR_NORMAL);
}

void scr_shutdown(void)
{
    union REGS r;

    /* Restore original video mode */
    r.h.ah = 0x00;
    r.h.al = saved_mode;
    int86(0x10, &r, &r);

    /* Restore cursor shape */
    r.h.ah = 0x01;
    r.w.cx = saved_cursor;
    int86(0x10, &r, &r);
}

void scr_clear(unsigned char attr)
{
    scr_fill(0, 0, SCR_WIDTH, SCR_HEIGHT, ' ', attr);
}

void scr_putc(int x, int y, char ch, unsigned char attr)
{
    cell_t far *p;
    if (x < 0 || x >= SCR_WIDTH || y < 0 || y >= SCR_HEIGHT)
        return;
    p = scr_cell(x, y);
    p->ch = (unsigned char)ch;
    p->attr = attr;
}

void scr_puts(int x, int y, const char *str, unsigned char attr)
{
    while (*str && x < SCR_WIDTH) {
        scr_putc(x, y, *str, attr);
        str++;
        x++;
    }
}

void scr_putsn(int x, int y, const char *str, int maxlen, unsigned char attr)
{
    int i = 0;
    while (*str && x < SCR_WIDTH && i < maxlen) {
        scr_putc(x, y, *str, attr);
        str++;
        x++;
        i++;
    }
}

void scr_hline(int x, int y, int len, char ch, unsigned char attr)
{
    int i;
    for (i = 0; i < len; i++)
        scr_putc(x + i, y, ch, attr);
}

void scr_vline(int x, int y, int len, char ch, unsigned char attr)
{
    int i;
    for (i = 0; i < len; i++)
        scr_putc(x, y + i, ch, attr);
}

void scr_box(int x, int y, int w, int h, unsigned char attr)
{
    int i;

    /* Corners */
    scr_putc(x, y, (char)BOX_TL, attr);
    scr_putc(x + w - 1, y, (char)BOX_TR, attr);
    scr_putc(x, y + h - 1, (char)BOX_BL, attr);
    scr_putc(x + w - 1, y + h - 1, (char)BOX_BR, attr);

    /* Top and bottom edges */
    for (i = 1; i < w - 1; i++) {
        scr_putc(x + i, y, (char)BOX_H, attr);
        scr_putc(x + i, y + h - 1, (char)BOX_H, attr);
    }

    /* Left and right edges */
    for (i = 1; i < h - 1; i++) {
        scr_putc(x, y + i, (char)BOX_V, attr);
        scr_putc(x + w - 1, y + i, (char)BOX_V, attr);
    }
}

void scr_box_single(int x, int y, int w, int h, unsigned char attr)
{
    int i;

    /* Corners */
    scr_putc(x, y, (char)BOX_STL, attr);
    scr_putc(x + w - 1, y, (char)BOX_STR, attr);
    scr_putc(x, y + h - 1, (char)BOX_SBL, attr);
    scr_putc(x + w - 1, y + h - 1, (char)BOX_SBR, attr);

    /* Top and bottom edges */
    for (i = 1; i < w - 1; i++) {
        scr_putc(x + i, y, (char)BOX_SH, attr);
        scr_putc(x + i, y + h - 1, (char)BOX_SH, attr);
    }

    /* Left and right edges */
    for (i = 1; i < h - 1; i++) {
        scr_putc(x, y + i, (char)BOX_SV, attr);
        scr_putc(x + w - 1, y + i, (char)BOX_SV, attr);
    }
}

void scr_fill(int x, int y, int w, int h, char ch, unsigned char attr)
{
    int row, col;
    for (row = 0; row < h; row++)
        for (col = 0; col < w; col++)
            scr_putc(x + col, y + row, ch, attr);
}

void scr_save_region(int x, int y, int w, int h, unsigned short far *buf)
{
    int row, col;
    for (row = 0; row < h; row++)
        for (col = 0; col < w; col++)
            *buf++ = scr_read_cell(x + col, y + row);
}

void scr_restore_region(int x, int y, int w, int h, const unsigned short far *buf)
{
    int row, col;
    for (row = 0; row < h; row++) {
        for (col = 0; col < w; col++) {
            unsigned short cell = *buf++;
            scr_putc(x + col, y + row, (char)(cell & 0xFF), (unsigned char)(cell >> 8));
        }
    }
}

int scr_getkey(void)
{
    union REGS r;
    r.h.ah = 0x00;
    int86(0x16, &r, &r);
    /* Return scancode in high byte, ASCII in low byte */
    return r.w.ax;
}

int scr_kbhit(void)
{
    unsigned char result = 0;
    /* INT 16h AH=01h: ZF=1 if no key, ZF=0 if key ready.
     * OpenWatcom's int86 cflag only captures CF, not ZF.
     * Use inline asm to check ZF directly. */
    _asm {
        mov ah, 01h
        int 16h
        jnz _has_key
        mov result, 0
        jmp _done
    _has_key:
        mov result, 1
    _done:
    }
    return result;
}

void scr_cursor_hide(void)
{
    union REGS r;
    r.h.ah = 0x01;
    r.w.cx = 0x2000;  /* start=32, end=0: invisible */
    int86(0x10, &r, &r);
}

void scr_cursor_show(void)
{
    union REGS r;
    r.h.ah = 0x01;
    r.w.cx = 0x0607;  /* normal underline cursor */
    int86(0x10, &r, &r);
}

void scr_cursor_pos(int x, int y)
{
    union REGS r;
    r.h.ah = 0x02;
    r.h.bh = 0;
    r.h.dh = (unsigned char)y;
    r.h.dl = (unsigned char)x;
    int86(0x10, &r, &r);
}
