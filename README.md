# OBS CRT Filter

A native OBS Studio effect filter that adds a tunable CRT display look to any
video source. It is designed to stay simple in OBS while still giving direct
control over the parts that matter: scanlines, screen curve, glow, color bleed,
vignette, and flicker.

The filter appears in OBS as **CRT TV** under a source's **Effect Filters**.

## Features

- Three look modes: **Consumer TV**, **Arcade Monitor**, and **Clean PVM**.
- Overall **Intensity** control.
- On/off toggles plus amount sliders for each CRT component.
- GPU shader implementation using OBS/libobs effect rendering.
- Windows 64-bit OBS plugin package layout.

## Controls

- **Look**: Changes the balance of the same user-facing effects.
  - **Consumer TV**: more curve, glow, bleed, vignette, and flicker.
  - **Arcade Monitor**: stronger scanlines and RGB mask.
  - **Clean PVM**: cleaner, reduced distortion.
- **Intensity**: Overall strength from 0 to 100.
- **Scanlines** / **Scanline Amount**: Horizontal CRT line darkening and RGB mask.
- **Screen Curve** / **Curve Amount**: Curved glass distortion.
- **Glow** / **Glow Amount**: Soft bloom around bright pixels.
- **Color Bleed** / **Bleed Amount**: Red/blue channel separation.
- **Vignette** / **Vignette Amount**: Darkened screen edges.
- **Flicker** / **Flicker Amount**: Optional brightness instability.

For every amount slider, `0` is no effect and `100` is a large visible effect.

## Compatibility

- Target platform: Windows 64-bit OBS Studio.
- Tested locally with OBS Studio 32.1.2.
- Build dependencies are fetched by the OBS plugin template CMake flow.

## Build

Prerequisites:

- Visual Studio 2022 or Visual Studio 2026 with C++ desktop tools.
- Windows 10 SDK 10.0.20348.0 or newer.
- CMake 3.28 or newer.
- Internet access for the first configure, because OBS dependencies are downloaded.

Visual Studio 2026:

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64
```

Visual Studio 2022:

```powershell
cmake --preset windows-vs2022-x64
cmake --build --preset windows-vs2022-x64
```

The first configure can take several minutes because it downloads OBS
dependencies and builds the development copy of libobs needed for native
plugins.

## Install

Install the Release build to OBS's structured Windows plugin directory:

Visual Studio 2026 build:

```powershell
cmake --install build_x64 --config Release
```

Visual Studio 2022 build:

```powershell
cmake --install build_vs2022_x64 --config Release
```

Installed layout:

```text
C:\ProgramData\obs-studio\plugins\obs-crt-filter\bin\64bit\obs-crt-filter.dll
C:\ProgramData\obs-studio\plugins\obs-crt-filter\data\effects\crt.effect
C:\ProgramData\obs-studio\plugins\obs-crt-filter\data\locale\en-US.ini
```

Restart OBS after installing, then add **CRT TV** from a source's filter list.

## Repository Notes

- `build_x64/`, `build_vs2022_x64/`, and `.deps/` are local build outputs and
  should not be committed.
- The project uses GPL-2.0-or-later licensing, matching the OBS plugin template.
- This repository intentionally ships source files only; release DLLs can be
  attached to GitHub Releases later.
