param(
    [string]$TimeZoneLibc = "build\switch-libnx-relwithdebinfo\_deps\dawn-src\third_party\abseil-cpp\absl\time\internal\cctz\src\time_zone_libc.cc"
)

if (-not (Test-Path $TimeZoneLibc)) {
    throw "Dawn Abseil time_zone_libc not found at $TimeZoneLibc"
}

$text = Get-Content $TimeZoneLibc -Raw
if ($text -match '#elif defined\(__SWITCH__\)') {
    Write-Host "Dawn Abseil time_zone_libc already patched"
    exit 0
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
// Uses the globals: 'timezone' and 'tzname' from newlib/libnx.
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

if (-not $text.Contains($anchor)) {
    throw "Could not find Dawn time zone helper anchor in $TimeZoneLibc"
}

$patched = $text.Replace($anchor, $insert)
if ($patched -eq $text) {
    throw "Failed to patch Dawn Abseil time_zone_libc for Switch"
}

Set-Content -Path $TimeZoneLibc -Value $patched -NoNewline
