param(
    [string]$BuildRoot = "build\switch-libnx-relwithdebinfo"
)

$TargetPath = "$BuildRoot\_deps\dawn-src\src\dawn\common\Platform.h"

if (-not (Test-Path $TargetPath)) {
    Write-Host "Dawn Platform.h not found at $TargetPath; skipping Switch platform patch"
    exit 0
}

$text = Get-Content $TargetPath -Raw
if ($text -match '#elif defined\(__SWITCH__\)' -and $text -notmatch '#else#endif') {
    Write-Host "Dawn Platform.h already has a Switch branch: $TargetPath"
    exit 0
}

if ($text -match '#else#endif') {
    $patched = $text.Replace('#else#endif', "#else`r`n#error `"Unsupported platform.`"`r`n#endif")
} else {
    $replacement = @"
#elif defined(__EMSCRIPTEN__)
#define DAWN_PLATFORM_IS_EMSCRIPTEN 1
#define DAWN_PLATFORM_IS_POSIX 1
#include <emscripten/emscripten.h>

#elif defined(__SWITCH__)
#define DAWN_PLATFORM_IS_SWITCH 1
#define DAWN_PLATFORM_IS_POSIX 1
#define DAWN_PLATFORM_IS_ARM 1
#define DAWN_PLATFORM_IS_ARM64 1

#else
#error "Unsupported platform."
#endif
"@

    $patched = [regex]::Replace(
        $text,
        '(?ms)#elif defined\(__EMSCRIPTEN__\)\s*#define DAWN_PLATFORM_IS_EMSCRIPTEN 1\s*#define DAWN_PLATFORM_IS_POSIX 1\s*#include <emscripten/emscripten\.h>\s*\n\s*#else\s*\n\s*#error "Unsupported platform\."\s*\n\s*#endif',
        $replacement,
        1)
}

if ($patched -eq $text) {
    throw "Could not find Dawn Platform.h platform branch in $TargetPath"
}

Set-Content -Path $TargetPath -Value $patched -NoNewline
Write-Host "Dawn Platform.h patched for Switch: $TargetPath"
