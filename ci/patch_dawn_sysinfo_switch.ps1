param(
    [string]$SysinfoFile = "build\switch-libnx-relwithdebinfo\_deps\dawn-src\third_party\abseil-cpp\absl\base\internal\sysinfo.cc"
)

function Patch-SysinfoFile {
    param([string]$Path)

    if (-not (Test-Path $Path)) {
        return $false
    }

    $text = Get-Content $Path -Raw
    if ($text -match 'reinterpret_cast<intptr_t>\(pthread_self\(\)\)') {
        Write-Host "Dawn Abseil sysinfo.cc already patched: $Path"
        return $true
    }

    $patched = $text.Replace(
        'static_cast<pid_t>(pthread_self())',
        'static_cast<pid_t>(reinterpret_cast<intptr_t>(pthread_self()))')
    if ($patched -eq $text) {
        $patched = $text.Replace(
            'static_cast<pid_t> (pthread_self())',
            'static_cast<pid_t>(reinterpret_cast<intptr_t>(pthread_self()))')
    }
    if ($patched -eq $text) {
        $patched = $text.Replace(
            'static_cast< pid_t >(pthread_self())',
            'static_cast<pid_t>(reinterpret_cast<intptr_t>(pthread_self()))')
    }
    if ($patched -eq $text) {
        throw "Failed to patch Dawn Abseil sysinfo.cc for Switch: $Path"
    }

    Set-Content -Path $Path -Value $patched -NoNewline
    Write-Host "Dawn Abseil sysinfo.cc patched: $Path"
    return $true
}

$patchedAny = $false
$candidatePaths = @(
    $SysinfoFile,
    "build\switch-libnx-relwithdebinfo\_deps\dawn-build\third_party\abseil\absl\base\internal\sysinfo.cc"
)

foreach ($candidate in $candidatePaths) {
    if (Patch-SysinfoFile -Path $candidate) {
        $patchedAny = $true
    }
}

if (-not $patchedAny) {
    throw "Dawn Abseil sysinfo.cc not found at any known Switch build path"
}
