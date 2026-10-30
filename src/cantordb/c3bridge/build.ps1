# Builds cantordb-shell-c3.exe: the C++ SeQL shell/parser on top of the C3 + Roaring database.
#
#   1. CRoaring (third_party/roaring.c) compiled with gcc
#   2. the C3 database as a static library with the C ABI from rewrite/capi.c3 (c3c, mingw-x64)
#   3. main/lexer/parser + the C++ shim in this folder, compiled with -DCANTORDB_C3 and linked to both
#
# Needs gcc/g++ and c3c on PATH. Output goes to ./build/.

$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$src = Split-Path $here -Parent
$rewrite = Join-Path $src 'rewrite'
$out = Join-Path $here 'build'
New-Item -ItemType Directory -Force $out | Out-Null

function Run { param($exe, [string[]]$arguments)
	& $exe @arguments
	if ($LASTEXITCODE -ne 0) { throw "$exe failed with exit code $LASTEXITCODE" }
}

Write-Host '== CRoaring'
Run gcc @('-O2', '-c', (Join-Path $rewrite 'third_party\roaring.c'), '-I', (Join-Path $rewrite 'third_party'), '-o', (Join-Path $out 'roaring.o'))

Write-Host '== C3 static library'
Push-Location $rewrite
try { Run c3c @('build', 'cantordb-bridge', '-O2', '-g0', '--target', 'mingw-x64') } finally { Pop-Location }
$lib = Join-Path $rewrite 'build\cantordb-bridge.lib'

Write-Host '== shell'
Push-Location $src
try {
	Run g++ @('-O2', '-std=c++17', '-DCANTORDB_C3', '-I.', 'main.cpp', 'lexer.cpp', 'parser.cpp', 'c3bridge/cantordb.cpp',
		$lib, (Join-Path $out 'roaring.o'), '-ldbghelp', '-o', (Join-Path $out 'cantordb-shell-c3.exe'))
} finally { Pop-Location }

Write-Host "Built $(Join-Path $out 'cantordb-shell-c3.exe')"
