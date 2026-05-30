param(
    [string]$TimeZoneLibc = "build\switch-libnx-relwithdebinfo\_deps\dawn-src\third_party\abseil-cpp\absl\time\internal\cctz\src\time_zone_libc.cc"
)

function Patch-TimeZoneFile {
    param([string]$Path)

    if (-not (Test-Path $Path)) {
        return $false
    }

    $text = Get-Content $Path -Raw
    if ($text -match '#elif defined\(__SWITCH__\)') {
        Write-Host "Dawn Abseil time_zone_libc already patched: $Path"
        return $true
    }

    $anchor = @"
#elif defined(__VXWORKS__)
// Uses the globals: 'timezone' and 'tzname'.
auto tm_gmtoff(const std::tm& tm) -> decltype(timezone + 0) {
  const bool is_dst = tm.tm_isdst > 0;
  return timezone + (is_dst ? 60 * 60 : 0);
}
auto tm_zone(const std::tm& tm) -> decltype(tzname[0]) {
  const bool is_dst = tm.tm_isdst > 0;
  return tzname[is_dst];
}
#else
"@

    $insert = @"
#elif defined(__VXWORKS__)
// Uses the globals: 'timezone' and 'tzname'.
auto tm_gmtoff(const std::tm& tm) -> decltype(timezone + 0) {
  const bool is_dst = tm.tm_isdst > 0;
  return timezone + (is_dst ? 60 * 60 : 0);
}
auto tm_zone(const std::tm& tm) -> decltype(tzname[0]) {
  const bool is_dst = tm.tm_isdst > 0;
  return tzname[is_dst];
}
#elif defined(__SWITCH__)
// libnx/newlib does not expose the nonstandard tm fields that CCTZ probes on
// other platforms. Use a fixed UTC fallback so the Switch build remains
// portable.
auto tm_gmtoff(const std::tm&) -> std::time_t {
  return 0;
}
auto tm_zone(const std::tm&) -> const char* {
  return "UTC";
}
#else
"@

    if (-not $text.Contains($anchor)) {
        throw "Could not find Dawn time zone helper anchor in $Path"
    }

    $patched = $text.Replace($anchor, $insert)
    if ($patched -eq $text) {
        throw "Failed to patch Dawn Abseil time_zone_libc for Switch: $Path"
    }

    Set-Content -Path $Path -Value $patched -NoNewline
    Write-Host "Dawn Abseil time_zone_libc patched: $Path"
    return $true
}

$patchedAny = $false
$candidatePaths = @(
    $TimeZoneLibc,
    "build\switch-libnx-relwithdebinfo\_deps\dawn-build\third_party\abseil\absl\time\internal\cctz\src\time_zone_libc.cc"
)

foreach ($candidate in $candidatePaths) {
    if (Patch-TimeZoneFile -Path $candidate) {
        $patchedAny = $true
    }
}

if (-not $patchedAny) {
    throw "Dawn Abseil time_zone_libc not found at any known Switch build path"
}
