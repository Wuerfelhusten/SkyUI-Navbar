param(
    [Parameter(Mandatory)][string]$ProjectDir,
    [Parameter(Mandatory)][string]$BuildDir,
    [Parameter(Mandatory)][string]$Plugin,
    [Parameter(Mandatory)][string]$Symbols,
    [Parameter(Mandatory)][string]$Version,
    [Parameter(Mandatory)][string]$Configuration
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Add-Type -AssemblyName System.IO.Compression.FileSystem

if ($Configuration -ne 'Release') { throw 'Release packages must use the Release configuration.' }
if ((Get-Item -LiteralPath $Plugin).VersionInfo.FileVersion -ne "$Version.0") {
    throw "DLL version does not match $Version."
}
$artifactDir = Join-Path $BuildDir 'packages'
$stem = "Navigation-Bar-for-SkyUI-$Version"
$packagePath = Join-Path $artifactDir "$stem.zip"
$sourcePath = Join-Path $artifactDir "$stem-source.zip"

function Get-BytesHash([byte[]]$Bytes) {
    $algorithm = [System.Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($algorithm.ComputeHash($Bytes))).Replace('-', '').ToLowerInvariant() }
    finally { $algorithm.Dispose() }
}

function Get-ZipHash($Entry) {
    $stream = $Entry.Open()
    $buffer = [System.IO.MemoryStream]::new()
    try {
        $stream.CopyTo($buffer)
        return Get-BytesHash $buffer.ToArray()
    } finally {
        $stream.Dispose()
        $buffer.Dispose()
    }
}

function Read-Entries($Archive) {
    $entries = @{}
    foreach ($entry in $Archive.Entries) {
        $name = $entry.FullName.Replace('\', '/')
        if ($name.EndsWith('/')) { continue }
        if ($name.StartsWith('/') -or $name.Contains(':') -or $name.Split('/') -contains '..') {
            throw "Unsafe archive path: $name"
        }
        if ($entries.ContainsKey($name)) { throw "Duplicate archive entry: $name" }
        $entries[$name] = $entry
    }
    return $entries
}

function Assert-File($Entries, [string]$Name, [string]$LocalPath) {
    if (-not $Entries.ContainsKey($Name)) { throw "Missing package file: $Name" }
    $expected = (Get-FileHash -LiteralPath $LocalPath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ((Get-ZipHash $Entries[$Name]) -ne $expected) { throw "Package file differs from build/source: $Name" }
}

$expectedRuntime = @{
    'SKSE/Plugins/SkyUINavbar.dll' = $Plugin
    'SKSE/Plugins/SkyUINavbar.json' = (Join-Path $ProjectDir 'config/SkyUINavbar.json')
    'Interface/NavBar.swf' = (Join-Path $BuildDir 'interface/NavBarForSkyUI/NavBar.swf')
}
foreach ($file in Get-ChildItem -LiteralPath (Join-Path $ProjectDir 'src/ui/translations') -Filter 'SkyUINavbar_*.txt') {
    $expectedRuntime["Interface/Translations/$($file.Name)"] = Join-Path $BuildDir "interface/Translations/$($file.Name)"
}
$archive = [System.IO.Compression.ZipFile]::OpenRead($packagePath)
try {
    $entries = Read-Entries $archive
    if (@($archive.Entries | Where-Object { $_.FullName -match '^(Documentation|docs|licenses)[/\\]|^LICENSE$|^THIRD_PARTY_NOTICES[.]md$' }).Count -ne 0) {
        throw 'Documentation and license files must remain in the repository.'
    }
    if ($entries.Count -ne $expectedRuntime.Count) { throw 'Unexpected runtime package contents.' }
    $manifest = foreach ($name in $expectedRuntime.Keys | Sort-Object) {
        Assert-File $entries $name $expectedRuntime[$name]
        [ordered]@{ path = $name; bytes = $entries[$name].Length; sha256 = (Get-ZipHash $entries[$name]) }
    }
    if (@($entries.Keys | Where-Object { $_ -like '*.swf' }).Count -ne 1) {
        throw 'The runtime package must contain exactly one SWF.'
    }
} finally { $archive.Dispose() }

$source = [System.IO.Compression.ZipFile]::OpenRead($sourcePath)
try {
    $entries = Read-Entries $source
    foreach ($name in $entries.Keys) {
        if ($name -match '(^|/)([.]git|[.]vs|build[^/]*|dist|out)/|CMakeUserPresets[.]json$|[.](dll|exe|pdb|log|zip|user)$|^(docs|Documentation|licenses)/|^(README|CHANGELOG|THIRD_PARTY_NOTICES)[.]md$|^LICENSE$') {
            throw "Local/build artifact leaked into source archive: $name"
        }
    }
    $projectRoot = (Resolve-Path -LiteralPath $ProjectDir).Path
    foreach ($directory in @('src', 'cmake', 'config', 'scripts', 'tests')) {
        foreach ($file in Get-ChildItem -LiteralPath (Join-Path $projectRoot $directory) -Recurse -File) {
            $name = $file.FullName.Substring($projectRoot.Length + 1).Replace('\', '/')
            Assert-File $entries $name $file.FullName
        }
    }
    foreach ($name in @('CMakeLists.txt', 'CMakePresets.json', 'vcpkg.json', '.clang-format', '.editorconfig', '.gitignore')) {
        Assert-File $entries $name (Join-Path $ProjectDir $name)
    }
} finally { $source.Dispose() }

# The main mod archive contains no PDB; keep matching symbols separately.
if (-not (Test-Path -LiteralPath $Symbols -PathType Leaf)) { throw 'Matching debug symbols are missing.' }
$symbolPath = Join-Path $artifactDir "$stem-symbols.zip"
$stream = [System.IO.File]::Open($symbolPath, [System.IO.FileMode]::Create)
try {
    $symbolArchive = [System.IO.Compression.ZipArchive]::new($stream, [System.IO.Compression.ZipArchiveMode]::Create, $true)
    try {
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($symbolArchive, $Symbols, 'SkyUINavbar.pdb') | Out-Null
    } finally { $symbolArchive.Dispose() }
} finally { $stream.Dispose() }

$utf8 = [System.Text.UTF8Encoding]::new($false)
$manifestJson = [ordered]@{ version = $Version; files = @($manifest) } | ConvertTo-Json -Depth 5
[System.IO.File]::WriteAllText((Join-Path $artifactDir "$stem-manifest.json"), $manifestJson + [Environment]::NewLine, $utf8)
$hashLines = foreach ($file in @($packagePath, $sourcePath, $symbolPath)) {
    "$((Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant())  $([System.IO.Path]::GetFileName($file))"
}
[System.IO.File]::WriteAllText((Join-Path $artifactDir 'SHA256SUMS.txt'), ($hashLines -join [Environment]::NewLine) + [Environment]::NewLine, $utf8)
Write-Host "Verified $Version release: $($expectedRuntime.Count) runtime files, complete source, matching DLL version, separate symbols and SHA-256 checksums."
