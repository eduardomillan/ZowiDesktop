<#
.SYNOPSIS
Set the dev_mode value in a JSON config file for packaging.

.DESCRIPTION
Invoked from the packaging .bat scripts; a separate script avoids cmd
escaping issues. Packaged builds ship with dev mode OFF; it can be
re-enabled at runtime via the DEV_MODE environment variable.

.PARAMETER Value
New dev_mode value: 'true' or 'false'.

.PARAMETER Path
Path to the JSON config file (e.g. src\config.json).

.EXAMPLE
powershell -ExecutionPolicy Bypass -File set_dev_mode.ps1 false "src\config.json"
#>

param(
    [string]$Value,
    [string]$Path
)

function Show-Usage {
    Write-Output "Usage: set_dev_mode.ps1 <Value> <Path>"
    Write-Output ""
    Write-Output "Set the dev_mode value in a JSON config file for packaging."
    Write-Output ""
    Write-Output "Arguments:"
    Write-Output "  Value   New dev_mode value: 'true' or 'false'"
    Write-Output "  Path    Path to the JSON config file (e.g. src\config.json)"
    Write-Output ""
    Write-Output "Example:"
    Write-Output "  powershell -ExecutionPolicy Bypass -File set_dev_mode.ps1 false src\config.json"
}

# Minimal help: -h / --help / -? . The .bat scripts always pass both arguments,
# so missing arguments only happen when the script is invoked manually.
$helpTokens = @('-h', '--help', '-?')
if ($helpTokens -contains $Value -or $helpTokens -contains $Path) {
    Show-Usage
    exit 0
}
if ([string]::IsNullOrEmpty($Value) -or [string]::IsNullOrEmpty($Path)) {
    Write-Error "set_dev_mode.ps1: Value and Path are required. Run 'set_dev_mode.ps1 -h' for usage."
    exit 1
}

# Set the dev_mode value in a JSON config file for packaging.
# Invoked from .bat scripts; a separate script avoids cmd escaping issues.
if (-not (Test-Path -LiteralPath $Path)) {
    Write-Error "set_dev_mode.ps1: file not found: $Path"
    exit 1
}

$c = [IO.File]::ReadAllText($Path)
$replaced = $c -replace '("dev_mode"\s*:\s*")[^"]*(")', ('${1}' + $Value + '${2}')
if ($replaced -ceq $c) {
    if ($c -match '"dev_mode"') {
        Write-Output "set_dev_mode.ps1: dev_mode already set to '$Value' in $Path"
        exit 0
    }
    Write-Error "set_dev_mode.ps1: dev_mode key not found in $Path"
    exit 1
}

[IO.File]::WriteAllText($Path, $replaced, (New-Object System.Text.UTF8Encoding($false)))
Write-Output "set_dev_mode.ps1: dev_mode set to '$Value' in $Path"