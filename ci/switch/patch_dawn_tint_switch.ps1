param(
    [string]$BuildRoot = "build\switch-libnx-relwithdebinfo"
)

$TargetPaths = @(
    "$BuildRoot\_deps\dawn-src\src\tint\utils\text_generator\text_generator.cc"
)

function Patch-File {
    param([string]$Path)

    if (-not (Test-Path $Path)) {
        return $false
    }

    $text = Get-Content $Path -Raw
    if ($text -match 'static_cast<unsigned char>\(c\) > 0x7f') {
        Write-Host "Dawn Tint text_generator already patched: $Path"
        return $true
    }

    $updated = [regex]::Replace(
        $text,
        '!isascii\(c\)',
        'static_cast<unsigned char>(c) > 0x7f',
        1)

    if ($updated -eq $text) {
        throw "Could not find Dawn Tint isascii check in $Path"
    }

    Set-Content -Path $Path -Value $updated -NoNewline
    Write-Host "Dawn Tint text_generator patched: $Path"
    return $true
}

$patchedAny = $false
foreach ($path in $TargetPaths) {
    if (Patch-File -Path $path) {
        $patchedAny = $true
    }
}

if (-not $patchedAny) {
    Write-Host "Dawn Tint text_generator not found at expected Switch build path; skipping"
    exit 0
}
