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

/* Mute/unmute music. Playback state advances silently when muted. */
void adlib_set_mute(int mute);
int  adlib_get_mute(void);

/* ------------------------------------------------------------------ */
/* Beat sync: poll-based audio-visual synchronization                  */
/* ------------------------------------------------------------------ */

/* Beat counter: increments on every note-on event.
 * Effects poll this to sync visuals to music. */
extern unsigned char adlib_beat;
extern unsigned char adlib_last_ch;   /* channel of last note-on (0-8) */

/* ------------------------------------------------------------------ */
/* Stingers: fire-and-forget sound effects on channels 3-8             */
/* ------------------------------------------------------------------ */

/* Stinger IDs */
#define STINGER_ALARM       0   /* two-tone klaxon (shared/urgent) */
#define STINGER_STAMP       1   /* bureaucratic thud (Axiom) */
#define STINGER_CLICK       2   /* typewriter click (Hushline) */
#define STINGER_BUZZ        3   /* harsh error buzz (Kestrel-9) */
#define STINGER_OMINOUS     4   /* slow diminished chord (Orchard) */
#define STINGER_REGISTER    5   /* metallic ka-ching (Cinder) */
#define STINGER_COUNT       6

/* Fire a stinger by ID. Non-blocking; plays on channels 3-8. */
void adlib_play_stinger(int id);

/* Advance stinger playback (called from engine_delay spin loop). */
void adlib_stinger_tick(void);

#endif /* ADLIB_H */
