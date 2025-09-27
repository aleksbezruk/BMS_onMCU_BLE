#!/bin/bash
# Build BMS App for Android (APK)
# This script builds the release APK for Android devices

echo "🤖 Building BMS App for Android..."
echo "=================================="

# Check if we're in the correct directory
if [ ! -f "pubspec.yaml" ]; then
    echo "❌ Error: Not in Flutter project directory"
    echo "Please run this script from the bms_app folder"
    exit 1
fi

# Clean previous builds
echo "🧹 Cleaning previous builds..."
flutter clean

# Get dependencies
echo "📦 Getting Flutter dependencies..."
flutter pub get

# Build Android APK
echo "🔨 Building Android APK (release)..."
flutter build apk --release

# Check if build was successful
if [ $? -eq 0 ]; then
    APK_PATH="build/app/outputs/flutter-apk/app-release.apk"
    if [ -f "$APK_PATH" ]; then
        APK_SIZE=$(du -h "$APK_PATH" | cut -f1)
        echo ""
        echo "✅ Android build completed successfully!"
        echo "📱 APK Location: $APK_PATH"
        echo "📏 APK Size: $APK_SIZE"
        echo ""
        echo "🚀 Ready to install on Android device:"
        echo "   adb install $APK_PATH"
    else
        echo "❌ APK file not found at expected location"
        exit 1
    fi
else
    echo "❌ Android build failed"
    exit 1
fi