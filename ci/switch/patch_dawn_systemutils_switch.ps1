param(
    [string]$BuildRoot = "build\switch-libnx-relwithdebinfo"
)

$TargetPath = "$BuildRoot\_deps\dawn-src\src\dawn\common\SystemUtils.cpp"

if (-not (Test-Path $TargetPath)) {
    Write-Host "Dawn SystemUtils.cpp not found at $TargetPath; skipping Switch system-utils patch"
    exit 0
}

$text = Get-Content $TargetPath -Raw
if ($text -match '#if defined\(__SWITCH__\)') {
    Write-Host "Dawn SystemUtils.cpp already has a Switch stub: $TargetPath"
    exit 0
}

$stub = @'
#if defined(__SWITCH__)
#include "dawn/common/SystemUtils.h"

#include <optional>
#include <string>
#include <utility>

namespace dawn {

const char* GetPathSeparator() {
    return "/";
}

std::pair<std::string, bool> GetEnvironmentVar(const char* variableName) {
    (void)variableName;
    return std::make_pair(std::string(), false);
}

bool SetEnvironmentVar(const char* variableName, const char* value) {
    (void)variableName;
    (void)value;
    return false;
}

std::optional<std::string> GetExecutablePath() {
    return {};
}

std::optional<std::string> GetExecutableDirectory() {
    return {};
}

std::optional<std::string> GetModulePath() {
    return {};
}

std::optional<std::string> GetModuleDirectory() {
    return {};
}

ScopedEnvironmentVar::ScopedEnvironmentVar(const char* variableName, const char* value)
    : mName(variableName), mOriginalValue{}, mIsSet(false) {
    (void)value;
}

ScopedEnvironmentVar::~ScopedEnvironmentVar() = default;

bool ScopedEnvironmentVar::Set(const char* variableName, const char* value) {
    (void)variableName;
    (void)value;
    return false;
}

}  // namespace dawn

#else
'@

$footer = "`r`n#endif  // !defined(__SWITCH__)`r`n"
$patched = $stub + $text + $footer
Set-Content -Path $TargetPath -Value $patched -NoNewline
Write-Host "Dawn SystemUtils.cpp stubbed for Switch: $TargetPath"
