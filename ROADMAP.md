# Roadmap

## Done
- Interactive design mockup (docs/mockups), version 10.

## Next
- Prototype the real-time fixer in C++ and listen for artefacts before anything else: the mockup uses ideal maths on a perfectly repeating loop, so its numbers are a best case.
- Decide the real-time method for phase rotation and per band phase (allpass, FIR or FFT based) and its latency; report the latency to the host.
- JUCE project, sidechain input, parameters, state.
- Port the mockup window natively (same layout, displays, tooltips, three colour looks).
- Tests: alignment maths, bypass is bit exact, reported latency matches the real one.
- Check sidechain routing in Bitwig, Ableton Live, Studio One, REAPER and Ardour.
- Installers (Linux Flatpak extension and plain VST3, Windows, macOS), PDF manual with a visual tour.

## Ideas
- Load your own kick and bass files in the mockup demo.
- Show how far each band can be turned.
- A switch for running the plugin on the kick track with the bass on the sidechain (the fixer then moves the kick).
