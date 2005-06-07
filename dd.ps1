<#
.SYNOPSIS
    Build/run/test driver for fast-draw.

.DESCRIPTION
    Sets up the MSVC x64 environment, configures CMake on demand, and drives the
    common tasks.

.EXAMPLE
    .\dd.ps1 build            # build release (default)
    .\dd.ps1 build debug      # build debug
    .\dd.ps1 run              # build release, then launch the app
    .\dd.ps1 run drawing.fdr  # ... and open a file
    .\dd.ps1 test             # build and run the test suite
    .\dd.ps1 rebuild          # configure from scratch, then build
    .\dd.ps1 clean            # delete the build tree
#>
[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [ValidateSet('build', 'rebuild', 'run', 'test', 'clean', 'help')]
    [string]$Command = 'build',

    [Parameter(Position = 1, ValueFromRemainingArguments = $true)]
    [string[]]$Rest = @()
)

$ErrorActionPreference = 'Stop'
# Without this, any stderr output from cmake/ninja becomes a terminating error
# and the real diagnostics are hidden behind a PowerShell exception.
$PSNativeCommandUseErrorActionPreference = $false
Set-StrictMode -Version Latest

$RepoRoot = $PSScriptRoot
$VsRoot = 'C:\Program Files\Microsoft Visual Studio\18\Enterprise'
$CMake = Join-Path $VsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$CTest = Join-Path $VsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe'
$VcVars = Join-Path $VsRoot 'VC\Auxiliary\Build\vcvars64.bat'

function Show-Help {
    Get-Help $PSCommandPath -Detailed | Out-String | Write-Host
}

# vcvars64.bat silently does nothing when VSCMD_VER is already set, which leaves
# a half-configured environment (cl.exe but no rc.exe), so clear it first.
function Enter-MsvcEnvironment {
    if ($env:FAST_DRAW_MSVC_READY -eq '1') { return }

    if (-not (Test-Path $VcVars)) { throw "vcvars64.bat not found at $VcVars" }

    Remove-Item Env:VSCMD_VER, Env:VSCMD_ARG_TGT_ARCH, Env:VSCMD_ARG_HOST_ARCH `
        -ErrorAction SilentlyContinue

    cmd /c "call `"$VcVars`" > nul 2>&1 && set" | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$') {
            Set-Item -Path "Env:$($matches[1])" -Value $matches[2] -ErrorAction SilentlyContinue
        }
    }

    $env:FAST_DRAW_MSVC_READY = '1'
}

function Resolve-Preset([string[]]$arguments) {
    foreach ($argument in $arguments) {
        switch ($argument.ToLowerInvariant()) {
            'debug' { return 'x64-debug' }
            'release' { return 'x64-release' }
        }
    }
    return 'x64-release'
}

function Remove-PresetArguments([string[]]$arguments) {
    $filtered = [string[]]@()
    foreach ($argument in $arguments) {
        if ($argument -notin @('debug', 'release')) { $filtered += $argument }
    }
    # The unary comma stops PowerShell unrolling an empty array to $null.
    return , $filtered
}

function Get-BuildDir([string]$preset) {
    return Join-Path $RepoRoot "build\$preset"
}

function Invoke-Build([string]$preset) {
    Enter-MsvcEnvironment
    if (-not (Test-Path $CMake)) { throw "cmake.exe not found at $CMake" }

    # Always reconfigure: deleting a source file breaks ninja's dependency graph
    # before it gets a chance to regenerate itself. The output is only worth
    # showing when it fails - third-party subprojects are noisy.
    $configureLog = & $CMake --preset $preset 2>&1
    if ($LASTEXITCODE -ne 0) {
        $configureLog | ForEach-Object { Write-Host $_ }
        throw "cmake configure failed ($preset)"
    }

    & $CMake --build --preset $preset
    if ($LASTEXITCODE -ne 0) { throw "build failed ($preset)" }
}

function Get-Exe([string]$preset, [string]$name) {
    $path = Join-Path (Get-BuildDir $preset) $name
    if (-not (Test-Path $path)) { throw "$name not found at $path" }
    return $path
}

switch ($Command) {
    'help' {
        Show-Help
    }

    'clean' {
        $buildRoot = Join-Path $RepoRoot 'build'
        if (Test-Path $buildRoot) {
            Remove-Item -Recurse -Force $buildRoot
            Write-Host "removed $buildRoot"
        }
        else {
            Write-Host 'nothing to clean'
        }
    }

    'rebuild' {
        $preset = Resolve-Preset $Rest
        $buildDir = Get-BuildDir $preset
        if (Test-Path $buildDir) { Remove-Item -Recurse -Force $buildDir }
        Invoke-Build $preset
    }

    'build' {
        Invoke-Build (Resolve-Preset $Rest)
    }

    'run' {
        $preset = Resolve-Preset $Rest
        Invoke-Build $preset

        $exe = Get-Exe $preset 'fast-draw.exe'
        $runArgs = Remove-PresetArguments $Rest

        if ($runArgs.Count -gt 0) {
            Write-Host "running $exe $($runArgs -join ' ')"
            & $exe @runArgs
        }
        else {
            Write-Host "running $exe"
            & $exe
        }
        exit $LASTEXITCODE
    }

    'test' {
        $preset = Resolve-Preset $Rest
        Invoke-Build $preset

        $code = 1
        Push-Location (Get-BuildDir $preset)
        try {
            & $CTest --output-on-failure @(Remove-PresetArguments $Rest)
            $code = $LASTEXITCODE
        }
        finally {
            Pop-Location
        }
        exit $code
    }
}
