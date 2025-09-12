#!/bin/bash

# 0x3 ESP8266 Advanced Project - Release Script
# This script creates a release build and generates release assets 

set -e

echo "🚀 0x3 ESP8266 Advanced Project - Release Builder"
echo "================================================="

# Get version from config.h
VERSION=$(grep -oP '#define PROJECT_VERSION "\K[^"]+' include/config.h || echo "unknown")
echo "🏷️  Creating release for version $VERSION"

# Check if we have clean repo
if command -v git &> /dev/null && [ -d .git ]; then
    if [ -n "$(git status --porcelain)" ]; then
        echo "⚠️  Warning: Repository has uncommitted changes"
        read -p "Continue anyway? [y/N]: " CONTINUE
        if [[ ! "$CONTINUE" =~ ^[Yy]$ ]]; then
            echo "❌ Release cancelled"
            exit 1
        fi
    fi
fi

echo
echo "🔨 Building release..."

# Clean previous build
echo " → Cleaning previous builds"
pio run --target clean

# Build for all defined environments
echo " → Building firmware"
pio run

# Create release directory
RELEASE_DIR="releases/v$VERSION"
mkdir -p "$RELEASE_DIR"

# Copy firmware files
echo " → Copying firmware files"
cp .pio/build/d1_mini/firmware.bin "$RELEASE_DIR/0x3-esp8266-v$VERSION-d1_mini.bin"

# Find other environments and copy their firmware too
for ENV_DIR in .pio/build/*; do
    if [ -d "$ENV_DIR" ] && [ "$(basename "$ENV_DIR")" != "d1_mini" ]; then
        ENV_NAME=$(basename "$ENV_DIR")
        if [ -f "$ENV_DIR/firmware.bin" ]; then
            cp "$ENV_DIR/firmware.bin" "$RELEASE_DIR/0x3-esp8266-v$VERSION-$ENV_NAME.bin"
        fi
    fi
done

# Create checksums
echo " → Generating checksums"
cd "$RELEASE_DIR" || exit
sha256sum ./*.bin > checksums.txt
cd - || exit

# Create archive
echo " → Creating release archive"
zip -r "$RELEASE_DIR/0x3-esp8266-v$VERSION-release.zip" "$RELEASE_DIR"

# Print release info
echo 
echo "✅ Release v$VERSION created successfully!"
echo
echo "📁 Release assets in: $RELEASE_DIR"
echo "📄 Firmware files:"
ls -l "$RELEASE_DIR"/*.bin

echo
echo "📦 Release archive: $RELEASE_DIR/0x3-esp8266-v$VERSION-release.zip"
echo

# Upload instructions
echo "To upload the release firmware:"
echo "pio run --target upload --upload-port YOUR_DEVICE_PORT"
echo "or"
echo "pio run --target upload --upload-port YOUR_DEVICE_IP --upload-protocol espota"
echo

echo "Happy hacking! 🚀"
