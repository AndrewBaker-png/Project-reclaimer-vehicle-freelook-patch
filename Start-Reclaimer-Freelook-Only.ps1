[CmdletBinding()]
param([switch]$Stop,[switch]$Worker)
$ErrorActionPreference='Stop'
$launcher=Join-Path $PSScriptRoot 'Halo3Freelook.exe'
if(!(Test-Path -LiteralPath $launcher)){$launcher=Join-Path $PSScriptRoot 'build-freelook-mp-r30\Halo3Freelook.exe'}
if(!(Test-Path -LiteralPath $launcher)){throw 'Build-Freelook-Only.ps1 must be run first.'}
$folder=Split-Path -Parent $launcher
$eventName='Local\Halo3FreelookOnlyMP30LoaderCancel'
$statePath=Join-Path $folder 'loader-state.json'
$logPath=Join-Path $folder 'loader.log'
function Set-LoaderState([string]$Status,[string]$Detail){
  $state=[ordered]@{status=$Status;detail=$Detail;updatedUtc=[DateTime]::UtcNow.ToString('o');loaderPid=$PID}
  [IO.File]::WriteAllText($statePath,($state | ConvertTo-Json))
  Add-Content -LiteralPath $logPath -Value ([DateTime]::UtcNow.ToString('o')+' '+$Status+' '+$Detail)
}
if($Stop){
  $cancelled=$false
  try{$event=[Threading.EventWaitHandle]::OpenExisting($eventName);$event.Set() | Out-Null;$event.Dispose();$cancelled=$true}catch [Threading.WaitHandleCannotBeOpenedException]{}
  & $launcher --reclaimer --stop
  $nativeResult=$LASTEXITCODE
  if($cancelled){Write-Output 'Pending automatic load cancelled.'}
  if($nativeResult -in @(0,1,2)){exit 0}
  exit $nativeResult
}
if(!$Worker){
  try{$pending=[Threading.EventWaitHandle]::OpenExisting($eventName);$pending.Dispose();Write-Output 'Freelook-only loader is already armed.';exit 0}catch [Threading.WaitHandleCannotBeOpenedException]{}
  $arguments='-NoProfile -ExecutionPolicy Bypass -File "'+$PSCommandPath+'" -Worker'
  $child=Start-Process -FilePath (Join-Path $PSHOME 'powershell.exe') -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $folder 'loader-console.log') -RedirectStandardError (Join-Path $folder 'loader-error.log')
  Write-Output ('Freelook-only loader armed (PID '+$child.Id+'). It will wait up to 30 minutes for Reclaimer/Halo 3; see '+$statePath)
  exit 0
}
$created=$false
$event=[Threading.EventWaitHandle]::new($false,[Threading.EventResetMode]::ManualReset,$eventName,[ref]$created)
if(!$created){$event.Dispose();exit 0}
try {
  Set-LoaderState 'waiting' 'Waiting for Reclaimer Halo 3 engine; no game memory changed yet.'
  $deadline=[DateTime]::UtcNow.AddMinutes(30)
  while([DateTime]::UtcNow -lt $deadline){
    if($event.WaitOne(500)){Set-LoaderState 'cancelled' 'Automatic load cancelled.';exit 0}
    $probe=@(& $launcher --reclaimer --probe 2>&1)
    $result=$LASTEXITCODE
    if($result -eq 1){continue}
    Add-Content -LiteralPath $logPath -Value ($probe -join [Environment]::NewLine)
    if($result -ne 0){Set-LoaderState 'refused' ('Compatibility probe refused; exit '+$result+'. No injection attempted.');exit $result}
    if($event.WaitOne(0)){Set-LoaderState 'cancelled' 'Automatic load cancelled before injection.';exit 0}
    Set-LoaderState 'loading' 'Verified engine ready; loading isolated F8 runtime.'
    $output=@(& $launcher --reclaimer 2>&1)
    $result=$LASTEXITCODE
    Add-Content -LiteralPath $logPath -Value ($output -join [Environment]::NewLine)
    if($result -eq 0){Set-LoaderState 'loaded' 'Freelook-only DLL loaded. Check freelook.log for runtime activation; hold F8 in a vehicle.'}
    else{Set-LoaderState 'refused' ('Loader refused or failed; exit '+$result+'. See loader.log.')}
    exit $result
  }
  Set-LoaderState 'expired' 'No ready engine within 30 minutes; run Start-Reclaimer-Freelook-Only again.'
} catch {Set-LoaderState 'error' $_.Exception.Message;exit 1}
finally {$event.Dispose()}
