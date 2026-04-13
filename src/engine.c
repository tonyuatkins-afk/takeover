/*
 * engine.c - .scn parser, state machine executor, variable system
 *
 * Architecture: all heavy data lives in module-static arrays.
 * Command strings use a shared string pool (22KB) referenced by
 * 16-bit offsets. This keeps each command at 12 bytes, allowing
 * 64 states x 32 commands in ~27KB of state storage.
 * Total static footprint ~57KB (engine + effects), fitting in
 * DGROUP with ~4.5KB headroom for stack.
 *
 * Only one scenario can be loaded at a time.
 */

#include "engine.h"
#include "effects.h"
#include "audio.h"
#include "adlib.h"
#include "hwdetect.h"
#include "screen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <dos.h>

/* ------------------------------------------------------------------ */
/* Internal limits                                                     */
/* ------------------------------------------------------------------ */

#define MAX_STATES       64
#define MAX_COMMANDS     32     /* per state */
#define MAX_VARS         48
#define MAX_CHOICES       8
#define MAX_TAGS         32
#define MAX_LINE        162     /* 160 chars + CRLF */
#define MAX_NAME         32
#define MAX_STRING      160     /* max length of any single string */
#define POOL_SIZE     22528     /* 22KB string pool */
#define POOL_NONE    0xFFFFu    /* sentinel: no string */

/* ------------------------------------------------------------------ */
/* Command types                                                       */
/* ------------------------------------------------------------------ */

enum {
    CMD_TEXT,
    CMD_TEXT_TAGGED,
    CMD_ATTR,
    CMD_DELAY,
    CMD_CLEAR,
    CMD_INPUT,
    CMD_CHOICE,
    CMD_GOTO,
    CMD_END,
    CMD_SET_STR,
    CMD_SET_NUM,
    CMD_IF_EQ,
    CMD_IF_NEQ,
    CMD_IF_GT,
    CMD_IF_LT,
    CMD_INC,
    CMD_DEC,
    CMD_EFFECT,
    CMD_REWRITE,
    CMD_LOCK_INPUT,
    CMD_FAKE_ERROR,
    CMD_HIJACK_PROMPT,
    CMD_TONE,
    CMD_SILENCE,
    CMD_NEWS_FALLBACK,
    CMD_NEWS_INJECT,
    CMD_MUSIC,
    CMD_MUSIC_STOP,
    CMD_GFX_FADE
};

/* Clear regions */
enum { CLEAR_SCREEN, CLEAR_MAIN, CLEAR_STATUS };

/* Attribute IDs */
enum {
    ATTR_ID_GREEN,
    ATTR_ID_BRIGHT_GREEN,
    ATTR_ID_AMBER,
    ATTR_ID_RED,
    ATTR_ID_BRIGHT_RED,
    ATTR_ID_WHITE,
    ATTR_ID_BRIGHT_WHITE,
    ATTR_ID_GREY,
    ATTR_ID_CYAN,
    ATTR_ID_BRIGHT_CYAN,
    ATTR_ID_INVERSE,
    ATTR_ID_COUNT
};

/* ------------------------------------------------------------------ */
/* Internal data structures                                            */
/* ------------------------------------------------------------------ */

/* A single command: 14 bytes. Strings are pool offsets. */
typedef struct {
    unsigned short type;    /* CMD_* */
    unsigned short str1;    /* pool offset for primary string */
    unsigned short str2;    /* pool offset for secondary string */
    unsigned short tag;     /* pool offset for @tag name */
    int num1;
    int num2;
} cmd_t;

/* A state: name + commands. ~480 bytes each. */
typedef struct {
    char name[MAX_NAME];
    cmd_t commands[MAX_COMMANDS];
    int cmd_count;
} state_t;

/* A variable */
typedef struct {
    char name[MAX_NAME];
    char str_val[40];       /* most values are short */
    int  num_val;
    int  is_numeric;
    int  in_use;
} var_t;

/* A tagged text location */
typedef struct {
    char tag[MAX_NAME];
    int  row;
    int  col;
    int  len;
    int  in_use;
} tag_t;

/* ------------------------------------------------------------------ */
/* Module-static storage                                               */
/* ------------------------------------------------------------------ */

static state_t g_states[MAX_STATES];    /* ~60KB */
static var_t   g_vars[MAX_VARS];        /* ~6KB */
static tag_t   g_tags[MAX_TAGS];        /* ~1.5KB */
static char    g_pool[POOL_SIZE];       /* 16KB string pool */
static int     g_pool_used;

/* ------------------------------------------------------------------ */
/* String pool                                                         */
/* ------------------------------------------------------------------ */

static const char *pool_str(unsigned short off)
{
    if (off == POOL_NONE) return "";
    return &g_pool[off];
}

static unsigned short pool_add(const char *s)
{
    int len = strlen(s) + 1;
    unsigned short off;
    if (g_pool_used + len > POOL_SIZE) return POOL_NONE;
    off = (unsigned short)g_pool_used;
    memcpy(&g_pool[off], s, len);
    g_pool_used += len;
    return off;
}

/* ------------------------------------------------------------------ */
/* Attribute mapping                                                   */
/* ------------------------------------------------------------------ */

static unsigned char attr_map[ATTR_ID_COUNT] = {
    SCR_ATTR(SCR_GREEN, SCR_BLACK),         /* GREEN */
    SCR_ATTR(SCR_LIGHTGREEN, SCR_BLACK),    /* BRIGHT_GREEN */
    SCR_ATTR(SCR_YELLOW, SCR_BLACK),        /* AMBER */
    SCR_ATTR(SCR_RED, SCR_BLACK),           /* RED */
    SCR_ATTR(SCR_LIGHTRED, SCR_BLACK),      /* BRIGHT_RED */
    SCR_ATTR(SCR_LIGHTGRAY, SCR_BLACK),     /* WHITE */
    SCR_ATTR(SCR_WHITE, SCR_BLACK),         /* BRIGHT_WHITE */
    SCR_ATTR(SCR_DARKGRAY, SCR_BLACK),      /* GREY */
    SCR_ATTR(SCR_CYAN, SCR_BLACK),          /* CYAN */
    SCR_ATTR(SCR_LIGHTCYAN, SCR_BLACK),     /* BRIGHT_CYAN */
    SCR_ATTR(SCR_BLACK, SCR_LIGHTGRAY)      /* INVERSE */
};

/* ------------------------------------------------------------------ */
/* Effect dispatch table                                               */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *name;
    effect_fn   func;
} effect_entry_t;

static effect_entry_t effect_table[] = {
    { "typing_effect",    fx_typing_effect },
    { "screen_flicker",   fx_screen_flicker },
    { "static_burst",     fx_static_burst },
    { "text_corrupt",     fx_text_corrupt },
    { "text_redact",      fx_text_redact },
    { "text_rewrite",     fx_text_rewrite_noop },
    { "color_shift",      fx_color_shift },
    { "blackout",         fx_blackout },
    { "red_alert",        fx_red_alert },
    { "screen_scroll",    fx_screen_scroll },
    { "cursor_possessed", fx_cursor_possessed },
    { "screen_melt",      fx_screen_melt },
    { "falling_chars",    fx_falling_chars },
    { "progress_bar",     fx_progress_bar },
    { "fake_bsod",        fx_fake_bsod },
    { "pulse_border",    fx_pulse_border },
    { "interference",    fx_interference },
    { NULL, NULL }
};

/* Main content area boundaries */
#define MAIN_TOP    1
#define MAIN_BOTTOM 22
#define MAIN_LEFT   1
#define MAIN_RIGHT  78
#define STATUS_ROW  24

/* ------------------------------------------------------------------ */
/* Timing                                                              */
/* ------------------------------------------------------------------ */

void engine_delay(int ms)
{
    unsigned long far *tick = (unsigned long far *)MK_FP(0x0040, 0x006C);
    unsigned long ticks;
    unsigned long start;

    if (ms <= 0) return;
    ticks = (unsigned long)ms * 182UL / 10000UL;
    if (ticks == 0) ticks = 1;
    start = *tick;
    while ((*tick - start) < ticks) {
        adlib_tick();   /* advance background music */
    }
}

/* ------------------------------------------------------------------ */
/* String helpers                                                      */
/* ------------------------------------------------------------------ */

static char *trim_left(char *s)
{
    while (*s == ' ' || *s == '\t')
        s++;
    return s;
}

static void trim_right(char *s)
{
    int len = strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t' ||
           s[len-1] == '\r' || s[len-1] == '\n'))
        s[--len] = '\0';
}

/* Extract quoted string into dest. Returns pointer past closing quote. */
static char *extract_quoted(char *src, char *dest, int dest_max)
{
    int i = 0;
    char *p = trim_left(src);
    if (*p != '"') return NULL;
    p++;
    while (*p && *p != '"' && i < dest_max - 1)
        dest[i++] = *p++;
    dest[i] = '\0';
    if (*p == '"') p++;
    return p;
}

/* Find "->" separator, return pointer past it (trimmed). */
static char *find_arrow(char *s)
{
    char *p = strstr(s, "->");
    if (!p) return NULL;
    return trim_left(p + 2);
}

/* ------------------------------------------------------------------ */
/* Variable system                                                     */
/* ------------------------------------------------------------------ */

static var_t *find_var(const char *name)
{
    int i;
    for (i = 0; i < MAX_VARS; i++) {
        if (g_vars[i].in_use && strcmp(g_vars[i].name, name) == 0)
            return &g_vars[i];
    }
    return NULL;
}

static var_t *alloc_var(const char *name)
{
    int i;
    var_t *v = find_var(name);
    if (v) return v;
    for (i = 0; i < MAX_VARS; i++) {
        if (!g_vars[i].in_use) {
            v = &g_vars[i];
            memset(v, 0, sizeof(*v));
            strncpy(v->name, name, MAX_NAME - 1);
            v->in_use = 1;
            return v;
        }
    }
    return NULL;
}

static void set_var(const char *name, const char *str_val,
                    int num_val, int is_numeric)
{
    var_t *v = alloc_var(name);
    if (!v) return;
    v->is_numeric = is_numeric;
    v->num_val = num_val;
    if (str_val)
        strncpy(v->str_val, str_val, sizeof(v->str_val) - 1);
    else
        v->str_val[0] = '\0';
}

const char *engine_get_var_str(engine_scenario_t *scn, const char *name)
{
    var_t *v = find_var(name);
    (void)scn;
    if (!v) return "";
    if (v->is_numeric)
        sprintf(v->str_val, "%d", v->num_val);
    return v->str_val;
}

int engine_get_var_num(engine_scenario_t *scn, const char *name)
{
    var_t *v = find_var(name);
    (void)scn;
    if (!v) return 0;
    return v->num_val;
}

/* Variable substitution: replace {var_name} with values */
static void substitute_vars(engine_scenario_t *scn, const char *src,
                            char *dest, int dest_len)
{
    int di = 0;
    const char *p = src;

    while (*p && di < dest_len - 1) {
        if (*p == '{') {
            char vname[MAX_NAME];
            const char *end = strchr(p + 1, '}');
            if (end) {
                int nlen = (int)(end - p - 1);
                const char *val;
                if (nlen >= MAX_NAME) nlen = MAX_NAME - 1;
                memcpy(vname, p + 1, nlen);
                vname[nlen] = '\0';
                val = engine_get_var_str(scn, vname);
                while (*val && di < dest_len - 1)
                    dest[di++] = *val++;
                p = end + 1;
            } else {
                dest[di++] = *p++;
            }
        } else {
            dest[di++] = *p++;
        }
    }
    dest[di] = '\0';
}

/* ------------------------------------------------------------------ */
/* Tag system                                                          */
/* ------------------------------------------------------------------ */

static tag_t *find_tag(const char *name)
{
    int i;
    for (i = 0; i < MAX_TAGS; i++) {
        if (g_tags[i].in_use && strcmp(g_tags[i].tag, name) == 0)
            return &g_tags[i];
    }
    return NULL;
}

static tag_t *alloc_tag(const char *name)
{
    int i;
    tag_t *t = find_tag(name);
    if (t) return t;
    for (i = 0; i < MAX_TAGS; i++) {
        if (!g_tags[i].in_use) {
            t = &g_tags[i];
            memset(t, 0, sizeof(*t));
            strncpy(t->tag, name, MAX_NAME - 1);
            t->in_use = 1;
            return t;
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* State lookup                                                        */
/* ------------------------------------------------------------------ */

static int find_state(int state_count, const char *name)
{
    int i;
    for (i = 0; i < state_count; i++) {
        if (strcmp(g_states[i].name, name) == 0)
            return i;
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/* Parser internals                                                    */
/* ------------------------------------------------------------------ */

static int parse_attr_name(const char *name)
{
    if (strcmp(name, "green") == 0)        return ATTR_ID_GREEN;
    if (strcmp(name, "bright_green") == 0) return ATTR_ID_BRIGHT_GREEN;
    if (strcmp(name, "amber") == 0)        return ATTR_ID_AMBER;
    if (strcmp(name, "red") == 0)          return ATTR_ID_RED;
    if (strcmp(name, "bright_red") == 0)   return ATTR_ID_BRIGHT_RED;
    if (strcmp(name, "white") == 0)        return ATTR_ID_WHITE;
    if (strcmp(name, "bright_white") == 0) return ATTR_ID_BRIGHT_WHITE;
    if (strcmp(name, "grey") == 0)         return ATTR_ID_GREY;
    if (strcmp(name, "cyan") == 0)         return ATTR_ID_CYAN;
    if (strcmp(name, "bright_cyan") == 0)  return ATTR_ID_BRIGHT_CYAN;
    if (strcmp(name, "inverse") == 0)      return ATTR_ID_INVERSE;
    return ATTR_ID_GREEN;
}

static int parse_clear_region(const char *name)
{
    if (strcmp(name, "screen") == 0) return CLEAR_SCREEN;
    if (strcmp(name, "main") == 0)   return CLEAR_MAIN;
    if (strcmp(name, "status") == 0) return CLEAR_STATUS;
    return CLEAR_SCREEN;
}

static int parse_end_type(const char *name)
{
    if (strcmp(name, "takeover") == 0)    return END_TAKEOVER;
    if (strcmp(name, "escape") == 0)      return END_ESCAPE;
    if (strcmp(name, "revelation") == 0)  return END_REVELATION;
    if (strcmp(name, "stalemate") == 0)   return END_STALEMATE;
    return END_TAKEOVER;
}

/* Parse one command line into a cmd_t. Returns 0 on success. */
static int parse_command(char *line, cmd_t *cmd, int line_num)
{
    char *colon, *keyword, *args;
    char buf1[MAX_STRING], buf2[MAX_STRING];

    memset(cmd, 0, sizeof(*cmd));
    cmd->str1 = POOL_NONE;
    cmd->str2 = POOL_NONE;
    cmd->tag  = POOL_NONE;

    colon = strchr(line, ':');
    if (!colon) {
        printf("Line %d: missing ':' in command\n", line_num);
        return -1;
    }

    *colon = '\0';
    keyword = trim_left(line);
    trim_right(keyword);
    args = trim_left(colon + 1);
    trim_right(args);

    if (strcmp(keyword, "text") == 0) {
        char *after = extract_quoted(args, buf1, MAX_STRING);
        if (!after) {
            printf("Line %d: text requires quoted string\n", line_num);
            return -1;
        }
        cmd->str1 = pool_add(buf1);
        after = trim_left(after);
        if (*after == '@') {
            cmd->type = CMD_TEXT_TAGGED;
            strncpy(buf2, after + 1, MAX_NAME - 1);
            buf2[MAX_NAME - 1] = '\0';
            trim_right(buf2);
            cmd->tag = pool_add(buf2);
        } else {
            cmd->type = CMD_TEXT;
        }
    }
    else if (strcmp(keyword, "attr") == 0) {
        cmd->type = CMD_ATTR;
        cmd->num1 = parse_attr_name(args);
    }
    else if (strcmp(keyword, "delay") == 0) {
        cmd->type = CMD_DELAY;
        cmd->num1 = atoi(args);
    }
    else if (strcmp(keyword, "clear") == 0) {
        cmd->type = CMD_CLEAR;
        cmd->num1 = parse_clear_region(args);
    }
    else if (strcmp(keyword, "input") == 0) {
        char *after, *target;
        cmd->type = CMD_INPUT;
        after = extract_quoted(args, buf1, MAX_STRING);
        if (!after) {
            printf("Line %d: input requires quoted prompt\n", line_num);
            return -1;
        }
        cmd->str1 = pool_add(buf1);
        target = find_arrow(after);
        if (!target) {
            printf("Line %d: input requires -> variable\n", line_num);
            return -1;
        }
        strncpy(buf2, target, MAX_NAME - 1);
        buf2[MAX_NAME - 1] = '\0';
        trim_right(buf2);
        cmd->str2 = pool_add(buf2);
    }
    else if (strcmp(keyword, "choice") == 0) {
        char *after, *target;
        cmd->type = CMD_CHOICE;
        after = extract_quoted(args, buf1, MAX_STRING);
        if (!after) {
            printf("Line %d: choice requires quoted label\n", line_num);
            return -1;
        }
        cmd->str1 = pool_add(buf1);
        target = find_arrow(after);
        if (!target) {
            printf("Line %d: choice requires -> target_state\n", line_num);
            return -1;
        }
        strncpy(buf2, target, MAX_NAME - 1);
        buf2[MAX_NAME - 1] = '\0';
        trim_right(buf2);
        cmd->str2 = pool_add(buf2);
    }
    else if (strcmp(keyword, "goto") == 0) {
        cmd->type = CMD_GOTO;
        strncpy(buf1, args, MAX_NAME - 1);
        buf1[MAX_NAME - 1] = '\0';
        cmd->str1 = pool_add(buf1);
    }
    else if (strcmp(keyword, "end") == 0) {
        cmd->type = CMD_END;
        cmd->num1 = parse_end_type(args);
    }
    else if (strcmp(keyword, "set") == 0) {
        char *eq = strchr(args, '=');
        char *val;
        int nlen;
        if (!eq) {
            printf("Line %d: set requires '='\n", line_num);
            return -1;
        }
        nlen = (int)(eq - args);
        if (nlen >= MAX_NAME) nlen = MAX_NAME - 1;
        memcpy(buf2, args, nlen);
        buf2[nlen] = '\0';
        trim_right(buf2);
        cmd->str2 = pool_add(buf2);

        val = trim_left(eq + 1);
        if (*val == '"') {
            cmd->type = CMD_SET_STR;
            extract_quoted(val, buf1, MAX_STRING);
            cmd->str1 = pool_add(buf1);
        } else {
            cmd->type = CMD_SET_NUM;
            cmd->num1 = atoi(val);
        }
    }
    else if (strcmp(keyword, "if") == 0) {
        char *p = args;
        int vi = 0;
        char *target;

        /* Variable name */
        while (*p && *p != '=' && *p != '!' && *p != '>' && *p != '<'
               && *p != ' ' && vi < MAX_NAME - 1)
            buf2[vi++] = *p++;
        buf2[vi] = '\0';
        cmd->str2 = pool_add(buf2);

        p = trim_left(p);
        if (p[0] == '=' && p[1] == '=') {
            cmd->type = CMD_IF_EQ;
            p = trim_left(p + 2);
        } else if (p[0] == '!' && p[1] == '=') {
            cmd->type = CMD_IF_NEQ;
            p = trim_left(p + 2);
        } else if (p[0] == '>') {
            cmd->type = CMD_IF_GT;
            p = trim_left(p + 1);
        } else if (p[0] == '<') {
            cmd->type = CMD_IF_LT;
            p = trim_left(p + 1);
        } else {
            printf("Line %d: if requires ==, !=, >, or <\n", line_num);
            return -1;
        }

        /* Value */
        if (*p == '"') {
            p = extract_quoted(p, buf1, MAX_STRING);
            if (!p) {
                printf("Line %d: if requires quoted value\n", line_num);
                return -1;
            }
            cmd->str1 = pool_add(buf1);
        } else {
            cmd->num1 = atoi(p);
            if (cmd->type == CMD_IF_EQ || cmd->type == CMD_IF_NEQ) {
                char nbuf[12];
                sprintf(nbuf, "%d", cmd->num1);
                cmd->str1 = pool_add(nbuf);
            }
            while (*p && *p != ' ' && *p != '\t') {
                if (*p == '-' && p[1] == '>') break;
                p++;
            }
        }

        /* -> target */
        target = find_arrow(p);
        if (!target) {
            printf("Line %d: if requires -> target_state\n", line_num);
            return -1;
        }
        strncpy(buf1, target, MAX_NAME - 1);
        buf1[MAX_NAME - 1] = '\0';
        trim_right(buf1);
        cmd->tag = pool_add(buf1);
    }
    else if (strcmp(keyword, "inc") == 0) {
        cmd->type = CMD_INC;
        cmd->str1 = pool_add(args);
    }
    else if (strcmp(keyword, "dec") == 0) {
        cmd->type = CMD_DEC;
        cmd->str1 = pool_add(args);
    }
    else if (strcmp(keyword, "effect") == 0) {
        char *p = args;
        int ni = 0;
        cmd->type = CMD_EFFECT;
        cmd->num1 = 500;
        cmd->num2 = 5;

        while (*p && *p != ' ' && *p != '\t' && ni < MAX_NAME - 1)
            buf1[ni++] = *p++;
        buf1[ni] = '\0';
        cmd->str1 = pool_add(buf1);

        p = trim_left(p);
        if (*p) {
            cmd->num1 = atoi(p);
            while (*p && *p != ' ' && *p != '\t') p++;
            p = trim_left(p);
            if (*p) cmd->num2 = atoi(p);
        }
    }
    else if (strcmp(keyword, "rewrite") == 0) {
        char *p = trim_left(args);
        cmd->type = CMD_REWRITE;
        if (*p == '@') {
            int ti = 0;
            p++;
            while (*p && *p != ' ' && *p != '\t' && ti < MAX_NAME - 1)
                buf2[ti++] = *p++;
            buf2[ti] = '\0';
            cmd->tag = pool_add(buf2);
            p = trim_left(p);
            extract_quoted(p, buf1, MAX_STRING);
            cmd->str1 = pool_add(buf1);
        } else {
            printf("Line %d: rewrite requires @tag\n", line_num);
            return -1;
        }
    }
    else if (strcmp(keyword, "lock_input") == 0) {
        cmd->type = CMD_LOCK_INPUT;
        cmd->num1 = atoi(args);
    }
    else if (strcmp(keyword, "fake_error") == 0) {
        cmd->type = CMD_FAKE_ERROR;
        extract_quoted(args, buf1, MAX_STRING);
        cmd->str1 = pool_add(buf1);
    }
    else if (strcmp(keyword, "hijack_prompt") == 0) {
        cmd->type = CMD_HIJACK_PROMPT;
        extract_quoted(args, buf1, MAX_STRING);
        cmd->str1 = pool_add(buf1);
    }
    else if (strcmp(keyword, "tone") == 0) {
        cmd->type = CMD_TONE;
        cmd->num1 = atoi(args);
        while (*args && *args != ' ' && *args != '\t') args++;
        args = trim_left(args);
        cmd->num2 = atoi(args);
    }
    else if (strcmp(keyword, "silence") == 0) {
        cmd->type = CMD_SILENCE;
        cmd->num1 = atoi(args);
    }
    else if (strcmp(keyword, "news_fallback") == 0) {
        cmd->type = CMD_NEWS_FALLBACK;
        extract_quoted(args, buf1, MAX_STRING);
        cmd->str1 = pool_add(buf1);
    }
    else if (strcmp(keyword, "news_inject") == 0) {
        cmd->type = CMD_NEWS_INJECT;
        strncpy(buf1, args, MAX_NAME - 1);
        buf1[MAX_NAME - 1] = '\0';
        cmd->str1 = pool_add(buf1);
    }
    else if (strcmp(keyword, "music") == 0) {
        cmd->type = CMD_MUSIC;
        extract_quoted(args, buf1, MAX_STRING);
        cmd->str1 = pool_add(buf1);
    }
    else if (strcmp(keyword, "music_stop") == 0) {
        cmd->type = CMD_MUSIC_STOP;
    }
    else if (strcmp(keyword, "gfx_fade") == 0) {
        cmd->type = CMD_GFX_FADE;
        cmd->num1 = atoi(args); /* duration ms, 0 = default 1000 */
    }
    else {
        printf("Line %d: unknown command '%s'\n", line_num, keyword);
        return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/* Public: load a scenario                                             */
/* ------------------------------------------------------------------ */

int engine_load(engine_scenario_t *scn, const char *filename)
{
    FILE *fp;
    char line[MAX_LINE];
    int line_num = 0;
    int cur_state = -1;
    int init_found = 0;
    int i, j;

    memset(scn, 0, sizeof(*scn));
    memset(g_states, 0, sizeof(g_states));
    memset(g_vars, 0, sizeof(g_vars));
    memset(g_tags, 0, sizeof(g_tags));
    g_pool_used = 1;  /* offset 0 reserved so POOL_NONE (0xFFFF) is distinct from offset 0 */
    g_pool[0] = '\0'; /* offset 0 = empty string */

    fp = fopen(filename, "r");
    if (!fp) {
        printf("Error: cannot open '%s'\n", filename);
        return -1;
    }

    while (fgets(line, MAX_LINE, fp)) {
        char *p;
        line_num++;

        trim_right(line);
        p = trim_left(line);

        if (*p == '\0' || *p == '#')
            continue;

        /* State header: [state: name] */
        if (*p == '[') {
            char *colon = strchr(p, ':');
            char *close = strchr(p, ']');
            char *name;

            if (!colon || !close || close <= colon) {
                printf("Line %d: malformed state header\n", line_num);
                fclose(fp);
                return -1;
            }

            if (scn->state_count >= MAX_STATES) {
                printf("Line %d: too many states (max %d)\n",
                       line_num, MAX_STATES);
                fclose(fp);
                return -1;
            }

            name = trim_left(colon + 1);
            *close = '\0';
            trim_right(name);

            cur_state = scn->state_count;
            strncpy(g_states[cur_state].name, name, MAX_NAME - 1);
            g_states[cur_state].cmd_count = 0;
            scn->state_count++;

            if (strcmp(name, "init") == 0)
                init_found = 1;
            continue;
        }

        if (cur_state < 0) {
            printf("Line %d: command outside of any state\n", line_num);
            fclose(fp);
            return -1;
        }

        {
            state_t *st = &g_states[cur_state];
            if (st->cmd_count >= MAX_COMMANDS) {
                printf("Line %d: too many commands in state '%s' (max %d)\n",
                       line_num, st->name, MAX_COMMANDS);
                fclose(fp);
                return -1;
            }
            if (parse_command(p, &st->commands[st->cmd_count], line_num) != 0) {
                fclose(fp);
                return -1;
            }
            st->cmd_count++;
        }
    }

    fclose(fp);

    if (!init_found) {
        printf("Error: no [state: init] found in '%s'\n", filename);
        return -1;
    }

    /* Validate: all targets exist, every state has an exit */
    for (i = 0; i < scn->state_count; i++) {
        state_t *st = &g_states[i];
        int has_exit = 0;

        for (j = 0; j < st->cmd_count; j++) {
            cmd_t *c = &st->commands[j];
            const char *target = NULL;

            switch (c->type) {
            case CMD_GOTO:
                target = pool_str(c->str1);
                has_exit = 1;
                break;
            case CMD_CHOICE:
                target = pool_str(c->str2);
                has_exit = 1;
                break;
            case CMD_END:
                has_exit = 1;
                break;
            case CMD_IF_EQ: case CMD_IF_NEQ:
            case CMD_IF_GT: case CMD_IF_LT:
                target = pool_str(c->tag);
                break;
            }

            if (target && *target &&
                find_state(scn->state_count, target) < 0) {
                printf("Error: state '%s' references undefined state '%s'\n",
                       st->name, target);
                return -1;
            }
        }

        if (!has_exit) {
            printf("Error: state '%s' has no exit (goto, choice, or end)\n",
                   st->name);
            return -1;
        }
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/* Runtime helpers                                                     */
/* ------------------------------------------------------------------ */

static unsigned char get_attr(engine_scenario_t *scn)
{
    if (scn->current_attr >= 0 && scn->current_attr < ATTR_ID_COUNT)
        return attr_map[scn->current_attr];
    return attr_map[ATTR_ID_GREEN];
}

static void advance_row(engine_scenario_t *scn)
{
    scn->cursor_row++;
    if (scn->cursor_row > MAIN_BOTTOM) {
        int row, col;
        for (row = MAIN_TOP; row < MAIN_BOTTOM; row++) {
            for (col = MAIN_LEFT; col <= MAIN_RIGHT; col++) {
                unsigned short cell = scr_read_cell(col, row + 1);
                scr_putc(col, row, (char)(cell & 0xFF),
                         (unsigned char)(cell >> 8));
            }
        }
        scr_hline(MAIN_LEFT, MAIN_BOTTOM, MAIN_RIGHT - MAIN_LEFT + 1,
                  ' ', get_attr(scn));
        scn->cursor_row = MAIN_BOTTOM;
    }
}

static void write_text(engine_scenario_t *scn, const char *text,
                       int row, int col, unsigned char attr)
{
    if (scn->typing_active) {
        int cps = scn->typing_cps;
        int dly;
        if (cps <= 0) cps = 30;
        dly = 1000 / cps;
        if (dly < 55) dly = 55;

        while (*text && col <= MAIN_RIGHT) {
            scr_putc(col, row, *text, attr);
            col++;
            text++;
            engine_delay(dly);
        }
        scn->typing_active = 0;
    } else {
        while (*text && col <= MAIN_RIGHT) {
            scr_putc(col, row, *text, attr);
            col++;
            text++;
        }
    }
}

static void flush_keyboard(void)
{
    while (scr_kbhit())
        scr_getkey();
}

static void read_input(engine_scenario_t *scn, const char *prompt,
                       char *buf, int buf_max)
{
    int px, len = 0;
    int key;
    unsigned char pattr = get_attr(scn);

    write_text(scn, prompt, scn->cursor_row, MAIN_LEFT, pattr);
    px = MAIN_LEFT + (int)strlen(prompt);

    scr_cursor_show();
    scr_cursor_pos(px, scn->cursor_row);

    buf[0] = '\0';

    while (1) {
        key = scr_getkey();

        if ((key & 0xFF) == 0x0D)
            break;
        else if ((key & 0xFF) == 0x08) {
            if (len > 0) {
                len--;
                px--;
                buf[len] = '\0';
                scr_putc(px, scn->cursor_row, ' ', ATTR_INPUT);
                scr_cursor_pos(px, scn->cursor_row);
            }
        }
        else if ((key & 0xFF) >= 0x20 && (key & 0xFF) < 0x7F) {
            if (len < buf_max - 1 && px <= MAIN_RIGHT) {
                buf[len] = (char)(key & 0xFF);
                len++;
                buf[len] = '\0';
                scr_putc(px, scn->cursor_row, buf[len-1], ATTR_INPUT);
                px++;
                scr_cursor_pos(px, scn->cursor_row);
            }
        }
    }

    scr_cursor_hide();
    advance_row(scn);
}

static int show_choices(engine_scenario_t *scn, cmd_t *choices,
                        int count)
{
    int sel = 0;
    int i, key;
    int start_row = scn->cursor_row;
    unsigned char nattr = get_attr(scn);

    while (1) {
        for (i = 0; i < count; i++) {
            int row = start_row + i;
            if (row > MAIN_BOTTOM) break;

            if (i == sel) {
                scr_hline(MAIN_LEFT, row, MAIN_RIGHT - MAIN_LEFT + 1,
                          ' ', ATTR_SELECTED);
                scr_putc(MAIN_LEFT, row, (char)BOX_ARROW, ATTR_SELECTED);
                scr_puts(MAIN_LEFT + 2, row,
                         pool_str(choices[i].str1), ATTR_SELECTED);
            } else {
                scr_hline(MAIN_LEFT, row, MAIN_RIGHT - MAIN_LEFT + 1,
                          ' ', nattr);
                scr_puts(MAIN_LEFT + 2, row,
                         pool_str(choices[i].str1), nattr);
            }
        }

        key = scr_getkey();

        if (key == KEY_UP && sel > 0)
            sel--;
        else if (key == KEY_DOWN && sel < count - 1)
            sel++;
        else if ((key & 0xFF) == 0x0D)
            break;
    }

    for (i = 0; i < count; i++) {
        int row = start_row + i;
        if (row > MAIN_BOTTOM) break;
        scr_hline(MAIN_LEFT, row, MAIN_RIGHT - MAIN_LEFT + 1, ' ', nattr);
    }

    scn->cursor_row = start_row;
    return sel;
}

/* ------------------------------------------------------------------ */
/* Public: reset and run                                               */
/* ------------------------------------------------------------------ */

void engine_reset(engine_scenario_t *scn)
{
    int i;
    scn->current_state = find_state(scn->state_count, "init");
    scn->running = 1;
    scn->result = -1;
    scn->current_attr = ATTR_ID_GREEN;
    scn->cursor_row = MAIN_TOP;
    scn->cursor_col = MAIN_LEFT;
    scn->typing_active = 0;
    scn->typing_cps = 30;
    scn->news_fallback[0] = '\0';
    scn->news_headline[0] = '\0';

    for (i = 0; i < MAX_VARS; i++)
        g_vars[i].in_use = 0;
    for (i = 0; i < MAX_TAGS; i++)
        g_tags[i].in_use = 0;
}

int engine_run(engine_scenario_t *scn)
{
    engine_reset(scn);

    if (scn->current_state < 0) {
        printf("Error: init state not found\n");
        return -1;
    }

    while (scn->running) {
        state_t *st = &g_states[scn->current_state];
        int ci;

        for (ci = 0; ci < st->cmd_count && scn->running; ci++) {
            cmd_t *c = &st->commands[ci];

            switch (c->type) {

            case CMD_TEXT:
            case CMD_TEXT_TAGGED:
            {
                char expanded[MAX_STRING];
                unsigned char attr = get_attr(scn);
                const char *raw = pool_str(c->str1);

                substitute_vars(scn, raw, expanded, MAX_STRING);

                if (c->type == CMD_TEXT_TAGGED && c->tag != POOL_NONE) {
                    tag_t *t = alloc_tag(pool_str(c->tag));
                    if (t) {
                        t->row = scn->cursor_row;
                        t->col = MAIN_LEFT;
                        t->len = (int)strlen(expanded);
                    }
                }

                write_text(scn, expanded, scn->cursor_row, MAIN_LEFT, attr);
                advance_row(scn);
                break;
            }

            case CMD_ATTR:
                scn->current_attr = c->num1;
                break;

            case CMD_DELAY:
                engine_delay(c->num1);
                break;

            case CMD_CLEAR:
                switch (c->num1) {
                case CLEAR_SCREEN:
                    scr_clear(get_attr(scn));
                    scn->cursor_row = MAIN_TOP;
                    break;
                case CLEAR_MAIN:
                    scr_fill(MAIN_LEFT, MAIN_TOP,
                             MAIN_RIGHT - MAIN_LEFT + 1,
                             MAIN_BOTTOM - MAIN_TOP + 1,
                             ' ', get_attr(scn));
                    scn->cursor_row = MAIN_TOP;
                    break;
                case CLEAR_STATUS:
                    scr_hline(0, STATUS_ROW, SCR_WIDTH, ' ', ATTR_STATUS);
                    break;
                }
                break;

            case CMD_INPUT:
            {
                char buf[42];
                char expanded[MAX_STRING];
                substitute_vars(scn, pool_str(c->str1), expanded, MAX_STRING);
                read_input(scn, expanded, buf, 41);
                set_var(pool_str(c->str2), buf, 0, 0);
                break;
            }

            case CMD_CHOICE:
            {
                cmd_t choices[MAX_CHOICES];
                int count = 0;
                int sel, target_idx;
                int cj;

                for (cj = ci; cj < st->cmd_count && count < MAX_CHOICES; cj++) {
                    if (st->commands[cj].type != CMD_CHOICE) break;
                    memcpy(&choices[count], &st->commands[cj], sizeof(cmd_t));
                    count++;
                }

                sel = show_choices(scn, choices, count);
                target_idx = find_state(scn->state_count,
                                        pool_str(choices[sel].str2));
                if (target_idx >= 0) {
                    scn->current_state = target_idx;
                    ci = st->cmd_count;
                }
                break;
            }

            case CMD_GOTO:
            {
                int idx = find_state(scn->state_count, pool_str(c->str1));
                if (idx >= 0) {
                    scn->current_state = idx;
                    ci = st->cmd_count;
                }
                break;
            }

            case CMD_END:
                scn->result = c->num1;
                scn->running = 0;
                break;

            case CMD_SET_STR:
                set_var(pool_str(c->str2), pool_str(c->str1), 0, 0);
                break;

            case CMD_SET_NUM:
                set_var(pool_str(c->str2), NULL, c->num1, 1);
                break;

            case CMD_IF_EQ:
            {
                const char *val = engine_get_var_str(scn, pool_str(c->str2));
                if (strcmp(val, pool_str(c->str1)) == 0) {
                    int t = find_state(scn->state_count, pool_str(c->tag));
                    if (t >= 0) { scn->current_state = t; ci = st->cmd_count; }
                }
                break;
            }
            case CMD_IF_NEQ:
            {
                const char *val = engine_get_var_str(scn, pool_str(c->str2));
                if (strcmp(val, pool_str(c->str1)) != 0) {
                    int t = find_state(scn->state_count, pool_str(c->tag));
                    if (t >= 0) { scn->current_state = t; ci = st->cmd_count; }
                }
                break;
            }
            case CMD_IF_GT:
            {
                int val = engine_get_var_num(scn, pool_str(c->str2));
                if (val > c->num1) {
                    int t = find_state(scn->state_count, pool_str(c->tag));
                    if (t >= 0) { scn->current_state = t; ci = st->cmd_count; }
                }
                break;
            }
            case CMD_IF_LT:
            {
                int val = engine_get_var_num(scn, pool_str(c->str2));
                if (val < c->num1) {
                    int t = find_state(scn->state_count, pool_str(c->tag));
                    if (t >= 0) { scn->current_state = t; ci = st->cmd_count; }
                }
                break;
            }

            case CMD_INC:
            {
                var_t *v = alloc_var(pool_str(c->str1));
                if (v) { v->is_numeric = 1; v->num_val++; }
                break;
            }
            case CMD_DEC:
            {
                var_t *v = alloc_var(pool_str(c->str1));
                if (v) { v->is_numeric = 1; v->num_val--; }
                break;
            }

            case CMD_EFFECT:
            {
                const char *name = pool_str(c->str1);
                if (strcmp(name, "typing_effect") == 0) {
                    scn->typing_active = 1;
                    scn->typing_cps = c->num2;
                    if (scn->typing_cps <= 0) scn->typing_cps = 30;
                } else {
                    int ei;
                    for (ei = 0; effect_table[ei].name != NULL; ei++) {
                        if (strcmp(effect_table[ei].name, name) == 0) {
                            effect_table[ei].func(c->num1, c->num2);
                            break;
                        }
                    }
                }
                break;
            }

            case CMD_REWRITE:
            {
                tag_t *t = find_tag(pool_str(c->tag));
                if (t) {
                    char expanded[MAX_STRING];
                    unsigned char attr = get_attr(scn);
                    substitute_vars(scn, pool_str(c->str1),
                                    expanded, MAX_STRING);
                    scr_hline(t->col, t->row,
                              MAIN_RIGHT - t->col + 1, ' ', attr);
                    write_text(scn, expanded, t->row, t->col, attr);
                    t->len = (int)strlen(expanded);
                }
                break;
            }

            case CMD_LOCK_INPUT:
                engine_delay(c->num1);
                flush_keyboard();
                break;

            case CMD_FAKE_ERROR:
            {
                unsigned short save_buf[76 * 5];
                char expanded[MAX_STRING];
                int ew, eh = 5, ex, ey;

                substitute_vars(scn, pool_str(c->str1),
                                expanded, MAX_STRING);
                ew = (int)strlen(expanded) + 4;
                if (ew < 30) ew = 30;
                if (ew > 76) ew = 76;
                ex = (SCR_WIDTH - ew) / 2;
                ey = (SCR_HEIGHT - eh) / 2;

                scr_save_region(ex, ey, ew, eh, save_buf);
                scr_fill(ex, ey, ew, eh, ' ', ATTR_ERROR);
                scr_box_single(ex, ey, ew, eh, ATTR_ERROR);
                scr_puts(ex + 2, ey + 1, expanded, ATTR_ERROR);
                scr_puts(ex + 2, ey + 3, "Press any key...",
                         SCR_ATTR(SCR_LIGHTGRAY, SCR_BLACK));

                flush_keyboard();
                scr_getkey();
                scr_restore_region(ex, ey, ew, eh, save_buf);
                break;
            }

            case CMD_HIJACK_PROMPT:
            {
                char expanded[MAX_STRING];
                int col;
                unsigned char attr = SCR_ATTR(SCR_LIGHTGRAY, SCR_BLACK);
                const char *p;

                substitute_vars(scn, pool_str(c->str1),
                                expanded, MAX_STRING);
                scr_hline(0, STATUS_ROW - 1, SCR_WIDTH, ' ', attr);
                scr_puts(0, STATUS_ROW - 1, "C:\\>", attr);
                col = 4;

                for (p = expanded; *p && col < SCR_WIDTH; p++) {
                    scr_putc(col, STATUS_ROW - 1, *p, attr);
                    col++;
                    engine_delay(80);
                }
                break;
            }

            case CMD_TONE:
                audio_tone(c->num1, c->num2);
                break;

            case CMD_SILENCE:
                audio_silence(c->num1);
                break;

            case CMD_NEWS_FALLBACK:
                strncpy(scn->news_fallback, pool_str(c->str1),
                        sizeof(scn->news_fallback) - 1);
                break;

            case CMD_NEWS_INJECT:
                strncpy(scn->news_headline, scn->news_fallback,
                        sizeof(scn->news_headline) - 1);
                set_var("news_headline", scn->news_headline, 0, 0);
                break;

            case CMD_MUSIC:
                if (g_hw.adlib)
                    adlib_play_track(pool_str(c->str1));
                break;

            case CMD_MUSIC_STOP:
                if (g_hw.adlib)
                    adlib_stop_track();
                break;

            case CMD_GFX_FADE:
            {
                /* Text-mode VGA palette fade to black and back.
                 * Works via DAC palette (ports 3C7/3C8/3C9) which
                 * works identically in text mode. VGA only. */
                if (g_hw.display == HW_DISP_VGA) {
                    unsigned char pal[48]; /* 16 text colors * 3 RGB */
                    int i, s, dur, steps;
                    dur = c->num1;
                    if (dur <= 0) dur = 1000;
                    steps = 16;

                    /* Read the 16 text-mode palette entries */
                    outp(0x3C7, 0);
                    for (i = 0; i < 48; i++)
                        pal[i] = (unsigned char)inp(0x3C9);

                    /* Fade out */
                    for (s = steps - 1; s >= 0; s--) {
                        outp(0x3C8, 0);
                        for (i = 0; i < 48; i++)
                            outp(0x3C9, (unsigned char)
                                 ((unsigned int)pal[i] * (unsigned int)s
                                  / (unsigned int)steps));
                        engine_delay(dur / (steps * 2));
                    }

                    /* Hold black */
                    engine_delay(dur / 4);

                    /* Fade in */
                    for (s = 1; s <= steps; s++) {
                        outp(0x3C8, 0);
                        for (i = 0; i < 48; i++)
                            outp(0x3C9, (unsigned char)
                                 ((unsigned int)pal[i] * (unsigned int)s
                                  / (unsigned int)steps));
                        engine_delay(dur / (steps * 2));
                    }
                }
                break;
            }

            } /* switch */
        } /* for commands */
    } /* while running */

    return scn->result;
}
