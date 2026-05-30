param(
    [string]$BuildRoot = "build\switch-libnx-relwithdebinfo"
)

$Targets = @(
    "$BuildRoot\_deps\dawn-src\third_party\abseil-cpp\absl\debugging\internal\elf_mem_image.h",
    "$BuildRoot\_deps\dawn-src\third_party\abseil-cpp\absl\debugging\internal\elf_mem_image.cc",
    "$BuildRoot\_deps\dawn-src\third_party\abseil-cpp\absl\debugging\internal\vdso_support.cc"
)

function Patch-StubFile {
    param([string]$Path)

    if (-not (Test-Path $Path)) {
        return $false
    }

    $text = Get-Content $Path -Raw
    if ($text -match '#if defined\(__SWITCH__\)') {
        Write-Host "Dawn Abseil ELF debug file already stubbed: $Path"
        return $true
    }

    $replacement = @"
#if defined(__SWITCH__)
// Switch/libnx does not provide the ELF/vDSO debugging APIs used by Abseil's
// host-side diagnostics helpers. Stub these translation units out entirely so
// the build can proceed on Switch.
#else
$text
#endif  // !defined(__SWITCH__)
"@

    Set-Content -Path $Path -Value $replacement -NoNewline
    Write-Host "Dawn Abseil ELF debug file stubbed: $Path"
    return $true
}

$patchedAny = $false
foreach ($path in $Targets) {
    if (Patch-StubFile -Path $path) {
        $patchedAny = $true
    }
}

if (-not $patchedAny) {
    Write-Host "Dawn Abseil ELF debug files not found at expected Switch build paths; skipping"
    exit 0
}
