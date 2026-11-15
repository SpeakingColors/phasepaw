# Roadmap

## Done
- C++ core (analyzer and fixer) with an end to end test, see docs/prototype-results.md. Polarity and time shift act on the whole bass and are exact; the phase rotation works inside the band and changes phase only (within 0.25 dB with the standard filter). Latency 52.7 ms at 48 kHz.
- Interactive design mockup (docs/mockups), version 12.

## Next
- Plugin shell (JUCE, VST3): sidechain input, block based analyzer and fixer, latency reported to the host, builds for Linux, Windows and macOS.
- Test on real kick and bass recordings and listen for artefacts.
- Per note mode: follow each bass note with lookahead and give it its own phase turn (for basslines with several notes).
- Per band fix: in the tests it added no low end and cost 351 ms; keep as an experiment or drop, to decide after listening.
- A mode with almost no latency when only polarity and time shift are used.
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
