# 🚂 SteamVox — SteamTrain Fast-Mixing Collection

**Current status: engineering preview, not a finished production release.**

Steampunk vocal processor in C++17 and JUCE 8, designed around three macro controls and the locked, seagull-themed approved SteamVox artwork. No advanced panel.

## Main controls

- **BLASTPIPE**: Tube-like saturation → transformer push → bite → parallel saturation and light warm compression.
- **BOILER**: Opto leveling → 4:1 FET into opto → 8:1 with dynamic correction/peak protection → parallel aggressive compression.
- **STEAM**: Four successive 25% journeys. Every quarter sweeps the HPF 50–155 Hz and 12 kHz high shelf 0–10 dB. Broad body curve peaks at +5 dB at 50%, then falls to +3 dB at 75%; 1.5 kHz presence rises to +4 dB; final-quarter excitement.
- **IN / OUT**: ±24 dB each.
- **MIX**: 0–100% wet/dry.
- **2x OS**: JUCE 2x oversampling, internally preallocated; non-oversampled path delayed to align with host-reported latency.

**Order**: IN → BLASTPIPE → BOILER → STEAM → final de-essing → wet/dry → OUT.

The acoustic processing is **experimental**: the compressor models are inspired by design categories, not verified circuit emulations of LA-2A, 1176, Distressor, Aphex, etc. No external brand's code is included.

## Build

Requirements: CMake 3.22+, C++17 and compiler toolchain. A JUCE checkout is automatically fetched at configure time for plugin builds.

DSP tests (no dependency on JUCE):

\`\`\`sh
cmake -S . -B dsp-build -DBUILD_TESTING=ON
cmake --build dsp-build --config Release
ctest --test-dir dsp-build -C Release --output-on-failure
\`\`\`

Windows VST3 (Internet required on first CMake configuration):

\`\`\`sh
cmake -S . -B vst-build -DSTEAMVOX_BUILD_PLUGIN=ON -DBUILD_TESTING=OFF
cmake --build vst-build --config Release --target SteamVox_VST3
\`\`\`

The workflow builds/tests the DSP on Windows and Linux, builds a Windows VST3, and uploads the VST3 build as a downloadable Actions artifact on successful runs.

## Approved SteamVox UI: exact artwork integration

The original approved soft-contrast seagull UI has been split into a clean faceplate, separate large/small brass knob sprites and a bronze VU needle. The JUCE editor implements individual host-automatable rotary controls, a transparent OS hot spot with a live amber indicator, responsive scaling with a single aspect-preserving coordinate system, and a real meter with independently animated needle.

The four source PNGs are delivered in `SteamVox_Final_UI_Pack.zip` in the development conversation. To enable the final artwork in Windows VST3 builds, **upload all four files to this branch's `assets/` folder**, with these exact names:

- `assets/steamvox_faceplate.png`
- `assets/steamvox_large_knob.png`
- `assets/steamvox_small_knob.png`
- `assets/steamvox_vu_needle.png`

The CMake build automatically detects all four, embeds them into JUCE BinaryData, and enables `STEAMVOX_HAS_ARTWORK=1`. This never requires editing the DSP. If any PNG is missing, the fallback vector UI stays enabled intentionally. The code is already on GitHub; **binary PNG files are not yet committed** due to the current connector's text-only write limitation.

**Important:** The approved faceplate is 1254×1135 pixels; all control/needle layout uses that design-space. Final in-Cubase visual inspection remains necessary after uploading the assets. Do not resize/merge the original artwork using image generation.

## Known limitations / remaining engineering work

- No verified hardware-accurate compressor/saturator algorithms; resonance control, de-essing and analog-like gain compensation need detailed listening validation.
- Test coverage is presently basic numeric safety and parameter travel; more audio regression, aliasing, automation edge, latency/null, mono/stereo, and quality/performance benchmarks are still required.
- Switching 2x oversampling during playback changes internal processing engines; a state-matched, clickless changeover requires additional work.
- Oversampling includes filter delay, and transient character must be auditioned in Cubase.
- JUCE licensing obligations must be checked for any commercial distribution.
- The VST3 must be host-scanned, listened to and stress-tested in Cubase before it can be released.

The working DSP and approved art are preserved independently, so graphics integration cannot silently change the audio algorithms.
