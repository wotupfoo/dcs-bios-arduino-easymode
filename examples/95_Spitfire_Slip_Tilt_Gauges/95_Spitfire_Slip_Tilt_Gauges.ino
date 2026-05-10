#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBiosEasyMode.h>

/* The pin configuration is in boardconfig.h.
 * Open that file and change the pinnouts as needed.
 * Arduino Mega2560 and Due are there as examples, but you can modify for any 
 * board by following the pin availability notes and examples in that file.
 */
// Hint: Left-click to put your cursor in the middle of boardconfig.h and press F12 to open it
#include "boardconfig.h"    

// *******************************************************************************
// SIDE SLIP GAUGE & TURN GAUGE
// *******************************************************************************
DcsBios::EasyMode::Servo_SG90 sideslipgauge(
    SpitfireLFMkIX_SIDESLIPGAUGE_A, // Telemetry source: altitude above mean sea level in feet
    SIDESLIP_SERVO_PIN             // Arduino pin connected to the servo signal wire
);
DcsBios::EasyMode::Servo_SG90 turngauge(
    SpitfireLFMkIX_TURNGAUGE_A, // Telemetry source: altitude above mean sea level in feet
    TURN_SERVO_PIN             // Arduino pin connected to the servo signal wire
);

// *******************************************************************************
// ARDUINO SETUP
// *******************************************************************************
void setup() {
    // Slip Gauge with +/- 15 degress of swing
    sideslipgauge.setDirection(true); // Set to true to reverse the needle direction
    sideslipgauge.setMinAngle(-15);   // -15 deg on the servo corresponds to full left slip
    sideslipgauge.setTrimDeg(90);     // "Zero" is at 90 deg on the servo
    sideslipgauge.setMaxAngle(15);    // +15 deg on the servo corresponds to full right slip

    // Slip Gauge with +/- 22 degress of swing
    turngauge.setDirection(true);   // Set to true to reverse the needle direction
    turngauge.setMinAngle(-22);      // -22 deg on the servo corresponds to full left turn
    turngauge.setTrimDeg(90);        // "Zero" is at 90 deg on the servo
    turngauge.setMaxAngle(22);      // +22 deg on the servo corresponds to full right turn

    DcsBios::EasyMode::setup();
}

// *******************************************************************************
// ARDUINO LOOP
// *******************************************************************************
void loop() {
    DcsBios::EasyMode::loop();
}
