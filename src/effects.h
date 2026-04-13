/*
 * effects.h - Visual effects declarations and RNG
 *
 * Effect functions take duration (ms) and intensity (1-10).
 * Called from the engine's effect dispatch table.
 */

#ifndef EFFECTS_H
#define EFFECTS_H

/* Effect function signature */
typedef void (*effect_fn)(int duration, int intensity);

/* RNG: seed once at startup, then use rng_next() */
void rng_seed(void);
unsigned int rng_next(void);

/* Implemented effects */
void fx_typing_effect(int duration, int intensity);
void fx_screen_flicker(int duration, int intensity);
void fx_static_burst(int duration, int intensity);
void fx_text_corrupt(int duration, int intensity);
void fx_text_redact(int duration, int intensity);
void fx_color_shift(int duration, int intensity);
void fx_blackout(int duration, int intensity);
void fx_red_alert(int duration, int intensity);
void fx_screen_scroll(int duration, int intensity);
void fx_cursor_possessed(int duration, int intensity);
void fx_screen_melt(int duration, int intensity);
void fx_falling_chars(int duration, int intensity);
void fx_progress_bar(int duration, int intensity);
void fx_fake_bsod(int duration, int intensity);

/* New enhanced effects */
void fx_pulse_border(int duration, int intensity);
void fx_interference(int duration, int intensity);

/* No-op stub for text_rewrite (handled by engine CMD_REWRITE) */
void fx_text_rewrite_noop(int duration, int intensity);

#endif /* EFFECTS_H */
