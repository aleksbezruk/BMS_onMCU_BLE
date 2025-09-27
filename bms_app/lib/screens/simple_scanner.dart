// Simple BLE Scanner for testing - shows ALL BLE devices
// This helps debug if the problem is with BLE scanning or BMS device filtering

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'dart:async';

class SimpleBLEScanner extends StatefulWidget {
  @override
  _SimpleBLEScannerState createState() => _SimpleBLEScannerState();
}

class _SimpleBLEScannerState extends State<SimpleBLEScanner> {
  List<ScanResult> _scanResults = [];
  bool _isScanning = false;
  StreamSubscription<List<ScanResult>>? _scanSubscription;
  
  @override
  void dispose() {
    _scanSubscription?.cancel();
    super.dispose();
  }
  
  void _startSimpleScan() async {
    setState(() {
      _scanResults.clear();
      _isScanning = true;
    });
    
    print('🔍 Starting simple BLE scan (all devices)...');
    
    try {
      // Start scanning for ALL BLE devices
      await FlutterBluePlus.startScan(timeout: Duration(seconds: 10));
      
      _scanSubscription = FlutterBluePlus.scanResults.listen((results) {
        print('📡 Found ${results.length} BLE devices');
        setState(() {
          _scanResults = results;
        });
        
        // Log all found devices
        for (ScanResult result in results) {
          String name = result.device.platformName.isEmpty 
              ? 'Unknown Device' 
              : result.device.platformName;
          print('  Device: "$name" (${result.device.remoteId}) RSSI: ${result.rssi}');
        }
      });
      
      // Stop scanning after 10 seconds
      Timer(Duration(seconds: 10), () {
        _stopScan();
      });
      
    } catch (e) {
      print('❌ Error starting simple scan: $e');
      setState(() {
        _isScanning = false;
      });
    }
  }
  
  void _stopScan() async {
    try {
      await FlutterBluePlus.stopScan();
      _scanSubscription?.cancel();
      setState(() {
        _isScanning = false;
      });
      print('🛑 Simple scan stopped. Found ${_scanResults.length} total devices.');
    } catch (e) {
      print('Error stopping scan: $e');
    }
  }
  
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Simple BLE Scanner - Shows ALL Devices'),
      ),
      body: Column(
        children: [
          // Scan button
          Padding(
            padding: EdgeInsets.all(16),
            child: SizedBox(
              width: double.infinity,
              child: ElevatedButton.icon(
                onPressed: _isScanning ? null : _startSimpleScan,
                icon: Icon(_isScanning ? Icons.hourglass_empty : Icons.search),
                label: Text(_isScanning ? 'Scanning...' : 'Scan for ALL BLE Devices'),
              ),
            ),
          ),
          
          // Status
          if (_isScanning)
            Container(
              padding: EdgeInsets.all(16),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  CircularProgressIndicator(strokeWidth: 2),
                  SizedBox(width: 16),
                  Text('Scanning for 10 seconds...'),
                ],
              ),
            ),
          
          // Results
          Expanded(
            child: _scanResults.isEmpty
                ? Center(
                    child: Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Icon(Icons.bluetooth_searching, size: 64, color: Colors.grey),
                        SizedBox(height: 16),
                        Text(
                          _isScanning 
                              ? 'Searching for BLE devices...'
                              : 'No BLE devices found.\nTap scan to search.',
                          textAlign: TextAlign.center,
                          style: TextStyle(color: Colors.grey[600]),
                        ),
                      ],
                    ),
                  )
                : ListView.builder(
                    itemCount: _scanResults.length,
                    itemBuilder: (context, index) {
                      ScanResult result = _scanResults[index];
                      String deviceName = result.device.platformName.isEmpty
                          ? 'Unknown Device'
                          : result.device.platformName;
                      
                      // Highlight BMS-like devices
                      bool isBMSLike = deviceName.toUpperCase().contains('BMS') ||
                                       deviceName.toUpperCase().contains('QN9080');
                      
                      return Card(
                        margin: EdgeInsets.symmetric(horizontal: 16, vertical: 4),
                        color: isBMSLike ? Colors.green[50] : null,
                        child: ListTile(
                          leading: Icon(
                            Icons.bluetooth,
                            color: isBMSLike ? Colors.green : Colors.blue,
                          ),
                          title: Text(
                            deviceName,
                            style: TextStyle(
                              fontWeight: isBMSLike ? FontWeight.bold : FontWeight.normal,
                              color: isBMSLike ? Colors.green[800] : null,
                            ),
                          ),
                          subtitle: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              Text('ID: ${result.device.remoteId}'),
                              Text('RSSI: ${result.rssi} dBm'),
                              if (isBMSLike)
                                Text(
                                  '🎯 POTENTIAL BMS DEVICE!',
                                  style: TextStyle(
                                    color: Colors.green[800],
                                    fontWeight: FontWeight.bold,
                                  ),
                                ),
                            ],
                          ),
                          trailing: Text(
                            '${result.rssi}',
                            style: TextStyle(
                              color: result.rssi > -60 
                                  ? Colors.green 
                                  : result.rssi > -80 
                                      ? Colors.orange 
                                      : Colors.red,
                              fontWeight: FontWeight.bold,
                            ),
                          ),
                        ),
                      );
                    },
                  ),
          ),
          
          // Instructions
          Container(
            padding: EdgeInsets.all(16),
            child: Text(
              'This scanner shows ALL BLE devices, not just BMS devices.\n'
              'If you see devices here, your BLE scanning works!\n'
              'Look for devices with "BMS" or "QN9080" in the name.',
              textAlign: TextAlign.center,
              style: TextStyle(fontSize: 12, color: Colors.grey[600]),
            ),
          ),
        ],
      ),
    );
  }
}