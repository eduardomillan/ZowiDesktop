param(
    [Parameter(Mandatory = $true)][string]$Value,
    [Parameter(Mandatory = $true)][string]$Path
)

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