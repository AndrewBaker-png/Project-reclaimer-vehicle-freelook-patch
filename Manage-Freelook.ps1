[CmdletBinding()]
param([ValidateSet('Install','Uninstall')][string]$Action='Install',[switch]$LibraryOnly)
$ErrorActionPreference='Stop'

# Fixed filenames only. The downloaded manifest never supplies deletion paths.
$script:PackageFiles=@(
  'Play-Reclaimer-with-Freelook.exe','Halo3Freelook.exe','Halo3VehicleFreelook-only-mp-r30.dll',
  'freelook.ini','README.txt','Start-Reclaimer-Freelook-Only.ps1','Start-Reclaimer-Freelook-Only.cmd',
  'Stop-Reclaimer-Freelook-Only.cmd','MINHOOK-LICENSE.txt','HCM-LICENSE.txt',
  'Setup-Freelook.exe','Uninstall-Freelook.exe','Manage-Freelook.ps1'
)
$script:GeneratedFiles=@('reclaimer-launcher.ini','loader-state.json','loader.log','freelook.log',
  'loader-console.log','loader-error.log')
$script:DefaultRegistryRoot='HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall'

function Get-FreelookFolder([string]$Folder) {
  $full=[IO.Path]::GetFullPath($Folder).TrimEnd('\')
  if($full -eq [IO.Path]::GetPathRoot($full).TrimEnd('\')){throw 'Choose a mod folder, not a drive root.'}
  $item=Get-Item -LiteralPath $full -Force
  if(!$item.PSIsContainer){throw 'The mod folder is missing.'}
  for($ancestor=$item;$null -ne $ancestor;$ancestor=$ancestor.Parent){
    if($ancestor.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Setup/uninstall cannot use a linked folder. Use the actual extracted mod folder.'}
  }
  return $full
}
function Get-FreelookKey([string]$Folder,[string]$RegistryRoot=$script:DefaultRegistryRoot) {
  $sha=[Security.Cryptography.SHA256]::Create()
  try{$id=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($Folder.ToLowerInvariant())))).Replace('-','').Substring(0,16)}finally{$sha.Dispose()}
  return Join-Path $RegistryRoot ('ReclaimerFreelook-'+$id)
}
function Get-FreelookManifest([string]$Folder,[switch]$Legacy) {
  $path=Join-Path $Folder 'manifest.json'
  if((Get-Item -LiteralPath $path -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'The package manifest must not be a link.'}
  $manifest=Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
  if($manifest.revision -ne 30 -or !$manifest.files){throw 'The package manifest is missing or unsupported. Extract the full mod ZIP again.'}
  $required=if($Legacy){@('Halo3Freelook.exe','Halo3VehicleFreelook-only-mp-r30.dll')}else{$script:PackageFiles}
  foreach($name in $required){
    if($manifest.files.$name -notmatch '^[0-9a-fA-F]{64}$'){throw ('Package manifest is incomplete: '+$name)}
  }
  return $manifest
}
function Assert-FreelookFile([string]$Path) {
  $item=Get-Item -LiteralPath $Path -Force
  if($item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)){throw ('Expected a regular mod file: '+$Path)}
}
function Install-Freelook([string]$Folder,[string]$RegistryRoot=$script:DefaultRegistryRoot) {
  $folder=Get-FreelookFolder $Folder
  $manifest=Get-FreelookManifest $folder
  foreach($name in $script:PackageFiles){Assert-FreelookFile (Join-Path $folder $name)}
  $target=Join-Path $folder 'Play-Reclaimer-with-Freelook.exe'
  $key=Get-FreelookKey $folder $RegistryRoot
  New-Item -Path $key -Force | Out-Null
  $values=@{
    DisplayName='Project Reclaimer Vehicle Freelook'; DisplayVersion=(Get-FreelookVersion $manifest).ToString();
    Publisher='Project Reclaimer Freelook contributors'; InstallLocation=$folder;
    UninstallString=('"'+(Join-Path $folder 'Uninstall-Freelook.exe')+'"');
    DisplayIcon=$target+',0'; InstallDate=(Get-Date -Format yyyyMMdd);
    URLInfoAbout='https://github.com/AndrewBaker-png/Project-reclaimer-aerial-vehicle-freelook-patch'
  }
  foreach($name in $values.Keys){New-ItemProperty -LiteralPath $key -Name $name -Value $values[$name] -PropertyType String -Force | Out-Null}
  foreach($name in @('NoModify','NoRepair')){New-ItemProperty -LiteralPath $key -Name $name -Value 1 -PropertyType DWord -Force | Out-Null}
  return $key
}
function Remove-FreelookFiles([string]$Folder,[string]$RegistryRoot=$script:DefaultRegistryRoot) {
  $folder=Get-FreelookFolder $Folder
  $manifest=Get-FreelookManifest $folder
  $key=Get-FreelookKey $folder $RegistryRoot
  if(Test-Path -LiteralPath $key){
    if((Get-ItemProperty -LiteralPath $key).InstallLocation -ne $folder){throw 'The installed-app entry belongs to another folder.'}
  }
  # Cancel any waiting load and confirm runtime shutdown before touching files.
  $helper=Join-Path $folder 'Start-Reclaimer-Freelook-Only.ps1'
  Assert-FreelookFile $helper
  $powershell=Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
  & $powershell -NoProfile -ExecutionPolicy Bypass -File $helper -Stop | Out-Null
  if($LASTEXITCODE -ne 0){throw 'The loader could not confirm shutdown. Fully close Reclaimer and try uninstalling again.'}
  $remove=@(); $preserve=@()
  foreach($name in @($script:PackageFiles+$script:GeneratedFiles+@('manifest.json'))){
    $path=Join-Path $folder $name
    if(!(Test-Path -LiteralPath $path)){continue}
    Assert-FreelookFile $path
    # Loaded DLLs/EXEs must be released before any removal; retry briefly while
    # the native uninstall bootstrap exits. Never terminate a running game.
    $deadline=[DateTime]::UtcNow.AddSeconds(3)
    do {
      try{$handle=[IO.File]::Open($path,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None);$handle.Dispose();break}
      catch{if([DateTime]::UtcNow -ge $deadline){throw ('Close Reclaimer and any mod tools, then retry uninstalling. File in use: '+$name)};Start-Sleep -Milliseconds 100}
    }while($true)
    # Preserve changed package documents/binaries; settings and logs are explicitly removed.
    if($name -in $script:PackageFiles -and $name -ne 'freelook.ini'){
      if((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $manifest.files.$name){$preserve+=$name;continue}
    }
    $remove+=$path
  }
  foreach($path in $remove){Remove-Item -LiteralPath $path -Force}
  if(Test-Path -LiteralPath $key){Remove-Item -LiteralPath $key -Force}
  # No recursion: shared folders and all unknown files/subfolders remain.
  if(@(Get-ChildItem -LiteralPath $folder -Force).Count -eq 0){
    Set-Location -LiteralPath ([IO.Path]::GetTempPath())
    [Environment]::CurrentDirectory=[IO.Path]::GetTempPath()
    Remove-Item -LiteralPath $folder
  }
  return $preserve
}

function Uninstall-Freelook([string]$Folder,[string]$RegistryRoot=$script:DefaultRegistryRoot) {
  $folder=Get-FreelookFolder $Folder
  $key=Get-FreelookKey $folder $RegistryRoot
  $mutex=New-Object Threading.Mutex($false,('Local\'+(Split-Path -Leaf $key)+'Setup'))
  $owned=$false
  try{
    try{$owned=$mutex.WaitOne(0)}catch [Threading.AbandonedMutexException]{$owned=$true}
    if(!$owned){throw 'Setup or uninstall is already working on this installation. Wait for it to finish.'}
    Remove-FreelookFiles $folder $RegistryRoot
  }finally{if($owned){$mutex.ReleaseMutex()};$mutex.Dispose()}
}
function Get-FreelookVersion($Manifest) {
  if($Manifest.packageVersion){
    try{return [version]$Manifest.packageVersion}catch{throw 'The package version is invalid.'}
  }
  $legacy=@{'r30'='30.0.0';'r30-audit-fixes'='30.1.0';'r30-auto-launch'='30.2.0';'r30-uninstaller'='30.3.0'}
  if($legacy.ContainsKey([string]$Manifest.build)){return [version]$legacy[[string]$Manifest.build]}
  throw 'The installed package has an unknown version. Choose its correct extracted mod folder.'
}
function Get-FreelookDestination([string]$Folder) {
  $full=[IO.Path]::GetFullPath($Folder).TrimEnd('\')
  if($full -eq [IO.Path]::GetPathRoot($full).TrimEnd('\')){throw 'Choose a mod folder, not a drive root.'}
  $ancestor=$full
  while(!(Test-Path -LiteralPath $ancestor)){$ancestor=Split-Path -Parent $ancestor;if(!$ancestor){throw 'The destination has no valid parent folder.'}}
  $item=Get-Item -LiteralPath $ancestor -Force
  if(!$item.PSIsContainer){throw 'The destination parent is not a folder.'}
  for(;$null -ne $item;$item=$item.Parent){
    if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Setup cannot use a linked destination folder.'}
  }
  return $full
}
function Find-FreelookInstallations([string]$RegistryRoot=$script:DefaultRegistryRoot) {
  if(!(Test-Path -LiteralPath $RegistryRoot)){return}
  foreach($key in @(Get-ChildItem -LiteralPath $RegistryRoot)){
    if($key.PSChildName -notlike 'ReclaimerFreelook-*'){continue}
    $entry=Get-ItemProperty -LiteralPath $key.PSPath
    if($entry.DisplayName -eq 'Project Reclaimer Vehicle Freelook' -and $entry.InstallLocation){
      [pscustomobject]@{Folder=$entry.InstallLocation;Version=$entry.DisplayVersion}
    }
  }
}
function Get-FreelookUpdatePlan([string]$Source,[string]$Destination) {
  $source=Get-FreelookFolder $Source
  $destination=Get-FreelookDestination $Destination
  if($source -eq $destination -or $source.StartsWith($destination+'\',[StringComparison]::OrdinalIgnoreCase) -or
     $destination.StartsWith($source+'\',[StringComparison]::OrdinalIgnoreCase)){
    throw 'Extract the new ZIP to a separate folder, then run its Setup-Freelook.exe. Source and installation folders must be separate; do not extract updates over your installed copy.'
  }
  $incoming=Get-FreelookManifest $source
  $incomingVersion=Get-FreelookVersion $incoming
  foreach($name in $script:PackageFiles){
    $path=Join-Path $source $name
    Assert-FreelookFile $path
    if((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $incoming.files.$name){throw ('The new package is damaged or modified: '+$name+'. Extract a fresh ZIP.')}
  }
  $mode='Install';$currentVersion=$null
  $manifestPath=Join-Path $destination 'manifest.json'
  if(Test-Path -LiteralPath $manifestPath){
    $current=Get-FreelookManifest $destination -Legacy
    $currentVersion=Get-FreelookVersion $current
    if($incomingVersion -lt $currentVersion){throw ('Downgrades are refused. Installed: '+$currentVersion+'; incoming: '+$incomingVersion)}
    $mode=if($incomingVersion -eq $currentVersion){'Repair'}else{'Update'}
  }else{
    foreach($name in @($script:PackageFiles+$script:GeneratedFiles)){
      if(Test-Path -LiteralPath (Join-Path $destination $name)){throw ('An unrecognized installation or unrelated file already occupies '+$name+'. Choose the actual existing mod folder with its manifest, or an empty folder.')}
    }
  }
  $replace=@();$preserve=@()
  foreach($name in @($script:PackageFiles+@('manifest.json','reclaimer-launcher.ini'))){
    $path=Join-Path $destination $name
    if(Test-Path -LiteralPath $path){
      Assert-FreelookFile $path
      if($name -in @('freelook.ini','reclaimer-launcher.ini')){$preserve+=$name}else{$replace+=$name}
    }
  }
  [pscustomobject]@{Source=$source;Destination=$destination;Mode=$mode;CurrentVersion=$currentVersion;
    IncomingVersion=$incomingVersion;Replace=$replace;Preserve=$preserve}
}
function Get-FreelookRunningGames([string[]]$Folders) {
  $names=@('MCC-Win64-Shipping')
  foreach($folder in $Folders){
    $config=Join-Path $folder 'reclaimer-launcher.ini'
    if(Test-Path -LiteralPath $config){
      Assert-FreelookFile $config
      foreach($line in @(Get-Content -LiteralPath $config)){
        if($line -match '^\s*Client\s*=\s*(.+?)\s*$'){$names+=[IO.Path]::GetFileNameWithoutExtension($Matches[1].Trim('"'))}
      }
    }
  }
  Get-Process -ErrorAction Stop | Where-Object {$_.ProcessName -like 'project-reclaimer-client*' -or $_.ProcessName -in $names}
}
function Assert-FreelookClosed([string[]]$Folders) {
  if(@(Get-FreelookRunningGames $Folders).Count){throw 'Fully close Project Reclaimer/MCC before installing or updating. No installed files have been replaced.'}
}
function Copy-FreelookPayload([string]$Source,[string]$Destination) {
  [IO.File]::Copy($Source,$Destination,$true)
}
function Get-FreelookRegistrySnapshot([string]$Key) {
  $snapshot=@{Exists=(Test-Path -LiteralPath $Key);Values=@{}}
  if($snapshot.Exists){
    $item=Get-Item -LiteralPath $Key
    foreach($name in $item.GetValueNames()){$snapshot.Values[$name]=@{Value=$item.GetValue($name);Kind=[string]$item.GetValueKind($name)}}
  }
  return $snapshot
}
function Restore-FreelookRegistry([string]$Key,$Snapshot) {
  if(Test-Path -LiteralPath $Key){Remove-Item -LiteralPath $Key -Force}
  if($Snapshot.Exists){
    New-Item -Path $Key -Force | Out-Null
    foreach($name in $Snapshot.Values.Keys){$entry=$Snapshot.Values[$name];New-ItemProperty -LiteralPath $Key -Name $name -Value $entry.Value -PropertyType $entry.Kind -Force | Out-Null}
  }
}
function Update-Freelook([string]$Source,[string]$Destination,[string]$RegistryRoot=$script:DefaultRegistryRoot) {
  # Recompute after UI confirmation. All destinations are fixed sibling files.
  $plan=Get-FreelookUpdatePlan $Source $Destination
  Assert-FreelookClosed @($plan.Source,$plan.Destination)
  $powershell=Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
  & $powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $plan.Source 'Start-Reclaimer-Freelook-Only.ps1') -Stop | Out-Null
  if($LASTEXITCODE -ne 0){throw 'The loader could not confirm shutdown. Close Reclaimer and retry. No installed files have been replaced.'}
  $names=@($script:PackageFiles+@('manifest.json'))
  $backup=$null;$attempted=New-Object 'System.Collections.Generic.List[string]'
  $key=Get-FreelookKey $plan.Destination $RegistryRoot
  $registry=Get-FreelookRegistrySnapshot $key
  $mutex=New-Object Threading.Mutex($false,('Local\'+(Split-Path -Leaf $key)+'Setup'))
  $ownsMutex=$false
  try {
    try{$ownsMutex=$mutex.WaitOne(0)}catch [Threading.AbandonedMutexException]{$ownsMutex=$true}
    if(!$ownsMutex){throw 'Another setup is already working on this installation. Wait for it to finish.'}
    $plan=Get-FreelookUpdatePlan $Source $Destination
    $registry=Get-FreelookRegistrySnapshot $key
    # Fail before replacing anything if a package file is mapped or locked.
    foreach($name in @($names+@('reclaimer-launcher.ini'))){
      $path=Join-Path $plan.Destination $name
      if(Test-Path -LiteralPath $path){
        Assert-FreelookFile $path
        try{$handle=[IO.File]::Open($path,'Open','ReadWrite','None');$handle.Dispose()}
        catch{throw ('Close Reclaimer and mod tools before updating. File in use: '+$name)}
      }
    }
    New-Item -ItemType Directory -Path $plan.Destination -Force | Out-Null
    $backupRoot=Join-Path $plan.Destination 'freelook-backups'
    if(Test-Path -LiteralPath $backupRoot){$null=Get-FreelookFolder $backupRoot}
    $backup=Join-Path $backupRoot ((Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[Guid]::NewGuid().ToString('N'))
    $previous=Join-Path $backup 'previous';$staged=Join-Path $backup 'incoming'
    New-Item -ItemType Directory -Path $previous,$staged | Out-Null
    $existing=@()
    foreach($name in @($names+@('reclaimer-launcher.ini'))){
      $path=Join-Path $plan.Destination $name
      if(Test-Path -LiteralPath $path){[IO.File]::Copy($path,(Join-Path $previous $name));$existing+=$name}
    }
    foreach($name in $names){Copy-FreelookPayload (Join-Path $plan.Source $name) (Join-Path $staged $name)}
    $stagedManifest=Get-FreelookManifest $staged
    foreach($name in $script:PackageFiles){
      if((Get-FileHash -LiteralPath (Join-Path $staged $name) -Algorithm SHA256).Hash -ne $stagedManifest.files.$name){throw ('Staged package verification failed: '+$name)}
    }
    if((Get-FreelookVersion $stagedManifest) -ne $plan.IncomingVersion){throw 'The source package changed while setup was running.'}
    $journal=@{state='prepared';source=$plan.Source;destination=$plan.Destination;from=[string]$plan.CurrentVersion;
      to=[string]$plan.IncomingVersion;existing=$existing;registry=$registry}
    $journalPath=Join-Path $backup 'transaction.json'
    [IO.File]::WriteAllText($journalPath,($journal|ConvertTo-Json -Depth 8))
    Assert-FreelookClosed @($plan.Source,$plan.Destination)
    foreach($name in $names){
      if($name -eq 'freelook.ini' -and $name -in $existing){continue}
      $attempted.Add($name)
      $journal.state='applying';$journal.attempted=@($attempted.ToArray())
      [IO.File]::WriteAllText($journalPath,($journal|ConvertTo-Json -Depth 8))
      Copy-FreelookPayload (Join-Path $staged $name) (Join-Path $plan.Destination $name)
    }
    $null=Install-Freelook $plan.Destination $RegistryRoot
    $journal.state='complete'
    [IO.File]::WriteAllText($journalPath,($journal|ConvertTo-Json -Depth 8))
    return [pscustomobject]@{Mode=$plan.Mode;Version=$plan.IncomingVersion;Folder=$plan.Destination;Backup=$backup}
  }catch{
    $reason=$_.Exception.Message
    if($attempted.Count){
      try {
        foreach($name in $attempted){
          $old=Join-Path $previous $name;$target=Join-Path $plan.Destination $name
          if($name -in $existing){[IO.File]::Copy($old,$target,$true)}
          elseif(Test-Path -LiteralPath $target){Assert-FreelookFile $target;Remove-Item -LiteralPath $target -Force}
        }
        Restore-FreelookRegistry $key $registry
        $journal.state='rolled-back'
        [IO.File]::WriteAllText($journalPath,($journal|ConvertTo-Json -Depth 8))
      }catch{throw ('Update failed and rollback could not finish. Keep the backup at '+$backup+'. Error: '+$_.Exception.Message)}
      throw ('Update failed; previous files and registration were restored. Backup: '+$backup+'. Cause: '+$reason)
    }
    throw $reason
  }finally{if($ownsMutex){$mutex.ReleaseMutex()};$mutex.Dispose()}
}

if($LibraryOnly){return}
Add-Type -AssemblyName System.Windows.Forms
$title='Project Reclaimer Vehicle Freelook'
try {
  if($Action -eq 'Install'){
    $installed=@(Find-FreelookInstallations)
    $destination=Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) 'Project Reclaimer Freelook'
    if($installed.Count -eq 1){$destination=$installed[0].Folder}
    $prompt=if($installed.Count -eq 1){'Found version '+$installed[0].Version+'. Use this installation?'}else{'Install/update in this folder?'}
    $choice=[Windows.Forms.MessageBox]::Show(($prompt+"`r`n"+$destination+"`r`n`r`nYes: use this folder. No: choose another folder."),$title,'YesNoCancel','Question')
    if($choice -eq 'Cancel'){exit 0}
    if($choice -eq 'No' -or $installed.Count -gt 1){
      $picker=New-Object Windows.Forms.FolderBrowserDialog
      $picker.Description='Choose your existing Freelook folder to update, or a new folder to install. Do not choose the freshly extracted source folder.'
      if(Test-Path -LiteralPath $destination){$picker.SelectedPath=$destination}
      try{if($picker.ShowDialog() -ne 'OK'){exit 0};$destination=$picker.SelectedPath}finally{$picker.Dispose()}
    }
    $plan=Get-FreelookUpdatePlan $PSScriptRoot $destination
    $details=$plan.Mode+' Freelook '+$plan.IncomingVersion+"`r`nDestination: "+$plan.Destination
    if($plan.CurrentVersion){$details+="`r`nInstalled version: "+$plan.CurrentVersion}
    $details+="`r`nReplace "+$plan.Replace.Count+" existing mod files. Keep your settings and saved game location.`r`nExisting files will be backed up; failed replacements will be rolled back.`r`nClose Reclaimer before continuing. Continue?"
    if([Windows.Forms.MessageBox]::Show($details,$title,'YesNo','Question') -ne 'Yes'){exit 0}
    $result=Update-Freelook $PSScriptRoot $destination
    [Windows.Forms.MessageBox]::Show(('Setup complete: '+$result.Version+"`r`nLaunch Play-Reclaimer-with-Freelook.exe in: "+$result.Folder+"`r`nBackup: "+$result.Backup+"`r`nUninstall through Windows Settings or the included Uninstall-Freelook.exe."),$title,'OK','Information') | Out-Null
  }else{
    $answer=[Windows.Forms.MessageBox]::Show('Close Reclaimer before continuing. Remove this copy of Freelook, its settings and logs? Reclaimer and unrelated files will be kept.',$title,'YesNo','Question')
    if($answer -ne 'Yes'){exit 0}
    $preserved=@(Uninstall-Freelook $PSScriptRoot)
    $message='Freelook has been uninstalled.'
    if($preserved.Count){$message+=' Modified package files were kept: '+($preserved -join ', ')}
    if(Test-Path -LiteralPath (Join-Path $PSScriptRoot 'freelook-backups')){$message+=' Update backups were kept in freelook-backups; remove them manually when no longer needed.'}
    [Windows.Forms.MessageBox]::Show($message,$title,'OK','Information') | Out-Null
  }
}catch{
  [Windows.Forms.MessageBox]::Show($_.Exception.Message,$title,'OK','Error') | Out-Null
  exit 1
}
