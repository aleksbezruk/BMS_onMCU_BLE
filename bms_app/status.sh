#!/bin/bash
# BMS App Build System Status
# Shows current build status and available actions

echo "📊 BMS App Build System Status"
echo "=============================="

# Check current directory
if [ ! -f "pubspec.yaml" ]; then
    echo "❌ Error: Not in Flutter project directory"
    echo "Please run this script from the bms_app folder"
    exit 1
fi

echo "📁 Current Directory: $(pwd)"
echo "📱 Project: $(grep 'name:' pubspec.yaml | cut -d' ' -f2)"
echo "🔢 Version: $(grep 'version:' pubspec.yaml | cut -d' ' -f2)"
echo ""

# Check build outputs
echo "🏗️  Build Status:"
echo "=================="

# Android APK
APK_PATH="build/app/outputs/flutter-apk/app-release.apk"
if [ -f "$APK_PATH" ]; then
    APK_SIZE=$(du -h "$APK_PATH" | cut -f1)
    APK_DATE=$(stat -c %y "$APK_PATH" | cut -d' ' -f1,2 | cut -d'.' -f1)
    echo "✅ Android APK: $APK_SIZE (built: $APK_DATE)"
else
    echo "❌ Android APK: Not built"
fi

# Linux Executable
LINUX_PATH="build/linux/x64/release/bundle/bms_app"
if [ -f "$LINUX_PATH" ]; then
    LINUX_SIZE=$(du -h "$LINUX_PATH" | cut -f1)
    LINUX_DATE=$(stat -c %y "$LINUX_PATH" | cut -d' ' -f1,2 | cut -d'.' -f1)
    echo "✅ Linux Executable: $LINUX_SIZE (built: $LINUX_DATE)"
else
    echo "❌ Linux Executable: Not built"
fi

echo ""

# Show available scripts
echo "🚀 Available Commands:"
echo "====================="
echo "🔨 Building:"
echo "   ./build_android.sh     - Build Android APK"
echo "   ./build_linux.sh       - Build Linux executable"
echo "   ./build_all.sh         - Build all platforms"
echo ""
echo "🎯 Running & Installing:"
echo "   ./run_linux.sh         - Run Linux app"
echo "   ./install_android.sh   - Install APK on Android device"
echo ""
echo "📚 Documentation:"
echo "   cat BUILD_README.md    - Full build system documentation"
echo ""

# Check Flutter environment
echo "🔧 Flutter Environment:"
echo "======================"
flutter --version | head -1
echo ""

# Show quick build command suggestions
if [ ! -f "$APK_PATH" ] && [ ! -f "$LINUX_PATH" ]; then
    echo "💡 Quick Start:"
    echo "   ./build_all.sh    # Build everything"
elif [ ! -f "$APK_PATH" ]; then
    echo "💡 Missing Android build:"
    echo "   ./build_android.sh"
elif [ ! -f "$LINUX_PATH" ]; then
    echo "💡 Missing Linux build:"
    echo "   ./build_linux.sh"
else
    echo "🎉 All builds available! Ready to deploy."
    echo "💡 Next steps:"
    echo "   ./install_android.sh  # Install on Android device"
    echo "   ./run_linux.sh        # Run on Linux desktop"
fi