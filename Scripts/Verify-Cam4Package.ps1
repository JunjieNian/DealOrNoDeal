param([string]$Package = 'Builds\DealOrNoDealStage-Studio-0.4.2\Windows')
$ErrorActionPreference = 'Stop'
$studioRoot = Split-Path $PSScriptRoot -Parent
$launcher = Join-Path $studioRoot ($Package + '\DealOrNoDealStage.exe')
$runs = @(
    @{ Name='Cam4-Packaged-1280'; Args=@('-DealBoardLayoutTest','-ResX=1280','-ResY=720','-seconds=18'); Marker='\[DealBoardLayoutTest\] Completed=true Failures=0 Viewport=1280x720' },
    @{ Name='Cam4-Packaged-1600'; Args=@('-DealBoardLayoutTest','-ResX=1600','-ResY=900','-seconds=18'); Marker='\[DealBoardLayoutTest\] Completed=true Failures=0 Viewport=1600x900' },
    @{ Name='Cam4-Packaged-1920'; Args=@('-DealBoardLayoutTest','-ResX=1920','-ResY=1080','-seconds=18'); Marker='\[DealBoardLayoutTest\] Completed=true Failures=0 Viewport=1920x1080' },
    @{ Name='Cam4-Packaged-Experience'; Args=@('-DealExperienceTest','-seconds=4'); Marker='\[DealExperienceTest\] Completed=true Failures=0' },
    @{ Name='Cam4-Packaged-NoDeal'; Args=@('-DealAutoTest','-seconds=3'); Marker='\[DealGameAutoTest\] Completed=true' },
    @{ Name='Cam4-Packaged-Accept'; Args=@('-DealAutoAcceptTest','-seconds=3'); Marker='\[DealGameAutoAcceptTest\] Completed=true' }
)
$results = @()
foreach ($run in $runs) {
    $logFile = Join-Path $studioRoot ('Saved\' + $run.Name + '.log')
    $arguments = @('-RenderOffscreen','-windowed','-ForceRes','-nosplash','-unattended','-DealSeed=20260906',('-abslog="' + $logFile + '"')) + $run.Args
    $process = Start-Process -FilePath $launcher -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru
    $lines = Get-Content -LiteralPath $logFile
    $problems = @($lines | Select-String -Pattern 'Error:|Fatal:|Fatal error|Ensure condition|Assertion failed')
    $markers = @($lines | Select-String -Pattern $run.Marker)
    $result = [pscustomobject]@{ Run=$run.Name; ExitCode=$process.ExitCode; Errors=$problems.Count; Results=($markers.Line -join ' | ') }
    $results += $result
    $result | ConvertTo-Json -Compress
    if ($process.ExitCode -ne 0 -or $problems.Count -gt 0 -or $markers.Count -eq 0) {
        throw ('Packaged verification failed: ' + $run.Name)
    }
}
$results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $studioRoot 'Saved\Cam4-PackageResults.json') -Encoding utf8
