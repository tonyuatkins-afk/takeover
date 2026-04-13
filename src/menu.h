/*
 * menu.h - Main menu and character selection interface
 *
 * Provides the title screen, character selection with descriptions,
 * and completion tracking via TAKEOVER.DAT.
 */

#ifndef MENU_H
#define MENU_H

#define MENU_NUM_SCENARIOS  5
#define MENU_MAX_SCENARIOS  8   /* room for expansion in save file */
#define TAKEOVER_DAT_MAGIC  0x544Bu  /* "TK" */
#define TAKEOVER_DAT_VER    1

/* Completion state: 0=unplayed, 1-4=ending type (matches END_* + 1) */
typedef struct {
    unsigned short magic;
    unsigned char version;
    unsigned char completed[MENU_MAX_SCENARIOS];
} takeover_save_t;

/* Load/save progress from TAKEOVER.DAT */
void menu_load_progress(takeover_save_t *save);
void menu_save_progress(const takeover_save_t *save);

/* Run the menu. Returns selected scenario index (0-4) or -1 for Esc. */
int menu_run(const takeover_save_t *save);

#endif /* MENU_H */
