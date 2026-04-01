#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBiosEasyMode.h>

/******************************************************************************
 * Spitfire Blind Panel telemetry output.
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

/*
 * Mega2560 Pin Availability:
 * - Digital Pins: 0-53
 * - Analog Pins: A0-A15 (usable as digital pins 54-69 if needed)
 * - Notes: Pins 0/1 are Serial RX/TX; avoid for stepper/servos. Pins 2-13 have PWM. Pins 50-53 are SPI.
 * - Current assignments: See code below.
 */
// Pin assignments (ordered by gauge position on panel)
// Air Speed Indicated (ASI)
#define AIRSPEED_STEPPER_PIN1     2   // Airspeed stepper pin 1
#define AIRSPEED_STEPPER_PIN2     3   // Airspeed stepper pin 2
#define AIRSPEED_STEPPER_PIN3     4   // Airspeed stepper pin 3
#define AIRSPEED_STEPPER_PIN4     5   // Airspeed stepper pin 4
#define AIRSPEED_ZERO_PIN         20  // Airspeed zero detect (optional)

// Artificial Horizon
#define AHORIZON_BANK_SERVO_PIN   6   // Artificial horizon bank servo (PWM)
#define AHORIZON_PITCH_SERVO_PIN  7   // Artificial horizon pitch servo (PWM)

// Rate of Climb Indicator
#define ROCLIMB_STEPPER_PIN1      16  // Rate of Climb stepper pin 1
#define ROCLIMB_STEPPER_PIN2      17  // Rate of Climb stepper pin 2
#define ROCLIMB_STEPPER_PIN3      18  // Rate of Climb stepper pin 3
#define ROCLIMB_STEPPER_PIN4      19  // Rate of Climb stepper pin 4
#define ROCLIMB_ZERO_PIN          21  // Rate of Climb zero

// Altimeter
#define ALTIMETER_STEPPER_PIN1    8   // Altimeter stepper pin 1
#define ALTIMETER_STEPPER_PIN2    9   // Altimeter stepper pin 2
#define ALTIMETER_STEPPER_PIN3    10  // Altimeter stepper pin 3
#define ALTIMETER_STEPPER_PIN4    11  // Altimeter stepper pin 4
#define ALTIMETER_ZERO_PIN        21  // Altimeter zero detect (optional)

// Gyroscopic Directional Indicator (DI)
#define DI_STEPPER_PIN1           12  // DI stepper pin 1
#define DI_STEPPER_PIN2           13  // DI stepper pin 2
#define DI_STEPPER_PIN3           14  // DI stepper pin 3
#define DI_STEPPER_PIN4           15  // DI stepper pin 4
#define DI_ZERO_PIN               22  // DI zero detect (optional)

// Side Slip and Turn Gauge
#define SIDESLIP_SERVO_PIN        44  // Sideslip gauge servo (PWM)
#define TURN_SERVO_PIN            45  // Turn gauge servo (PWM)

// *****************************************************************************
// AIR SPEED INDICATOR (ASI)
// *****************************************************************************
DcsBios::EasyStepper_28BYJ48 airspeedgauge(
    SpitfireLFMkIX_AIRSPEEDGAUGE, // Telemetry source
    AIRSPEED_STEPPER_PIN1,        // Arduino pin connected to the stepper driver input pin 1
    AIRSPEED_STEPPER_PIN2,        // Arduino pin connected to the stepper driver input pin 2
    AIRSPEED_STEPPER_PIN3,        // Arduino pin connected to the stepper driver input pin 3
    AIRSPEED_STEPPER_PIN4,        // Arduino pin connected to the stepper driver input pin 4
    AIRSPEED_ZERO_PIN,            // Zero angle detection input pin
);

// ******************************************************************************
// ARTIFICIAL HORIZON BANK & PITCH
// ******************************************************************************
DcsBios::EasyServo_SG90 ahorizonbank(
    SpitfireLFMkIX_AHORIZONBANK_A, // Telemetry source
    AHORIZON_BANK_SERVO_PIN       // Arduino pin connected to the servo signal wire
);

DcsBios::EasyServo_SG90 ahorizonpitch(
    SpitfireLFMkIX_AHORIZONPITCH_A, // Telemetry source
    AHORIZON_PITCH_SERVO_PIN       // Arduino pin connected to the servo signal wire
);

// ******************************************************************************
// RATE OF CLIMB INDICATOR
// 
// Developer note: DCS-BIOS doesn't have a Rate of Climb telemetry source. 
// So, we have to create it by subscribing to the Altitude source and 
// calculating the rate of change over time. Once calculated we can update 
// the stepper position based on the rate of climb.
// ******************************************************************************
unsigned int PreviousAltitude = 0;          // Previous altitude reading for ROC calculation
unsigned long LastRocUpdateMs = 0;          // Timestamp of last ROC update (2Hz = 500ms interval)
int RateOfClimbFtPerMin = 0;                // Calculated rate of climb in feet per minute

// Rate of Climb stepper (calculate from altitude deltas)
DcsBios::EasyStepper_Manual_28BYJ48 rateofclimbstepper(
    ROCLIMB_STEPPER_PIN1,      // Arduino pin connected to the stepper driver input pin 1
    ROCLIMB_STEPPER_PIN2,      // Arduino pin connected to the stepper driver input pin 2
    ROCLIMB_STEPPER_PIN3,      // Arduino pin connected to the stepper driver input pin 3
    ROCLIMB_STEPPER_PIN4,      // Arduino pin connected to the stepper driver input pin 4
    ROCLIMB_ZERO_PIN           // Zero angle detection input pin
);

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

DcsBios::EasyStepper_Manual_28BYJ48 altimeter3NeedleStepper(
    ALTIMETER_STEPPER_PIN1,    // Arduino pin connected to the stepper driver input pin 1
    ALTIMETER_STEPPER_PIN2,    // Arduino pin connected to the stepper driver input pin 2
    ALTIMETER_STEPPER_PIN3,    // Arduino pin connected to the stepper driver input pin 3
    ALTIMETER_STEPPER_PIN4     // Arduino pin connected to the stepper driver input pin 4
);
// *******************************************************************************
// GYROSCOPIC DIRECTIONAL INDICATOR (DI) with adjustment rotary encoder
// *******************************************************************************
DcsBios::EasyStepper_Manual_28BYJ48 DIStepper(
    DI_STEPPER_PIN1,          // Arduino pin connected to the stepper driver input pin 1
    DI_STEPPER_PIN2,          // Arduino pin connected to the stepper driver input pin 2
    DI_STEPPER_PIN3,          // Arduino pin connected to the stepper driver input pin 3
    DI_STEPPER_PIN4           // Arduino pin connected to the stepper driver input pin 4
);

// *******************************************************************************
// SIDE SLIP GAUGE & TURN GAUGE
// *******************************************************************************
DcsBios::EasyServo_SG90 sideslipgauge(
    SpitfireLFMkIX_SIDESLIPGAUGE_A, // Telemetry source: altitude above mean sea level in feet
    SIDESLIP_SERVO_PIN             // Arduino pin connected to the servo signal wire
);
DcsBios::EasyServo_SG90 turngauge(
    SpitfireLFMkIX_TURNGAUGE_A, // Telemetry source: altitude above mean sea level in feet
    TURN_SERVO_PIN             // Arduino pin connected to the servo signal wire
);

// *******************************************************************************
// ARDUINO SETUP
// *******************************************************************************
void setup() {
    // Airspeed
    airspeedgauge.setMaxAngle(720);

    // Artificial Horizon
    ahorizonbank.setMaxAngle(30);
    ahorizonpitch.setMaxAngle(30);

    // Rate of Climb
    rateofclimbstepper.configureBoundedBehavior(0.0f, 360.0f);
    rateofclimbstepper.setMaxRpm(20.0f); // adjust as needed
    rateofclimbstepper.setAccelRpmPerSec(40.0f);
    // configure altimeter stepper ranges
    altimeter3NeedleStepper.configureBoundedBehavior(0.0f, 360.0f);
    altimeter3NeedleStepper.setMaxRpm(20.0f); // adjust as needed
    altimeter3NeedleStepper.setAccelRpmPerSec(40.0f);
    altimeter3NeedleStepper.home(); // NOTE: Homing is NOT automatic; you must call home() explicitly.
                                     // Requires zero switch on ALTIMETER_ZERO_PIN (optional if no switch)

    // Gyro Directional Indicator (DI)
    DIStepper.configureBoundedBehavior(0.0f, 360.0f);
    DIStepper.setMaxRpm(20.0f); // adjust as needed
    DIStepper.setAccelRpmPerSec(40.0f);
    DIStepper.home(); // NOTE: Homing is NOT automatic; you must call home() explicitly.
                      // Requires zero switch on DI_ZERO_PIN (optional if no switch)

    // Slip and Turn
    sideslipgauge.setMaxAngle(30);
    turngauge.setMaxAngle(45);

    // Airspeed
    airspeedgauge.configureBoundedBehavior(0.0f, 720.0f);
    airspeedgauge.setMaxRpm(20.0f); // adjust as needed
    airspeedgauge.setAccelRpmPerSec(40.0f);
    airspeedgauge.home(); // NOTE: Homing is NOT automatic; you must call home() explicitly.
                          // Requires zero switch on AIRSPEED_ZERO_PIN (optional if no switch)

    DcsBios::setup();
}

// *******************************************************************************
// ARDUINO LOOP
// *******************************************************************************
void loop() {
    DcsBios::loop();

    // -------------------------------------
    // Spitfire Blind Panel Top Row
    // -------------------------------------

    // Service the Air Speed stepper
    airspeedgauge.loop();

    // Artificial Horizon servos 
    //      Automatically updated by DcsBios::loop() since they are EasyServo_SG90 instances.

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
    //      Automatically updated by DcsBios::loop() since they are EasyServo_SG90 instances.
}
