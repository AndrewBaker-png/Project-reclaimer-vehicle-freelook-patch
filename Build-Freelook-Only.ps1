[CmdletBinding()]
param([string]$ZigPath)
$ErrorActionPreference='Stop'
$root=$PSScriptRoot
$build=Join-Path $root 'build-freelook-mp-r30'
$env:ZIG_GLOBAL_CACHE_DIR=Join-Path $root 'tools\zig-cache'
$env:ZIG_LOCAL_CACHE_DIR=Join-Path $root 'tools\zig-local-cache'
$zig=if($ZigPath){$ZigPath}else{Join-Path $root 'tools\zig-x86_64-windows-0.14.1\zig.exe'}
if(!(Test-Path -LiteralPath $zig)){throw 'Pinned Zig compiler missing; see build-manifest.json.'}
New-Item -ItemType Directory -Path $build -Force | Out-Null
Push-Location $root
try {
  $common=@('-target','x86_64-windows-gnu','-O2','-fno-exceptions','-fno-rtti','-fms-extensions','-DFREELOOK_ONLY','-Isrc','-Ithird_party/minhook/include')
  $minhook=@('third_party/minhook/src/buffer.c','third_party/minhook/src/hook.c','third_party/minhook/src/trampoline.c','third_party/minhook/src/hde/hde64.c')
  $tests=@(
    @{Name='math_tests';Sources=@('tests/math_tests.cpp')},
    @{Name='hook_tests';Sources=@('tests/hook_tests.cpp','tests/hook_test_target.S','src/hooks.S')+$minhook},
    @{Name='camera_copy_tests';Sources=@('tests/camera_copy_tests.cpp')},
    @{Name='freelook_only_tests';Sources=@('-DFREELOOK_LIFTED_HOOKS','tests/freelook_only_tests.cpp','tests/lifted_hook_test_target.S','src/hooks.S')+$minhook},
    @{Name='module_path_tests';Sources=@('tests/module_path_tests.cpp')},
    @{Name='runtime_control_tests';Sources=@('tests/runtime_control_tests.cpp')}
  )
  foreach($test in $tests){
    $exe=Join-Path $build ($test.Name+'.exe')
    & $zig c++ @common @($test.Sources) '-luser32' '-o' $exe
    if($LASTEXITCODE){throw ($test.Name+' build failed')}
    & $exe
    if($LASTEXITCODE){throw ($test.Name+' failed')}
  }
  & $zig c++ @common '-DFREELOOK_LIFTED_HOOKS' '-shared' 'src/freelook_only.cpp' 'src/hooks.S' @minhook '-luser32' '-o' (Join-Path $build 'Halo3VehicleFreelook-only-mp-r30.dll')
  if($LASTEXITCODE){throw 'Freelook-only runtime build failed'}
  & $zig c++ @common 'src/launcher.cpp' '-municode' '-lbcrypt' '-o' (Join-Path $build 'Halo3Freelook.exe')
  if($LASTEXITCODE){throw 'Freelook-only launcher build failed'}
  & $zig c++ @common 'src/play_reclaimer.cpp' '-municode' '-Wl,--subsystem,windows' '-lcomdlg32' '-luser32' '-o' (Join-Path $build 'Play-Reclaimer-with-Freelook.exe')
  if($LASTEXITCODE){throw 'Combined Reclaimer launcher build failed'}
  foreach($action in @('Setup','Uninstall')){
    [string[]]$defines=if($action -eq 'Uninstall'){@('-DFREELOOK_UNINSTALL')}else{@()}
    & $zig c++ @common @defines 'src/maintenance.cpp' '-municode' '-Wl,--subsystem,windows' '-luser32' '-o' (Join-Path $build ($action+'-Freelook.exe'))
    if($LASTEXITCODE){throw ($action+' launcher build failed')}
  }
  & $zig c++ @common 'tests/play_client_fixture.cpp' '-municode' '-Wl,--subsystem,windows' '-o' (Join-Path $build 'play_client_fixture.exe')
  if($LASTEXITCODE){throw 'Combined launcher fixture build failed'}
  foreach($name in @('loader_test_launcher','runtime_lifecycle_tests')){
    & $zig c++ @common ('tests/'+$name+'.cpp') '-municode' '-o' (Join-Path $build ($name+'.exe'))
    if($LASTEXITCODE){throw ($name+' build failed')}
  }
  $powershell=Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
  & $powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $root 'tests\play_launcher_tests.ps1') -BuildDirectory $build
  if($LASTEXITCODE){throw 'Combined Reclaimer launcher tests failed'}
  & $powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $root 'tests\uninstall_tests.ps1') -BuildDirectory $build
  if($LASTEXITCODE){throw 'Uninstaller regression tests failed'}
  & $powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $root 'tests\update_tests.ps1') -BuildDirectory $build
  if($LASTEXITCODE){throw 'Installer update/rollback tests failed'}
  & $powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $root 'tests\loader_tests.ps1') -FixtureExecutable (Join-Path $build 'loader_test_launcher.exe') -WorkDirectory (Join-Path $build 'loader-test-runs')
  if($LASTEXITCODE){throw 'Loader lifecycle regression tests failed'}
  $lifecycle=Join-Path $build ('runtime-test-'+[Guid]::NewGuid().ToString('N'))
  New-Item -ItemType Directory -Path $lifecycle | Out-Null
  $testDll=Join-Path $lifecycle 'Halo3VehicleFreelook-only-mp-r30.dll'
  Copy-Item -LiteralPath (Join-Path $build 'Halo3VehicleFreelook-only-mp-r30.dll') -Destination $testDll
  Copy-Item -LiteralPath 'freelook-only.ini' -Destination (Join-Path $lifecycle 'freelook.ini')
  & (Join-Path $build 'runtime_lifecycle_tests.exe') $testDll 'probe-engine-worker'
  if($LASTEXITCODE){throw 'Actual DLL lifecycle regression test failed'}
  & node (Join-Path $root 'tools\verify-freelook-only.cjs')
  if($LASTEXITCODE){throw 'Freelook-only binary isolation check failed'}
  Copy-Item -LiteralPath 'freelook-only.ini' -Destination (Join-Path $build 'freelook.ini') -Force
  Copy-Item -LiteralPath 'Freelook-Only-MP-README.txt' -Destination (Join-Path $build 'README.txt') -Force
  foreach($name in @('MINHOOK-LICENSE.txt','HCM-LICENSE.txt')){Copy-Item -LiteralPath $name -Destination $build -Force}
  foreach($name in @('Start-Reclaimer-Freelook-Only.ps1','Start-Reclaimer-Freelook-Only.cmd','Stop-Reclaimer-Freelook-Only.cmd','Manage-Freelook.ps1','Add-Freelook-Defender-Exclusion.ps1')){Copy-Item -LiteralPath $name -Destination $build -Force}
  $files=@('Play-Reclaimer-with-Freelook.exe','Halo3Freelook.exe','Halo3VehicleFreelook-only-mp-r30.dll','freelook.ini','README.txt','Start-Reclaimer-Freelook-Only.ps1','Start-Reclaimer-Freelook-Only.cmd','Stop-Reclaimer-Freelook-Only.cmd','MINHOOK-LICENSE.txt','HCM-LICENSE.txt','Setup-Freelook.exe','Uninstall-Freelook.exe','Manage-Freelook.ps1','Add-Freelook-Defender-Exclusion.ps1')
  $hashes=[ordered]@{}
  foreach($name in $files){$hashes[$name]=(Get-FileHash (Join-Path $build $name) -Algorithm SHA256).Hash}
  $manifest=[ordered]@{revision=30;packageVersion='30.4.8';build='r30-installer';profile='F8 freelook only';hooks=@('legacy input X 0x187b89','legacy input Y 0x187bb2','legacy camera 0x17eb20','Reclaimer 0.10.0 native input 0x2f45f05','Reclaimer 0.10.0 lifted camera 0x584efe','Reclaimer 0.10.1 native input 0x30470d5','Reclaimer 0.10.1 lifted camera 0x584ede','Reclaimer 0.10.2 native input 0x37f5e44','Reclaimer 0.10.2 lifted camera 0x5b0785');checks='201 camera bases; 10,000 native hooks; 100,000 camera copies; actual F8 runtime hold/release, orbit/FOV isolation, exit, death, focus, menu lockout, vehicle identity, 128 revisited cameras and complete restoration; bounded module paths; runtime shutdown acknowledgment; PowerShell startup/cancellation/refusal/pending-load cases; actual DLL cancellation during startup; PE imports checked for absence of graphics/network libraries and HUD/mesh dependencies';multiplayer='Huge Grass multiplayer tested on Reclaimer 0.9.23 as reported by the project maintainer. Freelook 30.4.6 works in-game on Reclaimer 0.10.0 and Freelook 30.4.7 works in-game on Reclaimer 0.10.1, as reported by the project maintainer. Freelook 30.4.8 works in-game on Reclaimer 0.10.2, as reported by the project maintainer.';cockpit=$false;visor=$false;hud=$false;graphicsHooks=$false;customAssets=$false;files=$hashes}
  [IO.File]::WriteAllText((Join-Path $build 'manifest.json'),($manifest | ConvertTo-Json -Depth 5)+[Environment]::NewLine)
  $release=Join-Path $root 'release'
  New-Item -ItemType Directory -Path $release -Force | Out-Null
  $zip=Join-Path $release 'Halo3-Freelook-Only-MP-r30-installer.zip'
  $paths=@($files+@('manifest.json') | ForEach-Object {Join-Path $build $_})
  Compress-Archive -LiteralPath $paths -DestinationPath $zip -Force
  Write-Output ('Freelook-only MP package ready: '+$zip)
} finally {Pop-Location}
