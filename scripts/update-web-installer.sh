#!/usr/bin/env bash
# Copy PlatformIO build artifacts into docs/ for the esp-web-tools browser installer.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/Firmware-PIO/.pio/build/esp32dev"
DOCS="$ROOT/docs"

for bin in bootloader.bin partitions.bin firmware.bin; do
  if [[ ! -f "$BUILD/$bin" ]]; then
    echo "Missing $BUILD/$bin — run 'pio run' in Firmware-PIO first." >&2
    exit 1
  fi
done

cp "$BUILD/bootloader.bin" "$BUILD/partitions.bin" "$BUILD/firmware.bin" "$DOCS/"
echo "Updated docs/bootloader.bin, docs/partitions.bin, docs/firmware.bin"
echo "If this is a release, bump \"version\" in docs/manifest.json and commit the docs/ folder."
