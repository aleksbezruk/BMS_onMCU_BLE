# BLE Permission Issues - Complete Fix Guide

## 🚀 **What We Fixed in the App**

### 1. **Improved Permission Handler**
- Created `utils/permissions.dart` with better permission management
- Added user-friendly dialogs explaining why permissions are needed
- Automatic detection of permanently denied permissions

### 2. **Updated Android Manifest**
- Added proper permissions for different Android versions (6.0 to 14+)
- Configured legacy and modern Bluetooth permissions
- Added `neverForLocation` flag for Android 12+ to avoid location requirements

### 3. **Better User Experience**
- Permission explanation before requesting
- Step-by-step guide to app settings
- Clear error messages and solutions

---

## 📱 **Phone Settings You Need to Check**

### **Step 1: Enable Bluetooth**
1. Go to **Settings** → **Bluetooth**
2. Make sure Bluetooth is **ON**
3. Make sure your phone is **discoverable** (optional)

### **Step 2: Grant App Permissions**
1. Go to **Settings** → **Apps** → **BMS App**
2. Tap **Permissions**
3. Enable these permissions:
   - 🔵 **Bluetooth** (or **Nearby devices** on newer Android)
   - 📍 **Location** (required by Android for BLE scanning)

### **Step 3: Location Services**
1. Go to **Settings** → **Location** (or **Privacy** → **Location Services**)
2. Make sure **Location is ON**
3. This is required by Android for BLE scanning, even though we don't use your location

### **Step 4: Battery Optimization (Important!)**
1. Go to **Settings** → **Battery** → **Battery Optimization**
2. Find **BMS App**
3. Set to **"Don't optimize"** or **"Allow"**
4. This prevents Android from killing BLE connections in background

---

## 🔧 **Android Version Specific Steps**

### **Android 12+ (API 31+)**
- The app will ask for **"Nearby devices"** permission
- You might see **"Allow BMS App to find, connect to, and determine the relative position of nearby devices?"**
- Tap **"Allow"**

### **Android 10-11 (API 29-30)**
- The app will ask for **"Location"** permission
- You might see a scary message about location access
- This is required by Android for BLE scanning - we don't use your location
- Tap **"Allow"**

### **Android 6-9 (API 23-28)**
- Enable **Bluetooth** and **Location** permissions manually in app settings
- Make sure **Location Services** is enabled system-wide

---

## 🏃‍♂️ **Quick Test Steps**

1. **Install the new APK** (44.4MB)
2. **Open the app**
3. **Tap "Start Scan"**
4. **Follow the permission dialogs**
5. **If permissions are denied**, the app will guide you to settings

---

## ⚠️ **Common Issues & Solutions**

### **"Permission permanently denied"**
**Solution:** 
1. Open **Settings** → **Apps** → **BMS App** → **Permissions**
2. Enable all permissions manually
3. Restart the app

### **"Bluetooth scanning not working"**
**Solution:**
1. Turn Bluetooth **OFF and ON** again
2. Restart the app
3. Clear app cache: **Settings** → **Apps** → **BMS App** → **Storage** → **Clear Cache**

### **"Can't find BMS devices"**
**Solution:**
1. Make sure your BMS device is **powered on**
2. Make sure BMS device is **advertising** (check device documentation)
3. Try scanning closer to the device (< 5 meters)
4. Check if device name is "BMS_MCU" or "QN9080_BMS" (as configured)

### **"App keeps asking for permissions"**
**Solution:**
1. Grant all permissions at once when asked
2. Check if you accidentally denied "Don't ask again"
3. Clear app data and try again

---

## 🔍 **Debug Information**

The app now logs detailed permission information:
- Open the app
- Try to scan
- Check the permission status in debug logs
- The app will tell you exactly which permission is missing

---

## 📋 **Permission Summary**

| Permission | Why Needed | Android Version |
|------------|------------|----------------|
| **Bluetooth** | Connect to BMS devices | All |
| **Bluetooth Scan** | Find BMS devices | Android 12+ |
| **Bluetooth Connect** | Establish connection | Android 12+ |
| **Location** | Required by Android for BLE | Android 6-11 |

---

## 🎯 **Expected Behavior**

✅ **When working correctly:**
- App asks for permissions with clear explanations
- Scan finds BMS devices within ~30 seconds
- Can connect and see battery data
- No "permission denied" errors

❌ **If still having issues:**
1. Try the phone settings steps above
2. Restart your phone
3. Try on a different Android device (if available)
4. Check if your BMS device is actually broadcasting BLE advertisements

---

## 📱 **New APK Location**

The updated APK with better permission handling is at:
```
bms_app/build/app/outputs/flutter-apk/app-release.apk (44.4MB)
```

Install this new version and test the improved permission handling!