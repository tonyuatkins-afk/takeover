/*
 * effects.h - Visual effects declarations
 *
 * Effect functions take duration (ms) and intensity (1-10).
 * Called from the engine's effect dispatch table.
 */

#ifndef EFFECTS_H
#define EFFECTS_H

/* Effect function signature */
typedef void (*effect_fn)(int duration, int intensity);

/* Implemented effects */
void fx_typing_effect(int duration, int intensity);

/* Stub for unimplemented effects */
void fx_stub(int duration, int intensity);

#endif /* EFFECTS_H */
