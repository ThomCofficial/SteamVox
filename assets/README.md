# SteamVox — corrected UI artwork (mandatory)

**The previous upload contains the OLD SteamVox UI assets.**
They cause empty black control wells, baked-in static pointers, and two VU needles.

Please replace all four assets using the files inside the new
`SteamVox_CORRECT_UI_ASSETS.zip` shared in ChatGPT.
Upload on the **`dsp/engineering-preview-v02` branch**.

| File | Required dimensions |
| --- | --- |
| `steamvox_faceplate.png` | 1254 x 1135 |
| `steamvox_large_knob.png` | 512 x 512 |
| `steamvox_small_knob.png` | 512 x 512 |
| `steamvox_vu_needle.png` | 56 x 600 |

Rename the existing `steamvox_vu_needle.png.png` to
`steamvox_vu_needle.png` if needed, or delete the doubled-extension version
before uploading its replacement.

The CMake build now **fails explicitly** if old, wrong-size UI images are
present, rather than silently producing another broken VST3.
The code uses real independent APVTS sliders with sprites painted **over**
the transparent controls, exactly one needle, and the original proportional
layout. Do not replace the source DSP or draw knob indicators onto the faceplate.
