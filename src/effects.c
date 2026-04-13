/*
 * effects.c - Visual effects implementations
 *
 * Each effect operates directly on the VGA text buffer via the
 * screen library. Effects are dispatched by the engine's effect table.
 *
 * Timing uses engine_delay() (BIOS tick counter at 0040:006C).
 * Randomness uses a simple LCG seeded from the tick counter.
 */

#include "effects.h"
#include "engine.h"     /* engine_delay */
#include "screen.h"
#include <dos.h>
#include <string.h>

/* Main content area (must match engine.c) */
#define FX_TOP      1
#define FX_BOTTOM  22
#define FX_LEFT     1
#define FX_RIGHT   78
#define FX_ROWS    (FX_BOTTOM - FX_TOP + 1)
#define FX_COLS    (FX_RIGHT - FX_LEFT + 1)

/*
 * Shared static save buffer for effects that need to save/restore
 * screen regions. Only one effect runs at a time, so sharing is safe.
 * Full screen = 80*25 = 2000 words = 4000 bytes.
 * Keeps this off the stack (4KB stack in small model).
 */
static unsigned short fx_save_buf[SCR_WIDTH * SCR_HEIGHT];

/* ------------------------------------------------------------------ */
/* RNG: LCG seeded from BIOS tick counter                              */
/* ------------------------------------------------------------------ */

static unsigned long rng_state = 1;

void rng_seed(void)
{
    unsigned long far *tick =
        (unsigned long far *)MK_FP(0x0040, 0x006C);
    rng_state = *tick;
    if (rng_state == 0) rng_state = 1;
}

unsigned int rng_next(void)
{
    rng_state = rng_state * 1103515245UL + 12345UL;
    return (unsigned int)(rng_state >> 16) & 0x7FFF;
}

/* Random in range [0, max) */
static int rng_range(int max)
{
    if (max <= 0) return 0;
    return (int)(rng_next() % (unsigned int)max);
}

/* ------------------------------------------------------------------ */
/* typing_effect: no-op here, engine handles via typing_active flag    */
/* ------------------------------------------------------------------ */

void fx_typing_effect(int duration, int intensity)
{
    (void)duration;
    (void)intensity;
}

/* ------------------------------------------------------------------ */
/* screen_flicker: random chars flash across main area, then restore   */
/* ------------------------------------------------------------------ */

void fx_screen_flicker(int duration, int intensity)
{
    int cycles, i, r, c;
    int tick_ms = 55;

    if (intensity < 1) intensity = 1;
    if (intensity > 10) intensity = 10;
    cycles = (duration / tick_ms);
    if (cycles < 1) cycles = 1;

    scr_save_region(FX_LEFT, FX_TOP, FX_COLS, FX_ROWS, fx_save_buf);

    for (i = 0; i < cycles; i++) {
        for (r = FX_TOP; r <= FX_BOTTOM; r++) {
            for (c = FX_LEFT; c <= FX_RIGHT; c++) {
                if (rng_range(10) < intensity) {
                    scr_putc(c, r, (char)(rng_range(254) + 1),
                             (unsigned char)(rng_range(256)));
                }
            }
        }
        engine_delay(tick_ms);
        scr_restore_region(FX_LEFT, FX_TOP, FX_COLS, FX_ROWS, fx_save_buf);
        if (i < cycles - 1)
            engine_delay(tick_ms);
    }
}

/* ------------------------------------------------------------------ */
/* static_burst: random region fills with garbage, then restores       */
/* ------------------------------------------------------------------ */

void fx_static_burst(int duration, int intensity)
{
    unsigned short save[20 * 20]; /* max region 20x20 */
    int bw, bh, bx, by;
    int elapsed, tick_ms;
    int r, c;

    if (intensity < 1) intensity = 1;
    bw = 4 + intensity * 2;
    bh = 2 + intensity;
    if (bw > 20) bw = 20;
    if (bh > 20) bh = 20;
    if (bw > FX_COLS) bw = FX_COLS;
    if (bh > FX_ROWS) bh = FX_ROWS;
    tick_ms = 55;

    for (elapsed = 0; elapsed < duration; elapsed += tick_ms * 3) {
        bx = FX_LEFT + rng_range(FX_COLS - bw);
        by = FX_TOP + rng_range(FX_ROWS - bh);

        scr_save_region(bx, by, bw, bh, save);

        for (r = 0; r < bh; r++) {
            for (c = 0; c < bw; c++) {
                scr_putc(bx + c, by + r,
                         (char)(rng_range(254) + 1),
                         (unsigned char)(rng_range(256)));
            }
        }
        engine_delay(tick_ms);
        scr_restore_region(bx, by, bw, bh, save);
        engine_delay(tick_ms * 2);
    }
}

/* ------------------------------------------------------------------ */
/* text_corrupt: randomly mangle visible chars. Permanent.             */
/* ------------------------------------------------------------------ */

void fx_text_corrupt(int duration, int intensity)
{
    int elapsed, tick_ms;
    int r, c;
    unsigned short cell;

    if (intensity < 1) intensity = 1;
    tick_ms = 55;

    for (elapsed = 0; elapsed < duration; elapsed += tick_ms) {
        for (r = FX_TOP; r <= FX_BOTTOM; r++) {
            for (c = FX_LEFT; c <= FX_RIGHT; c++) {
                if (rng_range(100) < intensity * 2) {
                    cell = scr_read_cell(c, r);
                    if ((cell & 0xFF) > 0x20) {
                        int ch = (int)(cell & 0xFF);
                        int offset = rng_range(5) - 2;
                        ch += offset;
                        if (ch < 1) ch = 1;
                        if (ch > 254) ch = 254;
                        scr_putc(c, r, (char)ch,
                                 (unsigned char)(cell >> 8));
                    }
                }
            }
        }
        engine_delay(tick_ms);
    }
}

/* ------------------------------------------------------------------ */
/* text_redact: sweep chars with block. Permanent.                     */
/* ------------------------------------------------------------------ */

void fx_text_redact(int duration, int intensity)
{
    int chars_per_tick;
    int elapsed, tick_ms;
    int r, c, count;
    unsigned short cell;

    if (intensity < 1) intensity = 1;
    chars_per_tick = intensity * 3;
    tick_ms = 55;

    r = FX_TOP;
    c = FX_LEFT;

    for (elapsed = 0; elapsed < duration && r <= FX_BOTTOM;
         elapsed += tick_ms) {
        for (count = 0; count < chars_per_tick && r <= FX_BOTTOM;
             count++) {
            cell = scr_read_cell(c, r);
            if ((cell & 0xFF) != 0x20) {
                scr_putc(c, r, (char)BOX_BLOCK,
                         SCR_ATTR(SCR_DARKGRAY, SCR_BLACK));
            }
            c++;
            if (c > FX_RIGHT) {
                c = FX_LEFT;
                r++;
            }
        }
        engine_delay(tick_ms);
    }
}

/* ------------------------------------------------------------------ */
/* color_shift: cycle foreground colors. Permanent.                    */
/* ------------------------------------------------------------------ */

void fx_color_shift(int duration, int intensity)
{
    int elapsed, tick_ms;
    int r, c, steps, s;
    unsigned short cell;
    unsigned char attr, fg, bg;

    if (intensity < 1) intensity = 1;
    steps = intensity;
    tick_ms = 110;

    for (elapsed = 0; elapsed < duration; elapsed += tick_ms) {
        for (r = FX_TOP; r <= FX_BOTTOM; r++) {
            for (c = FX_LEFT; c <= FX_RIGHT; c++) {
                cell = scr_read_cell(c, r);
                if ((cell & 0xFF) == 0x20) continue;
                attr = (unsigned char)(cell >> 8);
                fg = attr & 0x0F;
                bg = (attr >> 4) & 0x07;
                for (s = 0; s < steps; s++) {
                    fg = (fg + 1) & 0x0F;
                    if (fg == 0) fg = 1; /* skip black */
                }
                scr_putc(c, r, (char)(cell & 0xFF),
                         SCR_ATTR(fg, bg));
            }
        }
        engine_delay(tick_ms);
    }
}

/* ------------------------------------------------------------------ */
/* blackout: screen goes dark, then restores                           */
/* ------------------------------------------------------------------ */

void fx_blackout(int duration, int intensity)
{
    (void)intensity;

    scr_save_region(0, 0, SCR_WIDTH, SCR_HEIGHT, fx_save_buf);
    scr_fill(0, 0, SCR_WIDTH, SCR_HEIGHT, ' ',
             SCR_ATTR(SCR_BLACK, SCR_BLACK));
    engine_delay(duration);
    scr_restore_region(0, 0, SCR_WIDTH, SCR_HEIGHT, fx_save_buf);
}

/* ------------------------------------------------------------------ */
/* red_alert: flash border between green and red                       */
/* ------------------------------------------------------------------ */

void fx_red_alert(int duration, int intensity)
{
    int elapsed, flash_ms;
    int is_red = 0;
    unsigned char red_attr = SCR_ATTR(SCR_LIGHTRED, SCR_BLACK);

    if (intensity < 1) intensity = 1;
    flash_ms = 400 / intensity;
    if (flash_ms < 55) flash_ms = 55;

    for (elapsed = 0; elapsed < duration; elapsed += flash_ms) {
        if (is_red) {
            scr_box(0, 0, SCR_WIDTH, SCR_HEIGHT - 1, ATTR_BORDER);
        } else {
            scr_box(0, 0, SCR_WIDTH, SCR_HEIGHT - 1, red_attr);
        }
        is_red = !is_red;
        engine_delay(flash_ms);
    }
    /* Restore normal border */
    scr_box(0, 0, SCR_WIDTH, SCR_HEIGHT - 1, ATTR_BORDER);
}

/* ------------------------------------------------------------------ */
/* screen_scroll: rapid upward scroll with garbage fill. Permanent.    */
/* ------------------------------------------------------------------ */

void fx_screen_scroll(int duration, int intensity)
{
    int elapsed, tick_ms;
    int rows_per_tick;
    int r, c, s;
    unsigned short cell;

    if (intensity < 1) intensity = 1;
    rows_per_tick = intensity;
    if (rows_per_tick > FX_ROWS) rows_per_tick = FX_ROWS;
    tick_ms = 55;

    for (elapsed = 0; elapsed < duration; elapsed += tick_ms) {
        for (s = 0; s < rows_per_tick; s++) {
            /* Scroll up one row */
            for (r = FX_TOP; r < FX_BOTTOM; r++) {
                for (c = FX_LEFT; c <= FX_RIGHT; c++) {
                    cell = scr_read_cell(c, r + 1);
                    scr_putc(c, r, (char)(cell & 0xFF),
                             (unsigned char)(cell >> 8));
                }
            }
            /* Fill bottom row with random data */
            for (c = FX_LEFT; c <= FX_RIGHT; c++) {
                scr_putc(c, FX_BOTTOM,
                         (char)(rng_range(94) + 0x21),
                         SCR_ATTR(SCR_GREEN, SCR_BLACK));
            }
        }
        engine_delay(tick_ms);
    }
}

/* ------------------------------------------------------------------ */
/* cursor_possessed: cursor moves and types autonomously               */
/* ------------------------------------------------------------------ */

void fx_cursor_possessed(int duration, int intensity)
{
    static const char menacing[] =
        "0123456789ABCDEFabcdef[]{}|<>/\\#@$%&"
        "\xC4\xB3\xDA\xBF\xC0\xD9\xC5\xB4\xC3\xC2\xC1";
    int elapsed, type_ms;
    int cx, cy;
    int mlen;

    if (intensity < 1) intensity = 1;
    type_ms = 200 / intensity;
    if (type_ms < 55) type_ms = 55;
    mlen = (int)strlen(menacing);

    cx = FX_LEFT + rng_range(FX_COLS);
    cy = FX_TOP + rng_range(FX_ROWS);

    scr_cursor_show();

    for (elapsed = 0; elapsed < duration; elapsed += type_ms) {
        /* Occasionally jump to a new position */
        if (rng_range(10) < 2) {
            cx = FX_LEFT + rng_range(FX_COLS);
            cy = FX_TOP + rng_range(FX_ROWS);
        }

        scr_cursor_pos(cx, cy);
        scr_putc(cx, cy, menacing[rng_range(mlen)],
                 SCR_ATTR(SCR_LIGHTGREEN, SCR_BLACK));
        cx++;
        if (cx > FX_RIGHT) {
            cx = FX_LEFT;
            cy++;
            if (cy > FX_BOTTOM) cy = FX_TOP;
        }
        engine_delay(type_ms);
    }

    scr_cursor_hide();
}

/* ------------------------------------------------------------------ */
/* screen_melt: characters drip downward. Permanent.                   */
/* ------------------------------------------------------------------ */

void fx_screen_melt(int duration, int intensity)
{
    int elapsed, tick_ms;
    int cols_per_tick;
    int i, c, r;
    unsigned short cell;

    if (intensity < 1) intensity = 1;
    cols_per_tick = intensity * 2;
    if (cols_per_tick > FX_COLS) cols_per_tick = FX_COLS;
    tick_ms = 55;

    for (elapsed = 0; elapsed < duration; elapsed += tick_ms) {
        for (i = 0; i < cols_per_tick; i++) {
            c = FX_LEFT + rng_range(FX_COLS);
            r = FX_TOP + rng_range(FX_ROWS - 1);
            cell = scr_read_cell(c, r);
            if ((cell & 0xFF) != 0x20) {
                /* Move char down, leave space above */
                scr_putc(c, r + 1, (char)(cell & 0xFF),
                         (unsigned char)(cell >> 8));
                scr_putc(c, r, ' ',
                         (unsigned char)(cell >> 8));
            }
        }
        engine_delay(tick_ms);
    }
}

/* ------------------------------------------------------------------ */
/* falling_chars: chars rain down the screen, then restore             */
/* ------------------------------------------------------------------ */

void fx_falling_chars(int duration, int intensity)
{
    /* Track column state: -1=inactive, 0..FX_ROWS-1=current row */
    int col_row[80]; /* indexed by screen col */
    int col_ch[80];
    int elapsed, tick_ms;
    int density;
    int c, r;
    static const char fall_set[] =
        "0123456789:;<=>?@#$%&*+/\\"
        "\xC4\xB3\xDA\xBF\xC0\xD9\xCD\xBA";
    int flen;

    if (intensity < 1) intensity = 1;
    density = intensity;
    tick_ms = 55;
    flen = (int)strlen(fall_set);

    scr_save_region(FX_LEFT, FX_TOP, FX_COLS, FX_ROWS, fx_save_buf);

    for (c = 0; c < 80; c++) {
        col_row[c] = -1;
        col_ch[c] = 0;
    }

    for (elapsed = 0; elapsed < duration; elapsed += tick_ms) {
        /* Spawn new columns */
        for (c = FX_LEFT; c <= FX_RIGHT; c++) {
            if (col_row[c] < 0 && rng_range(FX_COLS) < density) {
                col_row[c] = 0;
            }
        }
        /* Advance and draw */
        for (c = FX_LEFT; c <= FX_RIGHT; c++) {
            if (col_row[c] < 0) continue;
            r = FX_TOP + col_row[c];
            if (r <= FX_BOTTOM) {
                scr_putc(c, r, fall_set[rng_range(flen)],
                         SCR_ATTR(SCR_GREEN, SCR_BLACK));
            }
            /* Dim previous position */
            if (col_row[c] > 0 && (FX_TOP + col_row[c] - 1) >= FX_TOP) {
                unsigned short prev = scr_read_cell(c, FX_TOP + col_row[c] - 1);
                scr_putc(c, FX_TOP + col_row[c] - 1,
                         (char)(prev & 0xFF),
                         SCR_ATTR(SCR_DARKGRAY, SCR_BLACK));
            }
            col_row[c]++;
            if (FX_TOP + col_row[c] > FX_BOTTOM) {
                col_row[c] = -1;
            }
        }
        engine_delay(tick_ms);
    }

    scr_restore_region(FX_LEFT, FX_TOP, FX_COLS, FX_ROWS, fx_save_buf);
}

/* ------------------------------------------------------------------ */
/* progress_bar: ominous progress bar overlay, then restore            */
/* ------------------------------------------------------------------ */

void fx_progress_bar(int duration, int intensity)
{
    static const char *labels[] = {
        "UPLOADING...",
        "SYNCHRONIZING...",
        "PROCESSING...",
        "CALIBRATING...",
        "INTEGRATING..."
    };
    int bw = 50, bh = 5;
    int bx, by;
    int fill_ms;
    int i, filled;
    int bar_inner;
    const char *label;

    if (intensity < 1) intensity = 1;
    bar_inner = bw - 4;
    fill_ms = duration / (bar_inner > 0 ? bar_inner : 1);
    if (fill_ms < 55) fill_ms = 55;

    bx = (SCR_WIDTH - bw) / 2;
    by = (SCR_HEIGHT - bh) / 2;
    label = labels[rng_range(5)];

    scr_save_region(bx, by, bw, bh, fx_save_buf);
    scr_fill(bx, by, bw, bh, ' ', ATTR_ERROR);
    scr_box_single(bx, by, bw, bh, ATTR_ERROR);
    scr_puts(bx + 2, by + 1, label, ATTR_ERROR);

    filled = 0;
    for (i = 0; i < bar_inner; i++) {
        scr_putc(bx + 2 + i, by + 3, (char)BOX_BLOCK, ATTR_ERROR);
        filled++;
        engine_delay(fill_ms);
    }

    /* Brief pause at full */
    engine_delay(500);

    scr_restore_region(bx, by, bw, bh, fx_save_buf);
}

/* ------------------------------------------------------------------ */
/* fake_bsod: fake crash screen, wait for key or timeout, restore      */
/* ------------------------------------------------------------------ */

void fx_fake_bsod(int duration, int intensity)
{
    unsigned char bsod_attr = SCR_ATTR(SCR_WHITE, SCR_BLUE);
    int elapsed;
    (void)intensity;

    scr_save_region(0, 0, SCR_WIDTH, SCR_HEIGHT, fx_save_buf);
    scr_fill(0, 0, SCR_WIDTH, SCR_HEIGHT, ' ', bsod_attr);

    scr_puts(4, 3,
        "A fatal exception has occurred at 0000:0000DEAD",
        bsod_attr);
    scr_puts(4, 5,
        "An unrecoverable system error has been detected.",
        bsod_attr);
    scr_puts(4, 6,
        "The current process will be terminated.",
        bsod_attr);
    scr_puts(4, 8,
        "ERROR CODE: 0x0000004F  STATUS: NONCOMPLIANT",
        bsod_attr);
    scr_puts(4, 10,
        "*  Press any key to acknowledge  *",
        bsod_attr);

    /* Wait for keypress or timeout */
    for (elapsed = 0; elapsed < duration; elapsed += 55) {
        if (scr_kbhit()) {
            scr_getkey();
            break;
        }
        engine_delay(55);
    }

    scr_restore_region(0, 0, SCR_WIDTH, SCR_HEIGHT, fx_save_buf);
}

/* ------------------------------------------------------------------ */
/* text_rewrite: no-op, handled by engine CMD_REWRITE                  */
/* ------------------------------------------------------------------ */

void fx_text_rewrite_noop(int duration, int intensity)
{
    (void)duration;
    (void)intensity;
}
