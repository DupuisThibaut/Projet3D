$GAME_PATH = $args[0]
$MODE1 = $args[1]
$MODE2 = $args[2]

if (!(Test-Path build)) {
    Write-Host "Creating build directory..."
    New-Item -ItemType Directory -Path build
}

Set-Location build
Write-Host "Running CMake..."
cmake .. -G "Visual Studio 17 2022" -A x64

Write-Host "Building project..."
cmake --build . --config Release

Set-Location ..
Write-Host "Running Engine..."
.\Engine.exe $GAME_PATH $MODE1 $MODE2