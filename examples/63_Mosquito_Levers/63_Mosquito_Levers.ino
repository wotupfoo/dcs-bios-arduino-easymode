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
 *                           GND --|           |-- +5V
 *         PIN_FLAPS_DOWN      2 --|           |-- A7
 *         PIN_FLAPS_UP        3 --|           |-- A6 
 *         PIN_FLAPS_LOCK      4 --|   NANO    |-- A5 
 *         PIN_GEAR_DOWN       5 --|           |-- A4 
 *         PIN_GEAR_UP         6 --|           |-- A3
 *         PIN_GEAR_LOCK       7 --|           |-- A2
 *         PIN_BOMB_DOWN       8 --|           |-- A1
 *         PIN_BOMB_UP         9 --|           |-- A0 
 *                            10 --|           |-- REF┐ See NOTE 1
 *                            11 --|  +-----+  |-- 3V3┘ 3.3v as ADC REF (Optional)
 *                            12 --|  | USB |  |-- 13
 *                                 +--+_____+--+
 * NOTE 1 - Strap REF to 3v3 if your adc inputs are 3.3v (eg some Hall Sensors are 3.3v). 
 *          If the inputs are 5V, DO NOT CONNECT REF TO ANYTHING
 *          If you have a mix of 3.3v and 5v inputs, set ADC REF to 5v and expect
 *          the 3.3v ADC input channels to range from 0..~700 instead of 0..1023
 * 
 */

// Digital inputs
#define PIN_FLAPS_DOWN          2
#define PIN_FLAPS_UP            3
#define PIN_FLAPS_LOCK          4

#define PIN_GEAR_DOWN           5
#define PIN_GEAR_UP             6
#define PIN_GEAR_LOCK           7

#define PIN_BOMB_DOWN           8
#define PIN_BOMB_UP             9

#else
#error "Unsupported board - Please use an Arduino Nano or implement your own"
#endif

// ======================================================================
// ======================================================================
// DCS-BIOS Input Devices

// ======================================================================
// Bomb Bay
DcsBios::EasyMode::ActionButton BombBayUp(
    "CHASSIS_LEVER",
    "INC",
    PIN_BOMB_UP,

};

DcsBios::EasyMode::ActionButton BombBayDown(
    "CHASSIS_LEVER",
    "DEC",
    PIN_BOMB_DOWN
};

// ======================================================================
// Landing Gear ("Chassis")
DcsBios::EasyMode::ActionButton LandingGearUp(
    "CHASSIS_LEVER",        // DCS-BIOS Control
    "INC",                  // Message Argument
    PIN_GEAR_UP             // Arduino pin connected to the switch
};
DcsBios::EasyMode::ActionButton LandingGearDown(
    "CHASSIS_LEVER",        // DCS-BIOS Control
    "DEC",                  // Message Argument
    PIN_GEAR_DOWN           // Arduino pin connected to the switch
};

DcsBios::EasyMode::Switch2Pos LandingGearGate(
    "CHASSIS_GATE",         // DCS-BIOS Control
    PIN_GEAR_LOCK           // Arduino pin connected to the switch
);

// ======================================================================
// Flaps
DcsBios::EasyMode::ActionButton FlapsUp(
    "FLAPS_LVR",            // DCS-BIOS Control
    "INC",                  // Message Argument
    PIN_FLAPS_UP            // Arduino pin connected to the switch
};
DcsBios::EasyMode::ActionButton FlapsDown(
    "FLAPS_LVR",            // DCS-BIOS Control
    "DEC",                  // Message Argument
    PIN_FLAPS_DOWN          // Arduino pin connected to the switch
};

DcsBios::EasyMode::Switch2Pos FlapsGate(
    "FLAPS_GATE",           // DCS-BIOS Control
    PIN_FLAPS_LOCK          // Arduino pin connected to the switch
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
    
    // Send the state of the hardware every 5 seconds
    //DcsBios::EasyMode::refreshInterval(5000);
}

// ======================================================================
// LOOP
// ======================================================================
void loop() {
    // ====================
    // NORMAL DCS BIOS MODE
    // ====================

    DcsBios::EasyMode::loop();
}
