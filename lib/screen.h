/*
 * screen.h - Direct VGA/MDA text buffer rendering library
 *
 * Shared rendering engine for TAKEOVER and all DOS applications.
 * Writes directly to the text buffer (0xB800 for color, 0xB000 for mono).
 * Assumes 80x25 text mode (mode 3 or 7).
 *
 * Adapted from the NetISA screen library.
 */

#ifndef SCREEN_H
#define SCREEN_H

#include <dos.h>    /* MK_FP */

/* Screen dimensions */
#define SCR_WIDTH   80
#define SCR_HEIGHT  25

/* Cell structure: character + attribute byte */
typedef struct {
    unsigned char ch;
    unsigned char attr;
} cell_t;

/* CGA/VGA color constants (foreground, bits 0-3) */
#define SCR_BLACK           0x00
#define SCR_BLUE            0x01
#define SCR_GREEN           0x02
#define SCR_CYAN            0x03
#define SCR_RED             0x04
#define SCR_MAGENTA         0x05
#define SCR_BROWN           0x06
#define SCR_LIGHTGRAY       0x07
#define SCR_DARKGRAY        0x08
#define SCR_LIGHTBLUE       0x09
#define SCR_LIGHTGREEN      0x0A
#define SCR_LIGHTCYAN       0x0B
#define SCR_LIGHTRED        0x0C
#define SCR_LIGHTMAGENTA    0x0D
#define SCR_YELLOW          0x0E
#define SCR_WHITE           0x0F

/* Build attribute byte: fg (0-15), bg (0-7) */
#define SCR_ATTR(fg, bg)    ((unsigned char)(((bg) << 4) | (fg)))

/* Design language attributes */
#define ATTR_NORMAL     SCR_ATTR(SCR_LIGHTGRAY, SCR_BLACK)
#define ATTR_HIGHLIGHT  SCR_ATTR(SCR_WHITE, SCR_BLACK)
#define ATTR_SELECTED   SCR_ATTR(SCR_LIGHTGREEN, SCR_BLACK)
#define ATTR_HEADER     SCR_ATTR(SCR_WHITE, SCR_BLACK)
#define ATTR_BORDER     SCR_ATTR(SCR_GREEN, SCR_BLACK)
#define ATTR_STATUS     SCR_ATTR(SCR_BLACK, SCR_GREEN)
#define ATTR_ERROR      SCR_ATTR(SCR_LIGHTRED, SCR_BLACK)
#define ATTR_INPUT      SCR_ATTR(SCR_YELLOW, SCR_BLACK)
#define ATTR_DIM        SCR_ATTR(SCR_DARKGRAY, SCR_BLACK)
#define ATTR_INVERSE    SCR_ATTR(SCR_BLACK, SCR_LIGHTGRAY)

/* CP437 box drawing: double-line */
#define BOX_TL      0xC9  /* top-left double corner */
#define BOX_TR      0xBB  /* top-right double corner */
#define BOX_BL      0xC8  /* bottom-left double corner */
#define BOX_BR      0xBC  /* bottom-right double corner */
#define BOX_H       0xCD  /* horizontal double line */
#define BOX_V       0xBA  /* vertical double line */

/* CP437 box drawing: single-line */
#define BOX_STL     0xDA  /* top-left single corner */
#define BOX_STR     0xBF  /* top-right single corner */
#define BOX_SBL     0xC0  /* bottom-left single corner */
#define BOX_SBR     0xD9  /* bottom-right single corner */
#define BOX_SH      0xC4  /* horizontal single line */
#define BOX_SV      0xB3  /* vertical single line */

/* CP437 special characters */
#define BOX_BLOCK   0xDB  /* full block */
#define BOX_SHADE1  0xB0  /* light shade */
#define BOX_SHADE2  0xB1  /* medium shade */
#define BOX_SHADE3  0xB2  /* dark shade */
#define BOX_ARROW   0x10  /* right-pointing triangle */
#define BOX_BULLET  0xFE  /* small square bullet */

/* Keyboard scan codes (high byte from INT 16h) */
#define KEY_UP      0x4800
#define KEY_DOWN    0x5000
#define KEY_LEFT    0x4B00
#define KEY_RIGHT   0x4D00
#define KEY_ENTER   0x000D
#define KEY_ESC     0x001B
#define KEY_TAB     0x0F09
#define KEY_BKSP    0x0E08
#define KEY_PGUP    0x4900
#define KEY_PGDN    0x5100

/*
 * Text buffer segment: 0xB800 for CGA/EGA/VGA, 0xB000 for MDA.
 * Set automatically by scr_init() via INT 11h equipment check.
 */
extern unsigned short scr_segment;

/*
 * scr_read_cell - read char+attr word from the text buffer.
 * Returns unsigned short: low byte = character, high byte = attribute.
 * High-traffic path (effects read before write), so this is a macro.
 */
#define scr_read_cell(x, y) \
    (*(unsigned short far *)MK_FP(scr_segment, ((y) * SCR_WIDTH + (x)) * 2))

/* Function prototypes */
void scr_init(void);
void scr_shutdown(void);
void scr_clear(unsigned char attr);
void scr_putc(int x, int y, char ch, unsigned char attr);
void scr_puts(int x, int y, const char *str, unsigned char attr);
void scr_putsn(int x, int y, const char *str, int maxlen, unsigned char attr);
void scr_hline(int x, int y, int len, char ch, unsigned char attr);
void scr_vline(int x, int y, int len, char ch, unsigned char attr);
void scr_box(int x, int y, int w, int h, unsigned char attr);
void scr_box_single(int x, int y, int w, int h, unsigned char attr);
void scr_fill(int x, int y, int w, int h, char ch, unsigned char attr);
void scr_save_region(int x, int y, int w, int h, unsigned short far *buf);
void scr_restore_region(int x, int y, int w, int h, const unsigned short far *buf);
int  scr_getkey(void);
int  scr_kbhit(void);
void scr_cursor_hide(void);
void scr_cursor_show(void);
void scr_cursor_pos(int x, int y);

#endif /* SCREEN_H */
