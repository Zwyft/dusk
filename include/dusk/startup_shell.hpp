#ifndef DUSK_STARTUP_SHELL_HPP
#define DUSK_STARTUP_SHELL_HPP

#include <aurora/aurora.h>

#include <filesystem>
#include <string>

namespace dusk {

struct AuroraStartupConfig {
    std::u8string userPathStorage;
    AuroraConfig config{};
};

AuroraStartupConfig BuildAuroraStartupConfig(
    const std::filesystem::path& configPath,
    AuroraBackend desiredBackend,
    AuroraLogLevel logLevel,
    AuroraImGuiInitCallback imGuiInitCallback);

void PrepareStartupShell();
void ApplyRuntimePresentationPolicy(AuroraBackend activeBackend);
void ShutdownFrontendDiagnostics();
void ShutdownFrontendShell(bool includeAuroraShutdown, bool includeDiagnostics = true);

}  // namespace dusk

#endif  // DUSK_STARTUP_SHELL_HPP
