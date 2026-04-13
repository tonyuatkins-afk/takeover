/*
 * effects.c - Visual effects implementations
 *
 * Text buffer manipulation effects for the takeover experience.
 * Each effect operates directly on the VGA text buffer via the
 * screen library.
 *
 * typing_effect is implemented here as a flag setter; the actual
 * char-by-char rendering happens in engine.c's CMD_TEXT handler
 * when the typing_active flag is set.
 *
 * All other effects are stubbed pending implementation.
 */

#include "effects.h"

/* typing_effect: handled by engine via typing_active flag.
 * This function is a no-op because the engine reads the flag directly.
 * Duration and intensity are captured by the engine before calling. */
void fx_typing_effect(int duration, int intensity)
{
    (void)duration;
    (void)intensity;
    /* Engine sets typing_active=1 and typing_cps from intensity
     * before this is called. Nothing to do here. */
}

void fx_stub(int duration, int intensity)
{
    (void)duration;
    (void)intensity;
    /* Not yet implemented */
}
