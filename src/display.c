/*
 * display.c - Display detection and mode setup
 *
 * Hardware detection runs first (VGA/EGA/CGA/MDA, AdLib, FPU),
 * then scr_init() sets text mode based on detected adapter.
 */

#include "display.h"
#include "hwdetect.h"
#include "screen.h"

void display_init(void)
{
    hw_detect_all();
    scr_init();
}

void display_shutdown(void)
{
    scr_shutdown();
}
