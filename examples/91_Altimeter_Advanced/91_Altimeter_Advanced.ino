#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBiosEasyMode.h>

/*
 * Generic bounded altitude gauge example using the CommonData (all aircraft)
 * altimeter telemetry output.
 *
 * If you are using a 28BYJ-48 on a ULN2003 board, see
 * 1_Altimeter for the shorter copy-paste 28BYJ-48 journey version.
 */

/*
 * DCS-BIOS telemetry outputs do not all use the same numeric range.
 *
 * CommonData_ALT_MSL_FT_A already uses the full range possible (up to 65,575ft),
 * so the default inputMaxValue is already correct here.
 *
 * A Stepper is a good fit when your gauge needs more travel "over the top".
 * An Altimeter needle needs to climb past 12 o'clock to go from 999 feet to 1,000 feet
 * Servos can't do this. When they go from 355 degrees to 0 degrees they will whip back
 * around counter-clockwise to go to 0. 
 * Stepper can rotate indefinitely - like a clock, or very large numbers of rotations, like the 
 * Foot or 1,000 Foot or 10,000 Foot independent needles, or if all three are geared as one from
 * 0 to the maximum 65,535 feet DCS supports.
 * Steppers can be taught that 360 degrees is the same as 0 degrees and to move 1 degree.
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
 *                                                                 235,800 maximum degrees
 * So at the Altitude of 65,535 the needle would have turned 235,800 degrees. We put that into
 * the Maximum Angle in Degrees field. 
 * In programming we do not use commas in numbers and we're using decimal places so we need to 
 * put an 'f' after the number to tell the computer it's a decimal number ("floating point").
 * So, 65,535 max alititude becomes 235800.0f max angle in degrees.
 *
 * The last true/false setting tells the library whether zero is at the start
 * of the scale or in the middle of it. Altitude starts at the low end of the
 * range, so this example uses false.
 */
DcsBios::EasyMode::Stepper altimeterNeedle(
    CommonData_ALT_MSL_FT_A, // Telemetry source: altitude above mean sea level in feet
    8,                      // Stepper driver input pin 1
    9,                      // Stepper driver input pin 2
    10,                     // Stepper driver input pin 3
    11,                     // Stepper driver input pin 4
    12,                     // Zero angle detection input pin
    false                   // Zero is in the middle of the range
);

void setup() {
    altimeterNeedle.setMaxAngle(235800.0f);

    DcsBios::EasyMode::setup();
}

void loop() {
    DcsBios::EasyMode::loop();
}
