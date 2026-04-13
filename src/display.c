/*
 * display.c - Display detection and mode setup
 *
 * MDA detection and segment selection are handled by scr_init()
 * in the screen library. This module wraps that for the rest
 * of the application.
 */

#include "display.h"
#include "screen.h"

void display_init(void)
{
    scr_init();
}

void display_shutdown(void)
{
    scr_shutdown();
}
