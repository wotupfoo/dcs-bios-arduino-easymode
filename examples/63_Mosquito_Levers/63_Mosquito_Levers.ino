#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBiosEasyMode.h>

/*
 * Mosquito Lever panel - Bomb Bay, Landing Gear, Flaps
 * Using an Arduino Nano with eight digital inputs (active LOW).
 */

#if defined(ARDUINO_AVR_NANO)
/*
 * Arduino Nano pin usage for this panel:
 *
 *                                 +-----------+
 *                           TX1 --|    ...    |-- VIN
 *                           RX0 --|    ...    |-- GND
 *                           RST --|    ISP    |-- RST
 * --------GND               GND --|           |-- +5V
 *   L     PIN_FLAPS_LOCK      2 --|           |-- A7
 *         PIN_FLAPS_UP        3 --|           |-- A6 
 *   O     PIN_GEAR_LOCK       4 --|   NANO    |-- A5 
 *         PIN_GEAR_UP         5 --|           |-- A4 
 *   O     PIN_BOMB_UP         6 --|           |-- A3
 *         PIN_BOMB_DOWN       7 --|           |-- A2
 *   M     PIN_GEAR_DOWN       8 --|           |-- A1
 * ------- PIN_FLAPS_DOWN      9 --|           |-- A0 
 *                            10 --|           |-- REFâ” See NOTE 1
 *                            11 --|  +-----+  |-- 3V3â”˜ 3.3v as ADC REF (Optional)
 *                            12 --|  | USB |  |-- 13
 *                                 +--+_____+--+
 * NOTE 1 - Strap REF to 3v3 if your adc inputs are 3.3v (eg some Hall Sensors are 3.3v). 
 *          If the inputs are 5V, DO NOT CONNECT REF TO ANYTHING
 *          If you have a mix of 3.3v and 5v inputs, set ADC REF to 5v and expect
 *          the 3.3v ADC input channels to range from 0..~700 instead of 0..1023
 * 
 */

// Digital inputs (ACTIVE LOW)
#define PIN_FLAPS_LOCK          2 // INT0
#define PIN_FLAPS_UP            3 // INT1
#define PIN_GEAR_LOCK           4
#define PIN_GEAR_UP             5
#define PIN_BOMB_UP             6
#define PIN_BOMB_DOWN           7
#define PIN_GEAR_DOWN           8
#define PIN_FLAPS_DOWN          9

// Solenoids to hold the lever at UP or DOWN
// It is assumed that there is a spring to pull the lever back to MID when off
#define PIN_FLAPS_UP_HOLD       14 // A0
#define PIN_FLAPS_DOWN_HOLD     15 // A1
#define PIN_GEAR_UP_HOLD        16 // A2
#define PIN_GEAR_DOWN_HOLD      17 // A3
#define PIN_BOMB_UP_HOLD        18 // A4/SDA
#define PIN_BOMB_DOWN_HOLD      19 // A5/SCL

#else
#error "Unsupported board - Please use an Arduino Nano or implement your own"
#endif

// ======================================================================
// ======================================================================
// DCS-BIOS Input Devices

DcsBios::EasyMode::Switch2Pos LandingGearGate(
    "CHASSIS_GATE",         // DCS-BIOS Control
    PIN_GEAR_LOCK,          // Arduino pin connected to the switch
    true                    // Reverse
);

DcsBios::EasyMode::Switch2Pos FlapsGate(
    "FLAPS_GATE",           // DCS-BIOS Control
    PIN_FLAPS_LOCK,         // Arduino pin connected to the switch
    true                    // Reverse
);

// ======================================================================
// HELPER ROUTINES
// ======================================================================

// This class will handle all the state chanages from UP, MID, DOWN
// while coordinating with the changes DCS may make to them too
DcsBios::EasyMode::SyncingSwitch3Pos bomb(
    PIN_BOMB_UP,                // UP detection pin
    PIN_BOMB_DOWN,              // DOWN detection pin
    "BOMB_LEVER",               // DCS Command
    Mosquito_BOMB_DOORS_LVR,    // DCS Output Name
    PIN_BOMB_UP_HOLD,           // UP solenoid pin
    PIN_BOMB_DOWN_HOLD          // DOWN solenoid pin
    //,LOW                      // Uncomment if solenoids are active LOW
);

DcsBios::EasyMode::SyncingSwitch3Pos gear(
    PIN_GEAR_UP,                // UP detection pin
    PIN_GEAR_DOWN,              // DOWN detection pin
    "CHASSIS_LEVER",            // DCS Command
    Mosquito_CHASSIS_LVR,       // DCS Output Name
    PIN_GEAR_UP_HOLD,           // UP solenoid pin
    PIN_GEAR_DOWN_HOLD          // DOWN solenoid pin
    //,LOW                      // Uncomment if solenoids are active LOW
);

DcsBios::EasyMode::SyncingSwitch3Pos flaps(
    PIN_FLAPS_UP,               // UP detection pin
    PIN_FLAPS_DOWN,             // DOWN detection pin
    "FLAPS_LEVER",              // DCS Command 
    Mosquito_FLAPS_LVR,         // DCS Output Name
    PIN_GEAR_UP_HOLD,           // UP solenoid pin
    PIN_GEAR_DOWN_HOLD          // DOWN solenoid pin
    //,LOW                      // Uncomment if solenoids are active LOW
);

// ======================================================================
// SETUP
// ======================================================================
void setup() {
    // NORMAL DCS BIOS MODE
    DcsBios::EasyMode::setup();

    /* DCS can get out of sync with inputs because on game load they can be
        * in any position. Until DCS gets a message saying what the value is,
        * it can be mismatched. Therefore, you need to send out the physical 
        * state occassionally to get DCS in sync. 
        * DCS-BIOS EasyMode adds this refresh capability.
        * This is only a refresh interval, when input changes it is immediately
        * sent. So refresh can be many seconds, just not too long that
        * they are wrong for a long time when the game loads into a cockpit.
        * It could go out as much as every second and the work it creates would
        * likely have little impact on the game performance. But there is no 
        * need do "as little as possible; as much as needed". Thus, 5 seconds. */
    
    // Send the state of the stateful hardware every 5 seconds
    DcsBios::EasyMode::refreshInterval(5000);
    LandingGearGate.refresh(true);
    FlapsGate.refresh(true);

    bomb.begin();
    gear.begin();
    flaps.begin();

    Serial.println("Starting DCS-BIOS");
}

// ======================================================================
// LOOP
// ======================================================================
void loop() {
    // ====================
    // NORMAL DCS BIOS MODE
    // ====================

    DcsBios::EasyMode::loop();
    DcsBios::EasyMode::watchdog_refresh();

    bomb.update();
    gear.update();
    flaps.update();
}
