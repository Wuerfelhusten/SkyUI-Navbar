param(
    [Parameter(Mandatory)][string]$Plugin,
    [Parameter(Mandatory)][string]$Version,
    [Parameter(Mandatory)][string]$Dumpbin
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$info = (Get-Item -LiteralPath $Plugin).VersionInfo
if ($info.FileVersion -ne "$Version.0" -or $info.ProductVersion -ne "$Version.0") {
    throw "DLL version resource must match $Version."
}
if ($info.FileDescription -ne 'Navigation Bar for SkyUI') { throw 'Incorrect DLL product description.' }
$exports = & $Dumpbin /nologo /exports $Plugin
if ($LASTEXITCODE -ne 0) { throw 'Could not read DLL exports.' }
foreach ($symbol in @('SKSEPlugin_Load', 'SKSEPlugin_Query', 'SKSEPlugin_Version')) {
    if (-not ($exports -match "\b$symbol\b")) { throw "Missing required SKSE export: $symbol" }
}
$headers = & $Dumpbin /nologo /headers $Plugin
if ($LASTEXITCODE -ne 0 -or -not ($headers -match '8664 machine')) { throw 'Plugin must be x64.' }
Write-Host 'DLL version, product name, x64 architecture and legacy/current SKSE exports verified.'
