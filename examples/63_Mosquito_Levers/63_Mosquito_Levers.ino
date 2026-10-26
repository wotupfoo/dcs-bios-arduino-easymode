#include <Arduino.h>
#define DCSBIOS_DEFAULT_SERIAL
// Include the optional shield driver before EasyMode to enable its motor constructor.
#if defined(ARDUINO_AVR_LEONARDO) || defined(ARDUINO_AVR_UNO) || defined(ARDUINO_SAM_DUE)
#include <Adafruit_MotorShield.h>
#endif
#include <DcsBiosEasyMode.h>


/*
 * Mosquito Lever panel - Bomb Bay, Landing Gear, Flaps - with optional electro-mechanical actuators
 */

#if defined(ARDUINO_AVR_NANO)
/*
 * The Nano example doesn't use a motor shield. It optionally connects to an external ULN2003 driver board
 * to drive electromagnets (H2015 20x15mm). It can only "pull/hold" the lever, not "push" it back to center
 *
 * Arduino Nano pin usage for this panel:
 *
 *                                 +-----------+
 *                           TX1 --|    ...    |-- VIN
 *                           RX0 --|    ...    |-- GND
 *                           RST --|    ISP    |-- RST
 * --------GND               GND --|           |-- +5V
 *   L     PIN_FLAPS_LOCK    > 2 --|           |-- A7
 *         PIN_FLAPS_UP      > 3 --|           |-- A6 
 *   O     PIN_GEAR_LOCK     > 4 --|   NANO    |-- 19/A5 > PIN_BOMB_DOWN_HOLD_POS
 *         PIN_GEAR_UP       > 5 --|           |-- 18/A4 > PIN_BOMB_UP_HOLD_POS
 *   O     PIN_BOMB_UP       > 6 --|           |-- 17/A3 > PIN_GEAR_DOWN_HOLD_POS
 *         PIN_BOMB_DOWN     > 7 --|           |-- 16/A2 > PIN_GEAR_UP_HOLD_POS
 *   M     PIN_GEAR_DOWN     > 8 --|           |-- 15/A1 > PIN_FLAPS_DOWN_HOLD_POS
 * ------- PIN_FLAPS_DOWN    > 9 --|           |-- 14/A0 > PIN_FLAPS_UP_HOLD_POS
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

// Digital inputs (ACTIVE LOW)
#define PIN_FLAPS_LOCK          2 // INT0
#define PIN_FLAPS_UP            3 // INT1
#define PIN_GEAR_LOCK           4
#define PIN_GEAR_UP             5
#define PIN_BOMB_UP             6
#define PIN_BOMB_DOWN           7
#define PIN_GEAR_DOWN           8
#define PIN_FLAPS_DOWN          9

// Electromagnets to hold the lever at UP or DOWN
// It is assumed that there is a spring to pull the lever back to MID when off.
// The NANO does not have enough digital pins to drive the electromagnet in reverse
// using a H-Bridge. It can only drive an on/off like a ULN2003 darlington drive board.
// If you want to repell the lever back to middle you need to use an Arduino Leonoardo (see below)
#define PIN_FLAPS_UP_HOLD_POS       14 // A0
#define PIN_FLAPS_DOWN_HOLD_POS     15 // A1
#define PIN_GEAR_UP_HOLD_POS        16 // A2
#define PIN_GEAR_DOWN_HOLD_POS      17 // A3
#define PIN_BOMB_UP_HOLD_POS        18 // A4/SDA
#define PIN_BOMB_DOWN_HOLD_POS      19 // A5/SCL

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

// This SyncingRelativeSwitch3Pos will handle all the state changes 
// from UP, MID, DOWN while coordinating with the changes DCS may 
// make to them too.
// NOTE, unlike typical Switch 3 Pos which uses absolute values for
// the location (0,1,2), this uses logic to control relative "INC"
// and "DEC" messages.
// [Optional]
// If electromagnets are present to hold the lever in UP or DOWN with a 
// spring to pull the lever back to the center (MID) it is fully 
// automated. 
DcsBios::EasyMode::SyncingRelativeSwitch3Pos bomb(
    PIN_BOMB_UP,                // UP detection pin
    PIN_BOMB_DOWN,              // DOWN detection pin
    "BOMB_LEVER",               // DCS Command
    Mosquito_BOMB_DOORS_LVR,    // DCS Output Name
    PIN_BOMB_UP_HOLD_POS,           // UP electromagnet pin
    PIN_BOMB_DOWN_HOLD_POS          // DOWN electromagnet pin
    //,LOW                      // Uncomment if electromagnets are active LOW
);

DcsBios::EasyMode::SyncingRelativeSwitch3Pos gear(
    PIN_GEAR_UP,                // UP detection pin
    PIN_GEAR_DOWN,              // DOWN detection pin
    "CHASSIS_LEVER",            // DCS Command
    Mosquito_CHASSIS_LVR,       // DCS Output Name
    PIN_GEAR_UP_HOLD_POS,           // UP electromagnet pin
    PIN_GEAR_DOWN_HOLD_POS          // DOWN electromagnet pin
    //,LOW                      // Uncomment if electromagnets are active LOW
);

DcsBios::EasyMode::SyncingRelativeSwitch3Pos flaps(
    PIN_FLAPS_UP,               // UP detection pin
    PIN_FLAPS_DOWN,             // DOWN detection pin
    "FLAPS_LEVER",              // DCS Command 
    Mosquito_FLAPS_LVR,         // DCS Output Name
    PIN_FLAPS_UP_HOLD_POS,           // UP electromagnet pin
    PIN_FLAPS_DOWN_HOLD_POS          // DOWN electromagnet pin
    //,LOW                      // Uncomment if electromagnets are active LOW
);

#elif defined(ARDUINO_AVR_LEONARDO) || defined(ARDUINO_AVR_UNO) || defined(ARDUINO_SAM_DUE)
/*
 * The Leonardo, Uno, and Due use the AdaFruit MotorShield V3 with a I2C->PWM->H-bridge
 * Stacking two Motor Shields gives 8 DC motor bi-directional outputs. We use 6.
 *
 * Leonardo / Uno / Due with two Adafruit Motor Shields (V2 library):
 * Leonardo pinout (Uno uses the same D4..D11 input loom):
 *
 *                        +--[POWER]--[USB]--+
 *                        |     [RESET]      |
 *                        |                  |-- SCL (D3) -> shield SCL
 *                        |                  |-- SDA (D2) -> shield SDA
 *                        |                  |-- AREF
 *                      NC|                  |-- GND
 *                   IOREF|                  |-- D13
 *                   RESET|                  |-- D12
 *                     3V3|      UNO or      |-- D11 < PIN_FLAPS_DOWN
 *                      5V|     LEONARDO     |-- D10 < PIN_GEAR_DOWN
 *                     GND|                  |-- D9 < PIN_BOMB_DOWN
 *                     GND|                  |-- D8 < PIN_BOMB_UP
 *                     VIN|                  |
 *                        |                  |-- D7 < PIN_GEAR_UP
 *                      A0|                  |-- D6 < PIN_GEAR_LOCK
 *                      A1|                  |-- D5 < PIN_FLAPS_UP
 *                      A2|                  |-- D4 < PIN_FLAPS_LOCK
 *                      A3|                  |-- D3 (SCL, same pin above)
 *                      A4|       ...        |-- D2 (SDA, same pin above)
 *                      A5|       ...        |-- D1
 *                        |       ISP        |-- D0
 *                        +------------------+
 *
 * Due pinout, component side, USB/power at the top:
 * Reference: documentation/images/Arduino/Pinout-Due.png
 *
 *                        +--[POWER]--[PROG USB]--[NATIVE USB]-----+
 *                        |                              [RESET]   |
 *                        |                                        |-- SCL1 -> shield SCL (Wire1)
 *                        |                                        |-- SDA1 -> shield SDA (Wire1)
 *                        |                                        |-- AREF
 *                      NC|                                        |-- GND
 *                   IOREF|                                        |-- D13
 *                   RESET|                                        |-- D12
 *                     3V3|                                        |-- D11 < PIN_FLAPS_DOWN
 *                      5V|                                        |-- D10 < PIN_GEAR_DOWN
 *                     GND|            ARDUINO DUE 3.3V            |-- D9  < PIN_BOMB_DOWN
 *                     GND|                                        |-- D8  < PIN_BOMB_UP
 *                     VIN|                                        |-- D7  < PIN_GEAR_UP
 *                        |                                        |-- D6  < PIN_GEAR_LOCK
 *                      A0|                                        |-- D5  < PIN_FLAPS_UP
 *                      A1|                                        |-- D4  < PIN_FLAPS_LOCK
 *                      A2|                                        |-- D3
 *                      A3|                                        |-- D2
 *                      A4|                                        |-- D1 / TX0
 *                      A5|                  ...                   |-- D0 / RX0
 *                      A6|                  ...                   |
 *                      A7|                  SPI                   |-- D14 / TX3
 *                        |                                        |-- D15 / RX3
 *                      A8|                              [ERASE]   |-- D16 / TX2
 *                      A9|                                        |-- D17 / RX2
 *                     A10|                                        |-- D18 / TX1
 *                     A11|                                        |-- D19 / RX1
 *                    DAC0|                                        |-- D20 / SDA (Wire, not shield bus)
 *                    DAC1|                                        |-- D21 / SCL (Wire, not shield bus)
 *                   CANRX|                                        |
 *                   CANTX|                                        |
 *                        |      [ D22..D53 DUAL-ROW HEADER ]      |
 *                        +----------------------------------------+
 *
 * D4..D11 form one input loom, with a common GND return.
 * PROG USB is the Programming port used by Serial in this sketch.
 * SCL1/SDA1 near AREF are the stacked shield bus; use Wire1.
 *
 * Leonardo: SCL/SDA at the top duplicate D3/D2.
 * Uno R3:   SCL/SDA at the top duplicate A5/A4; D2/D3 are free.
 * Due: stacked shields use SDA1/SCL1 near AREF (Wire1), not D20/D21.
 * Set shield logic Vcc and I2C pull-ups to 3.3 V for Due.
 * Due uses the Programming USB port (Serial); calibration is not persisted.
 * All three boards use the same eight input pins on D4..D11.
 * 
 * AdaFruit MotorShield V3 (or clone) I2C driven 4xDC Motor or 2xStepper Motor board
 * https://www.amazon.com/dp/B0D9V7QHNX
 * "TB6612 Mosfet Stepper Motor PCA9685 Standard IIC I2C PWM Servo Driver Shield V2 
 * for Arduino Robot PWM Mega R3 Replace L293D"
 * 
 * Motor shield, component side (M2-GND-M1 at the top):
 *
 *                        +-------------------------------------------------------+
 *                        | [POWER + -]       [ M2 ][GND][ M1 ]       [SERVO 1/2] |
 *                        |  [VIN JUMPER]                                [RESET]  |-- SCL -> Leo D3 / Uno A5 / Due SCL1
 *                        |                                                       |-- SDA -> Leo D2 / Uno A4 / Due SDA1
 *                  IOREF |                                                       |-- AREF
 *                  RESET |           o o o o o o o o o o o o o o o o o           |-- GND
 *                    3V3 |           o o o o o o o o o o o o o o o o o           |-- D13
 *                     5V |           o o o o o o o o o o o o o o o o o           |-- D12
 *                    GND |                                                       |-- D11
 *                    GND |             A4 :                                      |-- D10
 *                    VIN |             A3 :      +---------+                     |-- D9
 *                        |      LOGIC  A2 :      | PCA9685 |                     |-- D8
 *                        |      3V/5V  A1 :      |I2C->PWM |                     |
 *                     A0 |             A0 :      +---------+                     |-- D7
 *                     A1 |                ^                                      |-- D6
 *                     A2 |          Address jumpers                              |-- D5
 *                     A3 |      (not Arduino analog pins)                        |-- D4
 *           A4 (Uno SDA) |                                                       |-- D3 (Leo SCL)
 *           A5 (Uno SCL) |         [TB6612]                     [TB6612]         |-- D2 (Leo SDA)
 *                        |                                                       |-- D1
 *                        |                            [ M3 ][GND][ M4 ]          |-- D0
 *                        +-------------------------------------------------------+
 *
 * Both shields stack on the Leonardo, Uno, or Due; only SDA/SCL carry magnet commands.
 * Header pins pass through to the Arduino for the physical switches.
 * Address pads are below the PCA9685 in the reference photograph; after
 * rotation they form the vertical A4..A0 column shown above.
 * Bridge the two A0 pads on the top board: 0x40 -> 0x41 on this board.
 * Leave all address pads open on the bottom board for 0x40.
 *
 * Bottom board 0x40: M1/M2 = BOMB UP/DOWN, M3/M4 = GEAR UP/DOWN.
 * Top board    0x41: M1/M2 = FLAPS UP/DOWN (logical motors 5/6).
 * Each board uses local getMotor(1)..getMotor(4) channel numbers.
 *
 *
 * Shield 1, address 0x40: motors 1/2 (M1/M2) = BOMB UP/DOWN,
 *                         motors 3/4 (M3/M4) = GEAR UP/DOWN.
 * Shield 2, address 0x41: motors 5/6 (M1/M2) = FLAPS UP/DOWN.
 * Both shields stack directly on the Leonardo, Uno, or Due and share SDA/SCL.
 *
 * !!! The new board uses the PCA9685 I2C->PWM chipset starting at 0x40 !!!
 *
 * Hardware must respond at 0x40/0x41 to match these declarations.
 *
 * Old Adafruit shields use 0x60/0x61 instead; their jumpers select
 * 0x60..0x7F. 
 * Change the declarations below for the shield you have.
 * Wire each magnet so FORWARD pulls and BACKWARD pushes.
 * Channels use fullOn(): set coil power with the external supply voltage.
 * A4/A5 are reserved for I2C on Uno and unused as inputs on Leonardo.
 * All eight switch inputs form one loom on D4..D11, in Nano input order.
 * Include a common GND return in the input loom.
 */

// Digital inputs (ACTIVE LOW): one loom on D4..D11, in Nano input order.
#define PIN_FLAPS_LOCK          4
#define PIN_FLAPS_UP            5
#define PIN_GEAR_LOCK           6
#define PIN_GEAR_UP             7
#define PIN_BOMB_UP             8
#define PIN_BOMB_DOWN           9
#define PIN_GEAR_DOWN           10
#define PIN_FLAPS_DOWN          11

Adafruit_MotorShield magnetShield1(0x40);   // PCA9685 chipset default address
Adafruit_MotorShield magnetShield2(0x41);   // Make sure you solder the A0 jumper!

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

// This SyncingRelativeSwitch3Pos will handle all the state changes 
// from UP, MID, DOWN while coordinating with the changes DCS may 
// make to them too.
// NOTE, unlike typical Switch 3 Pos which uses absolute values for
// the location (0,1,2), this uses logic to control relative "INC"
// and "DEC" messages.
// [Optional]
// If electromagnets are present to hold the lever in UP or DOWN with a 
// spring to pull the lever back to the center (MID) it is fully 
// automated. 
DcsBios::EasyMode::SyncingRelativeSwitch3Pos bomb(
    PIN_BOMB_UP,                // UP detection pin
    PIN_BOMB_DOWN,              // DOWN detection pin
    "BOMB_LEVER",               // DCS Command
    Mosquito_BOMB_DOORS_LVR,    // DCS Output Name
    magnetShield1.getMotor(1),   // UP electromagnet
    magnetShield1.getMotor(2)    // DOWN electromagnet
);

DcsBios::EasyMode::SyncingRelativeSwitch3Pos gear(
    PIN_GEAR_UP,                // UP detection pin
    PIN_GEAR_DOWN,              // DOWN detection pin
    "CHASSIS_LEVER",            // DCS Command
    Mosquito_CHASSIS_LVR,       // DCS Output Name
    magnetShield1.getMotor(3),   // UP electromagnet
    magnetShield1.getMotor(4)    // DOWN electromagnet
);

DcsBios::EasyMode::SyncingRelativeSwitch3Pos flaps(
    PIN_FLAPS_UP,               // UP detection pin
    PIN_FLAPS_DOWN,             // DOWN detection pin
    "FLAPS_LEVER",              // DCS Command 
    Mosquito_FLAPS_LVR,         // DCS Output Name
    magnetShield2.getMotor(1),   // UP electromagnet
    magnetShield2.getMotor(2)    // DOWN electromagnet
);

#else
#error "Unsupported board - Please use an Arduino Nano, Leonardo, Uno or Due or implement your own"
#endif

// ======================================================================
// SETUP
// ======================================================================
void setup() {
    // NORMAL DCS BIOS MODE
    DcsBios::EasyMode::setup();

#if defined(ARDUINO_AVR_LEONARDO) || defined(ARDUINO_AVR_UNO) || defined(ARDUINO_SAM_DUE)
    // Initialize both shields before the lever begin() calls enable full DC drive.
#if defined(ARDUINO_SAM_DUE)
    // The stacked R3 shield connector is on the Due's second I2C interface.
    if (!magnetShield1.begin(1600, &Wire1) || !magnetShield2.begin(1600, &Wire1)) {
#else
    if (!magnetShield1.begin() || !magnetShield2.begin()) {
#endif
        Serial.println("Magnet shield initialization failed; check addresses 0x40 and 0x41.");
        while (true) {
            DcsBios::EasyMode::watchdog_refresh();
        }
    }
#endif

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
