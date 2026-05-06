param(
    [string]$PdcursesRoot = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..\..'))

$candidates = @()
if ($PdcursesRoot) {
    $candidates += $PdcursesRoot
}
$candidates += @(
    (Join-Path $repoRoot 'lib\pdcursesmod'),
    (Join-Path $repoRoot 'submodules\pdcursesmod')
)

$targetFile = $null
foreach ($candidate in $candidates) {
    if (-not $candidate) { continue }
    $refreshPath = Join-Path $candidate 'pdcurses\refresh.c'
    if (Test-Path -LiteralPath $refreshPath) {
        $targetFile = [System.IO.Path]::GetFullPath($refreshPath)
        break
    }
}

if (-not $targetFile) {
    Write-Host 'PDCurses refresh.c not found; skipping local patch.'
    exit 0
}

$content = [System.IO.File]::ReadAllText($targetFile)

$marker = 'A changed region can begin on the dummy cell that follows a'
if ($content.Contains($marker)) {
    Write-Host "PDCurses refresh.c already patched: $targetFile"
    exit 0
}

$needle = "    assert( lineno < SP->lines);`r`n"
if (-not $content.Contains($needle)) {
    throw "Could not find expected insertion point in $targetFile"
}

$insertion = @"
#ifdef PDC_WIDE
    /* A changed region can begin on the dummy cell that follows a
       fullwidth character; that is not a legal starting point for the
       packet logic below, so widen the region to include the leading
       cell.  Likewise, if the region ends immediately before such a
       dummy cell, include it so the fullwidth character is updated as a
       unit. */
    if( x > 0 && (srcp[0] & A_CHARTEXT) == MAX_UNICODE)
    {
        x--;
        len++;
        srcp--;
    }
    if( x + len < COLS
            && (srcp[len - 1] & A_CHARTEXT) < MAX_UNICODE
            && (srcp[len] & A_CHARTEXT) == MAX_UNICODE)
        len++;
#endif
"@

$updated = $content.Replace($needle, $needle + $insertion)
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($targetFile, $updated, $utf8NoBom)

Write-Host "Patched PDCurses refresh.c: $targetFile"
