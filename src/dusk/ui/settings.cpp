#include "settings.hpp"

#include <aurora/aurora.h>
#include "aurora/gfx.h"
#include "bool_button.hpp"
#include "controller_config.hpp"
#include "d/d_com_inf_game.h"
#include "dusk/audio/DuskAudioSystem.h"
#include "dusk/audio/DuskDsp.hpp"
#include "dusk/config.hpp"
#include "dusk/file_select.hpp"
#include "dusk/hotkeys.h"
#include "dusk/imgui/ImGuiEngine.hpp"
#include "dusk/io.hpp"
#include "dusk/livesplit.h"
#include "dusk/main.h"
#include "dusk/platform_support.hpp"
#include "dusk/touch_controls.hpp"
#include "graphics_tuner.hpp"
#include "m_Do/m_Do_main.h"
#include "menu_bar.hpp"
#include "number_button.hpp"
#include "menu_bar.hpp"
#include "pane.hpp"
#include "prelaunch.hpp"
#include "save_states_window.hpp"
#include "ui.hpp"

#if DUSK_ENABLE_SENTRY_NATIVE
#include "dusk/crash_reporting.h"
#endif

#include <algorithm>
#include "fmt/format.h"

namespace dusk::ui {
namespace {

constexpr std::array kLanguageNames = {
    "English",
    "German",
    "French",
    "Spanish",
    "Italian",
};

constexpr std::array kCardFileTypes = {
    "Card Image",
    "GCI Folder",
};

constexpr std::array kFpsOverlayCornerNames = {
    "Top Left",
    "Top Right",
    "Bottom Left",
    "Bottom Right",
};

constexpr std::array kGyroInputModeLabels = {
    "Sensor",
    "Mouse",
};
constexpr std::array kBattleBGMModeLabels = {
    "On",
    "Off",
    "Off During Midna's Lament",
};

bool try_parse_backend(std::string_view backend, AuroraBackend& outBackend) {
    if (backend == "auto") {
        outBackend = BACKEND_AUTO;
        return true;
    }
    if (backend == "d3d11") {
        outBackend = BACKEND_D3D11;
        return true;
    }
    if (backend == "d3d12") {
        outBackend = BACKEND_D3D12;
        return true;
    }
    if (backend == "metal") {
        outBackend = BACKEND_METAL;
        return true;
    }
    if (backend == "vulkan") {
        outBackend = BACKEND_VULKAN;
        return true;
    }
    if (backend == "opengl") {
        outBackend = BACKEND_OPENGL;
        return true;
    }
    if (backend == "opengles") {
        outBackend = BACKEND_OPENGLES;
        return true;
    }
    if (backend == "webgpu") {
        outBackend = BACKEND_WEBGPU;
        return true;
    }
    if (backend == "null") {
        outBackend = BACKEND_NULL;
        return true;
    }

    return false;
}

std::string_view backend_name(AuroraBackend backend) {
    switch (backend) {
    default:
        return "Auto";
    case BACKEND_D3D12:
        return "D3D12";
    case BACKEND_D3D11:
        return "D3D11";
    case BACKEND_METAL:
        return "Metal";
    case BACKEND_VULKAN:
        return "Vulkan";
    case BACKEND_OPENGL:
        return "OpenGL";
    case BACKEND_OPENGLES:
        return "OpenGL ES";
    case BACKEND_WEBGPU:
        return "WebGPU";
    case BACKEND_NULL:
        return "Null";
    }
}

std::string_view backend_id(AuroraBackend backend) {
    switch (backend) {
    default:
        return "auto";
    case BACKEND_D3D12:
        return "d3d12";
    case BACKEND_D3D11:
        return "d3d11";
    case BACKEND_METAL:
        return "metal";
    case BACKEND_VULKAN:
        return "vulkan";
    case BACKEND_OPENGL:
        return "opengl";
    case BACKEND_OPENGLES:
        return "opengles";
    case BACKEND_WEBGPU:
        return "webgpu";
    case BACKEND_NULL:
        return "null";
    }
}

std::vector<AuroraBackend> available_backends() {
    std::vector<AuroraBackend> backends;
    backends.emplace_back(BACKEND_AUTO);
    size_t backendCount = 0;
    const AuroraBackend* raw = aurora_get_available_backends(&backendCount);
    for (size_t i = 0; i < backendCount; ++i) {
        // Do not expose NULL
        if (raw[i] != BACKEND_NULL) {
            backends.emplace_back(raw[i]);
        }
    }
    return backends;
}

AuroraBackend configured_backend() {
    AuroraBackend configuredBackend = BACKEND_AUTO;
    const auto configuredId = getSettings().backend.graphicsBackend.getValue();
    if (!try_parse_backend(configuredId, configuredBackend)) {
        configuredBackend = BACKEND_AUTO;
    }
    return configuredBackend;
}

void reset_for_speedrun_mode() {
    mDoMain::developmentMode = -1;

    getSettings().game.enableTurboKeybind.setSpeedrunValue(false);
    getSettings().game.enableResetKeybind.setSpeedrunValue(false);

    getSettings().game.damageMultiplier.setSpeedrunValue(1);
    getSettings().game.instantDeath.setSpeedrunValue(false);
    getSettings().game.noHeartDrops.setSpeedrunValue(false);
    getSettings().game.autoSave.setSpeedrunValue(false);
    getSettings().game.sunsSong.setSpeedrunValue(false);

    getSettings().game.infiniteHearts.setSpeedrunValue(false);
    getSettings().game.infiniteArrows.setSpeedrunValue(false);
    getSettings().game.infiniteSeeds.setSpeedrunValue(false);
    getSettings().game.infiniteBombs.setSpeedrunValue(false);
    getSettings().game.infiniteOil.setSpeedrunValue(false);
    getSettings().game.infiniteOxygen.setSpeedrunValue(false);
    getSettings().game.infiniteRupees.setSpeedrunValue(false);
    getSettings().game.enableIndefiniteItemDrops.setSpeedrunValue(false);

    getSettings().game.moonJump.setSpeedrunValue(false);
    getSettings().game.superClawshot.setSpeedrunValue(false);
    getSettings().game.alwaysGreatspin.setSpeedrunValue(false);
    getSettings().game.enableFastIronBoots.setSpeedrunValue(false);
    getSettings().game.canTransformAnywhere.setSpeedrunValue(false);
    getSettings().game.fastRoll.setSpeedrunValue(false);
    getSettings().game.fastSpinner.setSpeedrunValue(false);
    getSettings().game.freeMagicArmor.setSpeedrunValue(false);
    getSettings().game.invincibleEnemies.setSpeedrunValue(false);

    getSettings().game.pauseOnFocusLost.setSpeedrunValue(false);
    if constexpr (dusk::platform::SupportsFocusLossPause) {
        aurora_set_pause_on_focus_lost(false);
    }
    getSettings().backend.enableAdvancedSettings.setSpeedrunValue(false);
    getSettings().game.recordingMode.setSpeedrunValue(false);
    getSettings().game.debugFlyCam.setSpeedrunValue(false);
}

void restore_from_speedrun_mode() {
    config::EnumerateRegistered([](config::ConfigVarBase& cvar) { cvar.clearSpeedrunOverride(); });
    if constexpr (dusk::platform::SupportsFocusLossPause) {
        aurora_set_pause_on_focus_lost(getSettings().game.pauseOnFocusLost.getValue());
    }
}

const Rml::String kInternalResolutionHelpText =
    "Configure the resolution used for rendering the game. Higher values are more demanding on "
    "your graphics hardware.";
const Rml::String kShadowResolutionHelpText =
    "Configure the shadow-map resolution. Higher values improve shadow quality but increase GPU "
    "and memory usage.";
const Rml::String kResamplerHelpText =
    "Configure the sampling method used when scaling the internal resolution for final presentation.";
const Rml::String kBloomHelpText =
    "Configure the post-processing bloom effect. Classic uses the original bloom pass; Dusk uses "
    "a higher-quality bloom pass.";
const Rml::String kBloomBrightnessHelpText =
    "Configure bloom intensity. Higher values make bright areas glow more strongly.";
const Rml::String kDisplayBrightnessHelpText =
    "Adjust display brightness. 100% is default. Higher values brighten the output.";
const Rml::String kUnlockFramerateHelpText =
    "Uses inter-frame interpolation to enable higher frame rates.<br/><br/>May introduce minor "
    "visual artifacts or animation glitches.";

int float_setting_percent(ConfigVar<float>& var) {
    return static_cast<int>(var.getValue() * 100.0f + 0.5f);
}

bool gyro_enabled() {
    return getSettings().game.enableGyroAim ||
           (getSettings().game.enableGyroRollgoal &&
            getSettings().game.gyroMode.getValue() != GyroMode::Mouse);
}

void apply_display_brightness_setting() {
    const int brightnessPercent = std::clamp(getSettings().game.displayBrightness.getValue(), 25, 100);
    const int brightness = (brightnessPercent * 255) / 100;
    dComIfG_setBrightness(static_cast<u8>(brightness));
}

struct ConfigBoolProps {
    Rml::String key;
    Rml::String icon;
    Rml::String helpText;
    std::function<void(bool)> onChange;
    std::function<bool()> isDisabled;
};

SelectButton& config_bool_select(
    Pane& leftPane, Pane& rightPane, ConfigVar<bool>& var, ConfigBoolProps props) {
    auto& button = leftPane.add_child<BoolButton>(BoolButton::Props{
        .key = std::move(props.key),
        .icon = std::move(props.icon),
        .getValue = [&var] { return var.getValue(); },
        .setValue =
            [&var, callback = std::move(props.onChange)](bool value) {
                if (value == var.getValue()) {
                    return;
                }
                var.setValue(value);
                config::Save();
                if (callback) {
                    callback(value);
                }
            },
        .isDisabled = std::move(props.isDisabled),
        .isModified = [&var] { return var.getValue() != var.getDefaultValue(); },
    });
    leftPane.register_control(
        button, rightPane, [helpText = std::move(props.helpText)](Pane& pane) {
            pane.clear();
            pane.add_rml(helpText);
        });
    return button;
}

SelectButton& config_percent_select(Pane& leftPane, Pane& rightPane, ConfigVar<float>& var,
    Rml::String key, Rml::String helpText, int min, int max, int step = 5,
    std::function<bool()> isDisabled = {}) {
    auto& button = leftPane.add_child<NumberButton>(NumberButton::Props{
        .key = std::move(key),
        .getValue = [&var] { return float_setting_percent(var); },
        .setValue =
            [&var, min, max](int value) {
                var.setValue(std::clamp(value, min, max) / 100.0f);
                config::Save();
            },
        .isDisabled = std::move(isDisabled),
        .isModified = [&var] { return var.getValue() != var.getDefaultValue(); },
        .min = min,
        .max = max,
        .step = step,
        .suffix = "%",
    });
    leftPane.register_control(button, rightPane, [helpText = std::move(helpText)](Pane& pane) {
        pane.clear();
        pane.add_text(helpText);
    });
    return button;
}

template <typename T>
void graphics_tuner_control(Window& window, Pane& leftPane, Pane& rightPane, ConfigVar<T>& var,
    const GraphicsTunerProps& props, bool prelaunch) {
    leftPane.register_control(
        leftPane
            .add_select_button({
                .key = props.title,
                .getValue =
                    [&var, option = props.option] {
                        if constexpr (std::is_same_v<T, float>) {
                            return format_graphics_setting_value(
                                option, float_setting_percent(var));
                        } else {
                            return format_graphics_setting_value(
                                option, static_cast<int>(var.getValue()));
                        }
                    },
                .isModified = [&var] { return var.getValue() != var.getDefaultValue(); },
                .submit = false,
            })
            .on_nav_command([&window, props, prelaunch](Rml::Event&, NavCommand cmd) {
                if (cmd == NavCommand::Confirm || cmd == NavCommand::Left ||
                    cmd == NavCommand::Right) {
                    window.push(std::make_unique<GraphicsTuner>(props, prelaunch));
                    return true;
                }
                return false;
            }),
        rightPane, [helpText = props.helpText](Pane& pane) {
            pane.clear();
            pane.add_text(helpText);
        });
}

}  // namespace

SettingsWindow::SettingsWindow(bool prelaunch) : mPrelaunch(prelaunch) {
    if (prelaunch) {
        mSuppressNavFallback = true;
        add_tab("Prelaunch", [this](Rml::Element* content) {
            auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
            auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

            leftPane.register_control(
                leftPane
                    .add_select_button({
                        .key = "Disc Image",
                        .getValue =
                            [] {
                                const auto& path = prelaunch_state().configuredDiscPath;
                                std::string display;
                                if (path.empty()) {
                                    display = "(none)";
                                } else {
                                    display = display_name_for_path(path);
                                    if (display.empty()) {
                                        display = path;
                                    }
                                }
                                return display;
                            },
                        .isModified =
                            [] {
                                const auto& state = prelaunch_state();
                                const auto& active = state.activeDiscPath;
                                return !active.empty() && state.configuredDiscPath != active;
                            },
                    })
                    .on_pressed([] { open_iso_picker(); }),
                rightPane, [](Pane& pane) {
                    pane.add_rml("Set the disc image that Dusk uses to launch the game.<br/><br/>"
                                 "Changes require a restart.");
                });
            leftPane.register_control(
                leftPane.add_select_button({
                    .key = "Language",
                    .getValue =
                        [] {
                            const auto& state = prelaunch_state();
                            if (!state.configuredDiscCanLaunch || !state.configuredDiscInfo.isPal) {
                                return kLanguageNames[0];
                            }
                            const u8 idx = static_cast<u8>(getSettings().game.language.getValue());
                            return kLanguageNames[idx];
                        },
                    .isDisabled =
                        [] {
                            const auto& state = prelaunch_state();
                            return !state.configuredDiscCanLaunch ||
                                   !state.configuredDiscInfo.isPal;
                        },
                    .isModified =
                        [] {
                            return getSettings().game.language.getValue() !=
                                   prelaunch_state().initialLanguage;
                        },
                }),
                rightPane, [](Pane& pane) {
                    for (int i = 0; i < kLanguageNames.size(); i++) {
                        pane.add_button({
                                            .text = kLanguageNames[i],
                                            .isSelected =
                                                [i] {
                                                    return getSettings().game.language.getValue() ==
                                                           static_cast<GameLanguage>(i);
                                                },
                                        })
                            .on_pressed([i] {
                                mDoAud_seStartMenu(kSoundItemChange);
                                getSettings().game.language.setValue(static_cast<GameLanguage>(i));
                                config::Save();
                            });
                    }
                    pane.add_rml("<br/>Changes require a restart.");
                });
            leftPane.register_control(
                leftPane.add_select_button({
                    .key = "Graphics Backend",
                    .getValue = [] { return Rml::String{backend_name(configured_backend())}; },
                    .isModified =
                        [] {
                            return getSettings().backend.graphicsBackend.getValue() !=
                                   prelaunch_state().initialGraphicsBackend;
                        },
                }),
                rightPane, [](Pane& pane) {
                    const auto availableBackends = available_backends();
                    for (const auto backend : availableBackends) {
                        pane
                            .add_button({
                                .text = Rml::String{backend_name(backend)},
                                .isSelected = [backend] { return configured_backend() == backend; },
                            })
                            .on_pressed([backend] {
                                mDoAud_seStartMenu(kSoundItemChange);
                                getSettings().backend.graphicsBackend.setValue(
                                    std::string{backend_id(backend)});
                                config::Save();
                            });
                    }
                    pane.add_rml("<br/>Changes require a restart.");
                });
            leftPane.register_control(
                leftPane.add_select_button({
                    .key = "Save File Type",
                    .getValue =
                        [] {
                            return kCardFileTypes[getSettings().backend.cardFileType.getValue()];
                        },
                    .isModified =
                        [] {
                            return getSettings().backend.cardFileType.getValue() !=
                                   prelaunch_state().initialCardFileType;
                        },
                }),
                rightPane, [](Pane& pane) {
                    for (int i = 0; i < kCardFileTypes.size(); i++) {
                        pane
                            .add_button({
                                .text = kCardFileTypes[i],
                                .isSelected =
                                    [i] {
                                        return getSettings().backend.cardFileType.getValue() == i;
                                    },
                            })
                            .on_pressed([i] {
                                mDoAud_seStartMenu(kSoundItemChange);
                                getSettings().backend.cardFileType.setValue(i);
                                config::Save();
                            });
                    }
                });
        });
    }

    add_tab("Video", [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

        leftPane.add_section("Display", true);

        leftPane.register_control(leftPane.add_button("Toggle Fullscreen").on_pressed([] {
            mDoAud_seStartMenu(kSoundItemChange);
            getSettings().video.enableFullscreen.setValue(!getSettings().video.enableFullscreen);
            VISetWindowFullscreen(getSettings().video.enableFullscreen);
            config::Save();
        }),
            rightPane, [](Pane& pane) { pane.clear(); });
        leftPane.register_control(leftPane.add_button("Restore Default Window Size").on_pressed([] {
            mDoAud_seStartMenu(kSoundItemChange);
            getSettings().video.enableFullscreen.setValue(false);
            VISetWindowFullscreen(false);
            VISetWindowSize(FB_WIDTH * 2, FB_HEIGHT * 2);
            VICenterWindow();
        }),
            rightPane, [](Pane& pane) { pane.clear(); });
        config_bool_select(leftPane, rightPane, getSettings().video.enableVsync,
            {
                .key = "Enable VSync",
                .helpText = "Synchronizes the frame rate to your monitor's refresh rate.",
                .onChange = [](bool value) { aurora_enable_vsync(value); },
            });
        leftPane.register_control(
            leftPane.add_select_button({
                .key = "FPS Limit",
                .getValue = [] {
                    int v = getSettings().game.fpsLimit.getValue();
                    return v == 0 ? Rml::String{"Unlimited"} : Rml::String{std::to_string(v)};
                },
            }),
            rightPane, [](Pane& pane) {
                for (int fps : {0, 30, 60, 120}) {
                    pane.add_button({
                        .text = fps == 0 ? "Unlimited" : Rml::String{std::to_string(fps)},
                        .isSelected = [fps] { return getSettings().game.fpsLimit.getValue() == fps; },
                    }).on_pressed([fps] {
                        mDoAud_seStartMenu(kSoundItemChange);
                        getSettings().game.fpsLimit.setValue(fps);
                        if (fps == 120) {
                            getSettings().video.enableVsync.setValue(false);
                            aurora_enable_vsync(false);
                        } else if (fps == 30 || fps == 60) {
                            getSettings().video.enableVsync.setValue(true);
                            aurora_enable_vsync(true);
                        }
                        config::Save();
                    });
                }
                pane.add_rml("Limits the maximum framerate. Select 120 for high-refresh displays (disables VSync automatically).");
            });
        config_bool_select(leftPane, rightPane, getSettings().video.lockAspectRatio,
            {
                .key = "Lock 4:3 Aspect Ratio",
                .helpText = "Lock the game's aspect ratio to the original.",
                .onChange =
                    [](bool value) {
                        AuroraSetViewportPolicy(
                            value ? AURORA_VIEWPORT_FIT : AURORA_VIEWPORT_STRETCH);
                    },
            });
        if constexpr (dusk::platform::SupportsFocusLossPause) {
            config_bool_select(leftPane, rightPane, getSettings().game.pauseOnFocusLost,
                {
                    .key = "Pause on Focus Lost",
                    .helpText = "Pause the game when window focus is lost.",
                    .onChange = [](bool value) { aurora_set_pause_on_focus_lost(value); },
                    .isDisabled = [] { return IsMobile || getSettings().game.speedrunMode; },
                });
        }
        leftPane.register_control(
            leftPane.add_select_button({
                .key = "Show FPS Counter",
                .getValue =
                    [] {
                        if (!getSettings().video.enableFpsOverlay.getValue()) {
                            return Rml::String{"Off"};
                        }
                        const int idx = getSettings().video.fpsOverlayCorner.getValue();
                        return Rml::String{kFpsOverlayCornerNames[idx]};
                    },
                .isModified =
                    [] {
                        const auto& enable = getSettings().video.enableFpsOverlay;
                        const auto& corner = getSettings().video.fpsOverlayCorner;
                        return enable.getValue() != enable.getDefaultValue() ||
                               (enable.getValue() && corner.getValue() != corner.getDefaultValue());
                    },
            }),
            rightPane, [](Pane& pane) {
                pane.add_button(
                        {
                            .text = "Off",
                            .isSelected =
                                [] { return !getSettings().video.enableFpsOverlay.getValue(); },
                        })
                    .on_pressed([] {
                        mDoAud_seStartMenu(kSoundItemChange);
                        getSettings().video.enableFpsOverlay.setValue(false);
                        config::Save();
                    });
                for (int i = 0; i < static_cast<int>(kFpsOverlayCornerNames.size()); ++i) {
                    pane.add_button(
                            {
                                .text = kFpsOverlayCornerNames[i],
                                .isSelected =
                                    [i] {
                                        return getSettings().video.enableFpsOverlay.getValue() &&
                                               getSettings().video.fpsOverlayCorner.getValue() == i;
                                    },
                            })
                        .on_pressed([i] {
                            mDoAud_seStartMenu(kSoundItemChange);
                            getSettings().video.enableFpsOverlay.setValue(true);
                            getSettings().video.fpsOverlayCorner.setValue(i);
                            config::Save();
                        });
                }
                pane.add_rml(
                    "<br/>Display the current framerate in a corner of the screen while playing.");
            });

        leftPane.add_section("Resolution", true);
        graphics_tuner_control(*this, leftPane, rightPane,
            getSettings().game.internalResolutionScale,
            GraphicsTunerProps{
                .option = GraphicsOption::InternalResolution,
                .title = "Internal Resolution",
                .helpText = kInternalResolutionHelpText,
                .valueMin = 0,
                .valueMax = 12,
                .defaultValue = 0,
            }, mPrelaunch);
        graphics_tuner_control(*this, leftPane, rightPane,
            getSettings().game.shadowResolutionMultiplier,
            GraphicsTunerProps{
                .option = GraphicsOption::ShadowResolution,
                .title = "Shadow Resolution",
                .helpText = kShadowResolutionHelpText,
                .valueMin = 1,
                .valueMax = 8,
                .defaultValue = 1,
            }, mPrelaunch);
        graphics_tuner_control(*this, leftPane, rightPane, getSettings().game.resampler,
            GraphicsTunerProps{
                .option = GraphicsOption::Resampler,
                .title = "Output Resampling",
                .helpText = kResamplerHelpText,
                .valueMin = static_cast<int>(Resampler::Bilinear),
                .valueMax = static_cast<int>(Resampler::Area),
                .defaultValue = static_cast<int>(Resampler::Bilinear),
            }, mPrelaunch);

        leftPane.add_section("Post-Processing", true);
        graphics_tuner_control(*this, leftPane, rightPane, getSettings().game.bloomMode,
            GraphicsTunerProps{
                .option = GraphicsOption::BloomMode,
                .title = "Bloom",
                .helpText = kBloomHelpText,
                .valueMin = static_cast<int>(BloomMode::Off),
                .valueMax = static_cast<int>(BloomMode::Dusk),
                .defaultValue = static_cast<int>(BloomMode::Classic),
            }, mPrelaunch);
        graphics_tuner_control(*this, leftPane, rightPane, getSettings().game.bloomMultiplier,
            GraphicsTunerProps{
                .option = GraphicsOption::BloomMultiplier,
                .title = "Bloom Brightness",
                .helpText = kBloomBrightnessHelpText,
                .valueMin = 0,
                .valueMax = 100,
                .defaultValue = 100,
                .step = 10,
            }, mPrelaunch);

        leftPane.add_section("Rendering", true);
        config_bool_select(leftPane, rightPane, getSettings().game.enableTextureReplacements,
            {
                .key = "Use Texture Pack",
                .helpText = "Enable installed texture replacements.",
                .onChange = [](bool value) { aurora_set_texture_replacements_enabled(value); },
            });
        config_bool_select(leftPane, rightPane, getSettings().game.enableFrameInterpolation,
            {
                .key = "Unlock Framerate",
                .helpText = kUnlockFramerateHelpText,
            });
        config_bool_select(leftPane, rightPane, getSettings().game.enableDepthOfField,
            {
                .key = "Enable Depth of Field",
            });

        leftPane.register_control(
            leftPane.add_select_button({
                .key = "Anisotropic Filtering",
                .getValue = [] {
                    int v = getSettings().game.anisotropicFiltering.getValue();
                    return v == 0 ? Rml::String{"Off"} : Rml::String{std::to_string(v) + "x"};
                },
            }),
            rightPane, [](Pane& pane) {
                for (int af : {0, 2, 4, 8, 16}) {
                    pane.add_button({
                        .text = af == 0 ? "Off" : Rml::String{std::to_string(af) + "x"},
                        .isSelected = [af] { return getSettings().game.anisotropicFiltering.getValue() == af; },
                    }).on_pressed([af] {
                        mDoAud_seStartMenu(kSoundItemChange);
                        getSettings().game.anisotropicFiltering.setValue(af);
                        config::Save();
                    });
                }
                pane.add_rml("Improves texture clarity at oblique viewing angles. Higher values have a small performance cost. Requires a restart.");
            });
        config_bool_select(leftPane, rightPane, getSettings().game.enableMapBackground,
            {
                .key = "Enable Mini-Map Shadows",
            });
        leftPane.register_control(
            leftPane.add_child<NumberButton>(NumberButton::Props{
                .key = "Display Brightness",
                .getValue = [] { return getSettings().game.displayBrightness.getValue(); },
                .setValue =
                    [](int value) {
                        getSettings().game.displayBrightness.setValue(std::clamp(value, 25, 100));
                        config::Save();
                        apply_display_brightness_setting();
                    },
                .isModified =
                    [] {
                        return getSettings().game.displayBrightness.getValue() !=
                               getSettings().game.displayBrightness.getDefaultValue();
                    },
                .min = 25,
                .max = 100,
                .step = 5,
                .suffix = "%",
            }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_text(kDisplayBrightnessHelpText);
            });
        config_bool_select(leftPane, rightPane, getSettings().game.disableCutscenePillarboxing,
            {
                .key = "Disable Cutscene Pillarboxing",
                .helpText = "Keep wide cutscenes pillarboxed instead of cropping them to fill the screen.",
            });
    });

    add_tab("Input", [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

        auto addOption = [&](const Rml::String& key, ConfigVar<bool>& value,
                             const Rml::String& helpText, std::function<bool()> isDisabled = {}) {
            config_bool_select(leftPane, rightPane, value,
                {
                    .key = key,
                    .helpText = helpText,
                    .isDisabled = std::move(isDisabled),
                });
        };

        leftPane.add_section("Inputs", true);
        leftPane.register_control(leftPane.add_button("Configure Inputs").on_pressed([this] {
            push(std::make_unique<ControllerConfigWindow>(mPrelaunch));
        }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_text("Open input binding configuration.");
            });
        if constexpr (dusk::platform::SupportsBackgroundInputOption) {
            config_bool_select(leftPane, rightPane, getSettings().game.allowBackgroundInput,
                {
                    .key = "Allow Background Inputs",
                    .helpText = "Allow inputs even when the game window is not focused.",
                    .onChange = [](bool value) { aurora_set_background_input(value); },
                });
        }

        leftPane.add_section("Camera", true);
        addOption("Free Camera", getSettings().game.freeCamera,
            "Enables twin-stick camera control, letting the C-Stick move the camera vertically as "
            "well as horizontally.");
        addOption("Invert Camera X Axis", getSettings().game.invertCameraXAxis,
            "Invert horizontal camera movement.");
        addOption("Invert Camera Y Axis", getSettings().game.invertCameraYAxis,
            "Invert vertical camera movement when Free Camera is enabled.",
            [] { return !getSettings().game.freeCamera; });
        config_percent_select(leftPane, rightPane, getSettings().game.freeCameraSensitivity,
            "Free Camera Sensitivity", "Adjusts twin-stick camera sensitivity.", 50, 200, 5,
            [] { return !getSettings().game.freeCamera; });
        config_bool_select(leftPane, rightPane, getSettings().game.enableMouseFreeLook,
            {
                .key = "Mouse Free Look",
                .helpText = "Use the mouse to control the camera when Free Camera is enabled. Click the game window to capture the cursor.",
                .isDisabled = [] { return !getSettings().game.freeCamera; },
            });
        config_bool_select(leftPane, rightPane, getSettings().game.firstPersonFreeCam,
            {
                .key = "First Person Free Cam",
                .helpText = "Switch to first-person view when Free Camera is enabled, instead of orbiting behind Link.",
                .isDisabled = [] { return !getSettings().game.freeCamera; },
            });
        addOption("Invert First Person X Axis", getSettings().game.invertFirstPersonXAxis,
            "Invert horizontal movement while aiming with items or first person camera. Applies to both stick and gyro aiming.");
        addOption("Invert First Person Y Axis", getSettings().game.invertFirstPersonYAxis,
            "Invert vertical movement while aiming with items or first person camera. Applies to both stick and gyro aiming.");
        addOption("Invert Air/Swim X Axis", getSettings().game.invertAirSwimX,
            "Invert horizontal movement while flying or swimming.");
        addOption("Invert Air/Swim Y Axis", getSettings().game.invertAirSwimY,
            "Invert vertical movement while flying or swimming.");

        leftPane.add_section("Gyro", true);
        leftPane.register_control(
            leftPane.add_select_button({
                .key = "Gyro Input Method",
                .getValue =
                    [] {
                        const auto mode = getSettings().game.gyroMode.getValue();
                        const auto idx = static_cast<size_t>(mode);
                        return Rml::String{kGyroInputModeLabels[idx]};
                    },
                .isModified =
                    [] {
                        return getSettings().game.gyroMode.getValue() !=
                               getSettings().game.gyroMode.getDefaultValue();
                    },
            }),
            rightPane, [](Pane& pane) {
                for (size_t i = 0; i < kGyroInputModeLabels.size(); i++) {
                    pane
                        .add_button({
                            .text = Rml::String{kGyroInputModeLabels[i]},
                            .isSelected =
                                [i] {
                                    return getSettings().game.gyroMode.getValue() == static_cast<GyroMode>(i);
                                },
                        })
                        .on_pressed([i] {
                            mDoAud_seStartMenu(kSoundItemChange);
                            const GyroMode mode = static_cast<GyroMode>(i);
                            getSettings().game.gyroMode.setValue(mode);
                            config::Save();
                        });
                }
                pane.add_rml(
                    "<br/><b>Sensor</b> reads motion directly from a supported controller's gyro via SDL.<br/>"
                    "<br/><b>Mouse</b> treats mouse input as gyro, intended for use with the Steam Deck.<br/>"
                    "<br/>Mouse input cannot currently be used with Gyro Rollgoal.");
            });
        addOption("Gyro Aim", getSettings().game.enableGyroAim,
            "Enables gyro controls while in look mode, aiming a hawk, and aiming "
            "supported items.<br/><br/>Supported items include the Slingshot, Gale Boomerang, "
            "Hero's Bow, Clawshot(s), Ball and Chain, and Dominion Rod.");
        addOption("Gyro Rollgoal", getSettings().game.enableGyroRollgoal,
            "Enables gyro controls for Rollgoal in Hena's Cabin.",
            [] { return getSettings().game.gyroMode.getValue() == GyroMode::Mouse; });
        config_percent_select(leftPane, rightPane, getSettings().game.gyroSensitivityY,
            "Gyro Pitch Sensitivity", "Controls vertical gyro aiming sensitivity.", 25, 400, 5,
            [] { return !gyro_enabled(); });
        config_percent_select(leftPane, rightPane, getSettings().game.gyroSensitivityX,
            "Gyro Yaw Sensitivity", "Controls horizontal gyro aiming sensitivity.", 25, 400, 5,
            [] { return !gyro_enabled(); });
        config_percent_select(leftPane, rightPane, getSettings().game.gyroSensitivityRollgoal,
            "Rollgoal Sensitivity", "Controls how strongly gyro input tilts the Rollgoal table.",
            25, 400, 5,
            [] {
                return !getSettings().game.enableGyroRollgoal ||
                       getSettings().game.gyroMode.getValue() == GyroMode::Mouse;
            });
        config_percent_select(leftPane, rightPane, getSettings().game.gyroDeadband, "Gyro Deadband",
            "Ignores small gyro movement to reduce drift and jitter.", 0, 50, 1,
            [] { return !gyro_enabled(); });
        config_percent_select(leftPane, rightPane, getSettings().game.gyroSmoothing,
            "Gyro Smoothing", "Higher values smooth gyro input over time.", 0, 100, 1,
            [] { return !gyro_enabled(); });
        addOption("Invert Gyro Pitch", getSettings().game.gyroInvertPitch,
            "Invert vertical gyro aiming.", [] { return !gyro_enabled(); });
        addOption("Invert Gyro Yaw", getSettings().game.gyroInvertYaw,
            "Invert horizontal gyro aiming.", [] { return !gyro_enabled(); });

        leftPane.add_section("Tools", true);
        addOption("Turbo Key", getSettings().game.enableTurboKeybind,
            "Hold Tab to increase game speed by up to 4x.",
            [] { return getSettings().game.speedrunMode; });
        addOption("Reset Key (" + Rml::String{hotkeys::DO_RESET} + ")",
            getSettings().game.enableResetKeybind,
            "Press " + Rml::String{hotkeys::DO_RESET} + " to reset the game.",
            [] { return getSettings().game.speedrunMode; });
        addOption("Show Input Viewer", getSettings().game.showInputViewer,
            "Show the controller input overlay while playing.");
        addOption("Show Input Viewer Gyro", getSettings().game.showInputViewerGyro,
            "Show gyro values in the input viewer overlay.",
            [] { return !getSettings().game.showInputViewer.getValue(); });

        if (IsMobile) {
            leftPane.add_section("Touch Controls", true);
            config_bool_select(leftPane, rightPane, getSettings().touch.enabled,
                {
                    .key = "Touch Controls",
                    .helpText = "Show on-screen virtual buttons while playing.<br/><br/>"
                                "When a physical controller connects, the overlay hides "
                                "automatically and reappears when the controller disconnects.",
                    .onChange = [](bool) { config::Save(); },
                });
            config_percent_select(leftPane, rightPane, getSettings().touch.scale,
                "Button Size", "Size of the virtual buttons as a percentage of their default size.",
                50, 500, 10,
                [] { return !touch_controls::is_enabled(); });
            config_percent_select(leftPane, rightPane, getSettings().touch.opacity,
                "Opacity", "Transparency of the virtual buttons while playing.",
                10, 100, 5,
                [] { return !touch_controls::is_enabled(); });
            leftPane.register_control(
                leftPane.add_child<NumberButton>(NumberButton::Props{
                    .key = "Left Stick Deadzone",
                    .getValue = [] { return getSettings().touch.stickMainDeadzone.getValue(); },
                    .setValue = [](int value) {
                        getSettings().touch.stickMainDeadzone.setValue(value);
                        config::Save();
                    },
                    .isDisabled = [] { return !touch_controls::is_enabled(); },
                    .max = 50,
                    .suffix = "%",
                }),
                rightPane, [](Pane& pane) {
                    pane.clear();
                    pane.add_text("Deadzone for the left virtual joystick. Higher values require more finger movement before input registers.");
                });
            leftPane.register_control(
                leftPane.add_child<NumberButton>(NumberButton::Props{
                    .key = "Right Stick Deadzone",
                    .getValue = [] { return getSettings().touch.stickCDeadzone.getValue(); },
                    .setValue = [](int value) {
                        getSettings().touch.stickCDeadzone.setValue(value);
                        config::Save();
                    },
                    .isDisabled = [] { return !touch_controls::is_enabled(); },
                    .max = 50,
                    .suffix = "%",
                }),
                rightPane, [](Pane& pane) {
                    pane.clear();
                    pane.add_text("Deadzone for the right virtual joystick. Higher values require more finger movement before input registers.");
                });
            config_bool_select(leftPane, rightPane, getSettings().touch.menuTapNav,
                {
                    .key = "Tap to Confirm",
                    .helpText = "When enabled, tapping anywhere on screen (outside the virtual buttons) "
                                "acts as pressing the A button to confirm menu selections.",
                    .onChange = [](bool) { config::Save(); },
                    .isDisabled = [] { return !touch_controls::is_enabled(); },
                });
            config_bool_select(leftPane, rightPane, getSettings().touch.floatingCamera,
                {
                    .key = "Floating Camera Zone",
                    .helpText = "Shows an additional virtual joystick on the right side of the screen for camera control. The C-stick will be hidden when enabled.",
                    .onChange = [](bool) { config::Save(); },
                    .isDisabled = [] { return !touch_controls::is_enabled(); },
                });
            leftPane.register_control(
                leftPane.add_button("Customize Layout").on_pressed([] {
                    mDoAud_seStartMenu(kSoundItemChange);
                    touch_controls::enter_customize_mode();
                    if (auto* doc = ui::top_document()) doc->pop();
                }),
                rightPane, [](Pane& pane) {
                    pane.clear();
                    pane.add_text("Drag buttons to reposition them on screen.");
                    pane.add_rml("<br/><br/>Closes settings and enters layout mode. "
                                 "Tap <b>Done</b> when finished to save.");
                });
        }
    });

    add_tab("Audio", [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

        // TODO: Individual sliders for Main Music, Sub Music, Sound Effects, and Fanfare.
        leftPane.add_section("Volume", true);
        leftPane.register_control(
            leftPane.add_child<NumberButton>(NumberButton::Props{
                .key = "Master Volume",
                .getValue = [] { return getSettings().audio.masterVolume.getValue(); },
                .setValue =
                    [](int value) {
                        getSettings().audio.masterVolume.setValue(value);
                        config::Save();
                        audio::SetMasterVolume(value / 100.f);
                    },
                .isModified =
                    [] {
                        return getSettings().audio.masterVolume.getValue() !=
                               getSettings().audio.masterVolume.getDefaultValue();
                    },
                .max = 100,
                .suffix = "%",
            }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_text("Adjusts the volume of all sounds in the game.");
            });

        leftPane.add_section("Effects", true);
        config_bool_select(leftPane, rightPane, getSettings().audio.enableReverb,
            {
                .key = "Enable Reverb",
                .helpText = "Enables the reverb effect in game audio.",
                .onChange = [](bool value) { audio::SetEnableReverb(value); },
            });
        config_bool_select(leftPane, rightPane, getSettings().audio.enableHrtf,
            {
                .key = "Enable Spatial Sound",
                .helpText =
                    "Emulate surround sound via HRTF. Recommended only for use with headphones!",
                .onChange = [](bool value) { audio::EnableHrtf = value; },
            });
        config_bool_select(leftPane, rightPane, getSettings().audio.menuSounds,
            {
                .key = "Dusk Menu Sounds",
                .helpText = "Play sound effects when navigating the Dusk menu.",
            });

        leftPane.add_section("Tweaks", true);
        config_bool_select(leftPane, rightPane, getSettings().game.noLowHpSound,
            {
                .key = "No Low HP Sound",
                .helpText = "Disable the beeping sound when having low health.",
            });
        leftPane.register_control(leftPane.add_select_button({
                                      .key = "Battle Music",
                                      .getValue =
                                          [] {
                                              const auto mode =
                                                  getSettings().game.battleBGM.getValue();
                                              const auto idx = static_cast<size_t>(mode);
                                              return Rml::String{kBattleBGMModeLabels[idx]};
                                          },
                                      .isModified =
                                          [] {
                                              return getSettings().game.battleBGM.getValue() !=
                                                     getSettings().game.battleBGM.getDefaultValue();
                                          },
                                  }),
            rightPane, [](Pane& pane) {
                for (size_t i = 0; i < kBattleBGMModeLabels.size(); i++) {
                    pane.add_button({
                                        .text = Rml::String{kBattleBGMModeLabels[i]},
                                        .isSelected =
                                            [i] {
                                                return getSettings().game.battleBGM.getValue() ==
                                                       static_cast<BattleBGMMode>(i);
                                            },
                                    })
                        .on_pressed([i] {
                            mDoAud_seStartMenu(kSoundItemChange);
                            getSettings().game.battleBGM.setValue(static_cast<BattleBGMMode>(i));
                            config::Save();
                        });
                }
                pane.add_rml("<br/>On: Plays enemy music normally.<br/>"
                             "<br/>Off: Disables enemy music entirely.<br/>"
                             "<br/>Mute During Lament: Prevents enemy music while Midna's Lament "
                             "is playing. ");
            });
    });

    add_tab("Gameplay", [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

        auto addOption = [&](const Rml::String& key, ConfigVar<bool>& value,
                             const Rml::String& helpText) {
            config_bool_select(leftPane, rightPane, value,
                {
                    .key = key,
                    .helpText = helpText,
                });
        };
        auto addSpeedrunDisabledOption = [&](const Rml::String& key, ConfigVar<bool>& value,
                                             const Rml::String& helpText) {
            config_bool_select(leftPane, rightPane, value,
                {
                    .key = key,
                    .helpText = helpText,
                    .isDisabled = [] { return getSettings().game.speedrunMode; },
                });
        };

        leftPane.add_section("General", true);
        addOption("Mirror Mode", getSettings().game.enableMirrorMode,
            "Mirrors the world horizontally, matching the Wii version of the game.");
        leftPane.register_control(
            leftPane.add_select_button({
                .key = "Ingame HUD",
                .getValue =
                    [] {
                        const int val = static_cast<int>(getSettings().game.ingameHudMode.getValue());
                        if (val == static_cast<int>(IngameHudMode::On)) {
                            return "On";
                        } else if (val) {
                            return "Custom";
                        } else {
                            return "Off";
                        }
                    },
                .isModified =
                    [] {
                        const auto& hudMode = getSettings().game.ingameHudMode;
                        return hudMode.getValue() != hudMode.getDefaultValue();
                    },
            }),
            rightPane, [](Pane& pane) {
                pane.add_button({
                        .text = "All On",
                        .isSelected = 
                            [] {
                                return getSettings().game.ingameHudMode.getValue() 
                                        == IngameHudMode::On;
                            },
                        })
                    .on_pressed([] {
                        mDoAud_seStartMenu(kSoundItemChange);
                        getSettings().game.ingameHudMode.setValue(IngameHudMode::On);
                        config::Save();
                });
                pane.add_button({
                        .text = "All Off",
                        .isSelected = 
                            [] {
                                return getSettings().game.ingameHudMode.getValue() 
                                        == IngameHudMode::Off;
                            },
                        })
                    .on_pressed([] {
                        mDoAud_seStartMenu(kSoundItemChange);
                        getSettings().game.ingameHudMode.setValue(IngameHudMode::Off);
                        config::Save();
                });
                pane.add_rml("<br/>");
                constexpr std::array kIngameHudModeLabels = {
                    "Health",
                    "Rupees",
                    "Action Buttons",
                    "D-Pad",
                    "Lamp Meter",
                    "Oxygen Meter",
                    "Keys",
                    "Vessel of Light"
                };
                for (int i = 0; i < static_cast<int>(kIngameHudModeLabels.size()); i++) {
                    pane.add_button({
                        .text = kIngameHudModeLabels[i],
                        .isSelected = 
                            [i] {
                                return static_cast<int>(getSettings().game.ingameHudMode.getValue()) 
                                        & (1 << i);
                            },
                        })
                    .on_pressed([i] {
                        int val = static_cast<int>(getSettings().game.ingameHudMode.getValue());
                        if (val & (1 << i)) {
                            val &= ~(1 << i);
                        } else {
                            val |= (1 << i);
                        }

                        mDoAud_seStartMenu(kSoundItemChange);
                        getSettings().game.ingameHudMode.setValue(static_cast<IngameHudMode>(val));
                        config::Save();
                    });
                }
                pane.add_rml(
                    "Toggle various elements of the main HUD of the game."
                );
            }
        );
        addOption("Restore Wii 1.0 Glitches", getSettings().game.restoreWiiGlitches,
            "Restores patched glitches from Wii USA 1.0, the first released version.");
        addOption("Enable Rotating Link Doll", getSettings().game.enableLinkDollRotation,
            "Enables rotating Link in the collection menu with the C-Stick.");
        addOption("Hide Owl Statue Markers", getSettings().game.removeQuestMapMarkers,
            "Removes completed Owl Statue markers from the map and minimap.");

        leftPane.add_section("Difficulty", true);
        leftPane.register_control(
            leftPane.add_child<NumberButton>(NumberButton::Props{
                .key = "Damage Multiplier",
                .getValue = [] { return getSettings().game.damageMultiplier.getValue(); },
                .setValue =
                    [](int value) {
                        getSettings().game.damageMultiplier.setValue(value);
                        config::Save();
                    },
                .isDisabled = [] { return getSettings().game.speedrunMode; },
                .isModified =
                    [] {
                        return getSettings().game.damageMultiplier.getValue() !=
                               getSettings().game.damageMultiplier.getDefaultValue();
                    },
                .min = 1,
                .max = 8,
                .suffix = "×",
            }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_text("Multiplies incoming damage.");
            });
        addSpeedrunDisabledOption(
            "Instant Death", getSettings().game.instantDeath, "Any hit will instantly kill you.");
        addSpeedrunDisabledOption("No Heart Drops", getSettings().game.noHeartDrops,
            "Hearts will never drop from enemies, pots, and various other places.");

        leftPane.add_section("Quality of Life", true);
        addOption("Bigger Wallets", getSettings().game.biggerWallets,
            "Wallet sizes are like in the HD version. (500, 1000, 2000)");
        addOption("Disable Rupee Cutscenes", getSettings().game.disableRupeeCutscenes,
            "Rupees will not play cutscenes after you have collected them the first time.");
        addOption("Faster Climbing", getSettings().game.fastClimbing,
            "Quicker climbing on ladders and vines like the HD version.");
        addOption("Faster Tears of Light", getSettings().game.fastTears,
            "Tears of Light dropped by Shadow Insects pop out faster like the HD version.");
        config_bool_select(leftPane, rightPane, getSettings().game.autoSave,
            {
                .key = "Autosave",
                .helpText = "Autosaves the game when going to a new area, opening a dungeon door, "
                            "or getting a new item.",
                .isDisabled = [] { return getSettings().game.speedrunMode; },
            });
        addOption("Instant Saves", getSettings().game.instantSaves,
            "Skips the delay when writing to the Memory Card.");
        addOption("Hold B for Instant Text", getSettings().game.instantText,
            "Makes text scroll immediately by holding B.");
        addOption("No Climbing Miss Animation", getSettings().game.noMissClimbing,
            "Prevents Link from playing a struggle animation when grabbing ledges or "
            "climbing on vines.");
        addOption("No Rupee Returns", getSettings().game.noReturnRupees,
            "Always collect Rupees even if your Wallet is too full.");
        addOption("No Sword Recoil", getSettings().game.noSwordRecoil,
            "Link will not recoil when his sword hits walls.");
        addOption("No 2nd Fish for Cat", getSettings().game.no2ndFishForCat,
            "Skip needing to catch a second fish for Sera's cat.");
        addOption("Sun's Song (R+X)", getSettings().game.sunsSong,
            "Allows Wolf Link to howl and change the time of day.");
        addOption("Quick Transform (R+Y)", getSettings().game.enableQuickTransform,
            "Transform instantly by pressing R and Y simultaneously.");
        addOption("Fast Area Transitions", getSettings().game.fastAreaTransitions,
            "Reduces fade-out timing when transitioning between areas. Best for SSDs.");

        leftPane.add_section("Speedrunning", true);
        config_bool_select(leftPane, rightPane, getSettings().game.speedrunMode,
            {
                .key = "Speedrun Mode",
                .helpText =
                    "Enables speedrunning options while restricting certain gameplay modifiers.",
                .onChange =
                    [](bool enabled) {
                        if (enabled) {
                            reset_for_speedrun_mode();
                        } else {
                            restore_from_speedrun_mode();
                        }
                    },
            });
        config_bool_select(leftPane, rightPane, getSettings().game.liveSplitEnabled,
            {
                .key = "LiveSplit Connection",
                .helpText = "Connect to LiveSplit server on localhost:16834.",
                .onChange =
                    [](bool enabled) {
                        if (enabled) {
                            speedrun::connectLiveSplit();
                        } else {
                            speedrun::disconnectLiveSplit();
                        }
                    },
                .isDisabled = [] { return !getSettings().game.speedrunMode; },
            });
        config_bool_select(leftPane, rightPane, getSettings().game.showSpeedrunRTATimer,
            {
                .key = "Show RTA Timer",
                .helpText = "Display the speedrun timer overlay while speedrun mode is enabled.",
                .isDisabled = [] { return !getSettings().game.speedrunMode; },
            });
    });

    add_tab("Cheats", [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

        auto addCheat = [&](const Rml::String& key, ConfigVar<bool>& value,
                            const Rml::String& helpText) {
            config_bool_select(leftPane, rightPane, value,
                {
                    .key = key,
                    .helpText = helpText,
                    .isDisabled = [] { return getSettings().game.speedrunMode; },
                });
        };

        leftPane.add_section("Resources", true);
        addCheat("Infinite Hearts", getSettings().game.infiniteHearts, "Keeps your health full.");
        addCheat(
            "Infinite Arrows", getSettings().game.infiniteArrows, "Keeps your arrow count full.");
        addCheat("Infinite Seeds", getSettings().game.infiniteSeeds, "Keeps your slingshot pellets (seeds) full.");
        addCheat("Infinite Bombs", getSettings().game.infiniteBombs, "Keeps all bomb bags full.");
        addCheat("Infinite Oil", getSettings().game.infiniteOil, "Keeps your lantern oil full.");
        addCheat("Infinite Oxygen", getSettings().game.infiniteOxygen,
            "Keeps your underwater oxygen meter full.");
        addCheat(
            "Infinite Rupees", getSettings().game.infiniteRupees, "Keeps your rupee count full.");
        addCheat("No Item Timer", getSettings().game.enableIndefiniteItemDrops,
            "Item drops such as rupees and hearts will never disappear after they drop.");

        leftPane.add_section("Abilities", true);
        addCheat(
            "Moon Jump (R+A)", getSettings().game.moonJump, "Hold R and A to rise into the air.");
        addCheat("Super Clawshot", getSettings().game.superClawshot,
            "Extends Clawshot behavior beyond the normal game rules.");
        addCheat("Always Greatspin", getSettings().game.alwaysGreatspin,
            "Allows the Great Spin attack without requiring full health.");
        addCheat("Fast Iron Boots", getSettings().game.enableFastIronBoots,
            "Speeds up movement while wearing the Iron Boots.");
        addCheat("Can Transform Anywhere", getSettings().game.canTransformAnywhere,
            "Allows transforming even if NPCs are looking.");
        addCheat("Fast Roll", getSettings().game.fastRoll,
            "Makes Link's roll animation and movement twice as fast.");
        addCheat("Fast Spinner", getSettings().game.fastSpinner,
            "Speeds up Spinner movement while holding R.");
        addCheat("Free Magic Armor", getSettings().game.freeMagicArmor,
            "Lets the magic armor work without consuming rupees.");
        addCheat("Invincible Enemies", getSettings().game.invincibleEnemies,
            "Enemies cannot hurt you.");
        addCheat("Infinite Chu Jelly", getSettings().game.infiniteChuJelly,
            "Keeps your Chu Jelly supplies full.");
        addCheat("Transform without Shadow Crystal", getSettings().game.transformWithoutShadowCrystal,
            "Allows transforming into a wolf without the Shadow Crystal.");
    });

    add_tab("Interface", [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

        leftPane.add_section("Dusk", true);
#if DUSK_CAN_OPEN_DATA_FOLDER

        config_bool_select(leftPane, rightPane, getSettings().game.autoBackupSaves,
            {
                .key = "Auto-Backup Saves",
                .helpText = "Automatically create a timestamped backup of your save files on game launch. Backups are stored in the saves/backups/ folder.",
                .onChange = [](bool) { config::Save(); },
            });
            leftPane.register_control(
                leftPane.add_button("Open Data Folder").on_pressed([] {
                    mDoAud_seStartMenu(kSoundClick);
                    dusk::OpenDataFolder();
                }),
                rightPane, [](Pane& pane) {
                    pane.add_text(
                        "Open the folder where Dusk stores settings, saves, logs, texture "
                        "replacements, and other app data.");
                });

            auto& currentPath = getSettings().backend.customDataPath;
            leftPane.register_control(
                currentPath.getValue().empty()
                    ? leftPane.add_button("Set Custom Data Path")
                    : leftPane.add_button("Change Custom Data Path"),
                rightPane, [](Pane& pane) {
                    pane.clear();
                    pane.add_rml(fmt::format("<span class=\"data-folder-current\"><b>Current path:</b><br/>{}</span>",
                        dusk::io::fs_path_to_string(dusk::ConfigPath)));
                    pane.add_text(
                        "Set a custom folder for saves, settings, and mods. "
                        "Useful for syncing with cloud services like Syncthing. "
                        "Changes take effect after restart.");
                });
#endif
        leftPane.register_control(
            leftPane.add_select_button({
                .key = "Notifications",
                .getValue = [] {
                    const bool ach = getSettings().game.enableAchievementToasts.getValue();
                    const bool ctl = getSettings().game.enableControllerToasts.getValue();
                    if (!ach && !ctl) {
                        return Rml::String{"Off"};
                    }
                    if (ach && ctl) {
                        return Rml::String{"All"};
                    }
                    return Rml::String{"Some"};
                },
                .isModified = [] {
                    const auto& ach = getSettings().game.enableAchievementToasts;
                    const auto& ctl = getSettings().game.enableControllerToasts;
                    return ach.getValue() != ach.getDefaultValue() || ctl.getValue() != ctl.getDefaultValue();
                },
            }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_button("Select All").on_pressed([] {
                    mDoAud_seStartMenu(kSoundItemChange);
                    getSettings().game.enableAchievementToasts.setValue(true);
                    getSettings().game.enableControllerToasts.setValue(true);
                    config::Save();
                });
                pane.add_button("Select None").on_pressed([] {
                    mDoAud_seStartMenu(kSoundItemChange);
                    getSettings().game.enableAchievementToasts.setValue(false);
                    getSettings().game.enableControllerToasts.setValue(false);
                    config::Save();
                });

                pane.add_section("Types");
                pane.add_button(
                    {
                        .text = "Achievements",
                        .isSelected =
                        [] {
                            return getSettings().game.enableAchievementToasts.getValue();
                        },
                    })
                    .on_pressed([] {
                        mDoAud_seStartMenu(kSoundItemChange);
                        auto& v = getSettings().game.enableAchievementToasts;
                        v.setValue(!v.getValue());
                        config::Save();
                    });
                pane.add_button(
                    {
                        .text = "Missing Device",
                        .isSelected =
                            [] { return getSettings().game.enableControllerToasts.getValue(); },
                    })
                    .on_pressed([] {
                        mDoAud_seStartMenu(kSoundItemChange);
                        auto& v = getSettings().game.enableControllerToasts;
                        v.setValue(!v.getValue());
                        config::Save();
                    });
                pane.add_rml("<br/>Choose which notifications can be displayed.");
            });
#if DUSK_ENABLE_SENTRY_NATIVE
        auto& crashReporting = leftPane.add_child<BoolButton>(BoolButton::Props{
            .key = "Crash Reporting",
            .getValue =
                [] { return crash_reporting::get_consent() == crash_reporting::Consent::Given; },
            .setValue = [](bool enabled) { crash_reporting::set_consent(enabled); },
            .isDisabled =
                [] {
                    return crash_reporting::get_consent() == crash_reporting::Consent::Unavailable;
                },
            .isModified = [] { return false; },
        });
        leftPane.register_control(crashReporting, rightPane, [](Pane& pane) {
            pane.clear();
            pane.add_rml("Dusk can automatically send crash reports to the developers. Crash "
                         "reports contain the following:<br/>• Operating system version<br/>• CPU "
                         "architecture<br/>• GPU model & driver version<br/>• File paths (may "
                         "include account username)<br/>• Stack trace");
        });
#endif
        config_bool_select(leftPane, rightPane, getSettings().backend.skipPreLaunchUI,
            {
                .key = "Skip Dusk Main Menu",
                .helpText = "When starting Dusk, skip the main menu and boot straight into the "
                            "game if a disc image is available.",
            });
        config_bool_select(leftPane, rightPane, getSettings().backend.showPipelineCompilation,
            {
                .key = "Show Pipeline Compilation",
                .helpText = "Show an overlay when shaders are being compiled for your hardware.",
            });

        if constexpr (dusk::platform::SupportsDiscordRichPresence) {
            config_bool_select(leftPane, rightPane, getSettings().backend.discordEnabled,
                {
                    .key = "Discord Rich Presence",
                    .helpText = "Show the current game status on your Discord profile. Requires a restart to take effect.",
                    .onChange = [](bool) { config::Save(); },
                    .isDisabled = [] { return IsMobile; },
                });
        }
        if constexpr (dusk::platform::SupportsPortableDataPath) {
            config_bool_select(leftPane, rightPane, getSettings().backend.portableMode,
                {
                    .key = "Portable Mode",
                    .helpText = "Store all config, saves, and mods in a 'portable/' folder next to the Dusk executable instead of the system config directory. Requires a restart to take effect.",
                    .onChange = [](bool) { config::Save(); },
                });
        }
        if constexpr (dusk::platform::SupportsUpdateChecker) {
            config_bool_select(leftPane, rightPane, getSettings().backend.checkForUpdates,
                {
                    .key = "Check for Updates",
                    .helpText = "Checks GitHub releases for a new Dusk version on startup.<br/><br/>"
                                "No personal information is transmitted or collected.",
                });
        }
        if constexpr (dusk::platform::SupportsFocusLossPause) {
            config_bool_select(leftPane, rightPane, getSettings().game.pauseOnFocusLost,
                {
                    .key = "Pause On Focus Lost",
                    .helpText = "Pause the game when window focus is lost.",
                    .onChange = [](bool value) { aurora_set_pause_on_focus_lost(value); },
                });
        }
        config_bool_select(leftPane, rightPane, getSettings().backend.enableAdvancedSettings,
            {
                .key = "Enable Advanced Settings",
                .icon = "warning",
                .helpText = "Show advanced settings and debugging tools with "
                            "Shift+F1.<br/><br/><icon class=\"warning\"/> WARNING: Debugging tools "
                            "can easily break your game. Do not use on a regular save!",
                .onChange =
                    [](bool) {
                        for (auto& doc : get_document_stack()) {
                            if (dynamic_cast<MenuBar*>(doc.get())) {
                                doc = std::make_unique<MenuBar>();
                                break;
                            }
                        }
                    },
            });

        leftPane.add_section("Game", true);
        config_bool_select(leftPane, rightPane, getSettings().game.hideTvSettingsScreen,
            {
                .key = "Skip TV Settings Screen",
                .helpText = "Skips the TV calibration screen shown when loading a save.",
            });
        config_bool_select(leftPane, rightPane, getSettings().game.recordingMode,
            {
                .key = "Recording Mode",
                .helpText = "Disables the game HUD and all background music.<br/><br/>Useful for "
                            "recording footage.",
            });
        config_bool_select(leftPane, rightPane, getSettings().game.enableSaveStates,
            {
                .key = "Save States",
                .helpText = "Enable quick save/load via 3-finger tap or controller Select button.<br/><br/>"
                            "F1-F4 to load, Ctrl+F1-F4 to save on desktop.",
            });
    });

    add_tab("Save States", [this](Rml::Element* content) {
        push(std::make_unique<SaveStatesWindow>());
    });
}

void SettingsWindow::update() {
    if (mPrelaunch && top_document() == this) {
        try_push_verification_modal(*this);
    }

    Window::update();
}

}  // namespace dusk::ui
