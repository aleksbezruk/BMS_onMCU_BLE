// BLE Debug Helper for troubleshooting BLE issues
// Provides detailed logging and diagnostic information

import 'dart:async';
import 'dart:io';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:flutter/material.dart';

class BLEDebugHelper {
  static final List<String> _debugLogs = [];
  
  // Get all debug logs
  static List<String> get debugLogs => List.unmodifiable(_debugLogs);
  
  // Clear debug logs
  static void clearLogs() {
    _debugLogs.clear();
  }
  
  // Add debug log with timestamp
  static void log(String message) {
    String timestamp = DateTime.now().toString().substring(11, 19);
    String logEntry = '[$timestamp] $message';
    _debugLogs.add(logEntry);
    print(logEntry);
  }
  
  // Comprehensive BLE system check
  static Future<Map<String, dynamic>> performFullDiagnostic() async {
    log('Starting comprehensive BLE diagnostic...');
    
    Map<String, dynamic> diagnostic = {
      'timestamp': DateTime.now().toIso8601String(),
      'platform': Platform.operatingSystem,
      'bluetoothSupported': false,
      'bluetoothState': 'unknown',
      'permissions': {},
      'scanCapable': false,
      'errors': [],
      'recommendations': [],
    };
    
    try {
      // 1. Check Bluetooth support
      log('Checking Bluetooth support...');
      diagnostic['bluetoothSupported'] = await FlutterBluePlus.isSupported;
      log('Bluetooth supported: ${diagnostic['bluetoothSupported']}');
      
      if (!diagnostic['bluetoothSupported']) {
        diagnostic['errors'].add('Bluetooth not supported on this device');
        diagnostic['recommendations'].add('This device does not support Bluetooth');
        return diagnostic;
      }
      
      // 2. Check Bluetooth state
      log('Checking Bluetooth adapter state...');
      BluetoothAdapterState state = await FlutterBluePlus.adapterState.first;
      diagnostic['bluetoothState'] = state.toString();
      log('Bluetooth state: ${state}');
      
      if (state != BluetoothAdapterState.on) {
        diagnostic['errors'].add('Bluetooth is not enabled');
        diagnostic['recommendations'].add('Please turn on Bluetooth in system settings');
      }
      
      // 3. Check all permissions
      log('Checking permissions...');
      diagnostic['permissions'] = await _checkAllPermissions();
      
      // 4. Test scanning capability
      log('Testing scan capability...');
      diagnostic['scanCapable'] = await _testScanCapability();
      
      // 5. Generate recommendations based on findings
      _generateRecommendations(diagnostic);
      
      log('Diagnostic completed successfully');
      
    } catch (e, stackTrace) {
      log('Error during diagnostic: $e');
      log('Stack trace: $stackTrace');
      diagnostic['errors'].add('Diagnostic error: $e');
    }
    
    return diagnostic;
  }
  
  // Check all relevant permissions
  static Future<Map<String, dynamic>> _checkAllPermissions() async {
    Map<String, dynamic> permissionStatus = {};
    
    if (Platform.isAndroid) {
      List<Permission> permissions = [
        Permission.bluetooth,
        Permission.bluetoothScan,
        Permission.bluetoothConnect,
        Permission.bluetoothAdvertise,
        Permission.location,
        Permission.locationWhenInUse,
        Permission.locationAlways,
      ];
      
      for (Permission permission in permissions) {
        try {
          PermissionStatus status = await permission.status;
          permissionStatus[permission.toString()] = {
            'status': status.toString(),
            'isGranted': status.isGranted,
            'isDenied': status.isDenied,
            'isPermanentlyDenied': status.isPermanentlyDenied,
            'isRestricted': status.isRestricted,
          };
          log('Permission ${permission}: ${status}');
        } catch (e) {
          log('Error checking permission ${permission}: $e');
          permissionStatus[permission.toString()] = {'error': e.toString()};
        }
      }
    } else {
      permissionStatus['platform'] = 'Non-Android platform';
    }
    
    return permissionStatus;
  }
  
  // Test if scanning actually works
  static Future<bool> _testScanCapability() async {
    try {
      log('Attempting test scan...');
      
      // Try to start a very short scan
      bool scanStarted = false;
      Timer? scanTimer;
      
      // Listen for scan results
      StreamSubscription? scanSubscription;
      Completer<bool> scanCompleter = Completer<bool>();
      
      scanSubscription = FlutterBluePlus.scanResults.listen((results) {
        log('Scan results received: ${results.length} devices');
        if (!scanCompleter.isCompleted) {
          scanCompleter.complete(true);
        }
      });
      
      try {
        await FlutterBluePlus.startScan(timeout: Duration(seconds: 3));
        scanStarted = true;
        log('Scan started successfully');
        
        // Wait for scan completion or timeout
        scanTimer = Timer(Duration(seconds: 4), () {
          if (!scanCompleter.isCompleted) {
            log('Scan test timed out');
            scanCompleter.complete(false);
          }
        });
        
        bool scanWorked = await scanCompleter.future;
        log('Scan test result: $scanWorked');
        
        return scanWorked;
        
      } catch (e) {
        log('Error starting test scan: $e');
        if (!scanCompleter.isCompleted) {
          scanCompleter.complete(false);
        }
        return false;
      } finally {
        scanTimer?.cancel();
        scanSubscription?.cancel();
        if (scanStarted) {
          try {
            await FlutterBluePlus.stopScan();
            log('Test scan stopped');
          } catch (e) {
            log('Error stopping test scan: $e');
          }
        }
      }
    } catch (e) {
      log('Test scan capability error: $e');
      return false;
    }
  }
  
  // Generate recommendations based on diagnostic results
  static void _generateRecommendations(Map<String, dynamic> diagnostic) {
    List<String> recommendations = List<String>.from(diagnostic['recommendations'] ?? []);
    
    // Check permissions and add specific recommendations
    Map<String, dynamic> permissions = diagnostic['permissions'] ?? {};
    
    permissions.forEach((permission, details) {
      if (details is Map && details['isGranted'] == false) {
        if (permission.contains('bluetooth')) {
          recommendations.add('Grant Bluetooth permissions: Settings → Apps → BMS App → Permissions → Nearby devices → Allow');
        } else if (permission.contains('location')) {
          recommendations.add('Grant Location permission: Settings → Apps → BMS App → Permissions → Location → Allow');
          recommendations.add('Enable Location Services: Settings → Location → Turn ON');
        }
      }
    });
    
    // Check scan capability
    if (diagnostic['scanCapable'] == false) {
      recommendations.add('BLE scanning failed - check if another app is using Bluetooth');
      recommendations.add('Try restarting Bluetooth: Settings → Bluetooth → Turn OFF/ON');
      recommendations.add('Try restarting the phone');
    }
    
    // Check Bluetooth state
    if (diagnostic['bluetoothState'] != 'BluetoothAdapterState.on') {
      recommendations.add('Turn on Bluetooth: Settings → Bluetooth → ON');
    }
    
    diagnostic['recommendations'] = recommendations;
  }
  
  // Get Android version info
  static Future<String> getAndroidInfo() async {
    try {
      // This would need additional plugins to get detailed Android info
      return 'Android ${Platform.operatingSystemVersion}';
    } catch (e) {
      return 'Unknown Android version';
    }
  }
  
  // Create debug report widget
  static Widget buildDebugReport(Map<String, dynamic> diagnostic) {
    return Card(
      margin: EdgeInsets.all(16),
      child: Padding(
        padding: EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'BLE Debug Report',
              style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
            ),
            Divider(),
            
            // System Info
            _buildSection('System Information', [
              'Platform: ${diagnostic['platform']}',
              'Bluetooth Supported: ${diagnostic['bluetoothSupported']}',
              'Bluetooth State: ${diagnostic['bluetoothState']}',
              'Scan Capable: ${diagnostic['scanCapable']}',
            ]),
            
            // Permissions
            if (diagnostic['permissions'] != null)
              _buildPermissionSection(diagnostic['permissions']),
            
            // Errors
            if (diagnostic['errors']?.isNotEmpty ?? false)
              _buildSection('Errors', diagnostic['errors']),
            
            // Recommendations
            if (diagnostic['recommendations']?.isNotEmpty ?? false)
              _buildSection('Recommendations', diagnostic['recommendations']),
          ],
        ),
      ),
    );
  }
  
  static Widget _buildSection(String title, List<dynamic> items) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        SizedBox(height: 16),
        Text(
          title,
          style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
        ),
        SizedBox(height: 8),
        ...items.map((item) => Padding(
          padding: EdgeInsets.only(left: 16, bottom: 4),
          child: Text('• $item'),
        )),
      ],
    );
  }
  
  static Widget _buildPermissionSection(Map<String, dynamic> permissions) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        SizedBox(height: 16),
        Text(
          'Permissions',
          style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
        ),
        SizedBox(height: 8),
        ...permissions.entries.map((entry) {
          String permission = entry.key;
          Map<String, dynamic> details = entry.value is Map ? entry.value : {};
          bool isGranted = details['isGranted'] ?? false;
          
          return Padding(
            padding: EdgeInsets.only(left: 16, bottom: 4),
            child: Row(
              children: [
                Icon(
                  isGranted ? Icons.check_circle : Icons.error,
                  color: isGranted ? Colors.green : Colors.red,
                  size: 16,
                ),
                SizedBox(width: 8),
                Expanded(
                  child: Text(
                    '$permission: ${details['status'] ?? 'Unknown'}',
                    style: TextStyle(fontSize: 12),
                  ),
                ),
              ],
            ),
          );
        }),
      ],
    );
  }
}