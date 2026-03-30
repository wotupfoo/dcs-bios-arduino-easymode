#define DCSBIOS_DEFAULT_SERIAL
#include "DcsBiosEasyMode.h"

/*
 * Generic altitude gauge example using the CommonData (all aircraft)
 * altimeter telemetry output.
 */

/*
 * One important beginner note:
 * DCS-BIOS telemetry outputs do not all use the same numeric range.
 *
 * CommonData_ALT_MSL_FT_A already uses the full easy-mode input range, so we do not need
 * to pass a special inputMaxValue here.
 *
 * A servo is different from a stepper, though. A hobby servo can only move
 * through a limited angle, so we keep the sweep at 0..180 degrees. If your
 * gauge needs more travel than that, a stepper is usually the better choice.
 */
DcsBios::EasyServo altimeterNeedle(
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
