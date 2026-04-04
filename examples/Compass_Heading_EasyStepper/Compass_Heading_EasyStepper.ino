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
 * EasyMode::Stepper in StepperMode::Wrap wraps through 360 degrees smoothly.
 * This heading source is already angle-like, so setup() sets inputMaxValue to 360.
 * By default it wraps with modulus 360 and takes the shortest path, so
 * 355 degrees -> 1 degree moves forward to the 361-degree equivalent.
 *
 * inputZeroCentered tells the library whether zero is at the start of the scale
 * or in the middle of it. Heading is a normal 0..359 degree value, so this
 * example uses false. A turn gauge style signal with centered zero would
 * typically use true.
 *
 * If you are driving a geared mechanism and want absolute multi-turn motion instead,
 * call compassCard.setModulusEnabled(false) in setup().
 */
DcsBios::EasyMode::Stepper compassCard(
    CommonData_HDG_DEG, // Telemetry source: heading in degrees
    8,                 // Stepper driver input pin 1
    9,                 // Stepper driver input pin 2
    10,                // Stepper driver input pin 3
    11,                // Stepper driver input pin 4
    12,                                     // Zero angle detection input pin
    false,                                  // Zero is in the middle of the range
    DcsBios::EasyMode::StepperMode::Wrap    // Wrap through 360 degrees smoothly
);

void setup() {
    compassCard.setInputMaxValue(360);      // CommonData_HDG_DEG is already in degrees
    // compassCard.setModulusEnabled(false); // Use absolute degrees instead of wrapping at 360

    DcsBios::EasyMode::setup();
}

void loop() {
    DcsBios::EasyMode::loop();
}
