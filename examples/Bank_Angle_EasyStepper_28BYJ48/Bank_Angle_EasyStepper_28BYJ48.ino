#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBiosEasyMode.h>

/*
 * 28BYJ-48 stepper centered-zero bank angle example using the free P-51D plane.
 *
 * P_51D_AHORIZON_BANK_A is a good example signal for learning centered-zero
 * gauges because every DCS installation includes the P-51D for free.
 */

/*
 * This version uses the built-in 28BYJ-48 defaults for the common ULN2003
 * driver board setup, so the setup line can stay focused on the telemetry
 * source, pins, zero detector, and centered-zero flag.
 *
 * Some DCS-BIOS values behave like a normal scale from 0 up to about 65,000.
 * Others behave more like a signed scale, where the middle is zero and the
 * value can go negative or positive from there, roughly -32,000 to +32,000.
 *
 * The last true/false setting tells the library which kind of signal you have:
 *
 *   false -> 0 is at the low end of the range
 *   true  -> 0 is in the middle of the range
 *
 * The artificial horizon bank signal is a centered-zero value, so this example
 * uses true.
 */
DcsBios::EasyStepper_28BYJ48_Bounded bankAngleNeedle(
    P_51D_AHORIZON_BANK_A, // Telemetry source: P-51D artificial horizon bank angle
    8,                     // 28BYJ-48 / ULN2003 input pin 1
    9,                     // 28BYJ-48 / ULN2003 input pin 2
    10,                    // 28BYJ-48 / ULN2003 input pin 3
    11,                    // 28BYJ-48 / ULN2003 input pin 4
    12,                    // Zero angle detection input pin
    true                   // Zero is in the middle of the range
);

void setup() {
    // Change these to match the sweep used by your printed instrument face.
    bankAngleNeedle.setMinAngle(-45.0f);
    bankAngleNeedle.setMaxAngle(45.0f);

    DcsBios::setup();
}

void loop() {
    DcsBios::loop();
}
