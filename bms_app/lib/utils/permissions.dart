// BLE Permission Handler for Android
// Handles all BLE-related permissions and guides users through setup

import 'dart:io';
import 'package:permission_handler/permission_handler.dart';
import 'package:flutter/material.dart';

class BLEPermissions {
  // Check if all required permissions are granted
  static Future<bool> arePermissionsGranted() async {
    if (!Platform.isAndroid) return true;
    
    List<Permission> requiredPermissions = [
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      Permission.bluetoothAdvertise,
      Permission.location,
    ];
    
    for (Permission permission in requiredPermissions) {
      PermissionStatus status = await permission.status;
      if (!status.isGranted) {
        print('Permission $permission not granted: $status');
        return false;
      }
    }
    
    return true;
  }
  
  // Request all required permissions
  static Future<bool> requestPermissions() async {
    if (!Platform.isAndroid) return true;
    
    print('Requesting BLE permissions...');
    
    // Request permissions one by one with explanations
    Map<Permission, PermissionStatus> results = await [
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      Permission.bluetoothAdvertise,
      Permission.location,
    ].request();
    
    print('Permission results:');
    results.forEach((permission, status) {
      print('$permission: $status');
    });
    
    // Check if all permissions were granted
    bool allGranted = results.values.every((status) => status.isGranted);
    
    if (!allGranted) {
      print('Not all permissions granted');
      // Check if any were permanently denied
      bool anyPermanentlyDenied = results.values.any((status) => status.isPermanentlyDenied);
      if (anyPermanentlyDenied) {
        print('Some permissions permanently denied - need to open settings');
      }
    }
    
    return allGranted;
  }
  
  // Show permission explanation dialog
  static void showPermissionDialog(BuildContext context, VoidCallback onContinue) {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text('Bluetooth Permissions Required'),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                'This app needs Bluetooth permissions to:',
                style: TextStyle(fontWeight: FontWeight.bold),
              ),
              SizedBox(height: 8),
              _buildPermissionItem('🔍', 'Scan for BMS devices'),
              _buildPermissionItem('🔗', 'Connect to your BMS'),
              _buildPermissionItem('📍', 'Location (required by Android for BLE)'),
              SizedBox(height: 16),
              Text(
                'Your location data is NOT collected or stored.',
                style: TextStyle(
                  fontSize: 12,
                  color: Colors.grey[600],
                  fontStyle: FontStyle.italic,
                ),
              ),
            ],
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(context).pop(),
              child: Text('Cancel'),
            ),
            ElevatedButton(
              onPressed: () {
                Navigator.of(context).pop();
                onContinue();
              },
              child: Text('Grant Permissions'),
            ),
          ],
        );
      },
    );
  }
  
  // Show settings dialog when permissions are permanently denied
  static void showSettingsDialog(BuildContext context) {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Text('Open Settings'),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text('Bluetooth permissions were denied.'),
              SizedBox(height: 8),
              Text('To use this app, please:'),
              SizedBox(height: 8),
              _buildStepItem('1.', 'Tap "Open Settings"'),
              _buildStepItem('2.', 'Go to "Permissions"'),
              _buildStepItem('3.', 'Enable all permissions'),
              _buildStepItem('4.', 'Return to the app'),
            ],
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(context).pop(),
              child: Text('Cancel'),
            ),
            ElevatedButton(
              onPressed: () {
                Navigator.of(context).pop();
                openAppSettings();
              },
              child: Text('Open Settings'),
            ),
          ],
        );
      },
    );
  }
  
  static Widget _buildPermissionItem(String icon, String description) {
    return Padding(
      padding: EdgeInsets.symmetric(vertical: 2),
      child: Row(
        children: [
          Text(icon, style: TextStyle(fontSize: 16)),
          SizedBox(width: 8),
          Expanded(child: Text(description)),
        ],
      ),
    );
  }
  
  static Widget _buildStepItem(String number, String step) {
    return Padding(
      padding: EdgeInsets.symmetric(vertical: 2),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(number, style: TextStyle(fontWeight: FontWeight.bold)),
          SizedBox(width: 8),
          Expanded(child: Text(step)),
        ],
      ),
    );
  }
  
  // Check individual permission status
  static Future<PermissionStatus> getPermissionStatus(Permission permission) async {
    return await permission.status;
  }
  
  // Get detailed permission info for debugging
  static Future<Map<String, String>> getPermissionDetails() async {
    if (!Platform.isAndroid) return {'platform': 'Not Android'};
    
    Map<String, String> details = {};
    
    List<Permission> permissions = [
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      Permission.bluetoothAdvertise,
      Permission.location,
    ];
    
    for (Permission permission in permissions) {
      PermissionStatus status = await permission.status;
      details[permission.toString()] = status.toString();
    }
    
    return details;
  }
}