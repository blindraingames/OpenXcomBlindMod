param(
    [string]$BuildDirectory = "obj/accessibility-ninja/bin",
    [switch]$Smoke
)

$ErrorActionPreference = "Stop"
$repository = Split-Path -Parent $PSScriptRoot
$failures = [System.Collections.Generic.List[string]]::new()

function Add-Failure([string]$Message) {
    $script:failures.Add($Message)
}

$sourceFiles = Get-ChildItem -LiteralPath (Join-Path $repository "src") -Recurse -Include *.cpp,*.h
$codeKeys = $sourceFiles |
    Select-String -Pattern '"(STR_ACCESSIBILITY_[A-Z0-9_]+)"' -AllMatches |
    ForEach-Object { $_.Matches } |
    ForEach-Object { $_.Groups[1].Value } |
    Sort-Object -Unique

foreach ($language in @("en-US", "en-GB", "ru")) {
    $languagePath = Join-Path $repository "bin/common/Language/$language.yml"
    $languageKeys = Get-Content -LiteralPath $languagePath |
        ForEach-Object { if ($_ -match '^  (STR_ACCESSIBILITY_[A-Z0-9_]+):') { $Matches[1] } } |
        Sort-Object -Unique
    $missing = Compare-Object -ReferenceObject $languageKeys -DifferenceObject $codeKeys -PassThru |
        Where-Object { $_ -in $codeKeys }
    if ($missing) {
        Add-Failure "$language is missing: $($missing -join ', ')"
    }
}

$directTolk = $sourceFiles |
    Where-Object { $_.Name -notin @("TolkBridge.cpp", "TolkBridge.h") } |
    Select-String -Pattern '\bTolk_(Load|Unload|Output|Speak|Silence|DetectScreenReader)\b'
if ($directTolk) {
    Add-Failure "Direct Tolk calls exist outside TolkBridge."
}

$cmake = Get-Content -LiteralPath (Join-Path $repository "src/CMakeLists.txt") -Raw
Get-ChildItem -LiteralPath (Join-Path $repository "src/Accessibility") -Filter *.cpp | ForEach-Object {
    if ($cmake -notmatch [regex]::Escape("Accessibility/$($_.Name)")) {
        Add-Failure "$($_.Name) is not listed in src/CMakeLists.txt."
    }
}

foreach ($relative in @(
    "deps/lib/x64/Tolk.dll",
    "deps/lib/x64/nvdaControllerClient64.dll",
    "deps/lib/Win32/Tolk.dll",
    "deps/lib/Win32/nvdaControllerClient32.dll"
)) {
    if (-not (Test-Path -LiteralPath (Join-Path $repository $relative))) {
        Add-Failure "Missing runtime dependency: $relative"
    }
}

$dumpbin = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
if ($dumpbin) {
    foreach ($relative in @("deps/lib/x64/Tolk.dll", "deps/lib/Win32/Tolk.dll")) {
        $exports = (& $dumpbin.Source /nologo /exports (Join-Path $repository $relative) 2>$null) -join "`n"
        foreach ($symbol in @("Tolk_Load", "Tolk_Unload", "Tolk_Output", "Tolk_Silence", "Tolk_DetectScreenReader")) {
            if ($exports -notmatch "\b$symbol\b") {
                Add-Failure "$relative does not export $symbol."
            }
        }
    }
}

if ($Smoke) {
    $buildRoot = Join-Path $repository $BuildDirectory
    foreach ($language in @("en-US", "en-GB", "ru")) {
        $builtLanguagePath = Join-Path $buildRoot "common/Language/$language.yml"
        if (-not (Test-Path -LiteralPath $builtLanguagePath)) {
            Add-Failure "Built language file not found: $builtLanguagePath"
            continue
        }
        $builtLanguageKeys = Get-Content -LiteralPath $builtLanguagePath |
            ForEach-Object { if ($_ -match '^  (STR_ACCESSIBILITY_[A-Z0-9_]+):') { $Matches[1] } } |
            Sort-Object -Unique
        $missingBuiltKeys = Compare-Object -ReferenceObject $builtLanguageKeys -DifferenceObject $codeKeys -PassThru |
            Where-Object { $_ -in $codeKeys }
        if ($missingBuiltKeys) {
            Add-Failure "Built $language is missing: $($missingBuiltKeys -join ', ')"
        }
    }
    $executable = Join-Path $buildRoot "openxcom.exe"
    if (-not (Test-Path -LiteralPath $executable)) {
        Add-Failure "Smoke-test executable not found: $executable"
    }
    else {
        foreach ($test in @(
            @{ Name = "validation-noaccess"; Flag = "--no-accessibility" },
            @{ Name = "validation-access"; Flag = "--accessibility" }
        )) {
            $testDirectory = Join-Path $buildRoot $test.Name
            New-Item -ItemType Directory -Path $testDirectory -Force | Out-Null
            $arguments = @($test.Flag, "-playIntro", "false", "-language", "ru", "-user", $test.Name, "-cfg", $test.Name)
            $process = Start-Process -FilePath $executable -WorkingDirectory $buildRoot -WindowStyle Hidden -ArgumentList $arguments -PassThru
            Start-Sleep -Seconds 6
            if ($process.HasExited) {
                Add-Failure "$($test.Name) exited early with code $($process.ExitCode)."
            }
            else {
                Stop-Process -Id $process.Id -Force
                $process.WaitForExit()
            }
            $logPath = Join-Path $testDirectory "openxcom.log"
            if (-not (Test-Path -LiteralPath $logPath) -or (Get-Content -LiteralPath $logPath -Raw) -notmatch "OpenXcom started successfully!") {
                Add-Failure "$($test.Name) did not reach a successful startup."
            }
        }
    }
}

if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Output "ERROR: $_" }
    exit 1
}

Write-Output "Accessibility validation passed: $($codeKeys.Count) localized keys, isolated Tolk bridge, CMake coverage, x86/x64 runtime files$(if ($Smoke) { ', and two startup smoke tests' })."
