# Kids application implementation plan

Build a native `microcosm_kids` executable sharing `biosim` and `Renderer` with
upstream. Preserve the research application. No new runtime libraries.

1. Create a small, testable kids session controller. Curate real organisms and
   habitat scenes, use recorded interventions, and isolate saved data.
2. Create one coherent, responsive interface with five entrances: Pocket Pond,
   Tiny Detectives, Micro Safari, Wonder Lab, and A Drop of Adventure.
3. Connect observation, following, diagram view, local read-aloud, photo notebook,
   pause/reset, experiment prediction and measured before/after, story chapters.
4. Build, run real engine scenes, exercise journeys with native input, save
   screenshots at 1280x800 and 1600x900, inspect them, fix issues and repeat.
5. Run engine quick tests and focused kids behavior/science tests, documentation
   checks, record limitations and validation evidence, install a separate launcher,
   commit and push the fork.

Acceptance: every entrance reaches an actual simulation and returns home; no
placeholder controls; organism descriptions match data; no fabricated directional
light; biological time acceleration is disclosed; onion claims depend on measured
change; notebook survives restart; no changes to the original app's user data.
Images are references, not substitutes for screenshots. Independent biologist and
child usability reviews are future external validation, never claimed as done.

## Current evidence (2026-10-23)

Implementation and native-input journeys are complete. The 83 kids assertions,
notebook restart, local speech output, live algae ingestion, and two viewport
sizes have been checked. The shared inspector/export notch bounds errors were
reproduced and fixed; the inspector and kids controller pass sanitizer runs.
The complete upstream CTest run is still in progress. Do not mark verification
complete until its terminal result and the packaged-app smoke review are saved.

The separate user-local installation and smoke review have passed and were
published in commit `70dec1f`. Further native reviews added spoken choice guides,
corrected thumbnail proportions and the onion introduction, and improved initial
Safari framing. The full suite is still active; six candidate-division failures
are recorded in `docs/bugs/candidate-division-regressions.md`, with focused
geometry/sanitizer evidence. Do not replace this unresolved gate with the passing
focused kids tests.
