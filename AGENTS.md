# Dusk AGENTS

- **Gradle wrapper**: use `./gradlew` for all commands; never call `gradle` directly.
- **Asset sync**: run `./gradlew syncDuskAssets` (copies `res/` into generated assets) before lint or merge tasks; lint and merge depend on this.
- **Android build variants**:
  - `./gradlew assembleDebug` → unsigned debug APK, `app/build/outputs/apk/debug/`.
  - `./gradlew assembleRelease` → signed release APK, `app/build/outputs/apk/release/`.
- **Check pipeline**: `./gradlew check` runs lint → unit tests → integration tests; failures block PRs.
- **Emulator launch**: after `./gradlew installDebug`, start with `adb shell am start -n com.twilitrealm.dusk/.MainActivity`.
- **Release artifacts**: CI produces `app/build/outputs/apk/release/app-*.apk` and `mapping.txt`; keep for Play upload.
- **Mod folder**: place each mod folder (containing `.dds` textures and `mod.json`) directly in `mods/`; enable/disable via in‑game Mods screen.
- **Game dump verification**: provide a dump whose SHA‑1 matches one of the supported hashes listed in README before selecting it.