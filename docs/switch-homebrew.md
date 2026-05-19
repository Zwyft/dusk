# Nintendo Switch Scaffolding

This branch contains early Nintendo Switch scaffolding only.

## Current state

What exists today:

- a `DUSK_EXPERIMENTAL_SWITCH` CMake option
- a `TARGET_SWITCH=1` compile-time define when that option is enabled
- an experimental `cmake/toolchains/switch-libnx.cmake` scaffold
- a hidden `switch-libnx-relwithdebinfo` configure preset
- the Switch scaffold forces unsupported subsystems off at configure time:
  - movie playback
  - update checker
  - Sentry crash reporting
  - Discord Rich Presence
- the toolchain now validates the expected devkitPro/libnx layout before configure proceeds
- documentation for the expected bring-up path

What does not exist yet:

- a supported `libnx` toolchain file in this repo
- a `CMakePresets.json` entry for a Switch target
- a verified Aurora windowing / rendering backend for Switch
- Switch-specific filesystem, save-path, logging, and mod-path integration
- validated input, touch, gyro, and handheld / docked behavior
- audio, movie, and update-check validation on Switch
- CI builds for Switch

## Research baseline

The official homebrew stack for Switch is based on:

- `devkitA64`
- the `switch-dev` package group
- `libnx` for userland runtime APIs

The graphics path used by native Switch homebrew is not interchangeable with the
desktop / mobile backends used in this repository. The official low-level 3D
API exposed in the homebrew ecosystem is `deko3d`, which targets the Tegra X1
GPU directly. That means a future Switch build must either:

1. gain a real Aurora backend that works on top of the Switch graphics stack, or
2. introduce a separate rendering path for the platform

This is the main reason Switch support is still scaffolding-only.

## Repo-specific blockers

The largest blockers identified in this codebase are:

1. `TARGET_PC` is used very broadly across core engine code.
2. Startup and asset bootstrapping depend heavily on SDL path helpers.
3. Input assumes SDL gamepad events and desktop / Android style window events.
4. Touch controls are written around mobile semantics, not Switch handheld semantics.
5. The update checker is platform-specific and has no Switch backend.
6. Discord, Sentry, and some diagnostics paths need explicit platform gating.
7. Rendering currently assumes the existing Aurora backend matrix, not a Switch-native one.

See `docs/switch-port-audit.md` for the concrete audit list.

## Intended bring-up order

1. Add a dedicated `libnx` toolchain file and a hidden preset.
2. Audit Aurora backend support for the platform before attempting a real build.
3. Define filesystem locations for config, saves, logs, screenshots, and mods.
4. Add explicit `TARGET_SWITCH` feature gates for unsupported systems.
5. Validate controller mappings, touch assumptions, and gyro behavior for handheld and docked modes.
6. Produce a minimal boot-to-menu target before enabling gameplay support.
7. Add CI only after a local toolchain build is stable.

## Current recommendation

Do not treat the Switch target as buildable yet. The current scaffolding is only there to let platform-specific code paths start landing in controlled increments.

## Current app-shell boundary

The current startup boundary is still centered in `src/m_Do/m_Do_main.cpp`.

That function currently owns:

- config path resolution
- file logging startup
- pipeline cache seeding
- bundled controller DB loading
- SDL metadata initialization
- Aurora config population
- mobile-only SDL hint setup
- save-backup startup behavior
- Aurora backend initialization

For a real Switch app shell, those responsibilities should keep moving into
platform-aware helpers until `game_main()` mostly orchestrates startup instead
of embedding platform policy directly.

Recent progress:

- config/data path policy moved into `src/dusk/platform_support.cpp`
- bundled asset and URL policy moved into `src/dusk/platform_support.cpp`
- Aurora startup policy moved into `src/dusk/startup_shell.cpp`
- desktop-only restart and data-folder capabilities are now disabled for `TARGET_SWITCH`
- repeated frontend shutdown paths are now centralized in `src/dusk/startup_shell.cpp`
