Set-Location "C:\Users\artha\Downloads\Search_Engine\devfinder"

Write-Host "Recompiling..."
$env:Path += ";C:\msys64\ucrt64\bin"
g++ -shared -std=c++20 -Ibackend/include backend/src/devfinder/common/*.cpp backend/src/devfinder/crawler/*.cpp backend/src/devfinder/preprocessing/*.cpp backend/src/devfinder/index/*.cpp backend/src/devfinder/ranking/*.cpp backend/src/devfinder/query/*.cpp backend/src/devfinder/autocomplete/*.cpp backend/src/devfinder/server/*.cpp backend/cli/main_cli.cpp -o build/devfinder.dll -lcurl -lws2_32 -lwsock32

Add-Type -MemberDefinition '[DllImport("build/devfinder.dll", CallingConvention = CallingConvention.Cdecl)] public static extern int RunCommand(string cmd, string arg1, string arg2);' -Name "DevFinder" -Namespace "EngineLive3" -PassThru | Out-Null

Write-Host "Fetching machine learning repos..."
[EngineLive3.DevFinder]::RunCommand("fetch", "machine learning", "20") | Out-Null

Write-Host "Indexing data..."
[EngineLive3.DevFinder]::RunCommand("index", "", "") | Out-Null

Write-Host "Starting API server..."
[EngineLive3.DevFinder]::RunCommand("serve", "45678", "")
