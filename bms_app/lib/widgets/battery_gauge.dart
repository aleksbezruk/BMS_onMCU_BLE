// Custom Battery Gauge Widget for BMS Application
// Displays battery level with visual indicator and color coding

import 'package:flutter/material.dart';
import 'dart:math' as math;

class BatteryGauge extends StatelessWidget {
  final int level; // Battery level 0-100
  final double size;
  final bool showPercentage;
  final bool showIcon;
  final Color? customColor;
  
  const BatteryGauge({
    Key? key,
    required this.level,
    this.size = 120,
    this.showPercentage = true,
    this.showIcon = true,
    this.customColor,
  }) : super(key: key);
  
  @override
  Widget build(BuildContext context) {
    return Container(
      width: size,
      height: size,
      child: Stack(
        alignment: Alignment.center,
        children: [
          // Background circle and progress arc
          CustomPaint(
            size: Size(size, size),
            painter: BatteryGaugePainter(
              level: level,
              backgroundColor: Colors.grey[300]!,
              foregroundColor: customColor ?? _getBatteryColor(level),
              strokeWidth: size * 0.08,
            ),
          ),
          
          // Center content
          Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              if (showIcon)
                Icon(
                  _getBatteryIcon(level),
                  size: size * 0.25,
                  color: customColor ?? _getBatteryColor(level),
                ),
              if (showPercentage) ...[
                if (showIcon) SizedBox(height: size * 0.02),
                Text(
                  '$level%',
                  style: TextStyle(
                    fontSize: size * 0.12,
                    fontWeight: FontWeight.bold,
                    color: customColor ?? _getBatteryColor(level),
                  ),
                ),
              ],
            ],
          ),
        ],
      ),
    );
  }
  
  Color _getBatteryColor(int level) {
    if (level > 60) return Colors.green;
    if (level > 30) return Colors.orange;
    if (level > 20) return Colors.deepOrange;
    return Colors.red;
  }
  
  IconData _getBatteryIcon(int level) {
    if (level > 90) return Icons.battery_full;
    if (level > 60) return Icons.battery_5_bar;
    if (level > 40) return Icons.battery_4_bar;
    if (level > 20) return Icons.battery_3_bar;
    if (level > 10) return Icons.battery_2_bar;
    return Icons.battery_1_bar;
  }
}

class BatteryGaugePainter extends CustomPainter {
  final int level;
  final Color backgroundColor;
  final Color foregroundColor;
  final double strokeWidth;
  
  BatteryGaugePainter({
    required this.level,
    required this.backgroundColor,
    required this.foregroundColor,
    this.strokeWidth = 8.0,
  });
  
  @override
  void paint(Canvas canvas, Size size) {
    double radius = (size.width - strokeWidth) / 2;
    Offset center = Offset(size.width / 2, size.height / 2);
    
    // Background circle
    Paint backgroundPaint = Paint()
      ..color = backgroundColor
      ..style = PaintingStyle.stroke
      ..strokeWidth = strokeWidth
      ..strokeCap = StrokeCap.round;
    
    canvas.drawCircle(center, radius, backgroundPaint);
    
    // Foreground arc (battery level)
    if (level > 0) {
      Paint foregroundPaint = Paint()
        ..color = foregroundColor
        ..style = PaintingStyle.stroke
        ..strokeWidth = strokeWidth
        ..strokeCap = StrokeCap.round;
      
      double sweepAngle = (level / 100) * 2 * math.pi;
      canvas.drawArc(
        Rect.fromCircle(center: center, radius: radius),
        -math.pi / 2, // Start from top
        sweepAngle,
        false,
        foregroundPaint,
      );
    }
  }
  
  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => true;
}

// Mini version for list tiles
class BatteryMiniGauge extends StatelessWidget {
  final int level;
  final double size;
  
  const BatteryMiniGauge({
    Key? key, 
    required this.level,
    this.size = 40,
  }) : super(key: key);
  
  @override
  Widget build(BuildContext context) {
    return BatteryGauge(
      level: level,
      size: size,
      showPercentage: false,
      showIcon: false,
    );
  }
}

// Linear battery indicator (alternative style)
class BatteryLinearIndicator extends StatelessWidget {
  final int level;
  final double width;
  final double height;
  final bool showPercentage;
  
  const BatteryLinearIndicator({
    Key? key,
    required this.level,
    this.width = 200,
    this.height = 20,
    this.showPercentage = true,
  }) : super(key: key);
  
  @override
  Widget build(BuildContext context) {
    Color batteryColor = _getBatteryColor(level);
    
    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        Container(
          width: width,
          height: height,
          decoration: BoxDecoration(
            borderRadius: BorderRadius.circular(height / 2),
            border: Border.all(color: Colors.grey[400]!, width: 1),
          ),
          child: Stack(
            children: [
              // Background
              Container(
                decoration: BoxDecoration(
                  borderRadius: BorderRadius.circular(height / 2),
                  color: Colors.grey[200],
                ),
              ),
              // Battery level
              FractionallySizedBox(
                widthFactor: level / 100,
                child: Container(
                  decoration: BoxDecoration(
                    borderRadius: BorderRadius.circular(height / 2),
                    color: batteryColor,
                  ),
                ),
              ),
              // Battery tip
              Positioned(
                right: -4,
                top: height * 0.3,
                child: Container(
                  width: 4,
                  height: height * 0.4,
                  decoration: BoxDecoration(
                    color: Colors.grey[400],
                    borderRadius: BorderRadius.only(
                      topRight: Radius.circular(2),
                      bottomRight: Radius.circular(2),
                    ),
                  ),
                ),
              ),
            ],
          ),
        ),
        if (showPercentage) ...[
          SizedBox(height: 4),
          Text(
            '$level%',
            style: TextStyle(
              fontSize: 12,
              color: batteryColor,
              fontWeight: FontWeight.w500,
            ),
          ),
        ],
      ],
    );
  }
  
  Color _getBatteryColor(int level) {
    if (level > 60) return Colors.green;
    if (level > 30) return Colors.orange;
    if (level > 20) return Colors.deepOrange;
    return Colors.red;
  }
}