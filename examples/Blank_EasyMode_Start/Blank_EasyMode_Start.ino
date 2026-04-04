#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBiosEasyMode.h>

/*
 * Blank starting point for building a DCS-BIOS Easy Mode sketch.
 *
 * Workflow:
 *
 * 1. Open Bort and find the telemetry or control you want to use.
 * 2. Copy the code snippet that matches your hardware.
 * 3. Paste global declarations in the section below this comment.
 * 4. If the example tells you to change angles, trim, or other settings,
 *    place those lines in setup() before DcsBios::EasyMode::setup().
 * 5. Upload the sketch to your Arduino board.
 *
 * This file is intentionally simple so it can be used as a first project
 * template for beginners.
 */

// Paste copied DCS-BIOS Easy Mode objects here.
// Example:
// DcsBios::EasyMode::Servo_SG90 altimeterNeedle(CommonData_ALT_MSL_FT_A, 5);

void setup() {
    // Paste any extra setup lines here.
    // Example:
    // altimeterNeedle.setTrimDeg(3);
    // altimeterNeedle.setMaxAngle(180);

    DcsBios::EasyMode::setup();
}

void loop() {
    DcsBios::EasyMode::loop();
}
