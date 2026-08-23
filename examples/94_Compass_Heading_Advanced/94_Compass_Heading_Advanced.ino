#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBiosEasyMode.h>

/*
 * Generic continuous heading gauge example using the CommonData (all aircraft)
 * heading telemetry output.
 */

/*
 * This telemetry source is different from CommonData_ALT_MSL_FT_A.
 *
 * The easy-mode stepper can read the packed CommonData_HDG_DEG field directly, so the
 * user does not need to worry about the mask and shift details.
 *
 * EasyMode::Stepper with wrapAround() wraps through 360 degrees smoothly.
 * This heading source is already angle-like, so setup() sets inputMaxValue to 360.
 * By default it wraps with modulus 360 and takes the shortest path, so
 * 355 degrees -> 1 degree moves forward to the 361-degree equivalent.
 *
 * If you are driving a mechanism that should stay between 0 and 360 degrees
 * instead, leave out the compassCard.wrapAround() line in setup().
 */
const long STEPS_PER_OUTPUT_REVOLUTION = 200;

DcsBios::EasyMode::Stepper compassCard(
    CommonData_HDG_DEG, // Telemetry source: heading in degrees
    8,                 // Stepper driver input pin 1
    9,                 // Stepper driver input pin 2
    10,                // Stepper driver input pin 3
    11,                // Stepper driver input pin 4
    STEPS_PER_OUTPUT_REVOLUTION, // Stepper steps per output shaft revolution after any gearing
    12,                // Zero angle detection input pin
    LOW                // Zero switch is active when the pin reads LOW
);

void setup() {
    compassCard.wrapAround();               // Wrap through 360 degrees smoothly
    compassCard.setInputMaxValue(360);      // CommonData_HDG_DEG is already in degrees

    DcsBios::EasyMode::setup();
}

void loop() {
    DcsBios::EasyMode::loop();
}


