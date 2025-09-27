// Device Details Screen - Shows connected BMS device information
// Displays device info, GATT services, characteristics, and connection controls

import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import '../models/bms_device.dart';
import '../services/ble_service.dart';

class DeviceDetailsScreen extends StatefulWidget {
  final BMSDevice device;
  
  const DeviceDetailsScreen({Key? key, required this.device}) : super(key: key);
  
  @override
  _DeviceDetailsScreenState createState() => _DeviceDetailsScreenState();
}

class _DeviceDetailsScreenState extends State<DeviceDetailsScreen> {
  final BLEService _bleService = BLEService();
  List<BluetoothService> _services = [];
  bool _isLoadingServices = true;
  bool _isConnected = false;
  late StreamSubscription<BMSDevice?> _connectionSubscription;
  
  @override
  void initState() {
    super.initState();
    _initializeDeviceDetails();
  }
  
  Future<void> _initializeDeviceDetails() async {
    // Set initial connection state
    setState(() {
      _isConnected = true; // We arrive here after successful connection
    });
    
    // Listen to connection status changes
    _connectionSubscription = _bleService.connectedDeviceStream.listen((connectedDevice) {
      if (mounted) {
        setState(() {
          _isConnected = connectedDevice != null && 
                        connectedDevice.id == widget.device.id;
        });
      }
    });
    
    // Discover services and characteristics
    await _discoverServices();
  }
  
  Future<void> _discoverServices() async {
    try {
      print('🔍 Discovering services for ${widget.device.name}...');
      
      List<BluetoothService> services = await widget.device.bluetoothDevice.discoverServices();
      
      // Read initial values for readable characteristics
      for (BluetoothService service in services) {
        for (BluetoothCharacteristic characteristic in service.characteristics) {
          if (characteristic.properties.read) {
            try {
              await characteristic.read();
              print('📖 Read characteristic ${characteristic.uuid}');
            } catch (e) {
              print('❌ Failed to read ${characteristic.uuid}: $e');
            }
          }
        }
      }
      
      setState(() {
        _services = services;
        _isLoadingServices = false;
      });
      
      print('✅ Service discovery completed. Found ${services.length} services');
      
    } catch (e) {
      print('❌ Error discovering services: $e');
      setState(() {
        _isLoadingServices = false;
      });
      
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Failed to discover services: $e')),
        );
      }
    }
  }
  
  @override
  void dispose() {
    _connectionSubscription.cancel();
    super.dispose();
  }
  
  Future<void> _disconnectDevice() async {
    try {
      await _bleService.disconnect();
      
      if (mounted) {
        Navigator.of(context).pop(); // Return to scan screen
      }
    } catch (e) {
      print('❌ Error disconnecting: $e');
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Error disconnecting: $e')),
        );
      }
    }
  }
  
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Device Details'),
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        actions: [
          // Connection status indicator
          Container(
            padding: EdgeInsets.symmetric(horizontal: 16, vertical: 8),
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(
                  _isConnected ? Icons.bluetooth_connected : Icons.bluetooth_disabled,
                  color: _isConnected ? Colors.green : Colors.red,
                  size: 20,
                ),
                SizedBox(width: 8),
                Text(
                  _isConnected ? 'Connected' : 'Disconnected',
                  style: TextStyle(
                    color: _isConnected ? Colors.green : Colors.red,
                    fontWeight: FontWeight.bold,
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
      body: Column(
        children: [
          // Device Information Card
          _buildDeviceInfoCard(),
          
          // Services and Characteristics
          Expanded(
            child: _isLoadingServices
                ? _buildLoadingServices()
                : _buildServicesList(),
          ),
          
          // Disconnect Button
          _buildDisconnectButton(),
        ],
      ),
    );
  }
  
  Widget _buildDeviceInfoCard() {
    return Card(
      margin: EdgeInsets.all(16),
      child: Padding(
        padding: EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(Icons.bluetooth, color: Colors.blue, size: 32),
                SizedBox(width: 12),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        widget.device.name,
                        style: TextStyle(
                          fontSize: 20,
                          fontWeight: FontWeight.bold,
                        ),
                      ),
                      SizedBox(height: 4),
                      Text(
                        'Device Type: ${widget.device.deviceType}',
                        style: TextStyle(
                          fontSize: 14,
                          color: Colors.grey[600],
                        ),
                      ),
                    ],
                  ),
                ),
              ],
            ),
            
            Divider(height: 24),
            
            // Device Details
            _buildInfoRow('Device ID', widget.device.id),
            _buildInfoRow('RSSI', '${widget.device.rssi} dBm'),
            _buildInfoRow('Connection State', _isConnected ? 'Connected' : 'Disconnected'),
            _buildInfoRow('Discovered At', widget.device.discoveredAt.toString().substring(0, 19)),
          ],
        ),
      ),
    );
  }
  
  Widget _buildInfoRow(String label, String value) {
    return Padding(
      padding: EdgeInsets.symmetric(vertical: 4),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          SizedBox(
            width: 120,
            child: Text(
              '$label:',
              style: TextStyle(
                fontWeight: FontWeight.bold,
                color: Colors.grey[700],
              ),
            ),
          ),
          Expanded(
            child: Text(
              value,
              style: TextStyle(fontFamily: 'monospace'),
            ),
          ),
        ],
      ),
    );
  }
  
  Widget _buildLoadingServices() {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          CircularProgressIndicator(),
          SizedBox(height: 16),
          Text('Discovering services and characteristics...'),
        ],
      ),
    );
  }
  
  Widget _buildServicesList() {
    if (_services.isEmpty) {
      return Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(Icons.warning, size: 48, color: Colors.orange),
            SizedBox(height: 16),
            Text('No services discovered'),
            SizedBox(height: 8),
            ElevatedButton(
              onPressed: () {
                setState(() {
                  _isLoadingServices = true;
                });
                _discoverServices();
              },
              child: Text('Retry Discovery'),
            ),
          ],
        ),
      );
    }
    
    return ListView.builder(
      padding: EdgeInsets.all(16),
      itemCount: _services.length,
      itemBuilder: (context, index) {
        return _buildServiceCard(_services[index]);
      },
    );
  }
  
  Widget _buildServiceCard(BluetoothService service) {
    return Card(
      margin: EdgeInsets.only(bottom: 16),
      child: ExpansionTile(
        leading: Icon(Icons.settings_bluetooth, color: Colors.blue),
        title: Text(
          _getServiceName(service.uuid.toString()),
          style: TextStyle(fontWeight: FontWeight.bold),
        ),
        subtitle: Text(
          'UUID: ${service.uuid}',
          style: TextStyle(fontFamily: 'monospace', fontSize: 12),
        ),
        children: service.characteristics.map((characteristic) {
          return _buildCharacteristicTile(characteristic);
        }).toList(),
      ),
    );
  }
  
  Widget _buildCharacteristicTile(BluetoothCharacteristic characteristic) {
    List<String> properties = [];
    if (characteristic.properties.read) properties.add('Read');
    if (characteristic.properties.write) properties.add('Write');
    if (characteristic.properties.notify) properties.add('Notify');
    if (characteristic.properties.indicate) properties.add('Indicate');
    
    String value = 'No data';
    if (characteristic.lastValue.isNotEmpty) {
      // Try to display as readable data
      try {
        if (characteristic.lastValue.length == 1) {
          // Single byte - likely a number
          value = '${characteristic.lastValue[0]} (0x${characteristic.lastValue[0].toRadixString(16).padLeft(2, '0')})';
        } else {
          // Multiple bytes - show as hex
          value = characteristic.lastValue.map((b) => b.toRadixString(16).padLeft(2, '0')).join(' ');
        }
      } catch (e) {
        value = 'Binary data (${characteristic.lastValue.length} bytes)';
      }
    }
    
    return ListTile(
      contentPadding: EdgeInsets.only(left: 32, right: 16, top: 8, bottom: 8),
      leading: Icon(Icons.radio_button_unchecked, size: 16),
      title: Text(
        _getCharacteristicName(characteristic.uuid.toString()),
        style: TextStyle(fontSize: 14, fontWeight: FontWeight.w500),
      ),
      subtitle: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            'UUID: ${characteristic.uuid}',
            style: TextStyle(fontFamily: 'monospace', fontSize: 11),
          ),
          SizedBox(height: 2),
          Text(
            'Properties: ${properties.join(', ')}',
            style: TextStyle(fontSize: 11, color: Colors.blue[700]),
          ),
          if (characteristic.lastValue.isNotEmpty) ...[
            SizedBox(height: 2),
            Text(
              'Value: $value',
              style: TextStyle(fontSize: 11, color: Colors.green[700]),
            ),
          ],
        ],
      ),
      trailing: properties.contains('Read') 
          ? IconButton(
              icon: Icon(Icons.refresh, size: 16),
              onPressed: () => _readCharacteristic(characteristic),
            )
          : null,
    );
  }
  
  Future<void> _readCharacteristic(BluetoothCharacteristic characteristic) async {
    try {
      await characteristic.read();
      setState(() {}); // Refresh UI to show new value
      print('📖 Read ${characteristic.uuid}: ${characteristic.lastValue}');
    } catch (e) {
      print('❌ Failed to read ${characteristic.uuid}: $e');
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Failed to read characteristic: $e')),
      );
    }
  }
  
  Widget _buildDisconnectButton() {
    return Container(
      width: double.infinity,
      padding: EdgeInsets.all(16),
      child: ElevatedButton.icon(
        onPressed: _isConnected ? _disconnectDevice : null,
        icon: Icon(Icons.bluetooth_disabled),
        label: Text('Disconnect Device'),
        style: ElevatedButton.styleFrom(
          backgroundColor: Colors.red,
          foregroundColor: Colors.white,
          padding: EdgeInsets.symmetric(vertical: 16),
        ),
      ),
    );
  }
  
  String _getServiceName(String uuid) {
    // Convert common service UUIDs to readable names
    String shortUUID = uuid.toUpperCase();
    
    if (shortUUID.contains('180F')) return 'Battery Service';
    if (shortUUID.contains('1815')) return 'Automation IO Service';
    if (shortUUID.contains('1800')) return 'Generic Access';
    if (shortUUID.contains('1801')) return 'Generic Attribute';
    if (shortUUID.contains('180A')) return 'Device Information';
    
    return 'Custom Service';
  }
  
  String _getCharacteristicName(String uuid) {
    // Convert common characteristic UUIDs to readable names
    String shortUUID = uuid.toUpperCase();
    
    if (shortUUID.contains('2A19')) return 'Battery Level';
    if (shortUUID.contains('2A56')) return 'Digital Output';
    if (shortUUID.contains('2A58')) return 'Analog Input';
    if (shortUUID.contains('2A00')) return 'Device Name';
    if (shortUUID.contains('2A01')) return 'Appearance';
    if (shortUUID.contains('2A29')) return 'Manufacturer Name';
    if (shortUUID.contains('2A24')) return 'Model Number';
    
    return 'Custom Characteristic';
  }
}