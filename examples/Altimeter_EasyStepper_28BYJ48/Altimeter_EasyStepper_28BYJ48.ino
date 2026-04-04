#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBiosEasyMode.h>

/*
 * 28BYJ-48 bounded altitude gauge example using the CommonData (all aircraft)
 * altimeter telemetry output.
 */

/*
 * DCS-BIOS telemetry outputs do not all use the same numeric range.
 *
 * CommonData_ALT_MSL_FT_A uses the full range possible (up to 65,575ft),
 * so the default inputMaxValue is already correct here.
 * The Compass on the other hand does not use the full range, it only sends 0..359 degrees.
 *
 * WHEN TO USER A STEPPER MOTOR OR A SERVO MOTOR?
 * A Stepper is a good fit when your gauge needs to travel "over the top".
 * An Altimeter needle needs to turn clockwise past 12 o'clock to go from 999 feet to 1,000 feet
 * Servos can't do this. When they go from 355 degrees to 0 degrees they will whip back
 * around counter-clockwise to go to 0.
 * Stepper motors can rotate indefinitely - like a clock, or very large numbers of rotations, 
 * like the Altimeter Foot or 1,000 Foot or 10,000 Foot independent needles, or if all three 
 * are geared as one from 0 to the maximum 65,535 feet DCS supports.
 * This Stepper driver can be told that that 360 degrees is the same as 0 degrees and to move 1 degree.
 *
 * However, you likely need an additional mechanism to detect where zero is when powered up 
 * as the stepper location will be unknown at startup. For individual altimeter needles they can
 * all be rotated by subscribing to their individual DCS World telemetry names.
 * 
 * For this example, it's assumed they are all geared together using a single value of Altitude
 * telemetry from the "CommonData_ALT_MSL_FT_A" name and can go from 0 to 65,535 feet (full range).
 * 
 * As such, the maximum angle would be where the needle would be at 65,535 feet.
 * Since it for every hundred feet the needle would be at the same location for 35 feet as 135 feet
 * 
 * We can calculate where maximum altitude would be because 65,535 is the same as 35 feet as it loops
 * over every 100 feet.
 * 65,500 would be 655 loops of the needle = 655 * 360 degrees    = 235,800
 * 35 of 100 feet (1 revolution)           = 35/100 * 360 degrees =     126
            *                                                      --------
 *                                                                 235,926 maximum degrees
 * So at the Altitude of 65,535 the needle would have turned 235,926 degrees. We put that into
 * the Maximum Angle in Degrees field. 
 * In programming we do not use commas in numbers and we're using decimal places so we need to 
 * put an 'f' after the number to tell the computer it's a decimal number ("floating point").
 * So, 65,535 max alititude becomes 235926.0f max angle in degrees.
 *
 * The last true/false setting tells the library whether zero is at the start
 * of the scale or in the middle of it. Altitude starts at the low end of the
 * range, so this example uses false.
 */
DcsBios::EasyMode::Stepper_28BYJ48 altimeterNeedle(
    CommonData_ALT_MSL_FT_A, // Telemetry source: altitude above mean sea level in feet
    8,                      // 28BYJ-48 / ULN2003 input pin 1
    9,                      // 28BYJ-48 / ULN2003 input pin 2
    10,                     // 28BYJ-48 / ULN2003 input pin 3
    11,                     // 28BYJ-48 / ULN2003 input pin 4
    12,                     // Zero angle detection input pin
    false                   // Zero is in the middle of the range
);

void setup() {
    // This altimeter example needs many turns, not the default one-turn sweep.
    altimeterNeedle.setMaxAngle(235926.0f);

    // NOTE: Stepper homing is NOT automatic. To home on startup if you have a
    // zero switch on pin 12, uncomment the line below:
    // altimeterNeedle.home();

    DcsBios::EasyMode::setup();
}

void loop() {
    DcsBios::EasyMode::loop();
}
