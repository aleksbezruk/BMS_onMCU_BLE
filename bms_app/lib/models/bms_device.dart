// BMS Device Model
// Represents a BMS device discovered via BLE

import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'battery_data.dart';
import 'ble_constants.dart';

class BMSDevice {
  final BluetoothDevice bluetoothDevice;
  final String deviceType; // "PSOC63" or "QN9080"
  BMSData data;
  bool isConnected;
  int? rssi; // Signal strength
  DateTime discoveredAt;
  
  BMSDevice({
    required this.bluetoothDevice,
    required this.deviceType,
    BMSData? data,
    this.isConnected = false,
    this.rssi,
    DateTime? discoveredAt,
  }) : data = data ?? BMSData(),
       discoveredAt = discoveredAt ?? DateTime.now();
  
  // Device properties
  String get name => bluetoothDevice.platformName.isNotEmpty 
      ? bluetoothDevice.platformName 
      : 'Unknown BMS';
      
  String get id => bluetoothDevice.remoteId.toString();
  
  String get shortId => id.length > 8 ? id.substring(0, 8) : id;
  
  // Connection state
  Stream<BluetoothConnectionState> get connectionState => 
      bluetoothDevice.connectionState;
  
  // Check if device is a known BMS device
  bool get isValidBMSDevice => BLEConstants.isBMSDevice(name);
  
  // Get signal strength description
  String get signalStrengthDescription {
    if (rssi == null) return 'Unknown';
    if (rssi! >= -50) return 'Excellent';
    if (rssi! >= -60) return 'Good';
    if (rssi! >= -70) return 'Fair';
    return 'Poor';
  }
  
  // Get device type from name
  static String getDeviceTypeFromName(String deviceName) {
    return BLEConstants.getDeviceType(deviceName);
  }
  
  // Create BMSDevice from ScanResult
  static BMSDevice fromScanResult(ScanResult scanResult) {
    String deviceType = getDeviceTypeFromName(scanResult.device.platformName);
    
    // Extract battery level from advertisement data if available
    int batteryLevel = _extractBatteryLevelFromAdData(scanResult.advertisementData);
    
    return BMSDevice(
      bluetoothDevice: scanResult.device,
      deviceType: deviceType,
      data: BMSData(batteryLevel: batteryLevel),
      rssi: scanResult.rssi,
      discoveredAt: DateTime.now(),
    );
  }
  
  // Helper method to extract battery level from advertisement data
  static int _extractBatteryLevelFromAdData(AdvertisementData adData) {
    try {
      // Check if Battery Service data is available in advertisement
      if (adData.serviceData.containsKey(Guid(BLEConstants.batteryServiceUUID))) {
        List<int>? data = adData.serviceData[Guid(BLEConstants.batteryServiceUUID)];
        if (data != null && data.isNotEmpty) {
          return data[0].clamp(0, 100); // First byte should be battery level
        }
      }
      
      // Check manufacturer data for battery level (if available)
      if (adData.manufacturerData.isNotEmpty) {
        // This would depend on your specific manufacturer data format
        // For now, return 0 as we don't have specific format
      }
    } catch (e) {
      print('Error extracting battery level from advertisement: $e');
    }
    
    return 0; // Default if not available in advertisement
  }
  
  // Connect to the device
  Future<bool> connect() async {
    try {
      await bluetoothDevice.connect(timeout: BLEConstants.connectionTimeout);
      isConnected = true;
      return true;
    } catch (e) {
      print('Error connecting to device ${name}: $e');
      isConnected = false;
      return false;
    }
  }
  
  // Disconnect from the device
  Future<void> disconnect() async {
    try {
      await bluetoothDevice.disconnect();
      isConnected = false;
    } catch (e) {
      print('Error disconnecting from device ${name}: $e');
    }
  }
  
  // Update RSSI value
  void updateRSSI(int newRSSI) {
    rssi = newRSSI;
  }
  
  // Update connection status
  void updateConnectionStatus(bool connected) {
    isConnected = connected;
  }
  
  // Check if device was discovered recently (within last 30 seconds)
  bool get isRecentlyDiscovered {
    return DateTime.now().difference(discoveredAt).inSeconds < 30;
  }
  
  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      other is BMSDevice &&
          runtimeType == other.runtimeType &&
          id == other.id;
  
  @override
  int get hashCode => id.hashCode;
  
  @override
  String toString() {
    return 'BMSDevice(name: $name, type: $deviceType, id: $shortId, '
           'connected: $isConnected, rssi: $rssi, batteryLevel: ${data.batteryLevel}%)';
  }
}