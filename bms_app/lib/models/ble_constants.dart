// BLE Constants for BMS Application
// Based on GATT database from PSOC63 and QN9080 BMS devices

class BLEConstants {
  // Standard Service UUIDs
  static const String batteryServiceUUID = "0000180F-0000-1000-8000-00805F9B34FB";
  static const String automationIOServiceUUID = "00001815-0000-1000-8000-00805F9B34FB";
  static const String deviceInfoServiceUUID = "0000180A-0000-1000-8000-00805F9B34FB";
  
  // Standard Characteristic UUIDs
  static const String batteryLevelUUID = "00002A19-0000-1000-8000-00805F9B34FB";
  
  // Custom Automation IO Characteristics (from your GATT database)
  static const String digitalIOUUID = "37af9ae2-211d-4436-9d26-3a9ed02efeea";
  
  // Custom Battery Bank Voltage Characteristics
  static const String fullBatteryVoltageUUID = "e6b6c4f2-2e3a-4c8b-9d7e-f1a2b3c4d5e6"; // Custom UUID
  static const String bank1VoltageUUID = "f7c7d5g3-3f4b-5d9c-ae8f-g2b3c4d5e6f7";        // Custom UUID
  static const String bank2VoltageUUID = "g8d8e6h4-4g5c-6ead-bf9g-h3c4d5e6f7g8";        // Custom UUID
  static const String bank3VoltageUUID = "h9e9f7i5-5h6d-7fbe-cg0h-i4d5e6f7g8h9";        // Custom UUID
  static const String bank4VoltageUUID = "i0f0g8j6-6i7e-8gcf-dh1i-j5e6f7g8h9i0";        // Custom UUID
  
  // Device Information Characteristics
  static const String manufacturerNameUUID = "00002A29-0000-1000-8000-00805F9B34FB";
  static const String modelNumberUUID = "00002A24-0000-1000-8000-00805F9B34FB";
  
  // Client Characteristic Configuration Descriptor (CCCD) for notifications
  static const String cccdUUID = "00002902-0000-1000-8000-00805F9B34FB";
  
  // Device names from your BMS devices
  static const List<String> bmsDeviceNames = [
    "BMS_MCU",       // PSOC63 device
    "QN9080_BMS",    // QN9080 device
  ];
  
  // Device type identification
  static const String psoc63DeviceType = "PSOC63";
  static const String qn9080DeviceType = "QN9080";
  
  // Battery thresholds
  static const int criticalBatteryLevel = 20;  // Below this level is critical
  static const int lowBatteryLevel = 30;       // Below this level is low
  static const double voltageImbalanceThreshold = 0.2; // 200mV imbalance threshold
  
  // Scan settings
  static const Duration scanTimeout = Duration(seconds: 10);
  static const Duration connectionTimeout = Duration(seconds: 15);
  
  // Helper methods
  static bool isBMSDevice(String deviceName) {
    if (deviceName.isEmpty) return false;
    String lowerName = deviceName.toLowerCase();
    return bmsDeviceNames.any((name) => lowerName.contains(name.toLowerCase()));
  }
  
  static String getDeviceType(String deviceName) {
    if (deviceName.toLowerCase().contains("qn9080")) {
      return qn9080DeviceType;
    }
    return psoc63DeviceType; // Default to PSOC63
  }
}