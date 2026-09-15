param(
    [Parameter(Mandatory = $true)][string]$Source,
    [Parameter(Mandatory = $true)][string]$Output
)
$ErrorActionPreference = 'Stop'
$utf8 = [System.Text.UTF8Encoding]::new($false, $true)
$utf16 = [System.Text.UnicodeEncoding]::new($false, $true, $true)
$text = [System.IO.File]::ReadAllText($Source, $utf8)
$keys = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
$lines = $text.TrimEnd("`r", "`n") -split '\r?\n'
foreach ($line in $lines) {
    $parts = $line.Split("`t")
    if ($parts.Count -ne 2 -or $parts[0] -notmatch '^\$SkyUINavbar_[A-Za-z0-9]+$' -or
        [string]::IsNullOrWhiteSpace($parts[1]) -or -not $keys.Add($parts[0])) {
        throw "Invalid or duplicate translation line in ${Source}: $line"
    }
    # CommonLib's standard importer uses a 512-wchar line buffer (incl. CR/NUL).
    if ($line.Length -gt 509) { throw "Translation line exceeds Skyrim importer capacity: $($parts[0])" }
}
[System.IO.Directory]::CreateDirectory([System.IO.Path]::GetDirectoryName($Output)) | Out-Null
[System.IO.File]::WriteAllText($Output, (($lines -join "`r`n") + "`r`n"), $utf16)
