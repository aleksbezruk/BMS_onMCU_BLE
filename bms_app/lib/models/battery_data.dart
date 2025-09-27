// BMS Battery Data Model
// Represents the battery and voltage data from BMS device

class BMSData {
  int batteryLevel;           // Battery level percentage (0-100)
  double fullBatteryVoltage;  // Total battery pack voltage
  double bank1Voltage;        // Individual bank voltages
  double bank2Voltage;
  double bank3Voltage;
  double bank4Voltage;
  bool dischargeSwitchEnabled; // Discharge switch state
  DateTime lastUpdate;        // Last data update timestamp
  
  BMSData({
    this.batteryLevel = 0,
    this.fullBatteryVoltage = 0.0,
    this.bank1Voltage = 0.0,
    this.bank2Voltage = 0.0,
    this.bank3Voltage = 0.0,
    this.bank4Voltage = 0.0,
    this.dischargeSwitchEnabled = false,
    DateTime? lastUpdate,
  }) : lastUpdate = lastUpdate ?? DateTime.now();
  
  // Calculate total voltage from individual banks
  double get calculatedTotalVoltage => 
      bank1Voltage + bank2Voltage + bank3Voltage + bank4Voltage;
  
  // Get list of all bank voltages
  List<double> get bankVoltages => [bank1Voltage, bank2Voltage, bank3Voltage, bank4Voltage];
  
  // Check if battery is critically low
  bool get isCriticallyLow => batteryLevel < 20;
  
  // Check if battery is low
  bool get isLow => batteryLevel < 30 && batteryLevel >= 20;
  
  // Check for voltage imbalance between banks
  bool get hasVoltageImbalance {
    List<double> voltages = bankVoltages.where((v) => v > 0).toList();
    if (voltages.length < 2) return false;
    
    double max = voltages.reduce((a, b) => a > b ? a : b);
    double min = voltages.reduce((a, b) => a < b ? a : b);
    return (max - min) > 0.2; // 200mV imbalance threshold
  }
  
  // Get the bank with maximum voltage
  int get maxVoltageBankIndex {
    List<double> voltages = bankVoltages;
    double maxVoltage = voltages.reduce((a, b) => a > b ? a : b);
    return voltages.indexOf(maxVoltage);
  }
  
  // Get the bank with minimum voltage
  int get minVoltageBankIndex {
    List<double> voltages = bankVoltages;
    double minVoltage = voltages.reduce((a, b) => a < b ? a : b);
    return voltages.indexOf(minVoltage);
  }
  
  // Get voltage difference between max and min banks
  double get voltageImbalance {
    if (bankVoltages.where((v) => v > 0).length < 2) return 0.0;
    double max = bankVoltages.reduce((a, b) => a > b ? a : b);
    double min = bankVoltages.reduce((a, b) => a < b ? a : b);
    return max - min;
  }
  
  // Check if data is fresh (updated within last 30 seconds)
  bool get isDataFresh {
    return DateTime.now().difference(lastUpdate).inSeconds < 30;
  }
  
  // Update battery level
  void updateBatteryLevel(int level) {
    batteryLevel = level.clamp(0, 100);
    lastUpdate = DateTime.now();
  }
  
  // Update full battery voltage
  void updateFullBatteryVoltage(double voltage) {
    fullBatteryVoltage = voltage;
    lastUpdate = DateTime.now();
  }
  
  // Update individual bank voltage
  void updateBankVoltage(int bankIndex, double voltage) {
    switch (bankIndex) {
      case 0:
        bank1Voltage = voltage;
        break;
      case 1:
        bank2Voltage = voltage;
        break;
      case 2:
        bank3Voltage = voltage;
        break;
      case 3:
        bank4Voltage = voltage;
        break;
    }
    lastUpdate = DateTime.now();
  }
  
  // Update discharge switch state
  void updateDischargeSwitchState(bool enabled) {
    dischargeSwitchEnabled = enabled;
    lastUpdate = DateTime.now();
  }
  
  // Create a copy with updated values
  BMSData copyWith({
    int? batteryLevel,
    double? fullBatteryVoltage,
    double? bank1Voltage,
    double? bank2Voltage,
    double? bank3Voltage,
    double? bank4Voltage,
    bool? dischargeSwitchEnabled,
    DateTime? lastUpdate,
  }) {
    return BMSData(
      batteryLevel: batteryLevel ?? this.batteryLevel,
      fullBatteryVoltage: fullBatteryVoltage ?? this.fullBatteryVoltage,
      bank1Voltage: bank1Voltage ?? this.bank1Voltage,
      bank2Voltage: bank2Voltage ?? this.bank2Voltage,
      bank3Voltage: bank3Voltage ?? this.bank3Voltage,
      bank4Voltage: bank4Voltage ?? this.bank4Voltage,
      dischargeSwitchEnabled: dischargeSwitchEnabled ?? this.dischargeSwitchEnabled,
      lastUpdate: lastUpdate ?? this.lastUpdate,
    );
  }
  
  @override
  String toString() {
    return 'BMSData(batteryLevel: $batteryLevel%, '
           'fullVoltage: ${fullBatteryVoltage.toStringAsFixed(2)}V, '
           'banks: [${bank1Voltage.toStringAsFixed(2)}V, ${bank2Voltage.toStringAsFixed(2)}V, '
           '${bank3Voltage.toStringAsFixed(2)}V, ${bank4Voltage.toStringAsFixed(2)}V], '
           'dischargeSwitch: $dischargeSwitchEnabled, '
           'lastUpdate: $lastUpdate)';
  }
}