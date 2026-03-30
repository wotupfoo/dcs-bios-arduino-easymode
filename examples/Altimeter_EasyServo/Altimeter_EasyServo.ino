#define DCSBIOS_DEFAULT_SERIAL
#include "DcsBiosEasyMode.h"

/*
 * Generic altitude gauge example using the CommonData (all aircraft)
 * altimeter telemetry output.
 *
 * If you are using an SG90, see Altimeter_EasyServo_SG90 example for the simpler setup.
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
 * The SG90 (and MG90 for the metal gear version) is a small hobby servo with limited physical 
 * travel, so the defaults keep the sweep at 0..180 degrees.
 * If you need to modify one of the defaults, you can set them in the setup or do it later
 * in the Arduino setup(); function.
 * If you have a 2:1 gear added changing you need to change the maximum angle
 * from from 180 to 360 degrees. 
 *      altimeterNeedle.setMaxAngle(360);
 * 
 * If the needle zero point isn't where 0% is, you can trim it by adding a trim offset (only positive).
 * If it were 3 degrees you would use:
 *      altimeterNeedle.setTrimDeg(3);
 * 
 * These are the settings you can change:
 *   altimeterNeedle.setMinAngle(...)
 *   altimeterNeedle.setMaxAngle(...)
 *   altimeterNeedle.setTrimDeg(...)
 *   altimeterNeedle.setDirection(...)
 */
DcsBios::EasyServo altimeterNeedle(
    CommonData_ALT_MSL_FT_A, // Telemetry source: altitude above mean sea level in feet
    5,                       // Arduino pin connected to the servo signal wire
    0,                       // Minimum needle angle in degrees
    180,                     // Maximum needle angle in degrees
    false,                   // Reverse the direction (true or false)
    0                        // Trim Degrees: rotate the whole scale to match the printed dial face
);

void setup() {
    DcsBios::setup();
}

void loop() {
    DcsBios::loop();
}
