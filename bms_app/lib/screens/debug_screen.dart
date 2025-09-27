// Debug Screen for BLE troubleshooting
// Shows detailed diagnostic information and logs

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../utils/ble_debug.dart';

class DebugScreen extends StatefulWidget {
  @override
  _DebugScreenState createState() => _DebugScreenState();
}

class _DebugScreenState extends State<DebugScreen> {
  Map<String, dynamic>? _diagnostic;
  bool _isRunningDiagnostic = false;
  
  @override
  void initState() {
    super.initState();
    _runDiagnostic();
  }
  
  Future<void> _runDiagnostic() async {
    setState(() {
      _isRunningDiagnostic = true;
    });
    
    try {
      Map<String, dynamic> result = await BLEDebugHelper.performFullDiagnostic();
      setState(() {
        _diagnostic = result;
        _isRunningDiagnostic = false;
      });
    } catch (e) {
      setState(() {
        _isRunningDiagnostic = false;
      });
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Error running diagnostic: $e')),
      );
    }
  }
  
  void _copyDiagnosticToClipboard() {
    if (_diagnostic == null) return;
    
    String diagnosticText = _formatDiagnosticAsText(_diagnostic!);
    Clipboard.setData(ClipboardData(text: diagnosticText));
    
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('Diagnostic copied to clipboard')),
    );
  }
  
  String _formatDiagnosticAsText(Map<String, dynamic> diagnostic) {
    StringBuffer buffer = StringBuffer();
    buffer.writeln('BMS App - BLE Diagnostic Report');
    buffer.writeln('Generated: ${diagnostic['timestamp']}');
    buffer.writeln('=' * 40);
    
    buffer.writeln('\nSystem Information:');
    buffer.writeln('Platform: ${diagnostic['platform']}');
    buffer.writeln('Bluetooth Supported: ${diagnostic['bluetoothSupported']}');
    buffer.writeln('Bluetooth State: ${diagnostic['bluetoothState']}');
    buffer.writeln('Scan Capable: ${diagnostic['scanCapable']}');
    
    if (diagnostic['permissions'] != null) {
      buffer.writeln('\nPermissions:');
      Map<String, dynamic> permissions = diagnostic['permissions'];
      permissions.forEach((key, value) {
        if (value is Map) {
          buffer.writeln('$key: ${value['status']} (Granted: ${value['isGranted']})');
        }
      });
    }
    
    if (diagnostic['errors']?.isNotEmpty ?? false) {
      buffer.writeln('\nErrors:');
      for (String error in diagnostic['errors']) {
        buffer.writeln('• $error');
      }
    }
    
    if (diagnostic['recommendations']?.isNotEmpty ?? false) {
      buffer.writeln('\nRecommendations:');
      for (String rec in diagnostic['recommendations']) {
        buffer.writeln('• $rec');
      }
    }
    
    buffer.writeln('\nDebug Logs:');
    for (String log in BLEDebugHelper.debugLogs) {
      buffer.writeln(log);
    }
    
    return buffer.toString();
  }
  
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('BLE Debug'),
        actions: [
          IconButton(
            icon: Icon(Icons.refresh),
            onPressed: _runDiagnostic,
          ),
          IconButton(
            icon: Icon(Icons.copy),
            onPressed: _copyDiagnosticToClipboard,
          ),
        ],
      ),
      body: _isRunningDiagnostic
          ? Center(
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  CircularProgressIndicator(),
                  SizedBox(height: 16),
                  Text('Running BLE diagnostic...'),
                ],
              ),
            )
          : _diagnostic == null
              ? Center(child: Text('No diagnostic data available'))
              : SingleChildScrollView(
                  padding: EdgeInsets.all(16),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      // Quick Status Card
                      _buildQuickStatusCard(),
                      
                      SizedBox(height: 16),
                      
                      // Full diagnostic report
                      BLEDebugHelper.buildDebugReport(_diagnostic!),
                      
                      SizedBox(height: 16),
                      
                      // Debug logs
                      _buildDebugLogsCard(),
                      
                      SizedBox(height: 16),
                      
                      // Action buttons
                      _buildActionButtons(),
                    ],
                  ),
                ),
    );
  }
  
  Widget _buildQuickStatusCard() {
    if (_diagnostic == null) return SizedBox.shrink();
    
    bool bluetoothSupported = _diagnostic!['bluetoothSupported'] ?? false;
    bool scanCapable = _diagnostic!['scanCapable'] ?? false;
    bool bluetoothOn = _diagnostic!['bluetoothState'] == 'BluetoothAdapterState.on';
    
    Color statusColor = (bluetoothSupported && scanCapable && bluetoothOn) 
        ? Colors.green 
        : Colors.red;
    
    String statusText = (bluetoothSupported && scanCapable && bluetoothOn)
        ? 'BLE Ready'
        : 'BLE Issues Detected';
    
    return Card(
      color: statusColor.withOpacity(0.1),
      child: Padding(
        padding: EdgeInsets.all(16),
        child: Row(
          children: [
            Icon(
              (bluetoothSupported && scanCapable && bluetoothOn)
                  ? Icons.bluetooth_connected
                  : Icons.bluetooth_disabled,
              color: statusColor,
              size: 32,
            ),
            SizedBox(width: 16),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    statusText,
                    style: TextStyle(
                      fontSize: 18,
                      fontWeight: FontWeight.bold,
                      color: statusColor,
                    ),
                  ),
                  SizedBox(height: 4),
                  Text(
                    bluetoothSupported && scanCapable && bluetoothOn
                        ? 'Your device should be able to scan for BMS devices'
                        : 'Check the diagnostic details below for specific issues',
                    style: TextStyle(fontSize: 14),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
  
  Widget _buildDebugLogsCard() {
    List<String> logs = BLEDebugHelper.debugLogs;
    
    return Card(
      child: Padding(
        padding: EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Text(
                  'Debug Logs (${logs.length})',
                  style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
                ),
                TextButton(
                  onPressed: () {
                    setState(() {
                      BLEDebugHelper.clearLogs();
                    });
                  },
                  child: Text('Clear'),
                ),
              ],
            ),
            Divider(),
            Container(
              height: 200,
              child: logs.isEmpty
                  ? Center(child: Text('No logs available'))
                  : ListView.builder(
                      itemCount: logs.length,
                      itemBuilder: (context, index) {
                        return Text(
                          logs[index],
                          style: TextStyle(
                            fontSize: 12,
                            fontFamily: 'monospace',
                          ),
                        );
                      },
                    ),
            ),
          ],
        ),
      ),
    );
  }
  
  Widget _buildActionButtons() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        ElevatedButton.icon(
          onPressed: _runDiagnostic,
          icon: Icon(Icons.play_arrow),
          label: Text('Run Diagnostic Again'),
        ),
        SizedBox(height: 8),
        ElevatedButton.icon(
          onPressed: _copyDiagnosticToClipboard,
          icon: Icon(Icons.copy),
          label: Text('Copy Report to Clipboard'),
        ),
        SizedBox(height: 8),
        OutlinedButton.icon(
          onPressed: () {
            Navigator.of(context).pop();
          },
          icon: Icon(Icons.arrow_back),
          label: Text('Back to Scan'),
        ),
      ],
    );
  }
}