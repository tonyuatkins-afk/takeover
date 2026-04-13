/*
 * engine.h - Script engine public API
 *
 * The engine loads one .scn scenario at a time into module-internal
 * static storage. All heavy data (states, commands, string pool)
 * lives in engine.c. This header exposes only the control struct
 * and API functions.
 *
 * No dynamic allocation. Small memory model safe.
 */

#ifndef ENGINE_H
#define ENGINE_H

/* End result types (returned by engine_run) */
#define END_TAKEOVER    0
#define END_ESCAPE      1
#define END_REVELATION  2
#define END_STALEMATE   3

/*
 * Scenario control block. Small enough for the stack.
 * All heavy data (states, commands, vars) lives in engine.c statics.
 */
typedef struct {
    int state_count;
    int current_state;
    int running;
    int result;             /* END_* constant when finished */
    int current_attr;       /* attribute ID for current text */
    int cursor_row;         /* current output row */
    int cursor_col;         /* current output col */
    int typing_active;      /* 1 if next text uses typing effect */
    int typing_cps;         /* chars per second for typing effect */
    char news_fallback[80];
    char news_headline[80];
} engine_scenario_t;

/* Public API */
int  engine_load(engine_scenario_t *scn, const char *filename);
int  engine_run(engine_scenario_t *scn);
void engine_reset(engine_scenario_t *scn);

/* Variable access */
const char *engine_get_var_str(engine_scenario_t *scn, const char *name);
int         engine_get_var_num(engine_scenario_t *scn, const char *name);

/* Hardware-independent delay using BIOS tick counter */
void engine_delay(int ms);

/* AI control level (0-100, global across scenarios) */
int  engine_get_control(void);
void engine_add_control(int delta);

#endif /* ENGINE_H */
