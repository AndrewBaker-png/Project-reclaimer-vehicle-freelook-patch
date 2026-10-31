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
    @{Name='freelook_only_tests';Sources=@('tests/freelook_only_tests.cpp','src/hooks.S')+$minhook}
  )
  foreach($test in $tests){
    $exe=Join-Path $build ($test.Name+'.exe')
    & $zig c++ @common @($test.Sources) '-luser32' '-o' $exe
    if($LASTEXITCODE){throw ($test.Name+' build failed')}
    & $exe
    if($LASTEXITCODE){throw ($test.Name+' failed')}
  }
  & $zig c++ @common '-shared' 'src/freelook_only.cpp' 'src/hooks.S' @minhook '-luser32' '-o' (Join-Path $build 'Halo3VehicleFreelook-only-mp-r30.dll')
  if($LASTEXITCODE){throw 'Freelook-only runtime build failed'}
  & $zig c++ @common 'src/launcher.cpp' '-municode' '-lbcrypt' '-o' (Join-Path $build 'Halo3Freelook.exe')
  if($LASTEXITCODE){throw 'Freelook-only launcher build failed'}
  & node (Join-Path $root 'tools\verify-freelook-only.cjs')
  if($LASTEXITCODE){throw 'Freelook-only binary isolation check failed'}
  Copy-Item -LiteralPath 'freelook-only.ini' -Destination (Join-Path $build 'freelook.ini') -Force
  Copy-Item -LiteralPath 'Freelook-Only-MP-README.txt' -Destination (Join-Path $build 'README.txt') -Force
  foreach($name in @('MINHOOK-LICENSE.txt','HCM-LICENSE.txt')){Copy-Item -LiteralPath $name -Destination $build -Force}
  foreach($name in @('Start-Reclaimer-Freelook-Only.ps1','Start-Reclaimer-Freelook-Only.cmd','Stop-Reclaimer-Freelook-Only.cmd')){Copy-Item -LiteralPath $name -Destination $build -Force}
  $files=@('Halo3Freelook.exe','Halo3VehicleFreelook-only-mp-r30.dll','freelook.ini','README.txt','Start-Reclaimer-Freelook-Only.ps1','Start-Reclaimer-Freelook-Only.cmd','Stop-Reclaimer-Freelook-Only.cmd','MINHOOK-LICENSE.txt','HCM-LICENSE.txt')
  $hashes=[ordered]@{}
  foreach($name in $files){$hashes[$name]=(Get-FileHash (Join-Path $build $name) -Algorithm SHA256).Hash}
  $manifest=[ordered]@{revision=30;profile='F8 freelook only';hooks=@('input X 0x187b89','input Y 0x187bb2','camera 0x17eb20');checks='201 camera bases; 10,000 native hooks; 100,000 camera copies; actual F8 runtime hold/release, orbit/FOV isolation, exit, death, focus, menu lockout, vehicle identity and camera turnover; PE imports checked for absence of graphics/network libraries and HUD/mesh dependencies';multiplayer='Not yet tested with a second player';cockpit=$false;visor=$false;hud=$false;graphicsHooks=$false;customAssets=$false;files=$hashes}
  [IO.File]::WriteAllText((Join-Path $build 'manifest.json'),($manifest | ConvertTo-Json -Depth 5)+[Environment]::NewLine)
  $release=Join-Path $root 'release'
  New-Item -ItemType Directory -Path $release -Force | Out-Null
  $zip=Join-Path $release 'Halo3-Freelook-Only-MP-r30.zip'
  $paths=@($files+@('manifest.json') | ForEach-Object {Join-Path $build $_})
  Compress-Archive -LiteralPath $paths -DestinationPath $zip -Force
  Write-Output ('Freelook-only MP package ready: '+$zip)
} finally {Pop-Location}
