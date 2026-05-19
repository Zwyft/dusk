#include "dusk/startup_shell.hpp"

#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_misc.h>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <algorithm>

#include "d/d_com_inf_game.h"
#include "dusk/app_info.hpp"
#include "dusk/audio/DuskAudioSystem.h"
#include "dusk/config.hpp"
#include "dusk/crash_reporting.h"
#include "dusk/discord_presence.hpp"
#include "dusk/imgui/ImGuiConsole.hpp"
#include "dusk/imgui/ImGuiEngine.hpp"
#include "dusk/logging.h"
#include "dusk/main.h"
#include "dusk/mod_manager.hpp"
#include "dusk/platform_support.hpp"
#include "dusk/settings.h"
#include "dusk/ui/ui.hpp"
#include "fmt/format.h"
#include "m_Do/m_Do_machine.h"
#include "version.h"
#include <aurora/aurora.h>
#include <dolphin/vi.h>

namespace dusk {
namespace {

void AutoBackupSavesOnLaunch() {
    if (!getSettings().game.autoBackupSaves.getValue()) {
        return;
    }

    auto backupDir = ConfigPath / "saves" / "backups";
    std::error_code ec;
    if (!std::filesystem::exists(ConfigPath / "saves", ec)) {
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << "backup_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    auto dest = backupDir / oss.str();
    std::filesystem::create_directories(dest, ec);
    for (const auto& entry : std::filesystem::directory_iterator(ConfigPath / "saves", ec)) {
        if (ec) {
            break;
        }
        if (entry.is_regular_file()) {
            std::filesystem::copy_file(
                entry.path(),
                dest / entry.path().filename(),
                std::filesystem::copy_options::skip_existing,
                ec);
        }
    }
}

}  // namespace

AuroraStartupConfig BuildAuroraStartupConfig(
    const std::filesystem::path& configPath,
    AuroraBackend desiredBackend,
    AuroraLogLevel logLevel,
    AuroraImGuiInitCallback imGuiInitCallback) {
    AuroraStartupConfig startup{};
    startup.userPathStorage = configPath.u8string();

    startup.config.appName = AppName;
    startup.config.userPath =
        reinterpret_cast<const char*>(startup.userPathStorage.c_str());
    startup.config.vsync = getSettings().video.enableVsync;
    startup.config.startFullscreen = getSettings().video.enableFullscreen;
    startup.config.windowPosX = getSettings().video.windowPositionX;
    startup.config.windowPosY = getSettings().video.windowPositionY;
    startup.config.windowWidth = getSettings().video.windowWidth;
    startup.config.windowHeight = getSettings().video.windowHeight;
    startup.config.desiredBackend = desiredBackend;
    startup.config.logCallback = &aurora_log_callback;
    startup.config.logLevel = logLevel;
    startup.config.mem1Size = 256 * 1024 * 1024;
    startup.config.mem2Size = 24 * 1024 * 1024;
    startup.config.allowJoystickBackgroundEvents = getSettings().game.allowBackgroundInput;
    startup.config.pauseOnFocusLost =
        platform::SupportsFocusLossPause && getSettings().game.pauseOnFocusLost;
    startup.config.imGuiInitCallback = imGuiInitCallback;
    startup.config.maxTextureAnisotropy =
        static_cast<uint16_t>(getSettings().game.anisotropicFiltering.getValue());
    startup.config.allowTextureReplacements = getSettings().game.enableTextureReplacements;
    startup.config.allowTextureDumps = false;

    return startup;
}

void PrepareStartupShell() {
    SDL_SetAppMetadata("Dusklight", DUSK_VERSION_STRING, "dev.twilitrealm.dusk");

    mod_manager::initialize();

    if (IsMobile) {
        SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    }

    AutoBackupSavesOnLaunch();
}

void ApplyRuntimePresentationPolicy(AuroraBackend activeBackend) {
    VISetWindowTitle(
        fmt::format("Dusk {} [{}]", DUSK_WC_DESCRIBE, dusk::backend_name(activeBackend))
            .c_str());

    if (getSettings().video.lockAspectRatio) {
        AuroraSetViewportPolicy(AURORA_VIEWPORT_FIT);
    } else {
        AuroraSetViewportPolicy(AURORA_VIEWPORT_STRETCH);
    }

    VISetFrameBufferScale(getSettings().game.internalResolutionScale.getValue());
    aurora_set_resampler(
        getSettings().game.resampler.getValue() == dusk::Resampler::Area ? SAMPLER_AREA
                                                                         : SAMPLER_BILINEAR);
    dComIfG_setBrightness(static_cast<u8>(
        (std::clamp(getSettings().game.displayBrightness.getValue(), 25, 100) * 255) / 100));

    audio::SetMasterVolume(getSettings().audio.masterVolume / 100.0f);
    audio::SetEnableReverb(getSettings().audio.enableReverb);
    audio::EnableHrtf = getSettings().audio.enableHrtf;
}

void ShutdownFrontendDiagnostics() {
    crash_reporting::shutdown();
    ShutdownFileLogging();
    fflush(stdout);
    fflush(stderr);
}

void ShutdownFrontendShell(bool includeAuroraShutdown, bool includeDiagnostics) {
    if (includeDiagnostics) {
        ShutdownFrontendDiagnostics();
    }

#ifdef DUSK_DISCORD
    if constexpr (platform::SupportsDiscordRichPresence) {
        discord::shutdown();
    }
#endif

    ui::shutdown();
    if (includeAuroraShutdown) {
        aurora_shutdown();
    }
}

}  // namespace dusk
