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

## Approved UI artwork: integration instructions

The default build includes a code-drawn resizable steampunk UI, six functional macro/gain knobs, and a live seagull VU meter. **It is a functional development scaffold**, not the finished approved art.

The **approved version 3** faceplate and independently cropped knob sprites are supplied in the separately downloadable \`SteamVox_UI_Assets.zip\` pack in the ChatGPT conversation. Place the three PNG files from the ZIP's \`assets/\` directory in the repository's \`assets/\` directory:

- \`assets/steamvox_faceplate.png\`
- \`assets/steamvox_large_knob.png\`
- \`assets/steamvox_small_knob.png\`

CMake automatically detects these files and embeds them with JUCE BinaryData. The editor then paints the exact approved image at the original proportions while rendering dynamically rotating knob sprites and meter movement on top. **Artwork baking/retouching is not final:** the source image already depicts static controls, and the current procedural cleanup may leave visible remnants. Inspect the reconstructed artwork before any release. Do not stretch UI images independently or replace the approved design without approval.

## Known limitations / remaining engineering work

- No verified hardware-accurate compressor/saturator algorithms; resonance control, de-essing and analog-like gain compensation need detailed listening validation.
- Test coverage is presently basic numeric safety and parameter travel; more audio regression, aliasing, automation edge, latency/null, mono/stereo, and quality/performance benchmarks are still required.
- Switching 2x oversampling during playback changes internal processing engines; a state-matched, clickless changeover requires additional work.
- Oversampling includes filter delay, and transient character must be auditioned in Cubase.
- JUCE licensing obligations must be checked for any commercial distribution.
- The VST3 must be host-scanned, listened to and stress-tested in Cubase before it can be released.

The working DSP and approved art are preserved independently, so graphics integration cannot silently change the audio algorithms.
