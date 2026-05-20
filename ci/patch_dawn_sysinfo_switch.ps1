param(
    [string]$SysinfoFile = "build\switch-libnx-relwithdebinfo\_deps\dawn-src\third_party\abseil-cpp\absl\base\internal\sysinfo.cc"
)

if (-not (Test-Path $SysinfoFile)) {
    throw "Dawn Abseil sysinfo.cc not found at $SysinfoFile"
}

$text = Get-Content $SysinfoFile -Raw
if ($text -match 'reinterpret_cast<intptr_t>\(pthread_self\(\)\)') {
    Write-Host "Dawn Abseil sysinfo.cc already patched"
    exit 0
}

$pattern = 'static_cast<\s*pid_t\s*>\s*\(\s*pthread_self\s*\(\s*\)\s*\)'
$replacement = 'static_cast<pid_t>(reinterpret_cast<intptr_t>(pthread_self()))'
$patched = [regex]::Replace($text, $pattern, $replacement, 1)
if ($patched -eq $text) {
    throw "Failed to patch Dawn Abseil sysinfo.cc for Switch"
}

Set-Content -Path $SysinfoFile -Value $patched -NoNewline
