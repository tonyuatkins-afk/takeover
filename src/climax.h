/*
 * climax.h - Mode 13h climax sequences for each AI's ending
 *
 * Five unique procedural effects, one per AI character:
 *   axiom    - Grid Convergence (surveillance grid materializes)
 *   hushline - Redaction Sweep (document lines blacked out)
 *   kestrel  - Threat Radar (rotating sweep with hotspots)
 *   orchard  - Cozy Dissolution (warm plasma with closing iris)
 *   cinder   - Reality Glitch (warped self-referencing plasma)
 *
 * Requires VGA. Caller must check g_hw.display == HW_DISP_VGA.
 * Enters Mode 13h, runs effect for ~8 seconds, fades out,
 * returns to text mode.
 */

#ifndef CLIMAX_H
#define CLIMAX_H

/* Run a climax sequence by AI name. Blocks until complete.
 * Names: "axiom", "hushline", "kestrel", "orchard", "cinder" */
void climax_run(const char *ai_name);

#endif /* CLIMAX_H */
