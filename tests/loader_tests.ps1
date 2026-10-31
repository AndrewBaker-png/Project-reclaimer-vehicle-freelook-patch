[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$FixtureExecutable,[Parameter(Mandatory=$true)][string]$WorkDirectory)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$ps=Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
$processes=[Collections.Generic.List[Diagnostics.Process]]::new()
function Require([bool]$Condition,[string]$Detail){if(!$Condition){throw ('FAIL: '+$Detail)}}
function Await([scriptblock]$Condition,[string]$Detail){
  $deadline=[DateTime]::UtcNow.AddSeconds(10)
  while(!(& $Condition)){if([DateTime]::UtcNow -gt $deadline){throw ('TIMEOUT: '+$Detail)};Start-Sleep -Milliseconds 10}
}
function Touch([string]$Folder,[string]$Name){[IO.File]::WriteAllText((Join-Path $Folder $Name),'')}
function New-Case([string]$Mode){
  $folder=Join-Path $WorkDirectory ('loader '+$Mode+' '+[Guid]::NewGuid().ToString('N'))
  New-Item -ItemType Directory -Path $folder -Force | Out-Null
  Copy-Item -LiteralPath $FixtureExecutable -Destination (Join-Path $folder 'Halo3Freelook.exe')
  [IO.File]::WriteAllText((Join-Path $folder 'mode.txt'),$Mode)
  $name='Local\FreelookLoaderRegression-'+[Guid]::NewGuid().ToString('N')
  # Only replace the event namespace; execute the production control flow.
  $source=(Get-Content -Raw -LiteralPath (Join-Path $root 'Start-Reclaimer-Freelook-Only.ps1')).Replace('Local\Halo3FreelookOnlyMP30LoaderCancel',$name)
  $script=Join-Path $folder 'Start-Reclaimer-Freelook-Only.ps1'
  [IO.File]::WriteAllText($script,$source)
  return [pscustomobject]@{Folder=$folder;Script=$script;Event=$name}
}
function Launch($Case,[string]$Mode){
  $arguments='-NoProfile -ExecutionPolicy Bypass -File "'+$Case.Script+'" '+$Mode
  $label=if($Mode){$Mode.TrimStart('-')}else{'start'}
  $process=Start-Process -FilePath $ps -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $Case.Folder ($label+'.out.log')) -RedirectStandardError (Join-Path $Case.Folder ($label+'.err.log'))
  $null=$process.Handle
  $processes.Add($process);return $process
}
function Finished($Process,[int]$Expected,[string]$Detail){
  Require ($Process.WaitForExit(15000)) ($Detail+' exits');$Process.WaitForExit()
  Require ($Process.ExitCode -eq $Expected) ($Detail+' exit '+$Expected+', got '+$Process.ExitCode)
}
function State($Case){return Get-Content -Raw -LiteralPath (Join-Path $Case.Folder 'loader-state.json') | ConvertFrom-Json}
try {
  $case=New-Case 'waiting'
  $start=Launch $case '';Finished $start 0 'start helper'
  Await {Test-Path -LiteralPath (Join-Path $case.Folder 'loader-state.json')} 'waiting state'
  $pidToWait=(State $case).loaderPid
  $worker=[Diagnostics.Process]::GetProcessById($pidToWait);$processes.Add($worker)
  $null=$worker.Handle
  $duplicate=Launch $case '';Finished $duplicate 0 'duplicate start'
  Require ((State $case).loaderPid -eq $pidToWait) 'duplicate start keeps the original worker'
  $stop=Launch $case '-Stop';Finished $stop 0 'cancel waiting loader';Finished $worker 0 'waiting worker'
  Require ((State $case).status -eq 'cancelled') 'waiting cancellation recorded'
  Require (!(Test-Path -LiteralPath (Join-Path $case.Folder 'injection-started.marker'))) 'waiting cancellation never injects'

  $case=New-Case 'delayed'
  $worker=Launch $case '-Worker'
  Await {Test-Path -LiteralPath (Join-Path $case.Folder 'injection-started.marker')} 'delayed load starts'
  $cancel=[Threading.EventWaitHandle]::OpenExisting($case.Event)
  try {
    $stop=Launch $case '-Stop'
    Require ($cancel.WaitOne(5000)) 'Stop signals pending cancellation'
    Require (!$stop.HasExited) 'Stop does not finish while loading is blocked'
    Touch $case.Folder 'allow-load.marker'
    Await {Test-Path -LiteralPath (Join-Path $case.Folder 'stop-started.marker')} 'worker requests runtime shutdown'
    Require (!$stop.HasExited) 'Stop waits for runtime shutdown acknowledgment'
    Touch $case.Folder 'allow-stop.marker'
    Finished $stop 0 'in-flight cancellation';Finished $worker 0 'cancelled worker'
  } finally {$cancel.Dispose()}
  Require ((State $case).status -eq 'cancelled') 'in-flight cancellation is not recorded as loaded'
  Require (!(Test-Path -LiteralPath (Join-Path $case.Folder 'active.marker'))) 'no runtime remains after Stop succeeds'

  $case=New-Case 'normal';Touch $case.Folder 'allow-load.marker';Touch $case.Folder 'allow-stop.marker'
  $worker=Launch $case '-Worker';Finished $worker 0 'normal load'
  Require ((State $case).status -eq 'loaded') 'normal load is preserved'
  $stop=Launch $case '-Stop';Finished $stop 0 'stop already loaded runtime'
  Require (!(Test-Path -LiteralPath (Join-Path $case.Folder 'active.marker'))) 'loaded runtime stopped'

  $case=New-Case 'refused'
  $worker=Launch $case '-Worker';Finished $worker 6 'compatibility refusal'
  Require ((State $case).status -eq 'refused') 'compatibility refusal recorded'
  Require (!(Test-Path -LiteralPath (Join-Path $case.Folder 'injection-started.marker'))) 'refusal never injects'

  $case=New-Case 'pending'
  $worker=Launch $case '-Worker';Finished $worker 15 'indeterminate native load'
  Require ((State $case).loadMayBePending) 'pending load survives worker exit in state file'
  $stop=Launch $case '-Stop';Finished $stop 26 'unconfirmed load is never reported cancelled'
  Write-Output 'PASS: loader startup/duplicate, waiting cancel, in-flight cancel with shutdown acknowledgment, normal load/stop, refusal and indeterminate load'
} finally {
  foreach($process in $processes){
    try {if(!$process.HasExited){$process.Kill();$process.WaitForExit(2000) | Out-Null}} finally {$process.Dispose()}
  }
}
