#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBiosEasyMode.h>

/*
 * Minimal debug sketch for diagnosing cases where the board cannot service
 * the stepper(s) quickly enough.
 *
 * If a stepper instance detects that its servicing gap exceeded the allowed
 * time for its current motion, the callback below turns on LED_BUILTIN and
 * leaves it on.
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
DcsBios::EasyMode::Stepper_28BYJ48 compassCard(
    CommonData_HDG_DEG, // Telemetry source: heading in degrees
    13,                 // 28BYJ-48 / ULN2003 input pin 1
    14,                 // 28BYJ-48 / ULN2003 input pin 2
    15,                 // 28BYJ-48 / ULN2003 input pin 3
    16,                 // 28BYJ-48 / ULN2003 input pin 4
    DcsBios::EasyMode::NoPin,
    false,                                   // Zero is in the middle of the range
    DcsBios::EasyMode::StepperMode::Wrap     // Wrap through 360 degrees smoothly
);

/* When any Stepper Motor can't keep up it will call this function.
 * The main reason this will happen is if you have too many Stepper Motors.
 * The function turns on the builtin led and leaves it on.
 * There is extra information you can use to figure out what's going wrong if you
 * don't want to simply have fewer Steppers on this board.
 */
void onStepperTimingFault(
    unsigned int address,           // The compiled address of either altimeterNeedle or compassCard
    unsigned long serviceGapUs,     // How late in microseconds (uS) it was
    unsigned long allowedGapUs      // How many microseconds with a 1.5x (default) headroom were expected
) {
    (void)address;
    (void)serviceGapUs;
    (void)allowedGapUs;
    digitalWrite(LED_BUILTIN, HIGH);
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    altimeterNeedle.setMaxAngle(360);
    altimeterNeedle.setFaultCallback(onStepperTimingFault);    // Turn on fault checking

    compassCard.setInputMaxValue(360);                         // CommonData_HDG_DEG is already in degrees
    compassCard.setFaultCallback(onStepperTimingFault);    // Turn on fault checking

    DcsBios::EasyMode::setup();
}

void loop() {
    DcsBios::EasyMode::loop();
}
