param(
    [string]$BuildRoot = "build\switch-libnx-relwithdebinfo"
)

$TargetPath = "$BuildRoot\_deps\dawn-src\src\dawn\common\DynamicLib.cpp"

if (-not (Test-Path $TargetPath)) {
    Write-Host "Dawn DynamicLib.cpp not found at $TargetPath; skipping Switch dynamic-lib patch"
    exit 0
}

$text = Get-Content $TargetPath -Raw
if ($text -match '#if DAWN_PLATFORM_IS\(SWITCH\)') {
    Write-Host "Dawn DynamicLib.cpp already has a Switch stub: $TargetPath"
    exit 0
}

$stub = @'
#if defined(__SWITCH__)
#include "dawn/common/DynamicLib.h"

#include <span>
#include <utility>

namespace dawn {

DynamicLib::~DynamicLib() = default;

DynamicLib::DynamicLib(DynamicLib&& other) {
    std::swap(mHandle, other.mHandle);
    mNeedsClose = other.mNeedsClose;
}

DynamicLib& DynamicLib::operator=(DynamicLib&& other) {
    std::swap(mHandle, other.mHandle);
    mNeedsClose = other.mNeedsClose;
    return *this;
}

bool DynamicLib::Valid() const {
    return mHandle != nullptr;
}

bool DynamicLib::Open(const std::string&, std::string* error) {
    if (error != nullptr) {
        *error = "Dynamic library loading is unsupported on Switch";
    }
    return false;
}

bool DynamicLib::OpenLoaded(const std::string&, std::string* error) {
    if (error != nullptr) {
        *error = "Dynamic library loading is unsupported on Switch";
    }
    return false;
}

bool DynamicLib::Open(const std::string&,
                      std::span<const std::string>,
                      std::string* error) {
    if (error != nullptr) {
        *error = "Dynamic library loading is unsupported on Switch";
    }
    return false;
}

void DynamicLib::Close() {
    mNeedsClose = false;
    mHandle = nullptr;
}

void* DynamicLib::GetProc(const std::string&, std::string* error) const {
    if (error != nullptr) {
        *error = "Dynamic library loading is unsupported on Switch";
    }
    return nullptr;
}

}  // namespace dawn

#else
'@

$footer = "`r`n#endif  // !defined(__SWITCH__)`r`n"
$patched = $stub + $text + $footer
Set-Content -Path $TargetPath -Value $patched -NoNewline
Write-Host "Dawn DynamicLib.cpp stubbed for Switch: $TargetPath"
