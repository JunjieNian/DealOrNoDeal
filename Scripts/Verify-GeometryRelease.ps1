param([string]$Package = 'Builds\DealOrNoDealStage-Studio-0.4.2\Windows')
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'Verify-Cam4Package.ps1') -Package $Package
$studioRoot = Split-Path $PSScriptRoot -Parent
$launcher = Join-Path $studioRoot ($Package + '\DealOrNoDealStage.exe')
$shots = @(
    @{Name='Geometry-Wide'; Camera='-StageCamera=0'},
    @{Name='Geometry-Cases'; Camera='-StageCamera=2'},
    @{Name='Geometry-Board'; Camera='-StageCamera=3'},
    @{Name='Geometry-LeftStairs'; Camera='-StageInspectSide=-1'},
    @{Name='Geometry-RightStairs'; Camera='-StageInspectSide=1'}
)
foreach ($shot in $shots) {
    $logFile = Join-Path $studioRoot ('Saved\' + $shot.Name + '.log')
    $arguments = @('-RenderOffscreen','-windowed','-ForceRes','-nosplash','-unattended',
        '-ResX=1920','-ResY=1080','-DealCapturePreview','-DealCleanPreview','-CaptureDelay=4',
        '-seconds=7',('-CaptureName=' + $shot.Name),('-abslog="' + $logFile + '"'),$shot.Camera)
    $process = Start-Process -FilePath $launcher -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru
    $lines = Get-Content -LiteralPath $logFile
    $problems = @($lines | Select-String 'Error:|Fatal:|Fatal error|Ensure condition|Assertion failed')
    $capture = Join-Path (Split-Path $launcher -Parent) ('DealOrNoDealStage\Saved\Screenshots\Windows\' + $shot.Name + '.png')
    if ($process.ExitCode -ne 0 -or $problems.Count -gt 0 -or -not (Test-Path -LiteralPath $capture)) {
        throw ('Geometry capture failed: ' + $shot.Name)
    }
    [pscustomobject]@{Run=$shot.Name; ExitCode=$process.ExitCode; Errors=$problems.Count; Capture=$capture} | ConvertTo-Json -Compress
}
