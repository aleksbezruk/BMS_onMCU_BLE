#!/bin/bash
# Run BMS App on Linux Desktop
# This script runs the built Linux executable

echo "🐧 Starting BMS App on Linux Desktop..."
echo "======================================"

# Check if executable exists
EXECUTABLE_PATH="build/linux/x64/release/bundle/bms_app"

if [ ! -f "$EXECUTABLE_PATH" ]; then
    echo "❌ Linux executable not found!"
    echo "📁 Expected location: $EXECUTABLE_PATH"
    echo ""
    echo "🔨 Build the Linux app first:"
    echo "   ./build_linux.sh"
    exit 1
fi

echo "🚀 Launching BMS App..."
echo "📁 Executable: $EXECUTABLE_PATH"
echo ""

# Run the application
./$EXECUTABLE_PATH

echo ""
echo "👋 BMS App closed."