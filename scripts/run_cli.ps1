param(
    [Parameter(Mandatory = $true)][string]$Cmd,
    [string]$Arg1 = "",
    [string]$Arg2 = ""
)
# Runs one devfinder CLI command through build/devfinder.dll (Windows equivalent of backend/build/devfinder_cli).
# Examples:
#   scripts/run_cli.ps1 fetch-all 20
#   scripts/run_cli.ps1 index
#   scripts/run_cli.ps1 search "todo app"
Set-Location (Split-Path -Parent $PSScriptRoot)
$env:Path += ";C:\msys64\ucrt64\bin"
Add-Type -MemberDefinition '[DllImport("build/devfinder.dll", CallingConvention = CallingConvention.Cdecl)] public static extern int RunCommand(string cmd, string arg1, string arg2);' -Name "DevFinder" -Namespace "EngineCli"
exit [EngineCli.DevFinder]::RunCommand($Cmd, $Arg1, $Arg2)
