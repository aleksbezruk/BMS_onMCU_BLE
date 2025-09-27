// Targeted Permission Handler for Mixed Android Permission Requirements
// Handles the specific case where newer permissions are granted but legacy ones are denied

import 'dart:io';
import 'package:permission_handler/permission_handler.dart';
import 'package:flutter/material.dart';

class TargetedBLEPermissions {
  
  // Request only the missing critical permissions
  static Future<bool> requestCriticalMissingPermissions() async {
    if (!Platform.isAndroid) return true;
    
    print('🎯 Requesting critical missing BLE permissions...');
    
    // Check which permissions we actually need to request
    List<Permission> missingPermissions = [];
    
    // Check legacy Bluetooth permission (still needed on some Android versions)
    PermissionStatus bluetoothStatus = await Permission.bluetooth.status;
    if (!bluetoothStatus.isGranted) {
      missingPermissions.add(Permission.bluetooth);
      print('❌ Legacy Bluetooth permission missing');
    }
    
    // Check Location permission (required for BLE scanning on older Android)
    PermissionStatus locationStatus = await Permission.location.status;
    if (!locationStatus.isGranted) {
      missingPermissions.add(Permission.location);
      print('❌ Location permission missing');
    }
    
    if (missingPermissions.isEmpty) {
      print('✅ All critical permissions already granted');
      return true;
    }
    
    print('📱 Requesting ${missingPermissions.length} missing permissions...');
    
    // Request the missing permissions
    Map<Permission, PermissionStatus> results = await missingPermissions.request();
    
    // Check results
    bool allGranted = true;
    results.forEach((permission, status) {
      bool granted = status.isGranted;
      allGranted = allGranted && granted;
      print('${granted ? "✅" : "❌"} $permission: $status');
    });
    
    if (allGranted) {
      print('🎉 All critical permissions granted!');
    } else {
      print('⚠️ Some critical permissions still missing');
    }
    
    return allGranted;
  }
  
  // Get detailed status of all BLE-related permissions
  static Future<Map<String, bool>> getDetailedPermissionStatus() async {
    Map<String, bool> status = {};
    
    if (Platform.isAndroid) {
      List<Permission> permissions = [
        Permission.bluetooth,
        Permission.bluetoothScan,
        Permission.bluetoothConnect,
        Permission.bluetoothAdvertise,
        Permission.location,
      ];
      
      for (Permission permission in permissions) {
        PermissionStatus permStatus = await permission.status;
        status[permission.toString()] = permStatus.isGranted;
        print('📋 ${permission.toString()}: ${permStatus.isGranted ? "✅ Granted" : "❌ Denied"}');
      }
    }
    
    return status;
  }
  
  // Show targeted permission dialog for missing permissions
  static void showTargetedPermissionDialog(BuildContext context, List<String> missingPermissions) {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: Row(
            children: [
              Icon(Icons.warning, color: Colors.orange),
              SizedBox(width: 8),
              Text('Missing BLE Permissions'),
            ],
          ),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                'Your device needs these additional permissions for BLE scanning:',
                style: TextStyle(fontWeight: FontWeight.bold),
              ),
              SizedBox(height: 12),
              
              if (missingPermissions.contains('bluetooth'))
                _buildMissingPermissionItem(
                  '📱', 
                  'Legacy Bluetooth',
                  'Required for older Android versions'
                ),
              
              if (missingPermissions.contains('location'))
                _buildMissingPermissionItem(
                  '📍', 
                  'Location Access',
                  'Required by Android for BLE scanning\n(Your location is NOT tracked)'
                ),
              
              SizedBox(height: 16),
              Container(
                padding: EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: Colors.blue[50],
                  borderRadius: BorderRadius.circular(8),
                  border: Border.all(color: Colors.blue[200]!),
                ),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      '💡 Why these permissions?',
                      style: TextStyle(fontWeight: FontWeight.bold, color: Colors.blue[800]),
                    ),
                    SizedBox(height: 4),
                    Text(
                      'Android requires Location access for BLE scanning to protect privacy. Your location data is never collected or stored by this app.',
                      style: TextStyle(fontSize: 12, color: Colors.blue[700]),
                    ),
                  ],
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
                _requestMissingPermissions();
              },
              child: Text('Grant Permissions'),
            ),
          ],
        );
      },
    );
  }
  
  static Widget _buildMissingPermissionItem(String icon, String title, String description) {
    return Padding(
      padding: EdgeInsets.symmetric(vertical: 8),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(icon, style: TextStyle(fontSize: 20)),
          SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(title, style: TextStyle(fontWeight: FontWeight.bold)),
                Text(description, style: TextStyle(fontSize: 12, color: Colors.grey[600])),
              ],
            ),
          ),
        ],
      ),
    );
  }
  
  static Future<void> _requestMissingPermissions() async {
    await requestCriticalMissingPermissions();
  }
  
  // Quick check if we have the minimum permissions needed
  static Future<bool> hasMinimumPermissions() async {
    if (!Platform.isAndroid) return true;
    
    // For newer Android (12+), we need bluetoothScan and bluetoothConnect
    // For older Android, we need bluetooth and location
    
    PermissionStatus bluetoothScan = await Permission.bluetoothScan.status;
    PermissionStatus bluetoothConnect = await Permission.bluetoothConnect.status;
    PermissionStatus bluetooth = await Permission.bluetooth.status;
    PermissionStatus location = await Permission.location.status;
    
    bool hasModernPermissions = bluetoothScan.isGranted && bluetoothConnect.isGranted;
    bool hasLegacyPermissions = bluetooth.isGranted && location.isGranted;
    
    print('🔍 Permission check:');
    print('  Modern (Android 12+): ${hasModernPermissions ? "✅" : "❌"}');
    print('  Legacy (Android <12): ${hasLegacyPermissions ? "✅" : "❌"}');
    
    // We need either modern permissions OR legacy permissions
    return hasModernPermissions || hasLegacyPermissions;
  }
}