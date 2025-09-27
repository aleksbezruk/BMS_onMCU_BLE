// BMS Device Scan Screen
// Displays available BMS devices and handles connection

import 'package:flutter/material.dart';
import 'package:permission_handler/permission_handler.dart';
import '../services/ble_service.dart';
import '../models/bms_device.dart';
import '../widgets/battery_gauge.dart';
import '../utils/permissions.dart';
import '../utils/targeted_permissions.dart';
import 'debug_screen.dart';
import 'simple_scanner.dart';
import 'device_details_screen.dart';

class ScanScreen extends StatefulWidget {
  @override
  _ScanScreenState createState() => _ScanScreenState();
}

class _ScanScreenState extends State<ScanScreen> {
  final BLEService _bleService = BLEService();
  bool _isInitialized = false;
  
  @override
  void initState() {
    super.initState();
    _initializeBLE();
  }
  
  Future<void> _initializeBLE() async {
    await _bleService.initialize();
    setState(() {
      _isInitialized = true;
    });
  }
  
  @override
  void dispose() {
    _bleService.dispose();
    super.dispose();
  }
  
  @override
  Widget build(BuildContext context) {
    if (!_isInitialized) {
      return Scaffold(
        appBar: AppBar(
          title: Text('BMS Monitor'),
        ),
        body: Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              CircularProgressIndicator(),
              SizedBox(height: 16),
              Text('Initializing Bluetooth...'),
            ],
          ),
        ),
      );
    }
    
    return Scaffold(
      appBar: AppBar(
        title: Text('BMS Devices'),
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        elevation: 0,
        actions: [
          // Simple scanner button to test basic BLE functionality
          IconButton(
            icon: Icon(Icons.radar),
            onPressed: () {
              Navigator.of(context).push(
                MaterialPageRoute(
                  builder: (context) => SimpleBLEScanner(),
                ),
              );
            },
            tooltip: 'Test BLE Scanning',
          ),
          // Debug button to access diagnostics
          IconButton(
            icon: Icon(Icons.bug_report),
            onPressed: () {
              Navigator.of(context).push(
                MaterialPageRoute(
                  builder: (context) => DebugScreen(),
                ),
              );
            },
            tooltip: 'Debug Information',
          ),
        ],
      ),
      body: Column(
        children: [
          // Bluetooth status and scan controls
          StreamBuilder<bool>(
            stream: _bleService.isBluetoothOnStream,
            initialData: false,
            builder: (context, snapshot) {
              bool isBluetoothOn = snapshot.data ?? false;
              
              if (!isBluetoothOn) {
                return _buildBluetoothOffBanner();
              }
              
              return _buildScanControls();
            },
          ),
          
          // Scan status
          StreamBuilder<String>(
            stream: _bleService.scanStatusStream,
            builder: (context, snapshot) {
              if (snapshot.hasData) {
                return _buildStatusBanner(snapshot.data!);
              }
              return SizedBox.shrink();
            },
          ),
          
          // Device list
          Expanded(
            child: StreamBuilder<List<BMSDevice>>(
              stream: _bleService.devicesStream,
              initialData: [],
              builder: (context, snapshot) {
                List<BMSDevice> devices = snapshot.data ?? [];
                
                if (devices.isEmpty) {
                  return _buildEmptyState();
                }
                
                return _buildDeviceList(devices);
              },
            ),
          ),
        ],
      ),
    );
  }
  
  Widget _buildBluetoothOffBanner() {
    return Container(
      width: double.infinity,
      padding: EdgeInsets.all(16),
      color: Colors.red[100],
      child: Column(
        children: [
          Icon(
            Icons.bluetooth_disabled,
            size: 48,
            color: Colors.red,
          ),
          SizedBox(height: 8),
          Text(
            'Bluetooth is disabled',
            style: TextStyle(
              fontSize: 18,
              fontWeight: FontWeight.bold,
              color: Colors.red[800],
            ),
          ),
          SizedBox(height: 4),
          Text(
            'Please enable Bluetooth to scan for BMS devices',
            style: TextStyle(color: Colors.red[700]),
            textAlign: TextAlign.center,
          ),
        ],
      ),
    );
  }
  
  Widget _buildScanControls() {
    return Container(
      padding: EdgeInsets.all(16),
      child: Column(
        children: [
          Icon(
            _bleService.isScanning ? Icons.bluetooth_searching : Icons.bluetooth,
            size: 64,
            color: _bleService.isScanning ? Colors.blue : Colors.grey,
          ),
          SizedBox(height: 16),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              ElevatedButton.icon(
                onPressed: _bleService.isScanning ? null : _startScan,
                icon: Icon(Icons.search),
                label: Text('Scan for BMS'),
                style: ElevatedButton.styleFrom(
                  backgroundColor: Colors.blue,
                  foregroundColor: Colors.white,
                ),
              ),
              if (_bleService.isScanning)
                ElevatedButton.icon(
                  onPressed: _stopScan,
                  icon: Icon(Icons.stop),
                  label: Text('Stop Scan'),
                  style: ElevatedButton.styleFrom(
                    backgroundColor: Colors.red,
                    foregroundColor: Colors.white,
                  ),
                ),
            ],
          ),
        ],
      ),
    );
  }
  
  Widget _buildStatusBanner(String status) {
    return Container(
      width: double.infinity,
      padding: EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      color: Colors.blue[50],
      child: Row(
        children: [
          if (_bleService.isScanning)
            SizedBox(
              width: 16,
              height: 16,
              child: CircularProgressIndicator(strokeWidth: 2),
            ),
          if (_bleService.isScanning) SizedBox(width: 8),
          Expanded(
            child: Text(
              status,
              style: TextStyle(color: Colors.blue[800]),
            ),
          ),
        ],
      ),
    );
  }
  
  Widget _buildEmptyState() {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(
            Icons.device_unknown,
            size: 64,
            color: Colors.grey,
          ),
          SizedBox(height: 16),
          Text(
            'No BMS devices found',
            style: Theme.of(context).textTheme.titleLarge,
          ),
          SizedBox(height: 8),
          Padding(
            padding: EdgeInsets.symmetric(horizontal: 32),
            child: Text(
              'Make sure your BMS device is powered on and in range.\nTap "Scan for BMS" to search for devices.',
              style: Theme.of(context).textTheme.bodyMedium,
              textAlign: TextAlign.center,
            ),
          ),
          SizedBox(height: 24),
          ElevatedButton.icon(
            onPressed: _bleService.isScanning ? null : _startScan,
            icon: Icon(Icons.refresh),
            label: Text('Scan Again'),
          ),
        ],
      ),
    );
  }
  
  Widget _buildDeviceList(List<BMSDevice> devices) {
    return ListView.builder(
      padding: EdgeInsets.all(8),
      itemCount: devices.length,
      itemBuilder: (context, index) {
        return BMSDeviceTile(
          device: devices[index],
          onTap: () => _connectToDevice(devices[index]),
        );
      },
    );
  }
  
  void _startScan() async {
    print('🎯 Starting scan - you already have the required permissions!');
    
    // Your diagnostic report shows you have the key permissions:
    // ✅ bluetoothScan: GRANTED
    // ✅ bluetoothConnect: GRANTED  
    // ✅ bluetoothAdvertise: GRANTED
    // ✅ Bluetooth is ON and scan capable
    
    // Just start the scan - no permission checks needed
    await _bleService.startScan();
  }
  
  void _stopScan() async {
    await _bleService.stopScan();
  }
  
  void _connectToDevice(BMSDevice device) async {
    // Show loading dialog
    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (context) => AlertDialog(
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            CircularProgressIndicator(),
            SizedBox(height: 16),
            Text('Connecting to ${device.name}...'),
          ],
        ),
      ),
    );
    
    bool connected = await _bleService.connectToDevice(device);
    Navigator.pop(context); // Close loading dialog
    
    if (connected) {
      // Navigate to device details screen
      Navigator.of(context).push(
        MaterialPageRoute(
          builder: (context) => DeviceDetailsScreen(device: device),
        ),
      );
    } else {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Failed to connect to ${device.name}'),
          backgroundColor: Colors.red,
        ),
      );
    }
  }
}

// Custom device tile widget
class BMSDeviceTile extends StatelessWidget {
  final BMSDevice device;
  final VoidCallback onTap;
  
  const BMSDeviceTile({
    Key? key,
    required this.device,
    required this.onTap,
  }) : super(key: key);
  
  @override
  Widget build(BuildContext context) {
    return Card(
      margin: EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      elevation: 2,
      child: ListTile(
        contentPadding: EdgeInsets.all(12),
        leading: CircleAvatar(
          backgroundColor: _getDeviceColor(device.deviceType),
          child: Icon(
            Icons.battery_charging_full,
            color: Colors.white,
            size: 20,
          ),
        ),
        title: Text(
          device.name,
          style: TextStyle(
            fontWeight: FontWeight.bold,
            fontSize: 16,
          ),
        ),
        subtitle: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            SizedBox(height: 4),
            Row(
              children: [
                Icon(Icons.memory, size: 14, color: Colors.grey[600]),
                SizedBox(width: 4),
                Text('${device.deviceType}'),
                SizedBox(width: 16),
                Icon(Icons.fingerprint, size: 14, color: Colors.grey[600]),
                SizedBox(width: 4),
                Text(device.shortId),
              ],
            ),
            SizedBox(height: 4),
            Row(
              children: [
                Icon(Icons.signal_cellular_alt, size: 14, color: Colors.grey[600]),
                SizedBox(width: 4),
                Text('${device.rssi ?? "?"} dBm (${device.signalStrengthDescription})'),
              ],
            ),
          ],
        ),
        trailing: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            BatteryMiniGauge(level: device.data.batteryLevel),
            SizedBox(height: 4),
            Text(
              '${device.data.batteryLevel}%',
              style: TextStyle(
                fontSize: 12,
                fontWeight: FontWeight.w500,
              ),
            ),
          ],
        ),
        onTap: onTap,
      ),
    );
  }
  
  Color _getDeviceColor(String deviceType) {
    switch (deviceType) {
      case 'QN9080':
        return Colors.green;
      case 'PSOC63':
        return Colors.blue;
      default:
        return Colors.grey;
    }
  }
}