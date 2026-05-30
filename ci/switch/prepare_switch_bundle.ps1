param(
    [string]$BuildRoot = "build\switch-libnx-relwithdebinfo",
    [string]$SourceRoot = "."
)

$BundleRoot = Join-Path $BuildRoot "switch-package"
$RomfsRoot = Join-Path $BundleRoot "romfs"
$RomfsResRoot = Join-Path $RomfsRoot "res"
$ResRoot = Join-Path $SourceRoot "res"

New-Item -ItemType Directory -Force -Path $RomfsResRoot | Out-Null

if (Test-Path $ResRoot) {
    Copy-Item -Path (Join-Path $ResRoot '*') -Destination $RomfsResRoot -Recurse -Force
    Write-Host "Staged Switch romfs resources from $ResRoot to $RomfsResRoot"
} else {
    Write-Host "No res directory found at $ResRoot; skipping Switch bundle resource staging"
}

$ReadmePath = Join-Path $BundleRoot "README.txt"
$Readme = @"
Dusklight Switch bundle

Expected on-device layout:
  sdmc:/aurora
  sdmc:/dusk
  romfs:/res

This bundle stages the runtime resources used by the experimental Switch build.
"@
Set-Content -Path $ReadmePath -Value $Readme -NoNewline
Write-Host "Wrote Switch bundle README: $ReadmePath"
