#ifndef DUSK_PLATFORM_SUPPORT_HPP
#define DUSK_PLATFORM_SUPPORT_HPP

#include <filesystem>
#include <optional>
#include <string_view>

namespace dusk::platform {

inline constexpr bool IsSwitchTarget =
#if TARGET_SWITCH
    true;
#else
    false;
#endif

inline constexpr bool SupportsDiscordRichPresence = !IsSwitchTarget;
inline constexpr bool SupportsUpdateChecker = !IsSwitchTarget;
inline constexpr bool SupportsPortableDataPath = !IsSwitchTarget;
inline constexpr bool SupportsCustomDataPath = !IsSwitchTarget;
inline constexpr bool SupportsExternalCrashReporting = !IsSwitchTarget;
inline constexpr bool SupportsWindowStatePersistence = !IsSwitchTarget;
inline constexpr bool SupportsWindowSafeAreaQuery = !IsSwitchTarget;
inline constexpr bool SupportsExternalGamepadMappings = !IsSwitchTarget;
inline constexpr bool SupportsBackgroundInputOption = !IsSwitchTarget;
inline constexpr bool SupportsFocusLossPause = !IsSwitchTarget;

std::filesystem::path CalculateConfigPath(
    std::string_view cliConfigPath, std::string_view customDataPath, bool portableModeRequested);

std::filesystem::path DefaultSwitchDataRoot();
std::filesystem::path DefaultSwitchConfigRoot();
std::filesystem::path DefaultSwitchDiscRoot();
std::optional<std::filesystem::path> FindDefaultSwitchDiscPath();
std::filesystem::path BundledPath(const std::filesystem::path& relativePath);
std::filesystem::path BundledResourcePath(std::string_view assetName);
std::optional<std::filesystem::path> FindBundledPath(
    const std::filesystem::path& relativePath, bool includeBasePathFallback = true);
bool OpenExternalUrl(std::string_view url);

}  // namespace dusk::platform

#endif  // DUSK_PLATFORM_SUPPORT_HPP
