# Build and run all three components: C++ (if g++ available), Java server, and open browser
$cwd = Split-Path -Parent $MyInvocation.MyCommand.Definition
Set-Location $cwd

# Compile C++ if g++ is available
if (Get-Command g++ -ErrorAction SilentlyContinue) {
    Write-Host "Compiling C++..."
    g++ -std=c++17 main.c++ -O2 -o main.exe
}
else {
    Write-Host "g++ not found; skipping C++ build (ensure main.exe exists to launch it)."
}

# Compile Java
if (!(Test-Path .\out)) { New-Item -ItemType Directory -Path .\out | Out-Null }
Write-Host "Compiling Java..."
.\openJdk-25\bin\javac.exe -d out src\InventoryServer.java
if ($LASTEXITCODE -ne 0) { Write-Host "javac failed"; exit 1 }

# Run Java server (it will attempt to launch C++ exe if present)
Write-Host "Starting Java server..."
# prefer to run java in the project cwd so classpath resolution works even when path contains spaces
if (!(Test-Path out\InventoryServer.class)) {
    Write-Host "Compiled class not found: out\InventoryServer.class" -ForegroundColor Yellow
    Write-Host "If javac failed earlier, inspect the compiler output above." -ForegroundColor Yellow
}
# Run java with explicit argument list and working dir
$javaArgs = @('-cp', 'out', 'InventoryServer')
try {
    $proc = Start-Process -FilePath ".\openJdk-25\bin\java.exe" -ArgumentList $javaArgs -WorkingDirectory $pwd -NoNewWindow -PassThru -ErrorAction Stop
    Start-Sleep -Seconds 1
    Start-Process "http://localhost:8000/main.html"
    Write-Host "Launched server and opened browser (http://localhost:8000/main.html)"
}
catch {
    Write-Host "Failed to start Java server: $_" -ForegroundColor Red
    Write-Host "Try running: .\openJdk-25\bin\java.exe -cp out InventoryServer from the project root" -ForegroundColor Yellow
}