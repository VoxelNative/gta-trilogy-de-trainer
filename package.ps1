# Builds the release zip: dist\TrilogyTrainer-v<Version>.zip
# Usage: powershell -ExecutionPolicy Bypass -File package.ps1 -Version 1.1
param([Parameter(Mandatory = $true)][string]$Version)
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

cmd /c "`"$PSScriptRoot\build.bat`""
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

$stage = "dist\stage"
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
$games = [ordered]@{ "GTA III" = "III"; "GTA Vice City" = "VC"; "GTA San Andreas" = "SA" }
foreach ($name in $games.Keys) {
    $dir = New-Item -ItemType Directory -Force "$stage\$name"
    Copy-Item "build\version.dll" $dir
    Copy-Item "build\TrilogyTrainer.$($games[$name]).asi" $dir
}

@"
GTA Trilogy DE Trainer v$Version
For GTA III, Vice City and San Andreas - The Definitive Edition, game version 1.112.

INSTALL
Copy everything inside each game's folder in this zip into that game's
  Gameface\Binaries\Win64
folder (next to LibertyCity.exe / ViceCity.exe / SanAndreas.exe).
Start the game, load a save and press F11.

KEYS
F11  open / close the menu (arrows or numpad to move, Enter to select, Backspace to go back)
F5   teleport to the map waypoint
F6   no-clip on / off (WASD, Space / Ctrl up / down, Shift faster)
Num + / Num - / Num *   vehicle boost / stop / jump
Change keys in TrilogyTrainer.<game>.ini, created in the same folder on first run.

UNINSTALL
Delete version.dll, TrilogyTrainer.*.asi and the trainer's .ini / .log files.

More: https://github.com/VoxelNative/gta-trilogy-de-trainer
"@ | Set-Content -Encoding utf8 "$stage\README.txt"

Copy-Item LICENSE "$stage\LICENSE.txt"
$notices = "This trainer includes the following third-party software.`r`n`r`n" +
    "=== Dear ImGui (https://github.com/ocornut/imgui) ===`r`n" + (Get-Content -Raw third_party\imgui\LICENSE.txt) +
    "`r`n=== MinHook (https://github.com/TsudaKageyu/minhook) ===`r`n" + (Get-Content -Raw third_party\minhook\LICENSE.txt)
$notices | Set-Content -Encoding utf8 "$stage\THIRD-PARTY-NOTICES.txt"

$zip = "dist\TrilogyTrainer-v$Version.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path "$stage\*" -DestinationPath $zip
Remove-Item $stage -Recurse -Force
Write-Host "Created $zip"
