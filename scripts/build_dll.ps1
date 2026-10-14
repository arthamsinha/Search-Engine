$ErrorActionPreference = "Stop"
Set-Location (Split-Path -Parent $PSScriptRoot)
$env:Path += ";C:\msys64\ucrt64\bin"
New-Item -ItemType Directory -Force build | Out-Null

g++ -shared -std=c++20 -Ibackend/include backend/src/devfinder/common/*.cpp backend/src/devfinder/crawler/*.cpp backend/src/devfinder/preprocessing/*.cpp backend/src/devfinder/index/*.cpp backend/src/devfinder/ranking/*.cpp backend/src/devfinder/query/*.cpp backend/src/devfinder/autocomplete/*.cpp backend/src/devfinder/server/*.cpp backend/cli/main_cli.cpp -o build/devfinder.dll -lcurl -lws2_32 -lwsock32
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED"; exit 1 }
Write-Host "BUILD OK"
