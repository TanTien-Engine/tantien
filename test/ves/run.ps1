# Run the headless .ves test suite.
#
#   pwsh test/ves/run.ps1                 # run all drivers
#   pwsh test/ves/run.ps1 test_topo_sort  # run specific driver(s)
#   VES_TEST_EXE=<path> ...               # test a non-default build artifact
#
# Must run from the repo root (the .ves loader resolves imports via relative
# paths like src/script/ and test/ves/). Puts the OCCT + 3rdparty runtime DLLs
# on PATH the same way the editor needs them; ves_test.exe links the same stack
# but stages no DLLs of its own.

$ErrorActionPreference = "Stop"
$repo = (Resolve-Path "$PSScriptRoot\..\..").Path
Set-Location $repo

$defaultExe = Join-Path $repo "out\build\Release\ves_test.exe"
$exe = if ([string]::IsNullOrWhiteSpace($env:VES_TEST_EXE)) {
    $defaultExe
} else {
    $env:VES_TEST_EXE
}
if (-not (Test-Path -LiteralPath $exe)) {
    Write-Error "ves_test.exe not found at $exe — build the ves_test target first:`n  cmake --build out/build --config Release --target ves_test"
}

# The editor build stages the full OCCT/tbb/jemalloc/freetype/freeimage DLL set
# next to editor.exe; reuse that directory. Config-agnostic DLLs, so a Release
# exe loads them fine.
$dllDir = Join-Path $repo "out\RelWithDebInfo"
if (Test-Path $dllDir) {
    $env:PATH = "$dllDir;$env:PATH"
} else {
    # Fall back to the SDK bin dirs directly.
    $occt = Join-Path $repo "sdks\occt-vc144-64\win64\vc14\bin"
    $tp   = Join-Path $repo "sdks\3rdparty-vc14-64"
    $tbb  = (Get-ChildItem "$tp\tbb-*\bin" -Directory | Select-Object -First 1).FullName
    $env:PATH = "$occt;$tbb;$tp\jemalloc-vc14-64\bin;$tp\freetype-2.13.3-x64\bin;$tp\freeimage-3.18.0-x64\bin;$env:PATH"
}

[string[]]$drivers = if ($args.Count -gt 0) { $args } else { @("test_topo_sort", "test_reachability", "test_tree_build") }

& $exe @drivers
exit $LASTEXITCODE
