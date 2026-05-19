# Nintendo Switch Scaffolding

This branch contains early Nintendo Switch scaffolding only.

What exists today:

- a `DUSK_EXPERIMENTAL_SWITCH` CMake option
- a `TARGET_SWITCH=1` compile-time define when that option is enabled
- documentation for the expected bring-up path

What does not exist yet:

- a supported `libnx` toolchain file in this repo
- a working renderer/backend selection for Switch
- Switch-specific filesystem, save-path, and input integration
- audio, movie, and update-check validation on Switch
- CI builds for Switch

## Intended bring-up order

1. Add a dedicated `libnx` toolchain and preset.
2. Audit Aurora backend support for the platform.
3. Define filesystem locations for config, saves, logs, and mods.
4. Validate controller mappings and touch assumptions for handheld and docked modes.
5. Gate unsupported features behind `TARGET_SWITCH`.
6. Add a minimal boot-to-menu build before enabling gameplay support.

## Current recommendation

Do not treat the Switch target as buildable yet. The current scaffolding is only there to let platform-specific code paths start landing in controlled increments.
