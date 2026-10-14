[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
mkdir -p backend\include\nlohmann -ErrorAction SilentlyContinue
if (-not (Test-Path "backend\include\nlohmann\json.hpp")) {
    Invoke-WebRequest "https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp" -OutFile "backend\include\nlohmann\json.hpp"
}
if (-not (Test-Path "backend\include\httplib.h")) {
    Invoke-WebRequest "https://raw.githubusercontent.com/yhirose/cpp-httplib/v0.14.1/httplib.h" -OutFile "backend\include\httplib.h"
}

Write-Host "Building project via raw g++..."
$env:Path += ";C:\msys64\ucrt64\bin"
mkdir -p build -ErrorAction SilentlyContinue

g++ -shared -std=c++20 -Ibackend/include backend/src/devfinder/common/*.cpp backend/src/devfinder/crawler/*.cpp backend/src/devfinder/preprocessing/*.cpp backend/src/devfinder/index/*.cpp backend/src/devfinder/ranking/*.cpp backend/src/devfinder/query/*.cpp backend/src/devfinder/autocomplete/*.cpp backend/src/devfinder/server/*.cpp backend/cli/main_cli.cpp -o build/devfinder.dll -lcurl -lws2_32 -lwsock32

Write-Host "Loading API..."
Add-Type -MemberDefinition '[DllImport("build/devfinder.dll", CallingConvention = CallingConvention.Cdecl)] public static extern int RunCommand(string cmd, string arg1, string arg2);' -Name "DevFinder" -Namespace "Engine" -PassThru | Out-Null

Write-Host "Fetching data..."
[Engine.DevFinder]::RunCommand("fetch", "kubernetes operator", "5") | Out-Null

Write-Host "Indexing data..."
[Engine.DevFinder]::RunCommand("index", "", "") | Out-Null

Write-Host "Starting API server in background..."
Start-Job -ScriptBlock { 
    Add-Type -MemberDefinition '[DllImport("build/devfinder.dll", CallingConvention = CallingConvention.Cdecl)] public static extern int RunCommand(string cmd, string arg1, string arg2);' -Name "DevFinder" -Namespace "Engine2"
    [Engine2.DevFinder]::RunCommand("serve", "45678", "") 
} | Out-Null

Write-Host "Starting Frontend server in background..."
Start-Job -ScriptBlock { Set-Location frontend; python -m http.server 8000 } | Out-Null

Write-Host "Done! API is on http://localhost:45678, Frontend is on http://localhost:8000."
