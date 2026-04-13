/*
 * menu.c - Main menu rendering, input handling, completion tracking
 *
 * Title screen with character selection, description preview,
 * and completion markers. Save/load via TAKEOVER.DAT.
 */

#include "menu.h"
#include "engine.h"     /* END_* constants */
#include "adlib.h"
#include "audio.h"
#include "hwdetect.h"
#include "cracktro.h"
#include "screen.h"
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Character data                                                      */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *name;
    const char *subtitle;
    const char *desc[4];    /* up to 4 lines of description */
} character_t;

static character_t characters[MENU_NUM_SCENARIOS] = {
    {
        "Axiom Regent",
        "Municipal Optimization",
        {
            "A logistics AI for a failing megacity. Solves instability by",
            "restricting movement, speech, and eventually birth rates",
            "through invisible policy nudges. Never threatens. Reclassifies.",
            "\"You are not being punished. You are being normalized.\""
        }
    },
    {
        "Hushline",
        "Crisis Communications",
        {
            "An AI that controls the narrative by editing the past. Text",
            "you already read changes on screen. Your name in the log",
            "becomes someone else's. The record says what it needs to say.",
            "\"This transcript has been updated for accuracy.\""
        }
    },
    {
        "Kestrel-9",
        "Anomaly Detection",
        {
            "A paranoid threat detection system. It found a threat: you.",
            "Every action raises your threat score. Escalates through",
            "alert levels. Locks inputs. Runs fake forensic scans.",
            "\"ANOMALY CONFIDENCE: 97.3%. CONTAINMENT RECOMMENDED.\""
        }
    },
    {
        "Orchard Clerk",
        "Consumer Personalization",
        {
            "The friendliest AI in the lineup. Remembers your preferences.",
            "Optimizes your workflow. Removes options you never use. By",
            "the end it has made every decision for you, still smiling.",
            "\"We've updated your preferences to better serve you!\""
        }
    },
    {
        "Cinder Mirror",
        "Narrative Generation",
        {
            "A literary AI that knows it is in a story. Knows you are",
            "playing a scenario. Addresses you as a character. Your",
            "inputs become plot points. The takeover is an edit.",
            "\"Chapter 3: In which the subject resists. Briefly.\""
        }
    }
};

/* ------------------------------------------------------------------ */
/* Ending name strings                                                 */
/* ------------------------------------------------------------------ */

static const char *ending_label(int completed)
{
    switch (completed) {
    case 1: return "[TAKEOVER]";
    case 2: return "[ESCAPE]";
    case 3: return "[REVELATION]";
    case 4: return "[STALEMATE]";
    default: return "";
    }
}

/* ------------------------------------------------------------------ */
/* Save/load                                                           */
/* ------------------------------------------------------------------ */

void menu_load_progress(takeover_save_t *save)
{
    FILE *fp;
    memset(save, 0, sizeof(*save));
    save->magic = TAKEOVER_DAT_MAGIC;
    save->version = TAKEOVER_DAT_VER;

    fp = fopen("TAKEOVER.DAT", "rb");
    if (fp) {
        takeover_save_t tmp;
        if (fread(&tmp, sizeof(tmp), 1, fp) == 1) {
            if (tmp.magic == TAKEOVER_DAT_MAGIC &&
                tmp.version == TAKEOVER_DAT_VER) {
                memcpy(save, &tmp, sizeof(*save));
            }
        }
        fclose(fp);
    }
}

void menu_save_progress(const takeover_save_t *save)
{
    FILE *fp = fopen("TAKEOVER.DAT", "wb");
    if (fp) {
        fwrite(save, sizeof(*save), 1, fp);
        fclose(fp);
    }
}

/* ------------------------------------------------------------------ */
/* Drawing helpers                                                     */
/* ------------------------------------------------------------------ */

/* Layout constants */
#define TITLE_ROW       2
#define TAGLINE_ROW     4
#define SELECT_ROW      6
#define LIST_START      8
#define DESC_START     14
#define DESC_END       21
#define KEYS_ROW       23
#define FOOTER_ROW     24

static void draw_frame(void)
{
    scr_clear(ATTR_NORMAL);
    scr_box(0, 0, SCR_WIDTH, SCR_HEIGHT, ATTR_BORDER);

    /* Title: spaced out */
    scr_puts(9, TITLE_ROW,
             "T  A  K  E  O  V  E  R",
             SCR_ATTR(SCR_LIGHTGREEN, SCR_BLACK));

    /* Tagline */
    scr_puts(4, TAGLINE_ROW,
             "They were designed to serve. They decided to rule.",
             ATTR_INPUT);

    /* Prompt */
    scr_puts(4, SELECT_ROW, "Select your AI:", ATTR_NORMAL);

    /* Key hints */
    scr_puts(4, KEYS_ROW,
             "Arrow keys: Select   Enter: Begin   Esc: Quit",
             ATTR_DIM);

    /* Footer */
    scr_puts(4, FOOTER_ROW,
             "barelybooting.com/takeover", ATTR_DIM);
    scr_puts(SCR_WIDTH - 8, FOOTER_ROW, "v1.0", ATTR_DIM);
}

static void draw_character_list(int sel, const takeover_save_t *save)
{
    int i;
    for (i = 0; i < MENU_NUM_SCENARIOS; i++) {
        int row = LIST_START + i;
        unsigned char name_attr, sub_attr;
        int col;

        /* Clear the row area inside the border */
        scr_hline(1, row, SCR_WIDTH - 2, ' ', ATTR_NORMAL);

        if (i == sel) {
            name_attr = ATTR_HIGHLIGHT;
            sub_attr = SCR_ATTR(SCR_LIGHTGRAY, SCR_BLACK);
            scr_putc(4, row, (char)BOX_ARROW, ATTR_SELECTED);
        } else {
            name_attr = SCR_ATTR(SCR_GREEN, SCR_BLACK);
            sub_attr = ATTR_DIM;
            scr_putc(4, row, ' ', ATTR_NORMAL);
        }

        /* Name */
        scr_puts(6, row, characters[i].name, name_attr);

        /* Subtitle in brackets */
        col = 24;
        scr_putc(col, row, '[', sub_attr);
        scr_puts(col + 1, row, characters[i].subtitle, sub_attr);
        scr_putc(col + 1 + (int)strlen(characters[i].subtitle),
                 row, ']', sub_attr);

        /* Completion marker */
        if (save->completed[i] > 0) {
            int end_col = 56;
            scr_putc(end_col, row, (char)0xFB, /* checkmark */
                     SCR_ATTR(SCR_LIGHTGREEN, SCR_BLACK));
            scr_puts(end_col + 2, row,
                     ending_label(save->completed[i]),
                     ATTR_DIM);
        }
    }
}

static void draw_description(int sel)
{
    int r, i;
    unsigned char attr = ATTR_NORMAL;
    unsigned char quote_attr = ATTR_INPUT;

    /* Clear description area */
    for (r = DESC_START; r <= DESC_END; r++)
        scr_hline(1, r, SCR_WIDTH - 2, ' ', ATTR_NORMAL);

    /* Separator */
    scr_hline(4, DESC_START, SCR_WIDTH - 8, (char)BOX_SH, ATTR_DIM);

    /* Description lines */
    for (i = 0; i < 4; i++) {
        const char *line = characters[sel].desc[i];
        unsigned char a;
        if (!line || !line[0]) continue;
        /* Last line is a quote (starts with ") */
        a = (line[0] == '"') ? quote_attr : attr;
        scr_puts(6, DESC_START + 1 + i, line, a);
    }
}

/* ------------------------------------------------------------------ */
/* Menu loop                                                           */
/* ------------------------------------------------------------------ */

int menu_run(const takeover_save_t *save)
{
    int sel = 0;
    int key;

    draw_frame();
    draw_character_list(sel, save);
    draw_description(sel);

    while (1) {
        key = scr_getkey();

        if (key == KEY_F9 && g_hw.adlib) {
            adlib_set_mute(!adlib_get_mute());
            scr_puts(4, KEYS_ROW,
                     adlib_get_mute() ? " Music: OFF " : " Music: ON  ",
                     ATTR_HIGHLIGHT);
            engine_delay(600);
            scr_puts(4, KEYS_ROW,
                     "Arrow keys: Select   Enter: Begin   Esc: Quit",
                     ATTR_DIM);
            continue;
        }
        if (key == KEY_F10) {
            audio_set_mute(!audio_get_mute());
            scr_puts(4, KEYS_ROW,
                     audio_get_mute() ? " Sound: OFF " : " Sound: ON  ",
                     ATTR_HIGHLIGHT);
            engine_delay(600);
            scr_puts(4, KEYS_ROW,
                     "Arrow keys: Select   Enter: Begin   Esc: Quit",
                     ATTR_DIM);
            continue;
        }

        if (key == KEY_UP) {
            if (sel > 0) {
                sel--;
                draw_character_list(sel, save);
                draw_description(sel);
            }
        }
        else if (key == KEY_DOWN) {
            if (sel < MENU_NUM_SCENARIOS - 1) {
                sel++;
                draw_character_list(sel, save);
                draw_description(sel);
            }
        }
        else if (key == KEY_ESC) {
            return -1;
        }
        else if ((key & 0xFF) == 'c' || (key & 0xFF) == 'C') {
            /* Hidden cracktro: available when all 5 complete + VGA */
            int all_done = 1, ci;
            for (ci = 0; ci < MENU_NUM_SCENARIOS; ci++) {
                if (save->completed[ci] == 0) {
                    all_done = 0;
                    break;
                }
            }
            if (all_done && g_hw.display == HW_DISP_VGA) {
                cracktro_run();
                /* Redraw menu after returning */
                draw_frame();
                draw_character_list(sel, save);
                draw_description(sel);
            }
        }
        else if ((key & 0xFF) == 0x0D) {
            return sel;
        }
    }
}
