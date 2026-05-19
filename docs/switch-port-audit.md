# Nintendo Switch Port Audit

This document records the current technical state of a future Switch port.

It is intentionally concrete. The goal is to reduce guesswork before platform
code starts landing.

## External stack

The current research baseline is:

- `devkitA64` as the compiler toolchain
- `switch-dev` as the devkitPro package group
- `libnx` for runtime, filesystem, HID, services, and app lifecycle
- `deko3d` as the native low-level 3D homebrew API when a Switch-native GPU path is needed

## What already exists in this repo

- `CMakeLists.txt` contains `DUSK_EXPERIMENTAL_SWITCH`
- enabling that option adds `TARGET_SWITCH=1`
- `docs/switch-homebrew.md` declares the target as scaffolding-only

This is useful, but it is not enough to produce a build.

## Platform audit

### 1. Build system

Current state:

- no Switch toolchain file
- no Switch configure preset
- no platform packaging target
- no Switch CI workflow

Recent reduction:

- there is now an experimental toolchain scaffold
- there is now a hidden configure preset
- obviously unsupported subsystems are now forced off for the experimental Switch target:
  - movie playback
  - update checker
  - Sentry crash reporting
  - Discord Rich Presence
- desktop-window behaviors are now being fenced off for the experimental Switch target:
  - window-state persistence
  - focus-loss pause
  - background-input option
  - external controller DB loading
  - SDL window safe-area queries in UI / ImGui / presentation code
- the toolchain now validates the expected devkitPro/libnx directory layout before configure proceeds

Required next work:

- add `cmake/toolchains/switch-libnx.cmake`
- add a hidden `switch-libnx` preset to `CMakePresets.json`
- wire devkitPro environment checks early, with explicit failure messages

### 2. Windowing and app bootstrap

Current state:

- startup lives in `src/m_Do/m_Do_main.cpp`
- startup depends on SDL path helpers such as `SDL_GetPrefPath()` and `SDL_GetBasePath()`
- platform metadata and some initialization are currently SDL-centric
- startup still directly performs Aurora config population and backend bring-up

Recent reduction:

- path selection has been extracted to `src/dusk/platform_support.cpp`
- bundled asset lookup has been extracted to `src/dusk/platform_support.cpp`
- Aurora startup config assembly has been extracted to `src/dusk/startup_shell.cpp`
- frontend shutdown flow has been partially centralized in `src/dusk/startup_shell.cpp`
- restart and desktop data-folder capabilities are now explicitly disabled for `TARGET_SWITCH`

Required next work:

- define whether the Switch target boots through SDL on Switch or through a direct libnx path
- if SDL is kept, verify that the exact SDL feature set used here exists on the Switch target
- if SDL is not kept, split path / lifecycle / window bootstrap out of `m_Do_main.cpp`
- extract a Switch app-shell layer that owns:
  - startup policy
  - platform hints / metadata
  - backend creation
  - orderly shutdown

### 3. Rendering

Current state:

- the renderer is coupled to Aurora and GX translation paths
- the repo currently targets desktop, Android, and iOS through the existing backend stack
- no Switch-specific renderer selection exists

Risk:

- this is the biggest technical blocker in the port

Required next work:

- audit whether Aurora already has a usable Switch-capable backend
- if not, define whether Dusk should:
  - add one to Aurora, or
  - maintain a separate Switch renderer path

No serious Switch build should be promised before this is answered.

### 4. Filesystem and writable paths

Current state:

- config/save/log paths are derived from desktop/mobile helpers
- pipeline cache seeding assumes SDL base/pref path behavior
- bundled assets currently assume a future Switch packaging layout rooted at `romfs:/`

Required next work:

- define Switch locations for:
  - config
  - save data
  - logs
  - mods
  - screenshots
  - pipeline cache
- confirm the final bundled asset layout, especially:
  - `romfs:/res/...`
  - `romfs:/initial_pipeline_cache.db`
- isolate these decisions behind `TARGET_SWITCH` instead of scattering ad hoc conditionals

### 5. Input, controllers, touch, and gyro

Current state:

- controller mappings are SDL gamepad based
- touch controls are mobile-oriented
- gyro support is tied into current input assumptions

Required next work:

- map Joy-Con / Pro Controller inputs into the existing pad abstraction
- decide whether handheld touch should expose the existing mobile touch UI or a Switch-specific variant
- validate gyro read paths against Switch controller sensor capabilities

### 6. Networking and online-dependent systems

Current state:

- update checking is platform-backend specific
- Discord integration exists
- Sentry / crash diagnostics have desktop/mobile assumptions

Required next work:

- disable or replace unsupported services under `TARGET_SWITCH`
- add explicit compile-time gating for:
  - update checker
  - Discord presence
  - Sentry / external crash reporting

### 7. Audio and media

Current state:

- audio and movie support are already conditional on some platforms
- no Switch validation exists

Required next work:

- validate runtime audio backend compatibility
- decide whether movie playback support is enabled or disabled on first bring-up

### 8. UI and overlays

Current state:

- RmlUi is already in use
- some UI assumes desktop/mobile layout and pointer behavior

Required next work:

- validate menu focus and controller navigation without mouse-first assumptions
- review overlay interactions that currently expect Android touch regions

## Recommended implementation order

1. Add toolchain file and hidden preset.
2. Extract platform path logic from `src/m_Do/m_Do_main.cpp`.
3. Add `TARGET_SWITCH` feature gates for unsupported services.
4. Prove a booting app shell with logging, config path setup, and controller input.
5. Resolve renderer strategy.
6. Only then attempt a menu-capable build.

## Explicit non-goals for the next pass

The next pass should not attempt:

- a claimed runnable gameplay build
- CI integration
- release packaging
- feature parity with Android

Those are downstream of the renderer and platform bootstrap decisions.
