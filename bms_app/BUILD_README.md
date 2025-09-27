# BMS App Build System 🚀

This folder contains build scripts for the BMS Flutter application that supports both Android and Linux platforms.

## 📱 Quick Start

### Build for Android
```bash
./build_android.sh
```

### Build for Linux Desktop
```bash
./build_linux.sh
```

### Build for All Platforms
```bash
./build_all.sh
```

## 🎯 Available Scripts

### 🔨 Build Scripts

| Script | Description | Output |
|--------|-------------|---------|
| `build_android.sh` | Builds Android APK | `build/app/outputs/flutter-apk/app-release.apk` |
| `build_linux.sh` | Builds Linux executable | `build/linux/x64/release/bundle/bms_app` |
| `build_all.sh` | Builds both platforms | Both outputs above |

### 🚀 Run & Install Scripts

| Script | Description | Requirements |
|--------|-------------|--------------|
| `run_linux.sh` | Runs Linux app | Linux build completed |
| `install_android.sh` | Installs APK on device | ADB + connected Android device |

## 📋 Prerequisites

### For Android Development
- Flutter SDK installed
- Android SDK and tools
- USB debugging enabled on target device
- ADB (Android Debug Bridge) in PATH

### For Linux Development
- Flutter SDK installed
- Linux desktop development enabled
- Required Linux libraries (GTK3, etc.)

### Enable Linux Desktop (if needed)
```bash
flutter config --enable-linux-desktop
```

## 🎯 Build Outputs

### Android APK
- **Location:** `build/app/outputs/flutter-apk/app-release.apk`
- **Size:** ~44.8MB
- **Installation:** Use `./install_android.sh` or manual ADB install

### Linux Executable
- **Location:** `build/linux/x64/release/bundle/bms_app`
- **Bundle:** Complete directory with all dependencies
- **Run:** Use `./run_linux.sh` or execute directly

## 🔧 Build Features

All build scripts include:
- ✅ **Clean builds** (flutter clean)
- ✅ **Dependency updates** (flutter pub get)
- ✅ **Error handling** and validation
- ✅ **Build size reporting**
- ✅ **Success/failure feedback**
- ✅ **Helpful usage instructions**

## 📱 BMS App Features

The built applications include:
- 🔍 **BLE device scanning** and discovery
- 🔗 **BMS device connection** (PSOC63, QN9080)
- 📊 **GATT services exploration** with characteristic details
- 🔋 **Battery monitoring** and real-time data
- 🐛 **Debug tools** and diagnostics
- 🎨 **Modern Material Design 3** UI

## 🚀 Usage Examples

### Complete Build Workflow
```bash
# Build for all platforms
./build_all.sh

# Install on Android device
./install_android.sh

# Run on Linux desktop
./run_linux.sh
```

### Development Workflow
```bash
# Quick Android build and install
./build_android.sh && ./install_android.sh

# Quick Linux build and run
./build_linux.sh && ./run_linux.sh
```

## 🔍 Troubleshooting

### Android Build Issues
- Ensure Android SDK is properly configured
- Check `flutter doctor` for missing dependencies
- Verify USB debugging is enabled on device

### Linux Build Issues
- Enable Linux desktop: `flutter config --enable-linux-desktop`
- Install required system packages (GTK3, libayatana-appindicator3-dev)
- Check `flutter doctor` for Linux-specific requirements

### ADB Installation Issues
- Enable "Install unknown apps" in Android settings
- Check device authorization: `adb devices`
- Restart ADB daemon: `adb kill-server && adb start-server`

## 📊 Build Performance

Typical build times:
- **Android APK:** ~5-8 minutes (first build), ~2-3 minutes (subsequent)
- **Linux Executable:** ~2-4 minutes (first build), ~1-2 minutes (subsequent)
- **All Platforms:** ~7-12 minutes total

## 🎯 Development Tips

1. **Use `build_all.sh`** for complete builds before releases
2. **Use platform-specific scripts** during development
3. **Check build outputs** with the provided size and location info
4. **Use `./run_linux.sh`** for quick Linux testing
5. **Use `./install_android.sh`** for automated Android deployment

---

**Ready to build and deploy your BMS app across multiple platforms!** 🚀