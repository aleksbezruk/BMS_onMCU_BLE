// This is a basic Flutter widget test for BMS App.

import 'package:flutter_test/flutter_test.dart';

import 'package:bms_app/main.dart';

void main() {
  testWidgets('BMS App smoke test', (WidgetTester tester) async {
    // Build our app and trigger a frame.
    await tester.pumpWidget(BMSApp());

    // Verify that our app starts with BMS Monitor title.
    expect(find.text('BMS Devices'), findsOneWidget);

    // Verify that scan button is present
    expect(find.text('Scan for BMS'), findsOneWidget);
  });
}
