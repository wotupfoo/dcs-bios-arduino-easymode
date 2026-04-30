#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBiosEasyMode.h>

/* The pin configuration is in boardconfig.h.
 * Open that file and change the pinnouts as needed.
 * Arduino Mega2560 and Due are there as examples, but you can modify for any 
 * board by following the pin availability notes and examples in that file.
 */
// Hint: Left-click to put your cursor in the middle of boardconfig.h and press F12 to open it
#include "boardconfig.h"    

/******************************************************************************
 * Mosquito/Spitfire Blind Panel telemetry output.
 ******************************************************************************
 * Top Row of Gauges:
 * - Air Speed Indicated (ASI) with 28BJY-48 Stepper for full 720 degree sweep
 * - Artificial Horizon with separate servos for bank and pitch
 * - Rate of Climb with 28BJY-48 Stepper **** SPECIAL HANDLING NEEDED ****
 * Bottom Row of Gauges:
 * - Altimeter with 28BYJ-48 Stepper geared for all 3 needles **** SPECIAL HANDLING NEEDED ****
 * - Gyroscopic Directional Indicator (DI) with 28BYJ-48 Stepper and adjustment rotary encoder
 * - Side Slip and Turn Gauge with separate servos for each needle
 ******************************************************************************
 */

// *****************************************************************************
// AIR SPEED INDICATOR (ASI)
// *****************************************************************************
DcsBios::EasyMode::Stepper_28BYJ48 airspeedgauge(
    SpitfireLFMkIX_AIRSPEEDGAUGE, // Telemetry source
    AIRSPEED_STEPPER_PIN1,        // Arduino pin connected to the stepper driver input pin 1
    AIRSPEED_STEPPER_PIN2,        // Arduino pin connected to the stepper driver input pin 2
    AIRSPEED_STEPPER_PIN3,        // Arduino pin connected to the stepper driver input pin 3
    AIRSPEED_STEPPER_PIN4,        // Arduino pin connected to the stepper driver input pin 4
    AIRSPEED_ZERO_PIN,            // Zero angle detection input pin
    false                         // inputZeroCentered: false because 0 knots is at 0 degrees and max speed is at 720 degrees
);

// ******************************************************************************
// ARTIFICIAL HORIZON BANK & PITCH
// ******************************************************************************
DcsBios::EasyMode::Servo_SG90 ahorizonbank(
    SpitfireLFMkIX_AHORIZONBANK_A, // Telemetry source
    AHORIZON_BANK_SERVO_PIN       // Arduino pin connected to the servo signal wire
);

DcsBios::EasyMode::Servo_SG90 ahorizonpitch(
    SpitfireLFMkIX_AHORIZONPITCH_A, // Telemetry source
    AHORIZON_PITCH_SERVO_PIN       // Arduino pin connected to the servo signal wire
);

// ******************************************************************************
// RATE OF CLIMB INDICATOR
// 
// Developer note: DCS-BIOS doesn't have a Rate of Climb telemetry source.
// So, we create it by watching CommonData_ALT_MSL_FT and calculating the rate
// of change over time. Once calculated we can update the stepper position based
// on the rate of climb/fall.
// ******************************************************************************
unsigned int PreviousAltitude = 0;          // Previous altitude reading for ROC calculation
unsigned long LastRocUpdateMs = 0;          // Timestamp of last ROC update (2Hz = 500ms interval)
int RateOfClimbFtPerMin = 0;                // Calculated rate of climb in feet per minute

// Rate of Climb stepper (calculate from altitude deltas)
DcsBios::EasyMode::Stepper_Manual_28BYJ48 rateofclimbstepper(
    ROCLIMB_STEPPER_PIN1,      // Arduino pin connected to the stepper driver input pin 1
    ROCLIMB_STEPPER_PIN2,      // Arduino pin connected to the stepper driver input pin 2
    ROCLIMB_STEPPER_PIN3,      // Arduino pin connected to the stepper driver input pin 3
    ROCLIMB_STEPPER_PIN4,      // Arduino pin connected to the stepper driver input pin 4
    ROCLIMB_ZERO_PIN,          // Zero angle detection input pin
    true                       // inputZeroCentered: true because center is 0 ft/min
);

void updateRateOfClimbStepper(unsigned int altitudeFt) {
    unsigned long nowMs = millis();
    if (LastRocUpdateMs == 0) {
        PreviousAltitude = altitudeFt;
        LastRocUpdateMs = nowMs;
        rateofclimbstepper.setPosition(32768U);
        return;
    }

    unsigned long elapsedMs = nowMs - LastRocUpdateMs;
    if (elapsedMs < 500UL) return;

    long altitudeDeltaFt = (long)altitudeFt - (long)PreviousAltitude;
    float elapsedMinutes = (float)elapsedMs / 60000.0f;
    if (elapsedMinutes <= 0.0f) return;

    RateOfClimbFtPerMin = (int)((float)altitudeDeltaFt / elapsedMinutes);
    RateOfClimbFtPerMin = constrain(RateOfClimbFtPerMin, -4000, 4000);   // Maximum needle deflection

    // Map 0..32k..64k to -4000..0..+4000 ft/min for the stepper position
    const long centeredTarget = 32768L + ((long)RateOfClimbFtPerMin * 32767L) / 4000L;
    rateofclimbstepper.setPosition((unsigned int)centeredTarget);

    PreviousAltitude = altitudeFt;
    LastRocUpdateMs = nowMs;
}

void onCommonAltitudeChange(unsigned int newValue) {
    updateRateOfClimbStepper(newValue);
}
DcsBios::IntegerBuffer commonAltitudeBuffer(CommonData_ALT_MSL_FT, onCommonAltitudeChange);

// ******************************************************************************
// ALTIMETER (with special handling for all 3 needles on one stepper)
// Rotary Knob mechanically moves the Barometer scale. Push to Zero.
// 
// Developer note: DCS-BIOS doesn't have an Altitude telemetry source.
// So, we have to create it by subscribing to the Altitude 100's, 1,000's & 10,000's
// sources and sum them together with scaling factors. 
// Once summed we can update the stepper position based on the total altitude.
// 
// The stepper is geared to drive all 3 needles simultaneously:
// -    100s needle: 1 full rotation per 1,000 ft
// -  1,000s needle: 1 full rotation per 10,000 ft  
// - 10,000s needle: 1 full rotation per 100,000 ft
// Total range: 0-99,999 ft
// ******************************************************************************

unsigned int Altitude = 0;                  // Sum of all the Altitudes received from DCS-BIOS
unsigned int AltimeterHundreds = 0;         // Storage of the 100's of feet needle position received from DCS-BIOS
unsigned int AltimeterThousands = 0;        // Storage of the 1,000's of feet needle position received from DCS-BIOS
unsigned int AltimeterTenThousands = 0;     // Storage of the 10,000's of feet needle position received from DCS-BIOS

DcsBios::EasyMode::Stepper_Manual_28BYJ48 altimeter3NeedleStepper(
    ALTIMETER_STEPPER_PIN1,    // Arduino pin connected to the stepper driver input pin 1
    ALTIMETER_STEPPER_PIN2,    // Arduino pin connected to the stepper driver input pin 2
    ALTIMETER_STEPPER_PIN3,    // Arduino pin connected to the stepper driver input pin 3
    ALTIMETER_STEPPER_PIN4,    // Arduino pin connected to the stepper driver input pin 4
    ALTIMETER_ZERO_PIN,        // Zero angle detection input pin
    false                      // inputZeroCentered: false because 0 ft is at 0 degrees
);

// ALTIMETER Hundreds of Feet Needle
void onAltimeterhundredsChange(unsigned int newValue) {
    AltimeterHundreds = newValue;
    Altitude = AltimeterHundreds + (AltimeterThousands * 1000) + (AltimeterTenThousands * 10000);
    altimeter3NeedleStepper.setPosition(Altitude); // Update the stepper position immediately on change
}
DcsBios::IntegerBuffer altimeterhundredsBuffer(SpitfireLFMkIX_ALTIMETERHUNDREDS, onAltimeterhundredsChange);

// ALTIMETER Thousands of Feet Needle
void onAltimeterthousandsChange(unsigned int newValue) {
    AltimeterThousands = newValue;
    Altitude = AltimeterHundreds + (AltimeterThousands * 1000) + (AltimeterTenThousands * 10000);
    altimeter3NeedleStepper.setPosition(Altitude); // Update the stepper position immediately on change
}
DcsBios::IntegerBuffer altimeterthousandsBuffer(SpitfireLFMkIX_ALTIMETERTHOUSANDS, onAltimeterthousandsChange);

// ALTIMETER Ten's of Thousands of Feet Needle
void onAltimetertensthousandsChange(unsigned int newValue) {
    AltimeterTenThousands = newValue;
    Altitude = AltimeterHundreds + (AltimeterThousands * 1000) + (AltimeterTenThousands * 10000);
    altimeter3NeedleStepper.setPosition(Altitude); // Update the stepper position immediately on change
}
DcsBios::IntegerBuffer altimetertensthousandsBuffer(SpitfireLFMkIX_ALTIMETERTENSTHOUSANDS, onAltimetertensthousandsChange);

// *******************************************************************************
// GYROSCOPIC DIRECTIONAL INDICATOR (DI) with adjustment rotary encoder
// *******************************************************************************
DcsBios::EasyMode::Stepper_28BYJ48 DIStepper(
    SpitfireLFMkIX_DIGAUGE_A, // Telemetry source
    DI_STEPPER_PIN1,          // Arduino pin connected to the stepper driver input pin 1
    DI_STEPPER_PIN2,          // Arduino pin connected to the stepper driver input pin 2
    DI_STEPPER_PIN3,          // Arduino pin connected to the stepper driver input pin 3
    DI_STEPPER_PIN4,          // Arduino pin connected to the stepper driver input pin 4
    DI_ZERO_PIN,              // Zero angle detection input pin
    false                     // inputZeroCentered: false because 0 degrees is at 0 input
);

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
    while(1);   // Stop everything to prevent further damage to the stepper or missed steps. Requires manual reset.
}

// *******************************************************************************
// ARDUINO SETUP
// *******************************************************************************
void setup() {
    // Airspeed
    airspeedgauge.configureBoundedBehavior(0.0f, 720.0f);
    airspeedgauge.setMaxRpm(20.0f); // adjust as needed
    airspeedgauge.setAccelRpmPerSec(40.0f);
    airspeedgauge.setFaultCallback(onStepperTimingFault);    // Turn on fault checking
    airspeedgauge.home(); // NOTE: Homing is NOT automatic; you must call home() explicitly.
                          // Requires zero switch on AIRSPEED_ZERO_PIN (optional if no switch)

    // Artificial Horizon
    ahorizonbank.setMinAngle(-15);
    ahorizonbank.setTrimDeg(90);
    ahorizonbank.setMaxAngle(15);

    ahorizonpitch.setMinAngle(-15);
    ahorizonpitch.setTrimDeg(90);
    ahorizonpitch.setMaxAngle(30);

    // Rate of Climb
    // The gauge is +/- 4,000 ft/min full scale (+/- 135 degrees) with a center at 0 ft/min.
    rateofclimbstepper.configureBoundedBehavior(-135.0f, 135.0f); // adjust as needed for max climb/descent rate
    rateofclimbstepper.setMaxRpm(20.0f); // adjust as needed
    rateofclimbstepper.setAccelRpmPerSec(40.0f);
    rateofclimbstepper.setFaultCallback(onStepperTimingFault);    // Turn on fault checking
    rateofclimbstepper.home(); // NOTE: Homing is NOT automatic; you must call home() explicitly.
                               // Requires zero switch on ROCLIMB_ZERO_PIN (optional if no switch)

    // configure altimeter stepper ranges
    // 100,000ft full scale = 1,000 revolutions (360 degrees) of the hundred-foot needle
    altimeter3NeedleStepper.configureBoundedBehavior(0.0f, 360.0f*1000.0f);
    altimeter3NeedleStepper.setMaxRpm(20.0f); // adjust as needed
    altimeter3NeedleStepper.setAccelRpmPerSec(40.0f);
    altimeter3NeedleStepper.setFaultCallback(onStepperTimingFault);    // Turn on fault checking
    altimeter3NeedleStepper.home(); // NOTE: Homing is NOT automatic; you must call home() explicitly.
                                     // Requires zero switch on ALTIMETER_ZERO_PIN (optional if no switch)

    // Gyro Directional Indicator (DI)
    DIStepper.configureBoundedBehavior(0.0f, 360.0f);
    DIStepper.setMaxRpm(20.0f); // adjust as needed
    DIStepper.setAccelRpmPerSec(40.0f);
    DIStepper.setFaultCallback(onStepperTimingFault);    // Turn on fault checking
    DIStepper.home(); // NOTE: Homing is NOT automatic; you must call home() explicitly.
                      // Requires zero switch on DI_ZERO_PIN (optional if no switch)

    // Slip Gauge
    sideslipgauge.setDirection(-1);
    sideslipgauge.setMinAngle(-15);
    sideslipgauge.setTrimDeg(90);
    sideslipgauge.setMaxAngle(15);

    // Turn Gauge
    turngauge.setDirection(-1);
    turngauge.setMinAngle(-22);
    turngauge.setTrimDeg(90);
    turngauge.setMaxAngle(22);

    DcsBios::EasyMode::setup();
}

// *******************************************************************************
// ARDUINO LOOP
// *******************************************************************************
void loop() {
    DcsBios::EasyMode::loop();

    // -------------------------------------
    // Spitfire Blind Panel Top Row
    // -------------------------------------

    // Service the Air Speed stepper
//      Automatically updated by DcsBios::EasyMode::loop() since they are Servo_SG90 instances.

    // Artificial Horizon servos 
//      Automatically updated by DcsBios::EasyMode::loop() since they are Servo_SG90 instances.

    // Service the Rate of Climb stepper (calculated from altitude change over the last 1/2 second)
    rateofclimbstepper.loop();

    // -------------------------------------
    // Spitfire Blind Panel Bottom Row
    // -------------------------------------

    // Service the Altitude Gauge
    //      This is as manually created Stepper Motor solution because DCS-BIOS doesn't have 
    //      a source. Here in the main loop we need to call it's updating function to make 
    //      it move to any new desired position.
    altimeter3NeedleStepper.loop();

    // Service the DI stepper
    DIStepper.loop();

    // Slip and Turn servos
//      Automatically updated by DcsBios::EasyMode::loop() since they are Servo_SG90 instances.
}
