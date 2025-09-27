import 'package:flutter/material.dart';
import 'screens/scan_screen.dart';

void main() {
  runApp(BMSApp());
}

class BMSApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'BMS Monitor',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.blue),
        useMaterial3: true,
        appBarTheme: AppBarTheme(
          centerTitle: true,
          elevation: 0,
        ),
      ),
      home: ScanScreen(),
    );
  }
}
