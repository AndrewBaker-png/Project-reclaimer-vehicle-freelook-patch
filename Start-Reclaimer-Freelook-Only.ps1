[CmdletBinding()]
param([switch]$Stop,[switch]$Worker)
$ErrorActionPreference='Stop'
$launcher=Join-Path $PSScriptRoot 'Halo3Freelook.exe'
if(!(Test-Path -LiteralPath $launcher)){$launcher=Join-Path $PSScriptRoot 'build-freelook-mp-r30\Halo3Freelook.exe'}
if(!(Test-Path -LiteralPath $launcher)){throw 'Build-Freelook-Only.ps1 must be run first.'}
$folder=Split-Path -Parent $launcher
$eventName='Local\Halo3FreelookOnlyMP30LoaderCancel'
$doneEventName=$eventName+'Completed'
$statePath=Join-Path $folder 'loader-state.json'
$logPath=Join-Path $folder 'loader.log'
$completionConfirmed=$true
function Set-LoaderState([string]$Status,[string]$Detail){
  $state=[ordered]@{status=$Status;detail=$Detail;updatedUtc=[DateTime]::UtcNow.ToString('o');loaderPid=$PID;loadMayBePending=(!$completionConfirmed)}
  [IO.File]::WriteAllText($statePath,($state | ConvertTo-Json))
  Add-Content -LiteralPath $logPath -Value ([DateTime]::UtcNow.ToString('o')+' '+$Status+' '+$Detail)
}
if($Stop){
  $cancelled=$false
  $event=$null;$done=$null
  try {
    # Open completion first so it stays alive if the worker exits concurrently.
    try{$done=[Threading.EventWaitHandle]::OpenExisting($doneEventName)}catch [Threading.WaitHandleCannotBeOpenedException]{}
    try{$event=[Threading.EventWaitHandle]::OpenExisting($eventName)}catch [Threading.WaitHandleCannotBeOpenedException]{}
    if($event){
      $event.Set() | Out-Null
      if(!$done -or !$done.WaitOne(45000)){
        Write-Error 'Cancellation requested, but the loader could not confirm completion. A load may still be pending; close the client before trying again.' -ErrorAction Continue
        exit 26
      }
      $cancelled=$true
    }
  } finally {if($done){$done.Dispose()};if($event){$event.Dispose()}}
  & $launcher --reclaimer --stop
  $nativeResult=$LASTEXITCODE
  if($nativeResult -eq 2 -and (Test-Path -LiteralPath $statePath)){
    $previous=Get-Content -Raw -LiteralPath $statePath | ConvertFrom-Json
    if($previous.loadMayBePending){
      Write-Error 'A previous load is still unconfirmed. No stop event is available yet; close the client before trying again.' -ErrorAction Continue
      exit 26
    }
  }
  if($nativeResult -in @(0,1,2)){
    if($cancelled){Write-Output 'Automatic load cancelled; runtime shutdown confirmed or no runtime is present.'}
    exit 0
  }
  exit $nativeResult
}
if(!$Worker){
  try{$pending=[Threading.EventWaitHandle]::OpenExisting($eventName);$pending.Dispose();Write-Output 'Freelook-only loader is already armed.';exit 0}catch [Threading.WaitHandleCannotBeOpenedException]{}
  $arguments='-NoProfile -ExecutionPolicy Bypass -File "'+$PSCommandPath+'" -Worker'
  $child=Start-Process -FilePath (Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe') -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $folder 'loader-console.log') -RedirectStandardError (Join-Path $folder 'loader-error.log')
  $null=$child.Handle
  try {
    $readyDeadline=[DateTime]::UtcNow.AddSeconds(5)
    do {
      try{
        $pending=[Threading.EventWaitHandle]::OpenExisting($eventName);$pending.Dispose()
        Write-Output ('Freelook-only loader armed (PID '+$child.Id+'). It will wait up to 30 minutes for Reclaimer/Halo 3; see '+$statePath)
        exit 0
      }catch [Threading.WaitHandleCannotBeOpenedException]{}
      if($child.HasExited){$child.WaitForExit();exit $child.ExitCode}
      Start-Sleep -Milliseconds 20
    }while([DateTime]::UtcNow -lt $readyDeadline)
    throw 'Loader startup could not be confirmed; check loader-error.log.'
  } finally {$child.Dispose()}
}
$done=[Threading.EventWaitHandle]::new($false,[Threading.EventResetMode]::ManualReset,$doneEventName)
$created=$false
$event=[Threading.EventWaitHandle]::new($false,[Threading.EventResetMode]::ManualReset,$eventName,[ref]$created)
if(!$created){$done.Dispose();$event.Dispose();exit 0}
$done.Reset() | Out-Null
$completionConfirmed=$true
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
    $completionConfirmed=$false
    Set-LoaderState 'loading' 'Verified engine ready; loading isolated F8 runtime.'
    $output=@(& $launcher --reclaimer 2>&1)
    $result=$LASTEXITCODE
    # Exit 15 includes a remote LoadLibrary thread that may still be running.
    # A crash or unknown exit also cannot acknowledge that loading has ended.
    $completionConfirmed=$result -in (@(0..14)+@(16,17,20,21,22,24,25))
    Add-Content -LiteralPath $logPath -Value ($output -join [Environment]::NewLine)
    if($event.WaitOne(0)){
      $stopOutput=@(& $launcher --reclaimer --stop 2>&1)
      $stopResult=$LASTEXITCODE
      Add-Content -LiteralPath $logPath -Value ($stopOutput -join [Environment]::NewLine)
      if($stopResult -eq 0 -or ($completionConfirmed -and $result -ne 0 -and $stopResult -in @(1,2))){
        $completionConfirmed=$true
        Set-LoaderState 'cancelled' 'Automatic load cancelled; runtime shutdown confirmed or no runtime was loaded.'
        exit 0
      }
      $completionConfirmed=$false
      Set-LoaderState 'error' 'Cancellation requested, but runtime shutdown could not be confirmed. Close the client before trying again.'
      exit 26
    }
    if($result -eq 0){Set-LoaderState 'loaded' 'Freelook-only DLL loaded. Check freelook.log for runtime activation; hold F8 in a vehicle.'}
    else{Set-LoaderState 'refused' ('Loader refused or failed; exit '+$result+'. See loader.log.')}
    exit $result
  }
  Set-LoaderState 'expired' 'No ready engine within 30 minutes; run Start-Reclaimer-Freelook-Only again.'
} catch {Set-LoaderState 'error' $_.Exception.Message;exit 1}
finally {if($completionConfirmed){$done.Set() | Out-Null};$done.Dispose();$event.Dispose()}
