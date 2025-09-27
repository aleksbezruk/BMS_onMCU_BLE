#!/bin/bash
# Build BMS App for Linux Desktop
# This script builds the release executable for Linux

echo "🐧 Building BMS App for Linux Desktop..."
echo "========================================"

# Check if we're in the correct directory
if [ ! -f "pubspec.yaml" ]; then
    echo "❌ Error: Not in Flutter project directory"
    echo "Please run this script from the bms_app folder"
    exit 1
fi

# Check if Linux development is enabled
if ! flutter config | grep -q "linux.*true"; then
    echo "⚠️  Enabling Linux desktop development..."
    flutter config --enable-linux-desktop
fi

# Clean previous builds
echo "🧹 Cleaning previous builds..."
flutter clean

# Get dependencies
echo "📦 Getting Flutter dependencies..."
flutter pub get

# Build Linux executable
echo "🔨 Building Linux executable (release)..."
flutter build linux --release

# Check if build was successful
if [ $? -eq 0 ]; then
    EXECUTABLE_PATH="build/linux/x64/release/bundle/bms_app"
    if [ -f "$EXECUTABLE_PATH" ]; then
        EXECUTABLE_SIZE=$(du -h "$EXECUTABLE_PATH" | cut -f1)
        echo ""
        echo "✅ Linux build completed successfully!"
        echo "🖥️  Executable Location: $EXECUTABLE_PATH"
        echo "📏 Executable Size: $EXECUTABLE_SIZE"
        echo ""
        echo "🚀 Run the application:"
        echo "   ./$EXECUTABLE_PATH"
        echo ""
        echo "📁 Bundle contents:"
        ls -la build/linux/x64/release/bundle/
    else
        echo "❌ Executable file not found at expected location"
        exit 1
    fi
else
    echo "❌ Linux build failed"
    exit 1
fi