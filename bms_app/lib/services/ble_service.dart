// BLE Service for BMS Application
// Handles Bluetooth Low Energy communication with BMS devices

import 'dart:async';
import 'dart:io';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';
import '../models/bms_device.dart';
import '../models/ble_constants.dart';
import '../utils/permissions.dart';
import '../utils/ble_debug.dart';

class BLEService {
  static final BLEService _instance = BLEService._internal();
  factory BLEService() => _instance;
  BLEService._internal();
  
  // Stream controllers for real-time updates
  final StreamController<List<BMSDevice>> _devicesController = 
      StreamController<List<BMSDevice>>.broadcast();
  final StreamController<BMSDevice?> _connectedDeviceController = 
      StreamController<BMSDevice?>.broadcast();
  final StreamController<String> _scanStatusController = 
      StreamController<String>.broadcast();
  final StreamController<bool> _isBluetoothOnController = 
      StreamController<bool>.broadcast();
  
  // Private variables
  List<BMSDevice> _discoveredDevices = [];
  BMSDevice? _connectedDevice;
  StreamSubscription<List<ScanResult>>? _scanSubscription;
  StreamSubscription<BluetoothAdapterState>? _adapterStateSubscription;
  bool _isScanning = false;
  
  // Public getters for streams
  Stream<List<BMSDevice>> get devicesStream => _devicesController.stream;
  Stream<BMSDevice?> get connectedDeviceStream => _connectedDeviceController.stream;
  Stream<String> get scanStatusStream => _scanStatusController.stream;
  Stream<bool> get isBluetoothOnStream => _isBluetoothOnController.stream;
  
  // Getters
  List<BMSDevice> get discoveredDevices => List.unmodifiable(_discoveredDevices);
  BMSDevice? get connectedDevice => _connectedDevice;
  bool get isScanning => _isScanning;
  
  // Initialize the BLE service
  Future<void> initialize() async {
    print('Initializing BLE Service...');
    
    // Listen to adapter state changes
    _adapterStateSubscription = FlutterBluePlus.adapterState.listen((state) {
      bool isOn = state == BluetoothAdapterState.on;
      _isBluetoothOnController.add(isOn);
      print('Bluetooth adapter state: $state');
      
      if (!isOn && _isScanning) {
        stopScan();
      }
    });
    
    // Check initial Bluetooth state
    BluetoothAdapterState state = await FlutterBluePlus.adapterState.first;
    _isBluetoothOnController.add(state == BluetoothAdapterState.on);
    
    print('BLE Service initialized. Bluetooth state: $state');
  }
  
  // Check and request necessary permissions
  Future<bool> checkAndRequestPermissions() async {
    BLEDebugHelper.log('Checking BLE permissions...');
    
    if (Platform.isAndroid) {
      // For Android 12+ (API 31+), we primarily need the new permissions
      // Check if we have the modern permissions that are already granted
      PermissionStatus scanStatus = await Permission.bluetoothScan.status;
      PermissionStatus connectStatus = await Permission.bluetoothConnect.status;
      
      BLEDebugHelper.log('Modern permissions - Scan: $scanStatus, Connect: $connectStatus');
      
      // If we have the modern permissions, we should be good to go
      if (scanStatus.isGranted && connectStatus.isGranted) {
        BLEDebugHelper.log('✅ Modern BLE permissions granted - should be able to scan');
        
        // Check if Bluetooth is supported
        if (!(await FlutterBluePlus.isSupported)) {
          _scanStatusController.add('Bluetooth not supported on this device');
          BLEDebugHelper.log('❌ Bluetooth not supported');
          return false;
        }
        
        // Check if Bluetooth is enabled
        BluetoothAdapterState state = await FlutterBluePlus.adapterState.first;
        if (state != BluetoothAdapterState.on) {
          _scanStatusController.add('Please enable Bluetooth');
          BLEDebugHelper.log('❌ Bluetooth not enabled: $state');
          return false;
        }
        
        BLEDebugHelper.log('✅ All BLE requirements met');
        return true;
      }
      
      // If we don't have modern permissions, fall back to requesting them
      BLEDebugHelper.log('❌ Missing modern permissions, requesting...');
      Map<Permission, PermissionStatus> statuses = await [
        Permission.bluetoothScan,
        Permission.bluetoothConnect,
        Permission.bluetoothAdvertise,
      ].request();
      
      bool allGranted = statuses.values.every((status) => 
          status == PermissionStatus.granted);
      
      if (!allGranted) {
        BLEDebugHelper.log('❌ Some modern permissions were denied');
        statuses.forEach((permission, status) {
          BLEDebugHelper.log('$permission: $status');
        });
        _scanStatusController.add('Bluetooth permissions required');
        return false;
      }
    }
    
    // Check if Bluetooth is supported
    if (!(await FlutterBluePlus.isSupported)) {
      _scanStatusController.add('Bluetooth not supported on this device');
      BLEDebugHelper.log('❌ Bluetooth not supported');
      return false;
    }
    
    // Check if Bluetooth is enabled
    BluetoothAdapterState state = await FlutterBluePlus.adapterState.first;
    if (state != BluetoothAdapterState.on) {
      _scanStatusController.add('Please enable Bluetooth');
      BLEDebugHelper.log('❌ Bluetooth not enabled: $state');
      return false;
    }
    
    BLEDebugHelper.log('✅ All BLE permissions and requirements met');
    return true;
  }
  
  // Start scanning for BMS devices
  Future<void> startScan({Duration? timeout}) async {
    BLEDebugHelper.log('Starting BLE scan...');
    
    if (_isScanning) {
      BLEDebugHelper.log('Scan already in progress');
      return;
    }
    
    // Check permissions first
    if (!(await checkAndRequestPermissions())) {
      BLEDebugHelper.log('Permissions check failed');
      return;
    }
    
    try {
      _isScanning = true;
      _discoveredDevices.clear();
      _devicesController.add(_discoveredDevices);
      _scanStatusController.add('Scanning for BMS devices...');
      BLEDebugHelper.log('Scan status updated: Scanning for BMS devices...');
      
      // Start scanning with timeout
      Duration scanTimeout = timeout ?? BLEConstants.scanTimeout;
      BLEDebugHelper.log('Starting FlutterBluePlus scan with timeout: ${scanTimeout.inSeconds}s');
      
      await FlutterBluePlus.startScan(
        timeout: scanTimeout,
        // Note: On some platforms, filtering by service UUID might not work
        // in advertisement phase, so we'll filter manually
      );
      
      // Listen to scan results
      _scanSubscription = FlutterBluePlus.scanResults.listen((results) {
        BLEDebugHelper.log('Received scan results: ${results.length} devices');
        for (ScanResult result in results) {
          _processScanResult(result);
        }
      });
      
      // Auto-stop scanning after timeout
      Timer(scanTimeout, () {
        if (_isScanning) {
          BLEDebugHelper.log('Scan timeout reached, stopping scan');
          stopScan();
        }
      });
      
      BLEDebugHelper.log('BLE scan started successfully');
      
    } catch (e) {
      BLEDebugHelper.log('Error starting BLE scan: $e');
      _isScanning = false;
      _scanStatusController.add('Error starting scan: $e');
    }
  }
  
  // Stop scanning
  Future<void> stopScan() async {
    print('Stopping BLE scan...');
    
    if (!_isScanning) {
      print('Scan not in progress');
      return;
    }
    
    try {
      await FlutterBluePlus.stopScan();
      _scanSubscription?.cancel();
      _scanSubscription = null;
      _isScanning = false;
      _scanStatusController.add('Scan completed. Found ${_discoveredDevices.length} BMS device(s)');
      print('BLE scan stopped successfully');
    } catch (e) {
      print('Error stopping BLE scan: $e');
      _scanStatusController.add('Error stopping scan: $e');
    }
  }
  
  // Process individual scan result
  void _processScanResult(ScanResult result) {
    String deviceName = result.device.platformName;
    String deviceId = result.device.remoteId.toString();
    int rssi = result.rssi;
    
    BLEDebugHelper.log('Processing scan result: Name="$deviceName", ID=$deviceId, RSSI=$rssi');
    
    // Filter for BMS devices only
    if (!BLEConstants.isBMSDevice(deviceName)) {
      BLEDebugHelper.log('Device "$deviceName" is not a BMS device, skipping');
      return;
    }
    
    BLEDebugHelper.log('Found BMS device: $deviceName');
    
    // Check if device already exists
    int existingIndex = _discoveredDevices.indexWhere(
        (device) => device.id == deviceId);
    
    if (existingIndex >= 0) {
      // Update existing device with new RSSI
      BLEDebugHelper.log('Updating existing device RSSI: $deviceName ($rssi)');
      _discoveredDevices[existingIndex].updateRSSI(rssi);
    } else {
      // Add new device
      BMSDevice bmsDevice = BMSDevice.fromScanResult(result);
      _discoveredDevices.add(bmsDevice);
      BLEDebugHelper.log('Added new BMS device: ${bmsDevice.name} (${bmsDevice.deviceType}) - RSSI: $rssi');
    }
    
    // Notify listeners
    _devicesController.add(List.from(_discoveredDevices));
    BLEDebugHelper.log('Device list updated, total BMS devices: ${_discoveredDevices.length}');
  }
  
  // Connect to a BMS device
  Future<bool> connectToDevice(BMSDevice device) async {
    print('Connecting to ${device.name}...');
    
    if (_connectedDevice != null) {
      await disconnect();
    }
    
    try {
      bool connected = await device.connect();
      if (connected) {
        _connectedDevice = device;
        _connectedDeviceController.add(_connectedDevice);
        
        // Discover services and characteristics
        await _discoverServicesAndCharacteristics(device);
        
        print('Successfully connected to ${device.name}');
        return true;
      }
    } catch (e) {
      print('Error connecting to ${device.name}: $e');
    }
    
    return false;
  }
  
  // Disconnect from current device
  Future<void> disconnect() async {
    if (_connectedDevice != null) {
      print('Disconnecting from ${_connectedDevice!.name}...');
      await _connectedDevice!.disconnect();
      _connectedDevice = null;
      _connectedDeviceController.add(null);
      print('Disconnected successfully');
    }
  }
  
  // Discover services and characteristics
  Future<void> _discoverServicesAndCharacteristics(BMSDevice device) async {
    print('Discovering services for ${device.name}...');
    
    try {
      List<BluetoothService> services = await device.bluetoothDevice.discoverServices();
      
      for (BluetoothService service in services) {
        print('Found service: ${service.uuid}');
        
        // Handle Battery Service
        if (service.uuid.toString().toUpperCase() == 
            BLEConstants.batteryServiceUUID.toUpperCase()) {
          await _setupBatteryService(service, device);
        }
        
        // Handle Automation IO Service
        else if (service.uuid.toString().toUpperCase() == 
            BLEConstants.automationIOServiceUUID.toUpperCase()) {
          await _setupAutomationIOService(service, device);
        }
      }
      
      print('Service discovery completed for ${device.name}');
    } catch (e) {
      print('Error discovering services for ${device.name}: $e');
    }
  }
  
  // Setup Battery Service characteristics
  Future<void> _setupBatteryService(BluetoothService service, BMSDevice device) async {
    for (BluetoothCharacteristic characteristic in service.characteristics) {
      if (characteristic.uuid.toString().toUpperCase() == 
          BLEConstants.batteryLevelUUID.toUpperCase()) {
        
        print('Setting up Battery Level characteristic');
        
        // Read initial value
        if (characteristic.properties.read) {
          try {
            List<int> value = await characteristic.read();
            if (value.isNotEmpty) {
              device.data.updateBatteryLevel(value[0]);
              _connectedDeviceController.add(device);
              print('Battery level: ${value[0]}%');
            }
          } catch (e) {
            print('Error reading battery level: $e');
          }
        }
        
        // Enable notifications if supported
        if (characteristic.properties.notify) {
          try {
            await characteristic.setNotifyValue(true);
            characteristic.lastValueStream.listen((value) {
              if (value.isNotEmpty) {
                device.data.updateBatteryLevel(value[0]);
                _connectedDeviceController.add(device);
                print('Battery level updated: ${value[0]}%');
              }
            });
            print('Battery level notifications enabled');
          } catch (e) {
            print('Error enabling battery level notifications: $e');
          }
        }
      }
    }
  }
  
  // Setup Automation IO Service characteristics
  Future<void> _setupAutomationIOService(BluetoothService service, BMSDevice device) async {
    for (BluetoothCharacteristic characteristic in service.characteristics) {
      String uuid = characteristic.uuid.toString().toUpperCase();
      
      // Handle Digital IO characteristic (switch control)
      if (uuid == BLEConstants.digitalIOUUID.toUpperCase()) {
        print('Setting up Digital IO characteristic');
        
        // Read initial switch state
        if (characteristic.properties.read) {
          try {
            List<int> value = await characteristic.read();
            if (value.isNotEmpty) {
              bool switchEnabled = value[0] == 1;
              device.data.updateDischargeSwitchState(switchEnabled);
              _connectedDeviceController.add(device);
              print('Discharge switch state: $switchEnabled');
            }
          } catch (e) {
            print('Error reading switch state: $e');
          }
        }
        
        // Enable notifications for switch state changes
        if (characteristic.properties.notify) {
          try {
            await characteristic.setNotifyValue(true);
            characteristic.lastValueStream.listen((value) {
              if (value.isNotEmpty) {
                bool switchEnabled = value[0] == 1;
                device.data.updateDischargeSwitchState(switchEnabled);
                _connectedDeviceController.add(device);
                print('Switch state updated: $switchEnabled');
              }
            });
            print('Switch state notifications enabled');
          } catch (e) {
            print('Error enabling switch notifications: $e');
          }
        }
      }
      
      // Handle voltage characteristics (would be custom UUIDs based on your GATT database)
      // This is a placeholder - you'll need to update with actual UUIDs from your implementation
    }
  }
  
  // Write to digital IO characteristic (switch control)
  Future<bool> toggleDischargeSwitch(bool enable) async {
    if (_connectedDevice == null) {
      print('No device connected');
      return false;
    }
    
    try {
      List<BluetoothService> services = await _connectedDevice!.bluetoothDevice.discoverServices();
      
      for (BluetoothService service in services) {
        if (service.uuid.toString().toUpperCase() == 
            BLEConstants.automationIOServiceUUID.toUpperCase()) {
          
          for (BluetoothCharacteristic characteristic in service.characteristics) {
            if (characteristic.uuid.toString().toUpperCase() == 
                BLEConstants.digitalIOUUID.toUpperCase()) {
              
              if (characteristic.properties.write || characteristic.properties.writeWithoutResponse) {
                // Send command: [1,0,0,0] for enable, [0,0,0,0] for disable
                List<int> command = enable ? [1, 0, 0, 0] : [0, 0, 0, 0];
                await characteristic.write(command);
                
                print('Switch command sent: ${enable ? "ON" : "OFF"}');
                return true;
              }
            }
          }
        }
      }
    } catch (e) {
      print('Error toggling discharge switch: $e');
    }
    
    return false;
  }
  
  // Dispose of resources
  void dispose() {
    print('Disposing BLE Service...');
    stopScan();
    disconnect();
    _scanSubscription?.cancel();
    _adapterStateSubscription?.cancel();
    _devicesController.close();
    _connectedDeviceController.close();
    _scanStatusController.close();
    _isBluetoothOnController.close();
  }
}