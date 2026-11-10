# SteamVox — DSP Engineering Preview v0.2

SteamVox is the first module in the SteamTrain fast-mixing collection. The approved steampunk visual design with a seagull VU meter is **locked** and will be incorporated after DSP verification.

## Signal routing
Input → **BLASTPIPE** (tube-like saturation, transformer-like density, bite, parallel rich color) → **BOILER** (FET before optical compressor, parallel heavy compression, de-essing, corrective low-mid resonance reduction) → **STEAM** (HPF, 12 kHz shelf, broad 220 Hz body EQ, 1.5 kHz presence and excitation) → final sibilance protection → soft peak control → wet/dry MIX → Output.

Each main knob travels through four 25% macro sections. STEAM's HPF and 12 kHz shelf reset at each section and crossfade through a reset transition while subsequent tonal processing increases progressively. Compressor and nonlinear processing progress continuously.

## Build DSP test only
```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Engineering status
This repository currently contains **a C++17 DSP preview**, *not* a Cubase-ready VST3 and *not* a faithful reproduction of trademarked analog hardware. The Windows and Linux workflow checks compilation and finite output under parameter changes, multiple sample rates and a basic transition walk.

**Known unfinished items**: oversampling is represented by a parameter but not implemented; current EQ coefficients are recalculated at sample rate and need stable production-grade coefficient interpolation / boundary analysis; no true-peak limiting, robust level matching, advanced de-essing, stereo-linked gain reduction, VST3 wrapper, latency reporting, or real-time-safe host automation architecture. Compressor and tube/transformer behaviors are experimental approximations. Audio listening tests and Cubase stress testing have not been performed. Do not ship this build.

The design goal is to integrate final DSP into native animated controls, not use one static PNG with invisible hotspots.
