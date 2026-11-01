# Run from an administrator PowerShell window.
# In an installed package, the default is Halo3Freelook.exe beside this script.
# From the source checkout, supply -LoaderPath pointing to the installed loader.
[CmdletBinding(SupportsShouldProcess)]
param(
    [string]$LoaderPath = (Join-Path $PSScriptRoot 'Halo3Freelook.exe')
)

$ErrorActionPreference = 'Stop'

# Freelook loads its DLL into Reclaimer using WriteProcessMemory and
# CreateRemoteThread, then hooks the game's camera/input functions. Defender
# has flagged this behavior as Behavior:Win32/DefenseEvasion.A!ml on this PC.
# These operations implement the mod, but are also techniques used by malware.
# The exclusion records the user's trust in this loader; it is not proof that
# a binary is safe. Only use this helper for a build you trust.
# Exclude ONLY this executable path, never the game folder or all processes.
# Defender's other protection settings and detection history stay unchanged.
$fullPath = [IO.Path]::GetFullPath($LoaderPath)
if ([IO.Path]::GetFileName($fullPath) -ine 'Halo3Freelook.exe') {
    throw 'LoaderPath must name Halo3Freelook.exe.'
}
if ($fullPath.IndexOfAny([char[]]'*?[]') -ge 0) {
    throw 'Wildcards are not allowed in the loader path.'
}
if (Test-Path -LiteralPath $fullPath -PathType Container) {
    throw 'LoaderPath must be an executable file path, not a directory.'
}
# The file may be absent because Defender already quarantined it. This helper
# adds the exception only; it does not restore or execute quarantined files.
if ($PSCmdlet.ShouldProcess($fullPath, 'Add a Microsoft Defender file exclusion')) {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = [Security.Principal.WindowsPrincipal]::new($identity)
    if (!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        throw 'Run this script from PowerShell opened with Run as administrator.'
    }
    if ((Get-MpPreference).ExclusionPath -contains $fullPath) {
        Write-Output "Defender exclusion already exists: $fullPath"
        return
    }
    Add-MpPreference -ExclusionPath $fullPath
    if ((Get-MpPreference).ExclusionPath -notcontains $fullPath) {
        throw 'Defender did not retain the requested exclusion. Check its policy settings.'
    }
    Write-Output "Added Defender exclusion: $fullPath"
}

# To undo, run as administrator:
# Remove-MpPreference -ExclusionPath 'FULL PATH TO Halo3Freelook.exe'
