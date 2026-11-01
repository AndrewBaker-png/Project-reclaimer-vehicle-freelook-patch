param([Parameter(Mandatory=$true)][string]$BuildDirectory)
$ErrorActionPreference='Stop'
. (Join-Path (Split-Path -Parent $PSScriptRoot) 'Manage-Freelook.ps1') -LibraryOnly
$base=Join-Path $BuildDirectory ('up-'+[Guid]::NewGuid().ToString('N').Substring(0,8))
$registryRoot='HKCU:\Software\ReclaimerFreelookTests\'+[Guid]::NewGuid().ToString('N')
New-Item -ItemType Directory -Path $base | Out-Null
New-Item -Path $registryRoot -Force | Out-Null
function Assert([bool]$Condition,[string]$Message){if(!$Condition){throw $Message}}
function Expect-Failure([scriptblock]$Action,[string]$Pattern){
  $message=$null
  try{& $Action | Out-Null}catch{$message=$_.Exception.Message}
  Assert ($null -ne $message -and $message -like $Pattern) ('Expected refusal matching '+$Pattern+'; got: '+$message)
}
function Write-Manifest([string]$Folder,[string]$Version){
  $hashes=[ordered]@{}
  foreach($name in $script:PackageFiles){$hashes[$name]=(Get-FileHash -LiteralPath (Join-Path $Folder $name)).Hash}
  [IO.File]::WriteAllText((Join-Path $Folder 'manifest.json'),(@{revision=30;build='test-package';packageVersion=$Version;files=$hashes}|ConvertTo-Json -Depth 4))
}
function New-Package([string]$Name,[string]$Version='30.4.0',[int]$StopExit=0){
  $folder=Join-Path $base $Name
  New-Item -ItemType Directory -Path $folder | Out-Null
  foreach($file in $script:PackageFiles){[IO.File]::WriteAllText((Join-Path $folder $file),($Version+' '+$file))}
  [IO.File]::WriteAllText((Join-Path $folder 'Start-Reclaimer-Freelook-Only.ps1'),('param([switch]$Stop); if(!$Stop){exit 99}; exit '+$StopExit))
  Write-Manifest $folder $Version
  return $folder
}
function Get-BytesSnapshot([string]$Folder){
  $snapshot=@{}
  foreach($name in @($script:PackageFiles+@('manifest.json','reclaimer-launcher.ini'))){
    $path=Join-Path $Folder $name
    if(Test-Path -LiteralPath $path){$snapshot[$name]=(Get-FileHash -LiteralPath $path).Hash}
  }
  return $snapshot
}
function Assert-Snapshot([string]$Folder,$Snapshot){
  $actual=Get-BytesSnapshot $Folder
  Assert ($actual.Count -eq $Snapshot.Count) 'Rollback must restore the original file set.'
  foreach($name in $Snapshot.Keys){Assert ($actual[$name] -eq $Snapshot[$name]) ('Original bytes must survive: '+$name)}
}
$productionGames=${function:Get-FreelookRunningGames}
$script:OriginalCopy=${function:Copy-FreelookPayload}
$script:OriginalInstall=${function:Install-Freelook}
$script:FakeGame=$false;$script:FailTarget=$null;$script:RegistrationFail=$false
try {
  # Exercise the actual process guard against a harmless executable using the
  # Reclaimer naming convention, including the case where no engine is loaded.
  $fixture=Join-Path $base 'project-reclaimer-client-fixture.exe'
  Copy-Item -LiteralPath (Join-Path $BuildDirectory 'play_client_fixture.exe') -Destination $fixture
  $process=Start-Process -FilePath $fixture -ArgumentList '--stay-running' -PassThru
  try{Expect-Failure {Assert-FreelookClosed @($base)} '*Fully close*'}finally{$process.WaitForExit(6000) | Out-Null;$process.Dispose()}
  Write-Output 'PASS: production process guard detects a running client without a loaded engine.'
  function Get-FreelookRunningGames {param([string[]]$Folders);if($script:FakeGame){[pscustomobject]@{ProcessName='fake-client';Id=1}}}
  function Copy-FreelookPayload([string]$Source,[string]$Destination){
    if($Destination -eq $script:FailTarget -and !$script:FailedOnce){$script:FailedOnce=$true;[IO.File]::WriteAllText($Destination,'PARTIAL WRITE');throw 'Simulated interrupted copy'}
    & $script:OriginalCopy $Source $Destination
  }
  function Install-Freelook([string]$Folder,[string]$RegistryRoot=$script:DefaultRegistryRoot){
    $key=& $script:OriginalInstall $Folder $RegistryRoot
    if($script:RegistrationFail){New-ItemProperty -LiteralPath $key -Name 'PartialUpdate' -Value 'test' -Force | Out-Null;throw 'Simulated registration failure'}
    return $key
  }
  $source=New-Package 'new [source]'
  $target=New-Package 'installed copy' '30.3.0'
  [IO.File]::WriteAllText((Join-Path $target 'freelook.ini'),'USER SETTINGS')
  [IO.File]::WriteAllText((Join-Path $target 'reclaimer-launcher.ini'),'[Reclaimer]'+"`r`nClient=C:\Game\client.exe")
  [IO.File]::WriteAllText((Join-Path $target 'game.exe'),'GAME MUST SURVIVE')
  $key=Install-Freelook $target $registryRoot
  $plan=Get-FreelookUpdatePlan $source $target
  Assert ($plan.Mode -eq 'Update' -and $plan.Preserve.Count -eq 2) 'Must detect version upgrade and preserved settings.'
  $result=Update-Freelook $source $target $registryRoot
  Assert ($result.Mode -eq 'Update') 'Expected update result.'
  Assert ((Get-Content -LiteralPath (Join-Path $target 'freelook.ini') -Raw) -eq 'USER SETTINGS') 'User settings must survive.'
  Assert ((Get-Content -LiteralPath (Join-Path $target 'reclaimer-launcher.ini') -Raw) -match 'C:\\Game\\client.exe') 'Saved client path must survive.'
  Assert ((Get-Content -LiteralPath (Join-Path $target 'game.exe') -Raw) -eq 'GAME MUST SURVIVE') 'Game files must survive.'
  Assert ((Get-ItemProperty -LiteralPath $key).DisplayVersion -eq '30.4.0') 'Windows Settings must show installed package version.'
  Assert (Test-Path -LiteralPath (Join-Path $result.Backup 'previous\Halo3Freelook.exe')) 'Previous version must be backed up.'
  Assert ((Get-FreelookUpdatePlan $source $target).Mode -eq 'Repair') 'Same-version setup must offer repair.'
  Assert (@(Find-FreelookInstallations $registryRoot).Count -eq 1) 'Installed-copy discovery must find the registration.'

  $older=New-Package 'older' '30.2.0'
  Expect-Failure {Get-FreelookUpdatePlan $older $target} '*Downgrades*'
  Expect-Failure {Get-FreelookUpdatePlan $source $source} '*separate folder*'
  $collision=Join-Path $base 'unrelated folder'
  New-Item -ItemType Directory -Path $collision | Out-Null
  [IO.File]::WriteAllText((Join-Path $collision 'README.txt'),'UNRELATED')
  Expect-Failure {Get-FreelookUpdatePlan $source $collision} '*unrecognized*'
  $linked=Join-Path $base 'linked target'
  New-Item -ItemType Junction -Path $linked -Target $target | Out-Null
  try{Expect-Failure {Get-FreelookUpdatePlan $source $linked} '*linked*'}finally{[IO.Directory]::Delete($linked)}

  $snapshot=Get-BytesSnapshot $target
  Write-Output 'PASS: version upgrade, Settings registration, saved configuration, backup, repair detection, and destination/version refusals.'
  $script:FakeGame=$true
  Expect-Failure {Update-Freelook $source $target $registryRoot} '*Fully close*'
  $script:FakeGame=$false
  Assert-Snapshot $target $snapshot
  $handle=[IO.File]::Open((Join-Path $target 'Halo3VehicleFreelook-only-mp-r30.dll'),'Open','ReadWrite','None')
  try{Expect-Failure {Update-Freelook $source $target $registryRoot} '*File in use*'}finally{$handle.Dispose()}
  Assert-Snapshot $target $snapshot
  $mutexName='Local\'+(Split-Path -Leaf $key)+'Setup'
  $ready=Join-Path $base 'mutex-ready';$release=Join-Path $base 'mutex-release'
  $holderScript=Join-Path $base 'hold-mutex.ps1'
  $holderCode=@'
param($Name,$Ready,$Release)
$mutex=New-Object Threading.Mutex($false,$Name)
$null=$mutex.WaitOne()
try {
  [IO.File]::WriteAllText($Ready,'ready')
  $deadline=[DateTime]::UtcNow.AddSeconds(15)
  while(!(Test-Path -LiteralPath $Release) -and [DateTime]::UtcNow -lt $deadline){Start-Sleep -Milliseconds 50}
}finally{$mutex.ReleaseMutex();$mutex.Dispose()}
'@
  [IO.File]::WriteAllText($holderScript,$holderCode)
  $holder=Start-Process -FilePath (Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe') -ArgumentList ('-NoProfile -File "'+$holderScript+'" "'+$mutexName+'" "'+$ready+'" "'+$release+'"') -WindowStyle Hidden -PassThru
  try {
    $deadline=[DateTime]::UtcNow.AddSeconds(5)
    while(!(Test-Path -LiteralPath $ready) -and [DateTime]::UtcNow -lt $deadline){Start-Sleep -Milliseconds 50}
    Assert (Test-Path -LiteralPath $ready) 'Concurrency fixture must acquire the setup mutex.'
    Expect-Failure {Update-Freelook $source $target $registryRoot} '*Another setup*'
    Expect-Failure {Uninstall-Freelook $target $registryRoot} '*already working*'
    Assert-Snapshot $target $snapshot
  }finally{[IO.File]::WriteAllText($release,'release');$holder.WaitForExit(16000) | Out-Null;$holder.Dispose()}
  $refused=New-Package 'stop refused' '30.4.0' 26
  Expect-Failure {Update-Freelook $refused $target $registryRoot} '*shutdown*'
  Assert-Snapshot $target $snapshot
  $damaged=New-Package 'damaged'
  [IO.File]::WriteAllText((Join-Path $damaged 'Halo3Freelook.exe'),'CORRUPT')
  Expect-Failure {Update-Freelook $damaged $target $registryRoot} '*damaged or modified*'

  $script:FailTarget=Join-Path $target 'Halo3VehicleFreelook-only-mp-r30.dll'
  $script:FailedOnce=$false
  Expect-Failure {Update-Freelook $source $target $registryRoot} '*previous files and registration were restored*'
  Assert-Snapshot $target $snapshot
  $script:FailTarget=$null
  $script:RegistrationFail=$true
  Expect-Failure {Update-Freelook $source $target $registryRoot} '*previous files and registration were restored*'
  Assert-Snapshot $target $snapshot
  $entry=Get-ItemProperty -LiteralPath $key
  Assert ($entry.DisplayVersion -eq '30.4.0' -and !$entry.PartialUpdate) 'Rollback must restore registry values and remove partial additions.'
  $script:RegistrationFail=$false

  # Rollback must also remove files newly introduced into an older portable copy.
  $legacy=New-Package 'legacy' '30.2.0'
  foreach($name in @('Setup-Freelook.exe','Uninstall-Freelook.exe','Manage-Freelook.ps1')){Remove-Item -LiteralPath (Join-Path $legacy $name)}
  $oldManifest=Get-Content -LiteralPath (Join-Path $legacy 'manifest.json') -Raw | ConvertFrom-Json
  $oldManifest.PSObject.Properties.Remove('packageVersion');$oldManifest.build='r30-auto-launch'
  [IO.File]::WriteAllText((Join-Path $legacy 'manifest.json'),($oldManifest|ConvertTo-Json -Depth 4))
  $legacySnapshot=Get-BytesSnapshot $legacy
  $script:RegistrationFail=$true
  Expect-Failure {Update-Freelook $source $legacy $registryRoot} '*previous files and registration were restored*'
  $script:RegistrationFail=$false
  Assert-Snapshot $legacy $legacySnapshot
  Assert (!(Test-Path -LiteralPath (Get-FreelookKey $legacy $registryRoot))) 'Failed first registration must leave no installed-app entry.'
  $null=Update-Freelook $source $legacy $registryRoot
  Assert (Test-Path -LiteralPath (Join-Path $legacy 'Uninstall-Freelook.exe')) 'Legacy portable upgrade must add uninstall support.'

  $fresh=Join-Path $base 'brand new'
  $freshResult=Update-Freelook $source $fresh $registryRoot
  Assert ($freshResult.Mode -eq 'Install') 'Fresh destination must install.'
  $null=Update-Freelook $source $fresh $registryRoot
  $null=Uninstall-Freelook $fresh $registryRoot
  Assert (!(Test-Path -LiteralPath (Join-Path $fresh 'Play-Reclaimer-with-Freelook.exe'))) 'Updated installation must still uninstall.'
  Assert (Test-Path -LiteralPath $freshResult.Backup) 'Uninstall must preserve recovery backups.'
  Write-Output 'PASS: real running-client guard, version discovery, update/repair/fresh install, settings/client-path preservation, backups, downgrade/in-place/collision/corruption/shutdown/lock refusals, partial-file and registration rollback, legacy portable upgrade, and retry.'
}finally{
  Set-Item -LiteralPath Function:Get-FreelookRunningGames -Value $productionGames
  Set-Item -LiteralPath Function:Copy-FreelookPayload -Value $script:OriginalCopy
  Set-Item -LiteralPath Function:Install-Freelook -Value $script:OriginalInstall
  foreach($key in @(Get-ChildItem -LiteralPath $registryRoot)){Remove-Item -LiteralPath $key.PSPath -Force}
  Remove-Item -LiteralPath $registryRoot -Force
}
