#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBiosEasyMode.h>

/*
 * SG90 servo centered-zero pitch example.
 *
 * Set PITCH_SOURCE to your aircraft-specific DCS-BIOS telemetry macro
 * before compiling. Pitch is a common gauge concept, but the generated
 * DCS-BIOS macro name depends on the aircraft.
 */

// Aircraft with ADI_PITCH in the DCS-BIOS JSON docs:
// #define PITCH_SOURCE A_10C_ADI_PITCH_A    // A-10C
// #define PITCH_SOURCE AH_6J_ADI_PITCH_A    // AH-6J
// #define PITCH_SOURCE AV8BNA_ADI_PITCH_A   // AV-8B N/A
// #define PITCH_SOURCE F_16C_50_ADI_PITCH_A // F-16C Block 50

#ifndef PITCH_SOURCE
#error "Define PITCH_SOURCE to your aircraft-specific DCS-BIOS pitch macro before compiling this example."
#endif

/*
 * A servo converts an input value into a proportional angle.
 * Typical hobby servos like the SG90 can rotate about 180 degrees.
 *
 * This example maps PITCH_SOURCE to an SG90 servo.
 * DCS-BIOS will send the middle of the 0..65,535 range when the pitch gauge
 * is centered.
 *
 * As the pitch indication moves down or up, the value will decrease or
 * increase from that middle point.
 * A value of 0 from DCS-BIOS would map to full down.
 * A value of 32,768 (roughly half of 65,535) would map to centered.
 * A value of 65,535 from DCS-BIOS would map to full up.
 *
 * The EasyMode servo driver has built-in support to convert this DCS-BIOS
 * provided 0..65,535 range into the 0..180 degree information the servo needs.
 * It also knows how to treat the middle of that range as the centered position.
 *
 * 1) Center-zero behavior
 *    - Use adiPitchServo.setMinAngle(-X) / setMaxAngle(+X) for symmetric sweep.
 *    - If the physical mechanism offsets the zero position, use trim:
 *        adiPitchServo.setTrimDeg(3);
 *
 * 2) Max angle limits
 *    - SG90 defaults to about 180 degrees, but the desired travel may be smaller.
 *    - Use setMaxAngle() and setMinAngle() to clamp your gauge face range.
 *
 * 3) Direction inversion
 *    - If the servo rotates opposite to expected, flip with:
 *        adiPitchServo.setDirection(-1);
 *
 * 4) Limits and realistic range
 *    - Avoid commanding beyond the servo physical limit.
 *    - Keep setMin/Max within servo stroke and hardware linkage range.
 */

DcsBios::EasyMode::Servo_SG90 adiPitchServo(
    PITCH_SOURCE,           // Telemetry source: your aircraft-specific pitch output
    8                      // Arduino pin connected to the servo signal wire
);

void setup() {
    // Change these to match the sweep used by your printed instrument face.
    adiPitchServo.setMinAngle(-20.0f);
    adiPitchServo.setMaxAngle(20.0f);
    adiPitchServo.setTrimDeg(0.0f);
    adiPitchServo.setDirection(1);

    DcsBios::EasyMode::setup();
}

void loop() {
    DcsBios::EasyMode::loop();
}

