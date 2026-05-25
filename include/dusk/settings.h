#ifndef DUSK_CONFIG_H
#define DUSK_CONFIG_H

#include "dusk/config_var.hpp"

namespace dusk {

using namespace config;

enum class BloomMode : int {
    Off = 0,
    Classic = 1,
    Dusk = 2,
};

enum class GameLanguage : u8 {
    English = OS_LANGUAGE_ENGLISH,
    German = OS_LANGUAGE_GERMAN,
    French = OS_LANGUAGE_FRENCH,
    Spanish = OS_LANGUAGE_SPANISH,
    Italian = OS_LANGUAGE_ITALIAN,
};

enum class DiscVerificationState : u8 {
    Unknown = 0,
    Success,
    HashMismatch,
};

enum class GyroMode : u8 {
    Sensor = 0,
    Mouse = 1,
};

enum class BattleBGMMode : u8 {
    On = 0,
    Off = 1,
    Off_MidnaLament = 2,
};

enum class IngameHudMode : int {
    Off = 0,
    Health = 1,
    Rupees = 2,
    ActionButtons = 4,
    DPad = 8,
    LampMeter = 16,
    OxygenMeter = 32,
    Keys = 64,
    LightVessel = 128,
    On = 255,
};

namespace config {
template <>
struct ConfigEnumRange<BloomMode> {
    static constexpr auto min = BloomMode::Off;
    static constexpr auto max = BloomMode::Dusk;
};

template <>
struct ConfigEnumRange<GameLanguage> {
    static constexpr auto min = GameLanguage::English;
    static constexpr auto max = GameLanguage::Italian;
};

template <>
struct ConfigEnumRange<DiscVerificationState> {
    static constexpr auto min = DiscVerificationState::Unknown;
    static constexpr auto max = DiscVerificationState::HashMismatch;
};

template <>
struct ConfigEnumRange<GyroMode> {
    static constexpr auto min = GyroMode::Sensor;
    static constexpr auto max = GyroMode::Mouse;
};

template <>
struct ConfigEnumRange<BattleBGMMode> {
    static constexpr auto min = BattleBGMMode::On;
    static constexpr auto max = BattleBGMMode::Off_MidnaLament;
};

template <>
struct ConfigEnumRange<IngameHudMode> {
    static constexpr auto min = IngameHudMode::Off;
    static constexpr auto max = IngameHudMode::On;
};
}

// Persistent user settings

struct UserSettings {
    // Program settings

    struct {
        // Video
        ConfigVar<bool> enableFullscreen;
        ConfigVar<bool> enableVsync;
        ConfigVar<bool> lockAspectRatio;
        ConfigVar<bool> enableFpsOverlay;
        ConfigVar<int> fpsOverlayCorner;
        ConfigVar<int> windowPositionX;
        ConfigVar<int> windowPositionY;
    } video;

    struct {
        // Audio
        ConfigVar<int> masterVolume;
        ConfigVar<int> mainMusicVolume;
        ConfigVar<int> subMusicVolume;
        ConfigVar<int> soundEffectsVolume;
        ConfigVar<int> fanfareVolume;
        ConfigVar<bool> enableReverb;
        ConfigVar<bool> enableHrtf;
        ConfigVar<bool> menuSounds;
    } audio;

    // Game settings

    struct {
        ConfigVar<GameLanguage> language;

        // QoL
        ConfigVar<bool> enableQuickTransform;
        ConfigVar<bool> hideTvSettingsScreen;
        ConfigVar<bool> biggerWallets;
        ConfigVar<bool> noReturnRupees;
        ConfigVar<bool> disableRupeeCutscenes;
        ConfigVar<bool> noSwordRecoil;
        ConfigVar<int> damageMultiplier;
        ConfigVar<bool> noHeartDrops;
        ConfigVar<bool> instantDeath;
        ConfigVar<bool> fastClimbing;
        ConfigVar<bool> noMissClimbing;
        ConfigVar<bool> fastTears;
        ConfigVar<bool> no2ndFishForCat;
        ConfigVar<bool> instantSaves;
        ConfigVar<bool> instantText;
        ConfigVar<bool> sunsSong;
        ConfigVar<bool> autoSave;
        ConfigVar<bool> fastAreaTransitions;

        // Preferences
        ConfigVar<bool> enableMirrorMode;
        ConfigVar<IngameHudMode> ingameHudMode;
        ConfigVar<bool> pauseOnFocusLost;
        ConfigVar<bool> enableLinkDollRotation;
        ConfigVar<bool> enableAchievementToasts;
        ConfigVar<bool> enableControllerToasts;

        // Graphics
        ConfigVar<BloomMode> bloomMode;
        ConfigVar<float> bloomMultiplier;
        ConfigVar<bool> disableWaterRefraction;
        ConfigVar<bool> enableFrameInterpolation;
        ConfigVar<int> internalResolutionScale;
        ConfigVar<int> shadowResolutionMultiplier;
        ConfigVar<bool> enableDepthOfField;
        ConfigVar<bool> enableMapBackground;

        // Audio
        ConfigVar<bool> noLowHpSound;
        ConfigVar<BattleBGMMode> battleBGM;

        // Input
        ConfigVar<GyroMode> gyroMode;
        ConfigVar<bool> enableGyroAim;
        ConfigVar<bool> enableGyroRollgoal;
        ConfigVar<float> gyroSensitivityX;
        ConfigVar<float> gyroSensitivityY;
        ConfigVar<float> gyroSensitivityRollgoal;
        ConfigVar<float> gyroSmoothing;
        ConfigVar<float> gyroDeadband;
        ConfigVar<bool> gyroInvertPitch;
        ConfigVar<bool> gyroInvertYaw;
        ConfigVar<bool> freeCamera;
        ConfigVar<bool> invertCameraXAxis;
        ConfigVar<bool> invertCameraYAxis;
        ConfigVar<bool> invertFirstPersonXAxis;
        ConfigVar<bool> invertFirstPersonYAxis;
        ConfigVar<bool> invertAirSwimX;
        ConfigVar<bool> invertAirSwimY;
        ConfigVar<float> freeCameraSensitivity;
        ConfigVar<bool> debugFlyCam;
        ConfigVar<bool> debugFlyCamLockEvents;
        ConfigVar<bool> allowBackgroundInput;
        ConfigVar<bool> enableMouseFreeLook;
        ConfigVar<bool> firstPersonFreeCam;

        // Quality of Life
        ConfigVar<int> anisotropicFiltering;
        ConfigVar<int> fpsLimit;
        ConfigVar<bool> autoBackupSaves;

        // Cheats
        ConfigVar<bool> infiniteHearts;
        ConfigVar<bool> infiniteArrows;
        ConfigVar<bool> infiniteSeeds;
        ConfigVar<bool> infiniteBombs;
        ConfigVar<bool> infiniteOil;
        ConfigVar<bool> infiniteOxygen;
        ConfigVar<bool> infiniteRupees;
        ConfigVar<bool> enableIndefiniteItemDrops;
        ConfigVar<bool> moonJump;
        ConfigVar<bool> superClawshot;
        ConfigVar<bool> alwaysGreatspin;
        ConfigVar<bool> enableFastIronBoots;
        ConfigVar<bool> canTransformAnywhere;
        ConfigVar<bool> fastRoll;
        ConfigVar<bool> fastSpinner;
        ConfigVar<bool> freeMagicArmor;
        ConfigVar<bool> invincibleEnemies;
        ConfigVar<bool> infiniteChuJelly;
        ConfigVar<bool> transformWithoutShadowCrystal;

        // Technical
        ConfigVar<bool> restoreWiiGlitches;

        // Controls
        ConfigVar<bool> enableTurboKeybind;

        // Tools
        ConfigVar<bool> speedrunMode;
        ConfigVar<bool> liveSplitEnabled;
        ConfigVar<bool> recordingMode;
        ConfigVar<bool> enableSaveStates;
        ConfigVar<bool> removeQuestMapMarkers;
    } game;

    struct {
        // Touch controls
        ConfigVar<bool> enabled;
        ConfigVar<bool> menuTapNav;
        ConfigVar<float> opacity;
        ConfigVar<float> scale;
        ConfigVar<float> btnAX, btnAY;
        ConfigVar<float> btnBX, btnBY;
        ConfigVar<float> btnXX, btnXY;
        ConfigVar<float> btnYX, btnYY;
        ConfigVar<float> btnLX, btnLY;
        ConfigVar<float> btnRX, btnRY;
        ConfigVar<float> btnZX, btnZY;
        ConfigVar<float> btnStartX, btnStartY;
        ConfigVar<float> dpadX, dpadY;
        ConfigVar<float> stickMainX, stickMainY;
        ConfigVar<float> stickCX, stickCY;
        ConfigVar<int> stickMainDeadzone;
        ConfigVar<int> stickCDeadzone;
        ConfigVar<bool> floatingCamera;
        ConfigVar<float> floatingCameraX;
        ConfigVar<float> floatingCameraY;
        ConfigVar<float> floatingCameraSize;
    } touch;

    struct {
        ConfigVar<std::string> isoPath;
        ConfigVar<DiscVerificationState> isoVerification;
        ConfigVar<std::string> graphicsBackend;
        ConfigVar<bool> skipPreLaunchUI;
        ConfigVar<bool> showPipelineCompilation;
        ConfigVar<bool> wasPresetChosen;
        ConfigVar<bool> checkForUpdates;
        ConfigVar<int> cardFileType;
        ConfigVar<bool> enableAdvancedSettings;
        ConfigVar<bool> discordEnabled;
        ConfigVar<bool> portableMode;
        ConfigVar<std::string> customDataPath;
        ConfigVar<int> checklistDensityMode;
    } backend;
};

UserSettings& getSettings();

void registerSettings();

// Transient settings

struct CollisionViewSettings {
    bool enableTerrainView;
    bool enableWireframe;
    bool enableAtView;
    bool enableTgView;
    bool enableCoView;
    float terrainViewOpacity;
    float colliderViewOpacity;
    float drawRange;
};

struct TransientSettings {
    CollisionViewSettings collisionView;
    bool skipFrameRateLimit;
    bool moveLinkActive;
    bool stateShareLoadActive;
};

TransientSettings& getTransientSettings();

}

#endif // DUSK_CONFIG_H
