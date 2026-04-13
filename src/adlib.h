/*
 * adlib.h - AdLib/OPL2 FM synthesis audio driver
 *
 * Provides instrument loading, note playback, and background
 * ambient track support via the OPL2 chip (ports 388h/389h).
 * All patch/track data stored in far segments (zero DGROUP cost
 * for read-only data).
 *
 * Call adlib_tick() periodically (from engine_delay spin loop)
 * to advance background music. Safe to call at any frequency.
 */

#ifndef ADLIB_H
#define ADLIB_H

/* Initialize OPL2 chip. Call after hw_detect confirms AdLib. */
void adlib_init(void);

/* Shut down: silence all channels, reset chip */
void adlib_shutdown(void);

/* Start a background ambient track by name. Returns 0 on success. */
int adlib_play_track(const char *name);

/* Stop background track */
void adlib_stop_track(void);

/* Advance background music. Call from engine_delay spin loop.
 * Checks BIOS tick counter internally; negligible cost when idle. */
void adlib_tick(void);

#endif /* ADLIB_H */
