/*
 * cracktro.h - Demoscene cracktro easter egg
 *
 * Unlocked after completing all 5 scenarios. Plays a brief
 * demoscene-style intro with raster bars, DYCP sine scroller,
 * starfield, and 9-channel OPL2 chiptune.
 *
 * Requires VGA + AdLib for full experience. Degrades gracefully.
 */

#ifndef CRACKTRO_H
#define CRACKTRO_H

/* Run the cracktro sequence. Blocks until Esc pressed. */
void cracktro_run(void);

#endif /* CRACKTRO_H */
