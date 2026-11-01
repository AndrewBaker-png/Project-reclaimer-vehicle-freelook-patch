param([Parameter(Mandatory=$true)][string]$BuildDirectory)
$ErrorActionPreference='Stop'
. (Join-Path (Split-Path -Parent $PSScriptRoot) 'Manage-Freelook.ps1') -LibraryOnly
$base=Join-Path $BuildDirectory ('uninstall-test-'+[Guid]::NewGuid().ToString('N'))
$registryRoot='HKCU:\Software\ReclaimerFreelookTests\'+[Guid]::NewGuid().ToString('N')
New-Item -ItemType Directory -Path $base | Out-Null
New-Item -Path $registryRoot -Force | Out-Null
function Assert([bool]$Condition,[string]$Message){if(!$Condition){throw $Message}}
function New-Fixture([string]$Name,[int]$StopExit=0){
  $folder=Join-Path $base $Name
  New-Item -ItemType Directory -Path $folder | Out-Null
  foreach($file in $script:PackageFiles){[IO.File]::WriteAllText((Join-Path $folder $file),'fixture '+$file)}
  [IO.File]::WriteAllText((Join-Path $folder 'Start-Reclaimer-Freelook-Only.ps1'),('param([switch]$Stop); if(!$Stop){exit 99}; exit '+$StopExit))
  [IO.File]::WriteAllText((Join-Path $folder 'game.exe'),'GAME MUST SURVIVE')
  $hashes=[ordered]@{}
  foreach($file in $script:PackageFiles){$hashes[$file]=(Get-FileHash -LiteralPath (Join-Path $folder $file)).Hash}
  # Even a manifest entry for another file must never authorize its removal.
  $hashes['game.exe']=(Get-FileHash -LiteralPath (Join-Path $folder 'game.exe')).Hash
  [IO.File]::WriteAllText((Join-Path $folder 'manifest.json'),(@{revision=30;build='r30-uninstaller';files=$hashes}|ConvertTo-Json -Depth 4))
  return $folder
}
function Expect-Failure([scriptblock]$Action){
  $failed=$false
  try{& $Action | Out-Null}catch{$failed=$true}
  Assert $failed 'Expected uninstall/setup to refuse the operation.'
}
try {
  $one=New-Fixture 'mod [one] with spaces'
  $two=New-Fixture 'second mod'
  $keyOne=Install-Freelook $one -RegistryRoot $registryRoot
  $keyTwo=Install-Freelook $two -RegistryRoot $registryRoot
  Assert ($keyOne -ne $keyTwo) 'Separate package folders must have separate installed-app entries.'
  $entry=Get-ItemProperty -LiteralPath $keyOne
  Assert ($entry.UninstallString -eq ('"'+(Join-Path $one 'Uninstall-Freelook.exe')+'"')) 'Settings uninstall command must quote its executable path.'
  Assert ($entry.NoModify -eq 1 -and $entry.NoRepair -eq 1) 'Installed-app entry should expose uninstall only.'
  $again=Install-Freelook $one -RegistryRoot $registryRoot
  Assert ($again -eq $keyOne) 'Repeated setup must update the same entry.'
  foreach($file in $script:GeneratedFiles){[IO.File]::WriteAllText((Join-Path $one $file),'generated data')}
  [IO.File]::WriteAllText((Join-Path $one 'README.txt'),'USER EDIT MUST SURVIVE')
  New-Item -ItemType Directory -Path (Join-Path $one 'maps') | Out-Null
  [IO.File]::WriteAllText((Join-Path $one 'maps\user.map'),'MAP MUST SURVIVE')
  $preserved=@(Uninstall-Freelook $one -RegistryRoot $registryRoot)
  Assert ($preserved -contains 'README.txt') 'Changed documentation must be preserved.'
  Assert ((Get-Content -LiteralPath (Join-Path $one 'game.exe') -Raw) -eq 'GAME MUST SURVIVE') 'Unrelated game EXE must survive.'
  Assert (Test-Path -LiteralPath (Join-Path $one 'maps\user.map')) 'Unknown subfolders must survive.'
  Assert (!(Test-Path -LiteralPath (Join-Path $one 'freelook.ini'))) 'Mod settings must be removed.'
  Assert (!(Test-Path -LiteralPath (Join-Path $one 'Uninstall-Freelook.exe'))) 'Uninstaller must remove its own package copy.'
  Assert (!(Test-Path -LiteralPath $keyOne)) 'Uninstall must remove its installed-app entry.'
  Assert (Test-Path -LiteralPath $keyTwo) 'Other installations must remain registered.'

  $refused=New-Fixture 'shutdown refused' 26
  $refusedKey=Install-Freelook $refused -RegistryRoot $registryRoot
  Expect-Failure {Uninstall-Freelook $refused -RegistryRoot $registryRoot}
  Assert (Test-Path -LiteralPath (Join-Path $refused 'Halo3Freelook.exe')) 'Shutdown refusal must preserve files.'
  Assert (Test-Path -LiteralPath $refusedKey) 'Shutdown refusal must preserve the Settings entry.'

  $locked=New-Fixture 'locked runtime'
  $lockedKey=Install-Freelook $locked -RegistryRoot $registryRoot
  $handle=[IO.File]::Open((Join-Path $locked 'Halo3VehicleFreelook-only-mp-r30.dll'),'Open','ReadWrite','None')
  try{Expect-Failure {Uninstall-Freelook $locked -RegistryRoot $registryRoot}}finally{$handle.Dispose()}
  Assert (Test-Path -LiteralPath (Join-Path $locked 'Play-Reclaimer-with-Freelook.exe')) 'Locked runtime must prevent any file removal.'
  Assert (Test-Path -LiteralPath $lockedKey) 'Locked runtime must preserve Settings entry.'
  $null=Uninstall-Freelook $locked -RegistryRoot $registryRoot

  $linked=Join-Path $base 'linked mod'
  New-Item -ItemType Junction -Path $linked -Target $two | Out-Null
  Expect-Failure {Uninstall-Freelook $linked -RegistryRoot $registryRoot}
  Assert (Test-Path -LiteralPath (Join-Path $two 'Halo3Freelook.exe')) 'Linked folder target must survive.'
  [IO.Directory]::Delete($linked)

  Expect-Failure {Get-FreelookFolder ([IO.Path]::GetPathRoot($base))}
  # Refusing a missing manifest must leave the actual game untouched.
  $invalid=Join-Path $base 'not a package'
  New-Item -ItemType Directory -Path $invalid | Out-Null
  [IO.File]::WriteAllText((Join-Path $invalid 'game.exe'),'KEEP')
  Expect-Failure {Uninstall-Freelook $invalid -RegistryRoot $registryRoot}
  Assert (Test-Path -LiteralPath (Join-Path $invalid 'game.exe')) 'Invalid package must remain untouched.'
  $null=Uninstall-Freelook $two -RegistryRoot $registryRoot
  $empty=New-Fixture 'direct uninstall without setup'
  Remove-Item -LiteralPath (Join-Path $empty 'game.exe')
  $null=Uninstall-Freelook $empty -RegistryRoot $registryRoot
  Assert (!(Test-Path -LiteralPath $empty)) 'Direct uninstall without registration must remove an otherwise empty package folder.'
  Write-Output 'PASS: Settings registration/repeated setup, quoted uninstall command, separate copies, allowlisted removal, preserved game/maps/modified files, shutdown refusal, locked DLL refusal/retry, linked-folder refusal, drive-root refusal and invalid-package refusal.'
}finally{
  # Only this run's registry leaf and its direct test entries; never the live uninstall registry.
  foreach($key in @(Get-ChildItem -LiteralPath $registryRoot)){Remove-Item -LiteralPath $key.PSPath -Force}
  Remove-Item -LiteralPath $registryRoot -Force
}
