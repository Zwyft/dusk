#include "dusk/save_import.hpp"

#include "dusk/io.hpp"
#include "dusk/logging.h"
#include "dusk/main.h"

#include <SDL3/SDL.h>

#include <cstdlib>
#include <system_error>

namespace dusk::save_import {
namespace {

std::filesystem::path home_dir() {
#ifdef _WIN32
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile && *userProfile) {
        return std::filesystem::path(userProfile);
    }
#else
    const char* home = std::getenv("HOME");
    if (home && *home) {
        return std::filesystem::path(home);
    }
#endif
    return {};
}

} // namespace

std::filesystem::path saves_dir() {
    return dusk::ConfigPath / "saves";
}

std::filesystem::path detect_dolphin_saves() {
#if defined(__ANDROID__) || (defined(TARGET_OS_IOS) && TARGET_OS_IOS)
    return {};
#elif defined(_WIN32)
    auto home = home_dir();
    if (home.empty()) return {};
    auto path = home / "Documents" / "Dolphin Emulator" / "GC" / "USA" / "Card A";
    std::error_code ec;
    if (std::filesystem::exists(path, ec)) return path;
    return {};
#else
    auto home = home_dir();
    if (home.empty()) return {};

    const std::filesystem::path candidates[] = {
        home / ".local" / "share" / "dolphin-emu" / "GC" / "USA" / "Card A",
        home / "Library" / "Application Support" / "Dolphin" / "GC" / "USA" / "Card A",
    };

    std::error_code ec;
    for (const auto& p : candidates) {
        if (std::filesystem::exists(p, ec)) return p;
    }
    return {};
#endif
}

bool can_open_saves_dir() {
#if defined(TARGET_OS_IOS) && TARGET_OS_IOS
    return true;
#else
    return false;
#endif
}

void open_saves_dir() {
    auto dir = saves_dir();
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec)) {
        std::filesystem::create_directories(dir, ec);
    }

    auto url = std::string("file://") + io::fs_path_to_string(dir);
    if (!SDL_OpenURL(url.c_str())) {
        DuskLog.warn("Failed to open saves folder '{}': {}", io::fs_path_to_string(dir), SDL_GetError());
    }
}

bool import_from_dolphin() {
    auto src = detect_dolphin_saves();
    if (src.empty()) {
        DuskLog.warn("No Dolphin save directory detected.");
        return false;
    }

    auto dst = saves_dir();
    std::error_code ec;
    std::filesystem::create_directories(dst, ec);

    bool copiedAny = false;
    for (const auto& entry : std::filesystem::directory_iterator(src, ec)) {
        if (ec) break;
        if (!entry.is_regular_file(ec)) continue;

        const auto outFile = dst / entry.path().filename();
        std::filesystem::copy_file(entry.path(), outFile,
            std::filesystem::copy_options::overwrite_existing, ec);
        if (!ec) copiedAny = true;
    }

    if (!copiedAny) {
        DuskLog.warn("No Dolphin save files were imported from '{}'", io::fs_path_to_string(src));
    }
    return copiedAny;
}

} // namespace dusk::save_import
