#!/bin/bash
# Build BMS App for All Platforms
# This script builds both Android and Linux versions

echo "🚀 Building BMS App for All Platforms..."
echo "========================================"

# Record start time
START_TIME=$(date +%s)

# Check if we're in the correct directory
if [ ! -f "pubspec.yaml" ]; then
    echo "❌ Error: Not in Flutter project directory"
    echo "Please run this script from the bms_app folder"
    exit 1
fi

echo "📱 Starting Android build..."
echo "----------------------------"
./build_android.sh

if [ $? -ne 0 ]; then
    echo "❌ Android build failed, stopping..."
    exit 1
fi

echo ""
echo "🐧 Starting Linux build..."
echo "--------------------------"
./build_linux.sh

if [ $? -ne 0 ]; then
    echo "❌ Linux build failed"
    exit 1
fi

# Calculate total build time
END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))
MINUTES=$((DURATION / 60))
SECONDS=$((DURATION % 60))

echo ""
echo "🎉 All Builds Completed Successfully!"
echo "===================================="
echo "⏱️  Total Build Time: ${MINUTES}m ${SECONDS}s"
echo ""
echo "📱 Android APK:"
echo "   $(ls -la build/app/outputs/flutter-apk/app-release.apk 2>/dev/null || echo "   Not found")"
echo ""
echo "🐧 Linux Executable:"
echo "   $(ls -la build/linux/x64/release/bundle/bms_app 2>/dev/null || echo "   Not found")"
echo ""
echo "🎯 Ready for deployment on both platforms!"