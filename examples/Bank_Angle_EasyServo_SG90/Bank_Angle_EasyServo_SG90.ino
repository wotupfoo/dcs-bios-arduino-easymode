#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBiosEasyMode.h>

/*
 * SG90 servo centered-zero bank angle example using the free P-51D plane.
 *
 * P_51D_AHORIZON_BANK_A is a good example signal for learning centered-zero
 * gauges because every DCS World includes the P-51D for free. Start a Session
 * with the P51-D, run the DCS-BIOS monitor and set the Serial port to this board's
 * COM port. You should see the P_51D_AHORIZON_BANK_A value change as you bank
 * the plane left and right while in flight.
 */

/*
 * A Servo is device that converts an input value (from 0 to 180 degrees) and 
 * converts it into rotation proportional to the value it receives.
 *   A value of 0 would = 0% = 0 degrees.
 *   A value of 180 = 100% = Maximum degrees the Servo can do.
 * Typically a hobby servo, like the SG90 (and metal version MG90) can do 180 degrees.
 *
 * This example maps P_51D_AHORIZON_BANK_A to an SG90 servo.
 * DCS-BIOS will send the middle of the 0..65,535 range when the bank angle is level.
 * 
 * As the plane banks left or right, the value will decrease or increase from that 
 * middle point. 
 * A value of 0 from DCS-BIOS would map to full bank left.
 * A value of 32,768 (roughly half of 65,535) would map to level (0 degrees).
 * A value of 65,535 from DCS-BIOS would map to full bank right.
 * 
 * The EasyServo driver has built-in support to convert this DCS-BIOS provided 
 * 0..65,535 range into the 0..180 degree information the Servo needs to know. 
 * Additionally, it knows how to treat the middle of that range (90 degrees) as 
 * the "center-zero" position.
 *
 * 1) Center-zero behavior
 *    - Physical needle center may not correspond to 0° command.
 *    - Use bankAngleNeedle.setMinAngle(-X) / setMaxAngle(+X) for symmetric sweep.
 *    - If the physical mechanism offsets the zero position, use trim:
 *        bankAngleNeedle.setTrimDeg(3);
 *      (trim is additive to the raw servo angle, usually 0..180 scale)
 *
 * 2) Max angle limits
 *    - SG90 defaults to ~180°, but the desired travel may be smaller (e.g. 45°).
 *    - Use setMaxAngle() and setMinAngle() to clamp your gauge face range.
 *
 * 3) Direction inversion
 *    - If the servo rotates opposite to expected, flip with:
 *        bankAngleNeedle.setDirection(-1);
 *
 * 4) Limits and realistic range
 *    - Avoid commanding beyond the servo physical limit (damage risk).
 *    - Keep setMin/Max within servo stroke and hardware linkage range.
 *
 * Summary of controls in this library:
 *    ahorizonbank.setMinAngle(...)
 *    ahorizonbank.setMaxAngle(...)
 *    ahorizonbank.setTrimDeg(...)
 *    ahorizonbank.setDirection(...)
 */

DcsBios::EasyMode::Servo_SG90 ahorizonbank(
    P_51D_AHORIZON_BANK_A, // Telemetry source: P-51D artificial horizon bank angle
    8                      // Arduino pin connected to the servo signal wire
);

/* If you own the SpitfireLFMkIX plane, here is how you would do it for that aircraft.
 * Other aircraft have similar Telemetry Source names. Use Bort-EasyMode to find the name.
 */
//DcsBios::EasyMode::Servo_SG90 ahorizonbank(
//    SpitfireLFMkIX_AHORIZONBANK_A, // Telemetry source
//    PIN                          // Arduino pin connected to the servo signal wire
//);

void setup() {
    // Change these to match the sweep used by your printed instrument face.
    ahorizonbank.setMinAngle(-20.0f); // The Needle at full Left Turn is 20degrees Left of center
    ahorizonbank.setMaxAngle(20.0f); // The Needle at full Right Turn is 20degrees Right of center
    ahorizonbank.setTrimDeg(0.0f);   // Adjust as needed to align with printed dial zero
    ahorizonbank.setDirection(1);    // Set to -1 to reverse direction if needle moves opposite expected

    DcsBios::EasyMode::setup();
}

void loop() {
    DcsBios::EasyMode::loop();
}
