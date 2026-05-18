# Dusk

<div align="center">
  <img src="res/logo.png" alt="Dusk logo" width="560">

  <p>
    A reverse-engineered reimplementation of <strong>The Legend of Zelda: Twilight Princess</strong>,
    with modern platform support, runtime customization, mobile controls, and speedrun-focused tools.
  </p>

  <p>
    <a href="https://twilitrealm.dev">Website</a>
    ·
    <a href="https://discord.gg/dusktp">Discord</a>
  </p>
</div>

---

## What Dusk is

Dusk is a native reimplementation of Twilight Princess that runs on modern desktop and mobile platforms while preserving original game behavior as closely as possible.

This fork adds and maintains:

- a compact `RmlUi` settings interface
- Android/mobile touch controls
- built-in item checklist / tracker UI
- save states and state sharing
- texture replacement support and mod management
- speedrun tools including LiveSplit integration and timer overlays

No copyrighted game data is included. You must use your own supported game dump.

---

## Highlights

### Gameplay and quality-of-life

- Quick transform
- Fast climbing / fast spinner / fast roll
- Instant text and instant saves
- Auto-save
- Mirror mode
- Configurable HUD visibility
- Optional cutscene pillarboxing controls

### Graphics and presentation

- Internal resolution scaling
- Shadow resolution scaling
- Bloom controls
- Anisotropic filtering
- Optional frame interpolation
- Texture replacement support
- Multiple graphics backends depending on platform

### Tools and speedrunning

- Speedrun mode with temporary override restrictions
- LiveSplit connection
- RTA timer overlay
- Input viewer and gyro viewer
- Save states and quick saves
- Discord Rich Presence

### Mobile support

- Fully movable on-screen controls
- Adjustable touch control size and opacity
- Controller auto-hide behavior
- Touch-oriented settings layout

### Tracker / checklist

- `RmlUi` checklist window integrated into the in-game menu bar
- Local bundled tracker assets
- Category tabs for essential items, adventure items, dungeons, scents, and speedrun views
- Live collected-state updates for mapped items

---

## Supported platforms

| Platform | Notes |
|---|---|
| Windows | Native desktop build |
| macOS | Native desktop build |
| Linux | Native desktop build |
| Android | APK build with touch controls |

Backend availability depends on platform and driver support.

> [!IMPORTANT]
> At minimum, Dusk expects a GPU/backend combination that supports one of the available modern rendering paths. Compatibility varies by device, driver, and operating system.

---

## Supported game dumps

Verify your dump before first launch.

| Version | SHA-1 |
|---|---|
| GameCube USA | `75edd3ddff41f125d1b4ce1a40378f1b565519e7` |
| GameCube EUR | `2601822a488eeb86fb89db16ca8f29c2c953e1ca` |

Supported input formats include standard GameCube/Wii disc image variants such as ISO/GCM and RVZ.

> [!IMPORTANT]
> Dusk does not ship game content. You must provide your own legally obtained dump that matches a supported hash.

---

## Getting started

### Desktop

1. Download a build or compile from source.
2. Launch Dusk.
3. Select your supported disc image.
4. Configure controls, graphics, and gameplay settings.
5. Start the game.

### Android

1. Install the APK.
2. Launch Dusk.
3. Select your supported disc image or dump location.
4. Adjust touch controls if needed.
5. Start the game.

---

## Settings UI

The main settings menu uses grouped `RmlUi` sections instead of a long flat list.

Current sections include:

- Video
- Input
- Audio
- Gameplay
- Backend / advanced options
- Mobile / touch controls
- Save state tools

This keeps the interface compact on desktop and practical on mobile screens.

---

## Mods and texture replacements

Dusk supports texture replacement mods and a simple mods folder workflow.

- place each mod in `mods/`
- each mod folder should contain a `mod.json`
- supported texture assets are typically `.dds`
- enable or disable mods from the in-game Mods UI

If portable mode is enabled, saves, settings, and mods live alongside the executable instead of the default user config directory.

---

## Save states

Dusk includes both quick saves and named save states.

- quick save slots for fast testing
- full quick save support
- named save states
- clipboard/state-pack import flows

These tools are intended for convenience, debugging, routing, and practice workflows.

---

## Touch controls

Mobile builds include a configurable virtual controller with:

- editable positions
- opacity and scale controls
- draggable sticks and buttons
- auto-hide when a physical controller is connected

Touch controls are configured in-game and persist between sessions.

---

## Building from source

Desktop build instructions live in:

- `docs/building.md`

Android-specific workflow in this repo uses Gradle:

```sh
./gradlew syncDuskAssets
./gradlew assembleDebug
```

Other useful tasks:

```sh
./gradlew assembleRelease
./gradlew check
```

> [!NOTE]
> `syncDuskAssets` should be run before Android lint/check flows so the generated asset tree is up to date.

---

## Repository layout

| Path | Purpose |
|---|---|
| `src/` | Game code, platform code, UI, runtime systems |
| `include/` | Public/project headers |
| `res/` | Runtime UI assets, fonts, tracker data, bundled images |
| `mods/` | User-installed mod folders |
| `docs/` | Build and project documentation |
| `.forgejo/workflows/` | Forgejo CI workflows |
| `.github/workflows/` | GitHub Actions workflows |

---

## Contributing

Pull requests are welcome.

Before submitting:

- keep changes focused
- preserve behavior outside the intended scope
- run the relevant checks for the platform you changed
- keep settings additions grouped in the existing compact UI structure

See:

- `docs/building.md`
- `docs/code-conventions.md`

---

## Credits

Dusk builds on work from:

- the [zeldaret/tp](https://github.com/zeldaret/tp) decompilation effort
- the GameCube/Wii reverse-engineering community
- the [Aurora](https://github.com/encounter/aurora) project
- the Twilight Princess speedrunning community
- Dusk contributors and downstream fork maintainers

<br>

<div align="center">
  <a href="https://github.com/encounter/aurora">
    <img src="assets/aurora-powered.png" alt="Powered by Aurora" width="760">
  </a>
</div>
