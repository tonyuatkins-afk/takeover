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
    scr_hline(0, SCR_HEIGHT - 1, SCR_WIDTH, ' ', ATTR_STATUS);
    scr_puts(1, SCR_HEIGHT - 1, " TAKEOVER ", ATTR_STATUS);

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
        scr_init();
        rng_seed();
        scr_clear(ATTR_NORMAL);
        scr_box(0, 0, SCR_WIDTH, SCR_HEIGHT - 1, ATTR_BORDER);
        scr_hline(0, SCR_HEIGHT - 1, SCR_WIDTH, ' ', ATTR_STATUS);
        scr_puts(1, SCR_HEIGHT - 1, " TAKEOVER ", ATTR_STATUS);
        result = engine_run(&scenario);
        show_result(result);
        scr_shutdown();
        return 0;
    }

    /* Normal mode: menu loop */
    {
        takeover_save_t save;

        scr_init();
        rng_seed();
        menu_load_progress(&save);

        while (1) {
            int sel = menu_run(&save);
            int result;

            if (sel < 0) break;  /* Esc */
            if (sel >= MENU_NUM_SCENARIOS) continue;

            result = run_scenario(scenario_files[sel]);

            if (result >= 0) {
                /* Store completion: END_* + 1 (0 = unplayed) */
                save.completed[sel] = (unsigned char)(result + 1);
                menu_save_progress(&save);
            }
        }

        scr_shutdown();
    }

    return 0;
}
