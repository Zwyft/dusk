#include "dusk/platform_support.hpp"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_misc.h>
#include <string>
#include <system_error>

#include "dusk/app_info.hpp"
#include "dusk/logging.h"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

namespace dusk::platform {
namespace {

void migrate_directory(const std::filesystem::path& from, const std::filesystem::path& to) {
    std::error_code ec;
    std::filesystem::create_directories(to, ec);
    if (ec) {
        return;
    }

    for (std::filesystem::recursive_directory_iterator it(
             from, std::filesystem::directory_options::skip_permission_denied, ec);
        it != std::filesystem::recursive_directory_iterator(); it.increment(ec))
    {
        if (ec) {
            return;
        }

        const auto relativePath = std::filesystem::relative(it->path(), from, ec);
        if (ec) {
            return;
        }

        const auto targetPath = to / relativePath;
        if (it->is_directory(ec)) {
            std::filesystem::create_directories(targetPath, ec);
            if (ec) {
                return;
            }
        } else if (it->is_regular_file(ec) && !std::filesystem::exists(targetPath, ec)) {
            std::filesystem::create_directories(targetPath.parent_path(), ec);
            if (ec) {
                return;
            }
            std::filesystem::copy_file(
                it->path(), targetPath, std::filesystem::copy_options::skip_existing, ec);
            if (ec) {
                return;
            }
        }
    }
}

std::filesystem::path portable_data_root() {
    std::error_code ec;
    const auto exeDir = std::filesystem::current_path(ec);
    if (ec) {
        return {};
    }
    return exeDir / "portable";
}

}  // namespace

std::filesystem::path DefaultSwitchDataRoot() {
    return "sdmc:/switch/Dusk";
}

std::filesystem::path BundledPath(const std::filesystem::path& relativePath) {
#if TARGET_SWITCH
    return std::filesystem::path("romfs:/") / relativePath;
#else
    const char* basePath = SDL_GetBasePath();
    if (basePath != nullptr && basePath[0] != '\0') {
        return std::filesystem::path(basePath) / relativePath;
    }
    return relativePath;
#endif
}

std::filesystem::path BundledResourcePath(std::string_view assetName) {
    return BundledPath(std::filesystem::path("res") / std::string(assetName));
}

std::optional<std::filesystem::path> FindBundledPath(
    const std::filesystem::path& relativePath, bool includeBasePathFallback) {
    const auto primaryPath = BundledPath(relativePath);
    std::error_code ec;
    if (std::filesystem::exists(primaryPath, ec)) {
        return primaryPath;
    }

#if !TARGET_SWITCH
    if (includeBasePathFallback) {
        const char* basePath = SDL_GetBasePath();
        if (basePath != nullptr && basePath[0] != '\0') {
            const auto fallbackPath = std::filesystem::path(basePath) / relativePath;
            ec.clear();
            if (std::filesystem::exists(fallbackPath, ec)) {
                return fallbackPath;
            }
        }
    }
#endif

    return std::nullopt;
}

std::filesystem::path CalculateConfigPath(
    std::string_view cliConfigPath, std::string_view customDataPath, bool portableModeRequested) {
    if (!cliConfigPath.empty()) {
        return std::filesystem::path(std::string(cliConfigPath));
    }

    if constexpr (SupportsCustomDataPath) {
        if (!customDataPath.empty()) {
            return std::filesystem::path(std::string(customDataPath));
        }
    }

#if TARGET_SWITCH
    return DefaultSwitchDataRoot();
#endif

    if constexpr (SupportsPortableDataPath) {
        std::error_code ec;
        const auto portablePath = portable_data_root();
        if (portableModeRequested || (!portablePath.empty() &&
                std::filesystem::exists(portablePath.parent_path() / "portable.txt", ec))) {
            std::filesystem::create_directories(portablePath, ec);
            return portablePath;
        }
    }

#if defined(__APPLE__) && TARGET_OS_IOS && !TARGET_OS_TV
    const char* documentsPath = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
    if (!documentsPath) {
        DuskLog.fatal("Unable to get iOS Documents path: {}", SDL_GetError());
    }

    std::filesystem::path configPath = reinterpret_cast<const char8_t*>(documentsPath);

    char* oldPrefPath = SDL_GetPrefPath(dusk::OrgName, dusk::AppName);
    if (oldPrefPath) {
        const std::filesystem::path oldConfigPath = reinterpret_cast<const char8_t*>(oldPrefPath);
        SDL_free(oldPrefPath);

        std::error_code ec;
        if (oldConfigPath != configPath && std::filesystem::exists(oldConfigPath, ec)) {
            migrate_directory(oldConfigPath, configPath);
        }
    }

    return configPath;
#else
    const char* result = SDL_GetPrefPath(dusk::OrgName, dusk::AppName);
    if (!result) {
        DuskLog.fatal("Unable to get PrefPath: {}", SDL_GetError());
    }

    return reinterpret_cast<const char8_t*>(result);
#endif
}

bool OpenExternalUrl(std::string_view url) {
#if TARGET_SWITCH
    static_cast<void>(url);
    return false;
#else
    return SDL_OpenURL(std::string(url).c_str());
#endif
}

}  // namespace dusk::platform
