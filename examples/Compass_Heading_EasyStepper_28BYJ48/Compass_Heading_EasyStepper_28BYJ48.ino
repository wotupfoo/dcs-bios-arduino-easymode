#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBiosEasyMode.h>

/*
 * 28BYJ-48 continuous heading gauge example using the CommonData (all aircraft)
 * heading telemetry output.
 */

/*
 * This version uses the built-in 28BYJ-48 defaults for the common ULN2003
 * driver board setup, so the constructor can stay focused on the DCS-BIOS
 * source and the four motor pins.
 *
 * EasyStepper_28BYJ48_Continuous treats the received value as degrees.
 * By default it wraps with modulus 360 and takes the shortest path, so
 * 355 degrees -> 1 degree moves forward to the 361-degree equivalent.
 *
 * The last true/false setting tells the library whether zero is at the start
 * of the scale or in the middle of it. Heading is a normal 0..359 degree
 * value, so this example uses false. A turn gauge style signal with centered
 * zero would typically use true.
 *
 * If you are driving a geared mechanism and want absolute multi-turn motion instead,
 * call compassCard.setModulusEnabled(false) in setup().
 */
DcsBios::EasyStepper_28BYJ48_Continuous compassCard(
    CommonData_HDG_DEG, // Telemetry source: heading in degrees
    8,                 // 28BYJ-48 / ULN2003 input pin 1
    9,                 // 28BYJ-48 / ULN2003 input pin 2
    10,                // 28BYJ-48 / ULN2003 input pin 3
    11,                // 28BYJ-48 / ULN2003 input pin 4
    12,                // Zero angle detection input pin
    false              // Zero is in the middle of the range
);

void setup() {
    // compassCard.setModulusEnabled(false); // Use absolute degrees instead of wrapping at 360

    // NOTE: Stepper homing is NOT automatic. To home on startup if you have a
    // zero switch on pin 12, uncomment the line below:
    // compassCard.home();

    DcsBios::setup();
}

void loop() {
    DcsBios::loop();
}
