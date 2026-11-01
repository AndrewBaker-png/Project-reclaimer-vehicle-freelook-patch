param([Parameter(Mandatory=$true)][string]$BuildDirectory)
$ErrorActionPreference='Stop'
$work=Join-Path $BuildDirectory ('play-test-'+[Guid]::NewGuid().ToString('N'))
$package=Join-Path $work 'mod with spaces'
$game=Join-Path $work 'game with spaces'
New-Item -ItemType Directory -Path $package,$game | Out-Null
Copy-Item -LiteralPath (Join-Path $BuildDirectory 'Play-Reclaimer-with-Freelook.exe') -Destination $package
Copy-Item -LiteralPath (Join-Path $BuildDirectory 'play_client_fixture.exe') -Destination (Join-Path $game 'client.exe')
# The GUI uses its real saved-settings and process-launch paths, but never a real game or injector.
Set-Content -LiteralPath (Join-Path $package 'Halo3Freelook.exe') -Value 'fixture presence check only'
$order=Join-Path $game 'launch-order.txt'
$escapedOrder=$order.Replace("'","''")
Set-Content -LiteralPath (Join-Path $package 'Start-Reclaimer-Freelook-Only.ps1') -Value ("Add-Content -LiteralPath '"+$escapedOrder+"' -Value 'loader'; exit 0")
Set-Content -LiteralPath (Join-Path $package 'reclaimer-launcher.ini') -Value ("[Reclaimer]`r`nClient="+(Join-Path $game 'client.exe'))
foreach($attempt in 1..2){
  $launcher=Start-Process -FilePath (Join-Path $package 'Play-Reclaimer-with-Freelook.exe') -PassThru
  if(!$launcher.WaitForExit(15000)){throw 'Packaged launcher did not exit; check test fixture.'}
  if($launcher.ExitCode -ne 0){throw 'Packaged launcher returned an error.'}
  $deadline=[DateTime]::UtcNow.AddSeconds(10)
  do {
    $lines=@(Get-Content -LiteralPath $order -ErrorAction SilentlyContinue)
    if($lines.Count -eq $attempt*2){break}
    Start-Sleep -Milliseconds 50
  }while([DateTime]::UtcNow -lt $deadline)
  if(($lines -join ',') -ne ((@('loader','client')*$attempt) -join ',')){
    throw 'Expected loader before client on each launch, using the client working directory.'
  }
}
Write-Output 'PASS: packaged GUI launcher remembers client, starts loader before game, handles paths with spaces, and sets game working directory on repeated launches.'
