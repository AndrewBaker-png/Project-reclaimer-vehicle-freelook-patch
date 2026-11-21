[CmdletBinding()]
param([Parameter(Mandatory)][string]$Client,[Parameter(Mandatory)][string]$MccPath)
$ErrorActionPreference='Stop'
if(Get-Process | Where-Object {$_.ProcessName -like 'project-reclaimer-client*'}){throw 'Another Reclaimer process is running; isolated smoke test refused.'}
if((Get-FileHash -LiteralPath $Client -Algorithm SHA256).Hash -ne '547fa26921fbe22d5279e41a18eb91b5825f744593d08d50736eac476a901582'){throw 'Unexpected client hash.'}
$root=Join-Path $PSScriptRoot ('smoke-0108-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $root | Out-Null
$env:RECLAIMER_PROFILE_DIR=Join-Path $root 'profile'
$env:RECLAIMER_CACHE_DIR=(Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) 'ProjectReclaimer/Cache')
$env:RECLAIMER_DISCOVERY_DIR=Join-Path $root 'discovery'
$env:RECLAIMER_VIDEOS_DIR=Join-Path $root 'videos'
$build=Join-Path (Split-Path -Parent $PSScriptRoot) 'build-freelook-mp-r30'
foreach($file in @('Halo3Freelook.exe','Halo3VehicleFreelook-only-mp-r30.dll','freelook.ini')){Copy-Item -LiteralPath (Join-Path $build $file) -Destination $root}
$loader=Join-Path $root 'Halo3Freelook.exe'
$test=Start-Process -FilePath $Client -ArgumentList ('"' + (Resolve-Path -LiteralPath $MccPath).Path + '" --headless --map hugeass_remastered_mcc --seconds 60') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $root 'client-output.log') -RedirectStandardError (Join-Path $root 'client-error.log')
$loaded=$false
try {
  $deadline=[DateTime]::UtcNow.AddSeconds(40)
  do {
    if($test.HasExited){throw 'Test client exited before its engine became ready.'}
    $probe=@(& $loader --reclaimer --probe 2>&1);$result=$LASTEXITCODE
    if($result -eq 0){break}
    if($result -ne 1){throw ($probe -join "`n")}
    Start-Sleep -Milliseconds 500
  }while([DateTime]::UtcNow -lt $deadline)
  if($result -ne 0 -or ($probe -join ' ') -notmatch ('Verified Reclaimer 0\.10\.8 PID '+$test.Id+',')){throw 'Probe did not identify the owned 0.10.8 test process.'}
  $probe | Set-Content (Join-Path $root 'probe.txt')
  & $loader --reclaimer
  if($LASTEXITCODE){throw 'Runtime load failed.'}
  $loaded=$true
  $testIni=Join-Path $root 'freelook.ini'
  $iniText=Get-Content -Raw -LiteralPath $testIni
  [IO.File]::WriteAllText($testIni,($iniText -replace 'HoldVirtualKey=119','HoldVirtualKey=5'))
  Start-Sleep -Seconds 12
  $runtime=Get-Content -LiteralPath (Join-Path $root 'freelook.log') -Raw
  if($runtime -notmatch 'ACTIVE: Reclaimer 0.10.8 native input and render-output-only camera'){throw 'Runtime activation not confirmed.'}
  if($runtime -notmatch 'hold binding changed to 5'){throw 'Live rebind was not observed.'}
  $samples=[regex]::Matches($runtime,'input=(\d+) camera=(\d+).*cameraWrites=(\d+) renderObserverMatches=(\d+)')
  if(!$samples.Count){throw 'No runtime sample.'}
  $sample=$samples[$samples.Count-1]
  $inputs=[long]$sample.Groups[1].Value;$frames=[long]$sample.Groups[2].Value;$writes=[long]$sample.Groups[3].Value;$matched=[long]$sample.Groups[4].Value
  if($frames -le 0 -or $inputs -le 0 -or $matched -le 0){throw 'Input/render bridge/primary observer was not exercised.'}
  if($writes -ne 0){throw 'Unexpected idle camera write.'}
  & $loader --reclaimer --stop
  if($LASTEXITCODE){throw 'Runtime stop was not acknowledged.'}
  $loaded=$false
  if(!$test.WaitForExit(55000)){throw 'Timed client did not exit.'}
  if($test.ExitCode -ne 0){throw ('Test client failed: '+$test.ExitCode)}
  $report=[ordered]@{build='30.7.1';client='0.10.8';clientSha256=(Get-FileHash -LiteralPath $Client).Hash.ToLower();map='Hugegrass original';inputCallbacks=$inputs;renderCallbacks=$frames;primaryObserverMatches=$matched;idleCameraWrites=$writes;liveRebindObserved=$true;shutdownAcknowledged=$true;clientExitCode=$test.ExitCode;f8GameplayTested=$false;multiplayerTested=$false;job=$root;utc=[DateTime]::UtcNow.ToString('o')}
  $report | ConvertTo-Json | Set-Content (Join-Path $PSScriptRoot 'smoke-0108-result.json')
  $report | ConvertTo-Json
} finally {
  if($loaded -and !$test.HasExited){& $loader --reclaimer --stop | Out-Null}
  # Only this test's process tree is owned by the script.
  if(!$test.HasExited){& taskkill.exe /PID $test.Id /T /F | Out-Null}
}
