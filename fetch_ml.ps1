Set-Location "C:\Users\artha\Downloads\Search_Engine\devfinder"
Add-Type -MemberDefinition '[DllImport("build/devfinder.dll", CallingConvention = CallingConvention.Cdecl)] public static extern int RunCommand(string cmd, string arg1, string arg2);' -Name "DevFinder" -Namespace "EngineLive2" -PassThru | Out-Null

Write-Host "Fetching machine learning repos..."
[EngineLive2.DevFinder]::RunCommand("fetch", "machine learning", "20") | Out-Null

Write-Host "Indexing data..."
[EngineLive2.DevFinder]::RunCommand("index", "", "") | Out-Null

Write-Host "Starting API server..."
[EngineLive2.DevFinder]::RunCommand("serve", "45678", "")
