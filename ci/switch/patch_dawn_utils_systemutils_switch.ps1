param(
    [string]$BuildRoot = "build\switch-libnx-relwithdebinfo"
)

$TargetPath = "$BuildRoot\_deps\dawn-src\src\dawn\utils\SystemUtils.cpp"

if (-not (Test-Path $TargetPath)) {
    Write-Host "Dawn utils/SystemUtils.cpp not found at $TargetPath; skipping Switch utils patch"
    exit 0
}

$text = Get-Content $TargetPath -Raw
if ($text -match '#if defined\(__SWITCH__\)' -and $text -match 'svcSleepThread') {
    Write-Host "Dawn utils/SystemUtils.cpp already has a Switch sleep path: $TargetPath"
    exit 0
}

$patched = @'
#if defined(__SWITCH__)
#include "dawn/utils/SystemUtils.h"

#include <chrono>
#include <thread>

namespace utils {

void USleep(unsigned int usecs) {
    std::this_thread::sleep_for(std::chrono::microseconds(usecs));
}

}  // namespace utils

#else
'@ + $text + @'
#endif  // !defined(__SWITCH__)
'@

Set-Content -Path $TargetPath -Value $patched -NoNewline
Write-Host "Dawn utils/SystemUtils.cpp stubbed for Switch: $TargetPath"
