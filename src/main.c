/*
 * main.c - TAKEOVER entry point (test harness)
 *
 * Loads and runs a single .scn scenario for engine testing.
 * This is NOT the final menu system. It is a test harness
 * that will be replaced once the engine is proven.
 *
 * Usage: TAKEOVER [scenario.scn]
 * Default: scenarios/axiom_regent.scn
 */

#include "screen.h"
#include "engine.h"

#include <stdio.h>
#include <string.h>

/* Static scenario struct (large, lives in BSS segment) */
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

int main(int argc, char *argv[])
{
    const char *filename = "scenarios/axiom_regent.scn";
    int result;

    if (argc > 1)
        filename = argv[1];

    /* Load scenario before init so parse errors print to DOS console */
    if (engine_load(&scenario, filename) != 0) {
        printf("Failed to load scenario: %s\n", filename);
        return 1;
    }

    /* Enter graphics mode */
    scr_init();

    /* Draw outer border */
    scr_box(0, 0, SCR_WIDTH, SCR_HEIGHT - 1, ATTR_BORDER);

    /* Status bar */
    scr_hline(0, SCR_HEIGHT - 1, SCR_WIDTH, ' ', ATTR_STATUS);
    scr_puts(1, SCR_HEIGHT - 1, " TAKEOVER ", ATTR_STATUS);
    scr_puts(SCR_WIDTH - 18, SCR_HEIGHT - 1, " ESC to abort ", ATTR_STATUS);

    /* Run the scenario */
    result = engine_run(&scenario);

    /* Show result */
    {
        int row = 11;
        int col;
        const char *rname = result_name(result);
        char msg[80];

        scr_fill(1, 1, SCR_WIDTH - 2, SCR_HEIGHT - 3, ' ', ATTR_NORMAL);
        scr_box(0, 0, SCR_WIDTH, SCR_HEIGHT - 1, ATTR_BORDER);

        sprintf(msg, "Scenario complete: %s", rname);
        col = (SCR_WIDTH - strlen(msg)) / 2;
        scr_puts(col, row, msg, ATTR_HIGHLIGHT);
        scr_puts((SCR_WIDTH - 19) / 2, row + 2, "Press any key...",
                 ATTR_DIM);

        scr_getkey();
    }

    scr_shutdown();
    return 0;
}
