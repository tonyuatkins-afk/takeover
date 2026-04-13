/*
 * display.h - Display detection and mode setup
 *
 * Thin wrapper around scr_init/scr_shutdown.
 * MDA vs CGA/VGA detection is handled inside the screen library.
 */

#ifndef DISPLAY_H
#define DISPLAY_H

void display_init(void);
void display_shutdown(void);

#endif /* DISPLAY_H */
