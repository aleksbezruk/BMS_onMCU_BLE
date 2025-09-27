// Utilities for BMS Application

class Utils {
  // Check if platform supports BLE
  static bool get isBlePlatform {
    // BLE is supported on mobile platforms (Android, iOS)
    // Note: Linux support for BLE exists but might have limitations
    return true; // For now, we'll assume BLE support
  }
  
  // Format RSSI value for display
  static String formatRSSI(int? rssi) {
    if (rssi == null) return 'Unknown';
    return '$rssi dBm';
  }
  
  // Format voltage for display
  static String formatVoltage(double voltage) {
    return '${voltage.toStringAsFixed(2)}V';
  }
  
  // Format battery level for display
  static String formatBatteryLevel(int level) {
    return '$level%';
  }
  
  // Get battery level color
  static String getBatteryLevelDescription(int level) {
    if (level > 80) return 'Excellent';
    if (level > 60) return 'Good';
    if (level > 40) return 'Fair';
    if (level > 20) return 'Low';
    return 'Critical';
  }
  
  // Calculate time since last update
  static String getTimeSinceUpdate(DateTime lastUpdate) {
    Duration difference = DateTime.now().difference(lastUpdate);
    
    if (difference.inMinutes < 1) {
      return 'Just now';
    } else if (difference.inMinutes < 60) {
      return '${difference.inMinutes}m ago';
    } else if (difference.inHours < 24) {
      return '${difference.inHours}h ago';
    } else {
      return '${difference.inDays}d ago';
    }
  }
}