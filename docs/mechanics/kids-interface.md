# Kids interface

The `microcosm_kids` executable provides a separate interface for ages 6–10.
It uses the same `World`, `Run`, organism content and `Renderer` as the original
laboratory. It does not approximate biology with decorative animations.

## Experiences

- Pocket Pond offers three curated freshwater habitat preparations. This is not
  a natural abundance survey. Organisms are deliberately placed where children
  can find them and food bacteria are supplied.
- Tiny Detectives compares Paramecium and Euglena. The question concerns cilia
  versus a flagellum, directly represented in their factory definitions and
  locomotion components. The lamp is uniform; directional light is not offered.
- Micro Safari prepares Paramecium, Amoeba, Euglena or Volvox, with explanations
  drawn from their existing body plans and biology. Volvox is a colony, not one
  giant cell. Labels do not imply that cells have intentions or human organs.
- Wonder Lab uses the existing living onion epidermis preparation and changes
  `environment.salinity` through the intervention log. The explanation of
  shrinking contents appears only after mean simulated tissue-cell volume drops
  by at least 5% from baseline. The left comparison is a captured baseline, not a separate simulation. Both
  comparison panes show the same central field at the same scale. The camera and optical mode stay fixed during the comparison.
  A 512-micrometre field retains the factory spatial resolution while reducing
  the number of onion cells drawn and simulated. Returning to fresh water changes the medium; it does not rewind
  the experiment. Start over prepares the original sample again.
- A Drop of Adventure provides three short chapters about Paramecium, follows
  the actual selected cell, and leads into free exploration.

## Interpretation

Microscope view is a simulated bright-field image. See the parts is a model view,
not a claim that every depicted structure can be resolved in a light microscope.
The original engine's time convention is retained: swimming and water physics
run in real time; growth and digestion use a biological scale of 60. The footer
states this. Osmotic changes retain the original preparation's timescale.

The scientific foundations and limits are inherited from the existing
[osmosis](../content/plates.md) content and the engine's
[mechanics index](README.md). See the factory `beings` TOML definitions for
Paramecium, Amoeba, Euglena and Volvox and their cited physiology. This interface
adds no new biological mechanism or species.

## Interaction and persistence

`src/kids/session.*` is the engine controller and is tested without a display.
`src/kids/main.cpp` draws the native interface with raylib. All changes to a
running world go through `Run::interventions`. Camera movement, visual mode,
selection and narration do not change the biological model.

The app stores its photo notebook separately in
`~/.local/share/open-microcosm-kids`. Set `MICROCOSM_KIDS_DATA` for isolated tests.
Photos use the engine's `savePhoto`, including scale/provenance sidecars.
The notebook is local and has no accounts or network services.

Linux read-aloud connects to the installed Speech Dispatcher service over its
local SSIP socket. It keeps one connection and uses `CANCEL SELF` to stop only
this application’s speech. It is optional and requires a working speech service. Text remains on screen when speech is unavailable. No external speech
API is used. Other platforms currently retain on-screen text only.

Tab and Enter navigate buttons; Space pauses a live sample; +/- zoom. Esc stops
narration. Mouse wheel zooms, right/middle drag pans, and a click inspects a cell.
The interface uses a 1440x900 design canvas scaled to fit the window, preserving
layout and touch target proportions. This is not a phone interface.

## Verification

`microcosm_kids_tests` checks factory species, movement types, habitat scenes,
recorded interventions, pause, reset and measured salt-induced plasmolysis.
`--experience 0..4 --screenshot /absolute/path.png` opens a repeatable live scene
and saves a screenshot after 90 frames. Omit `--experience` for the home screen.
Use actual input to exercise actions beyond those initial scenes. F10 captures
the whole interface and state JSON in the profile’s `screenshots/` folder
(or `MICROCOSM_KIDS_QA_DIR`). The native-input drivers are
`tools/qa_kids_ui.py` and `tools/qa_kids_finish.py`; they require `xdotool`
for development reviews only.

No independent child usability study or external biologist review has been
performed. Visual inspection and engine tests do not substitute for those.

## Review refinements

The coach's read-aloud controls keep a fixed position when explanations change.
Amoeba explanations switch only when the engine reports an active phagocytic
cup or completed ingestion; offering food does not fabricate a successful meal.
Diagram leader lines anchor to the actual nucleus, chloroplast or boundary
particles, with a separate colony label for Volvox. They do not turn the colony
into one giant nucleated cell. Saved photos record the view mode and build ID;
interface labels are not burned into microscope photos.

The notebook loads only the visible page's four textures, rather than every
saved image at startup. F10 filenames include a timestamp to preserve captures
across restarts. The native-input driver closes through the window manager;
it must not destroy a GLFW window while leaving its process running.

Saved photos open in a full-size reading view with their caption and local
read-aloud. Escape or Back to notebook returns to the collection. Pagination
controls are disabled at the ends. Captions describe the recorded sample or
observation, rather than storing a prompt to click something. Linux storage
honours `XDG_DATA_HOME`; the default remains `~/.local/share/open-microcosm-kids`.

Kids photos use the app font for a clear scale bar. Run time and seed remain in
its JSON sidecar rather than being stamped across the child's picture. The
original laboratory retains its existing stamped-photo defaults.

Home, habitat selection and organism selection provide a local “Hear choices”
guide before a child enters an activity. Starting an activity or changing pages
stops the previous guide. The guide controls sit below the transient status
banner so both remain readable while speech is playing.

Picker previews preserve the renderer's proportions: they are cropped to fill
their thumbnail, never stretched. Their camera zoom fits the chosen organism,
and a visible note explains that preview sizes differ. Live Safari also frames
the selected organism initially; the live scale bar remains authoritative.
The Wonder Lab introduction is rendered from its actual onion preparation,
separately from the four organism previews. Notebook photos preserve the entire
image with proportional fitting rather than cropping away their scale bar.
