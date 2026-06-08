# AutoDialer filesystem upload script
# Builds a LittleFS image from the data/ folder and flashes it to the board.
#
# Usage:
#   .\upload.ps1              # uses default port COM5
#   .\upload.ps1 -Port COM3   # specify a different port

param(
    [string]$Port = "COM5"
)

$ErrorActionPreference = "Stop"

$MKLITTLEFS = "$PSScriptRoot\mklittlefs.exe"  #Tested with v4.0.2 from https://github.com/earlephilhower/mklittlefs/releases
$DATA_DIR   = "$PSScriptRoot\data"
$IMAGE      = "$PSScriptRoot\data.bin"
$FS_OFFSET  = "0x290000"
$FS_SIZE    = 1441792   # 1.5 MB  (384 blocks × 4096 bytes)

Write-Host "Building LittleFS image from $DATA_DIR ..." -ForegroundColor Cyan
& $MKLITTLEFS -c $DATA_DIR -s $FS_SIZE $IMAGE
if ($LASTEXITCODE -ne 0) { Write-Error "mklittlefs failed"; exit 1 }
Write-Host "Image built: $IMAGE" -ForegroundColor Green

Write-Host "Flashing to $Port at $FS_OFFSET ..." -ForegroundColor Cyan
python -m esptool --chip esp32s3 --port $Port --baud 921600 write_flash $FS_OFFSET $IMAGE
if ($LASTEXITCODE -ne 0) { Write-Error "esptool failed"; exit 1 }

Write-Host "Done." -ForegroundColor Green
