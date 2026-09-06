param([string]$Version = '0.4.2')
$ErrorActionPreference = 'Stop'
$studioRoot = Split-Path $PSScriptRoot -Parent
$target = Join-Path $studioRoot ('Builds\DealOrNoDealStage-Studio-' + $Version + '\Windows\DealOrNoDealStage.exe')
$icon = Join-Path $studioRoot 'Branding\DealOrNoDealStage-AppIcon.ico'
if (-not (Test-Path -LiteralPath $target -PathType Leaf)) { throw 'Packaged launcher does not exist.' }
if (-not (Test-Path -LiteralPath $icon -PathType Leaf)) { throw 'Application icon does not exist.' }
$desktop = [Environment]::GetFolderPath('Desktop')
$link = Join-Path $desktop ('Deal or No Deal - Studio Experience ' + $Version + '.lnk')
$previous = Join-Path $desktop 'Deal or No Deal - Studio Experience 0.4.1.lnk'
$shell = New-Object -ComObject WScript.Shell
if (-not (Test-Path -LiteralPath $link) -and (Test-Path -LiteralPath $previous)) {
    $old = $shell.CreateShortcut($previous)
    if ($old.TargetPath -eq (Join-Path $studioRoot 'Builds\DealOrNoDealStage-Studio-0.4.1\Windows\DealOrNoDealStage.exe')) {
        Move-Item -LiteralPath $previous -Destination $link
    }
}
$shortcut = $shell.CreateShortcut($link)
$shortcut.TargetPath = $target
$shortcut.WorkingDirectory = Split-Path $target -Parent
$shortcut.IconLocation = "$icon,0"
$shortcut.Description = 'Deal or No Deal - Studio Experience ' + $Version
$shortcut.Save()
$check = $shell.CreateShortcut($link)
if ($check.TargetPath -ne $target -or $check.IconLocation -ne "$icon,0") { throw 'Shortcut verification failed.' }
[pscustomobject]@{Shortcut=$link; Target=$check.TargetPath; Icon=$check.IconLocation} | ConvertTo-Json -Compress
