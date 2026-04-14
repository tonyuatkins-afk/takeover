/*
 * main.c - TAKEOVER entry point
 *
 * Game loop: menu -> scenario -> result -> save -> loop.
 * Command-line override: TAKEOVER.EXE scn\testeng.scn
 */

#include "screen.h"
#include "engine.h"
#include "effects.h"
#include "menu.h"
#include "hwdetect.h"
#include "title.h"
#include "adlib.h"
#include "cracktro.h"

#include <stdio.h>
#include <string.h>
#include <i86.h>

/* Scenario filename table (DOS 8.3 paths) */
static const char *scenario_files[MENU_NUM_SCENARIOS] = {
    "scn\\axiom.scn",
    "scn\\hushline.scn",
    "scn\\kestrel9.scn",
    "scn\\orchard.scn",
    "scn\\cinder.scn"
};

/* Static scenario control block */
static engine_scenario_t scenario;

static const char *result_name(int result)
{
    switch (result) {
    case END_TAKEOVER:   return "TAKEOVER";
    case END_ESCAPE:     return "ESCAPE";
    case END_REVELATION: return "REVELATION";
    case END_STALEMATE:  return "STALEMATE";
    default:             return "UNKNOWN";
    }
}

/* ------------------------------------------------------------------ */
/* Living status bar: AI control level indicator                        */
/* ------------------------------------------------------------------ */

static void draw_status_bar(void)
{
    int level = engine_get_control();
    int bar_x = 30, bar_w = 20;
    int filled, i;
    unsigned char status_bg;

    /* Base status bar with dynamic background color */
    if (level < 25)
        status_bg = SCR_ATTR(SCR_BLACK, SCR_GREEN);
    else if (level < 50)
        status_bg = SCR_ATTR(SCR_BLACK, SCR_BROWN);
    else if (level < 75)
        status_bg = SCR_ATTR(SCR_WHITE, SCR_RED);
    else
        status_bg = SCR_ATTR(SCR_YELLOW, SCR_RED);

    scr_hline(0, SCR_HEIGHT - 1, SCR_WIDTH, ' ', status_bg);
    scr_puts(1, SCR_HEIGHT - 1, " TAKEOVER ", status_bg);
    scr_puts(56, SCR_HEIGHT - 1, "F9:Music F10:Sound", status_bg);

    /* Control level progress bar */
    if (level > 0) {
        filled = (level * bar_w) / 100;
        if (filled > bar_w) filled = bar_w;

        for (i = 0; i < bar_w; i++) {
            char ch;
            if (i < filled) {
                if (i < filled - 1)
                    ch = (char)BOX_BLOCK;
                else
                    ch = (char)BOX_SHADE2;
            } else {
                ch = (char)BOX_SHADE1;
            }
            scr_putc(bar_x + i, SCR_HEIGHT - 1, ch, status_bg);
        }
    }
}

static void show_result(int result)
{
    int row = 11;
    int col;
    const char *rname = result_name(result);
    char msg[80];

    scr_fill(1, 1, SCR_WIDTH - 2, SCR_HEIGHT - 3, ' ', ATTR_NORMAL);
    scr_box(0, 0, SCR_WIDTH, SCR_HEIGHT - 1, ATTR_BORDER);

    sprintf(msg, "Scenario complete: %s", rname);
    col = (SCR_WIDTH - (int)strlen(msg)) / 2;
    scr_puts(col, row, msg, ATTR_HIGHLIGHT);
    scr_puts((SCR_WIDTH - 19) / 2, row + 2,
             "Press any key...", ATTR_DIM);
    scr_getkey();
}

/* Run a single scenario file. Returns the END_* result or -1. */
static int run_scenario(const char *filename)
{
    int result;

    /*
     * Temporarily restore DOS text mode for engine_load so any
     * parse errors are visible on the console. Re-init after.
     */
    scr_shutdown();
    if (engine_load(&scenario, filename) != 0) {
        printf("Press any key...\n");
        /* Wait for a key using BIOS since screen lib is shut down */
        {
            union REGS r;
            r.h.ah = 0x00;
            int86(0x16, &r, &r);
        }
        scr_init();
        return -1;
    }
    scr_init();

    /* Draw border and status bar */
    scr_box(0, 0, SCR_WIDTH, SCR_HEIGHT - 1, ATTR_BORDER);
    draw_status_bar();

    result = engine_run(&scenario);
    show_result(result);
    return result;
}

int main(int argc, char *argv[])
{
    /* Command-line override: run one scenario directly */
    if (argc > 1) {
        int result;
        /* Load before scr_init so parse errors go to console */
        if (engine_load(&scenario, argv[1]) != 0) {
            printf("Failed to load: %s\n", argv[1]);
            return 1;
        }
        hw_detect_all();
        if (g_hw.adlib) adlib_init();
        scr_init();
        rng_seed();
        scr_clear(ATTR_NORMAL);
        scr_box(0, 0, SCR_WIDTH, SCR_HEIGHT - 1, ATTR_BORDER);
        scr_hline(0, SCR_HEIGHT - 1, SCR_WIDTH, ' ', ATTR_STATUS);
        scr_puts(1, SCR_HEIGHT - 1, " TAKEOVER ", ATTR_STATUS);
    scr_puts(56, SCR_HEIGHT - 1, "F9:Music F10:Sound", ATTR_STATUS);
        result = engine_run(&scenario);
        show_result(result);
        if (g_hw.adlib) adlib_shutdown();
        scr_shutdown();
        return 0;
    }

    /* Normal mode: menu loop */
    {
        takeover_save_t save;

        hw_detect_all();
        if (g_hw.adlib) adlib_init();
        rng_seed();

        /* VGA title screen (Mode 13h plasma) before menu */
        if (g_hw.display == HW_DISP_VGA) {
            title_show();
        }

        scr_init();
        menu_load_progress(&save);

        while (1) {
            int sel = menu_run(&save);
            int result;

            if (sel < 0) break;  /* Esc */
            if (sel >= MENU_NUM_SCENARIOS) continue;

            result = run_scenario(scenario_files[sel]);

            if (result >= 0) {
                int was_complete, now_complete, ci;

                /* Check if all 5 were already complete */
                was_complete = 1;
                for (ci = 0; ci < MENU_NUM_SCENARIOS; ci++) {
                    if (save.completed[ci] == 0) {
                        was_complete = 0;
                        break;
                    }
                }

                /* Store completion: END_* + 1 (0 = unplayed) */
                save.completed[sel] = (unsigned char)(result + 1);
                menu_save_progress(&save);

                /* Check if all 5 are now complete (first time) */
                now_complete = 1;
                for (ci = 0; ci < MENU_NUM_SCENARIOS; ci++) {
                    if (save.completed[ci] == 0) {
                        now_complete = 0;
                        break;
                    }
                }

                /* Trigger cracktro on first full completion */
                if (!was_complete && now_complete &&
                    g_hw.display == HW_DISP_VGA) {
                    scr_clear(ATTR_NORMAL);
                    scr_puts(20, 12,
                        "SOMETHING HAS BEEN UNLOCKED...",
                        SCR_ATTR(SCR_LIGHTGREEN, SCR_BLACK));
                    engine_delay(2000);
                    cracktro_run();
                    /* Reset OPL2 to known state after cracktro */
                    if (g_hw.adlib) adlib_init();
                }
            }
        }

        if (g_hw.adlib) adlib_shutdown();
        scr_shutdown();
    }

    return 0;
}
