#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBiosEasyMode.h>

/*
 * Advanced rate of climb example for an SG90 servo, derived from altitude.
 *
 * Use this when your aircraft does not expose a direct VVI / variometer /
 * vertical-speed gauge output. Instead, this example watches the common
 * DCS-BIOS altitude metadata and calculates feet per minute from the change
 * in altitude over time.
 */

unsigned int previousAltitudeFt = 0;
unsigned long lastRocUpdateMs = 0;

DcsBios::EasyMode::Servo_SG90 rateOfClimbNeedle(
    0xFFFF,                  // No direct DCS source: this example drives the servo from derived values
    8                        // Arduino pin connected to the servo signal wire
);

void updateRateOfClimbFromAltitude(unsigned int altitudeFt) {
    const unsigned long nowMs = millis();
    if (lastRocUpdateMs == 0) {
        previousAltitudeFt = altitudeFt;
        lastRocUpdateMs = nowMs;
        rateOfClimbNeedle.setRawValue(32768U);
        return;
    }

    const unsigned long elapsedMs = nowMs - lastRocUpdateMs;
    if (elapsedMs < 500UL) return; // update at 2 Hz

    const long altitudeDeltaFt = (long)altitudeFt - (long)previousAltitudeFt;
    const float elapsedMinutes = (float)elapsedMs / 60000.0f;
    if (elapsedMinutes <= 0.0f) return;

    long rateOfClimbFtPerMin = (long)((float)altitudeDeltaFt / elapsedMinutes);
    rateOfClimbFtPerMin = constrain(rateOfClimbFtPerMin, -4000L, 4000L);

    const unsigned int centeredTarget =
        (unsigned int)(32768L + (rateOfClimbFtPerMin * 32767L) / 4000L);
    rateOfClimbNeedle.setRawValue(centeredTarget);

    previousAltitudeFt = altitudeFt;
    lastRocUpdateMs = nowMs;
}

void onAltitudeChange(unsigned int newValue) {
    updateRateOfClimbFromAltitude(newValue);
}
DcsBios::IntegerBuffer altitudeBuffer(CommonData_ALT_MSL_FT, onAltitudeChange);

void setup() {
    // Change these to match the sweep used by your printed instrument face.
    rateOfClimbNeedle.setMinAngle(-135.0f);
    rateOfClimbNeedle.setMaxAngle(135.0f);
    rateOfClimbNeedle.setTrimDeg(0.0f);
    rateOfClimbNeedle.setDirection(1);

    DcsBios::EasyMode::setup();
}

void loop() {
    DcsBios::EasyMode::loop();
}
