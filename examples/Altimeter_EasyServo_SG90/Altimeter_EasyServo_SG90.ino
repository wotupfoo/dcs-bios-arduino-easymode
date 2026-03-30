#define DCSBIOS_DEFAULT_SERIAL
#include "DcsBiosEasyMode.h"

/*
 * SG90-specific altitude gauge example using the CommonData (all aircraft)
 * altimeter telemetry output.
 *
 * This is the same idea as the generic EasyServo example (Altimeter_EasyServo),
 * but it sets the SG90 servo known information so you don't have to set them
 * yourself.
 * 
 * For developers: Other hobby servos can be easily added to src/internal/DcsBiosEasyServos.h
 */

/*
 * A Servo is device that converts a rotational value (eg 45 degrees) and converts it into
 * rotation proportional to the value it receives.
 *   A value of 0 would = 0% = 0 degrees.
 *   A value of 65,535 = 100% = Maximum degrees the Servo can do. 180 degrees for the SG90
 * Typically a hobby servo can do 180 degrees. This is not great for an altimeter!
 * This example using a Servo for an Altimeter isn't very practical but is here for completeness.
 * You are much better using a Stepper motor (see the example Altimeter_EasyStepper)
 * because Servos can't rotate indefinitely (eg climbing from 0 feet to 30,000 feet).
 * 
 * One important beginner note:
 * DCS-BIOS telemetry outputs do not all send the same numeric range.
 *
 * "CommonData_ALT_MSL_FT_A" uses the full input range, so we do not need convert it.
 * The Altitude value can be from 0 to 65576 feet (the full range of the "16 bit integer" number).
 * The other provided example (Compass_Heading_EasyStepper) only gets values from 0 to 355 degrees.
 *
 * The SG90 is still a small hobby servo with limited physical travel, so the
 * example keeps the sweep at 0..180 degrees. That is a safe starting point
 * for a user who wants something that works before doing fine tuning.
 */
DcsBios::EasyServo_SG90 altimeterNeedle(
    CommonData_ALT_MSL_FT_A, // Telemetry source: altitude above mean sea level in feet
    5,          // Arduino pin connected to the servo signal wire
    0,          // Minimum needle angle in degrees
    180,        // Maximum needle angle in degrees
    false,      // Reverse the direction (true or false)
    0           // Trim Degrees: rotate the whole scale to match the printed dial face
);

void setup() {
    DcsBios::setup();
}

void loop() {
    DcsBios::loop();
}
