/*
 * title.h - Animated plasma title screen
 *
 * Requires VGA. Caller must check g_hw.display == HW_DISP_VGA
 * before calling. Shows plasma animation with TAKEOVER logo,
 * waits for keypress, fades out, returns.
 */

#ifndef TITLE_H
#define TITLE_H

/* Show title screen. Enters Mode 13h, blocks until keypress, fades out.
 * Caller must restore text mode after (scr_init or vga_leave_13h). */
void title_show(void);

#endif /* TITLE_H */
