Set-Location "C:\Users\artha\Downloads\Search_Engine\devfinder"
Add-Type -MemberDefinition '[DllImport("build/devfinder.dll", CallingConvention = CallingConvention.Cdecl)] public static extern int RunCommand(string cmd, string arg1, string arg2);' -Name "DevFinder" -Namespace "EngineLive"
[EngineLive.DevFinder]::RunCommand("serve", "45678", "")
