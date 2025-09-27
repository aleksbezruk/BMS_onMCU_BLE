#!/bin/bash
# Install BMS App on Connected Android Device
# This script installs the built APK on an Android device via ADB

echo "📱 Installing BMS App on Android Device..."
echo "=========================================="

# Check if APK exists
APK_PATH="build/app/outputs/flutter-apk/app-release.apk"

if [ ! -f "$APK_PATH" ]; then
    echo "❌ Android APK not found!"
    echo "📁 Expected location: $APK_PATH"
    echo ""
    echo "🔨 Build the Android app first:"
    echo "   ./build_android.sh"
    exit 1
fi

# Check if ADB is available
if ! command -v adb &> /dev/null; then
    echo "❌ ADB (Android Debug Bridge) not found!"
    echo "📦 Install Android SDK platform-tools to get ADB"
    echo "🔗 https://developer.android.com/studio/releases/platform-tools"
    exit 1
fi

# Check for connected devices
echo "🔍 Checking for connected Android devices..."
DEVICES=$(adb devices | grep -v "List of devices" | grep "device$" | wc -l)

if [ $DEVICES -eq 0 ]; then
    echo "❌ No Android devices found!"
    echo ""
    echo "📱 Make sure your Android device is:"
    echo "   • Connected via USB"
    echo "   • Developer options enabled"
    echo "   • USB debugging enabled"
    echo "   • Device authorized for debugging"
    echo ""
    echo "🔍 Current ADB devices:"
    adb devices
    exit 1
fi

echo "✅ Found $DEVICES Android device(s)"
echo ""

# Install the APK
echo "🔨 Installing BMS App APK..."
APK_SIZE=$(du -h "$APK_PATH" | cut -f1)
echo "📏 APK Size: $APK_SIZE"

adb install -r "$APK_PATH"

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ BMS App installed successfully!"
    echo "📱 Look for 'BMS App' in your Android device's app drawer"
    echo ""
    echo "🚀 You can now:"
    echo "   • Open the BMS App on your device"
    echo "   • Connect to your BMS via Bluetooth"
    echo "   • Monitor battery data and GATT services"
else
    echo ""
    echo "❌ Installation failed!"
    echo "🔧 Try:"
    echo "   • Enable 'Install unknown apps' in Android settings"
    echo "   • Check USB connection"
    echo "   • Restart ADB: adb kill-server && adb start-server"
fi