#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBiosEasyMode.h>

/*
 * Mosquito Fuel Panel telemetry output.
 * Using an Arduino Nano
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
    *            FUEL_STBD_CUTOUT 2 --|           |-- A7
    *            FUEL_PORT_CUTOUT 3 --|           |-- A6
    *                             4 --|   NANO    |-- A5
    *               TANK_PRESSURE 5 --|           |-- A4
    *    FUEL_COCK_PORT_OUTER_PIN 6 --|           |-- A3
    *      FUEL_COCK_PORT_OFF_PIN 7 --|           |-- A2
    *     FUEL_COCK_PORT_MAIN_PIN 8 --|           |-- A1
    *     FUEL_COCK_STBD_MAIN_PIN 9 --|           |-- A0
    *     FUEL_COCK_STBD_OFF_PIN 10 --|           |-- REF
    *   FUEL_COCK_STBD_OUTER_PIN 11 --|  +-----+  |-- 3V3
    *                            12 --|  | USB |  |-- 13
    *                                 +--+_____+--+
    *
    */
#define TANK_PRESSURE_PIN    5

#define FUEL_PORT_CUTOUT_PIN 3
#define FUEL_STBD_CUTOUT_PIN 2

#define FUEL_COCK_PORT_OUTER_PIN 9
//#define FUEL_COCK_PORT_OFF_PIN   10
#define FUEL_COCK_PORT_MAIN_PIN  11

#define FUEL_COCK_STBD_MAIN_PIN  6
//#define FUEL_COCK_STBD_OFF_PIN   7
#define FUEL_COCK_STBD_OUTER_PIN 8
#else
#error "Unsupported board - Please use an Arduino Nano or implement your own"
#endif

// Tank Pressurizing Lever
DcsBios::EasyMode::Switch2Pos tankPress("TANK_PRESS", TANK_PRESSURE_PIN);

// Port Fuel Cutout
DcsBios::EasyMode::Switch2Pos portFuelCutout("PORT_FUEL_CUTOUT", FUEL_PORT_CUTOUT_PIN);

// Starboard Fuel Cutout
DcsBios::EasyMode::Switch2Pos stbdFuelCutout("STBD_FUEL_CUTOUT", FUEL_STBD_CUTOUT_PIN);

// Port Fuel Cock
DcsBios::EasyMode::Switch3Pos portFuelCock("PORT_FUEL_COCK", 
                                           FUEL_COCK_PORT_MAIN_PIN, 
                                           FUEL_COCK_PORT_OUTER_PIN);
// Starboard Fuel Cock
DcsBios::EasyMode::Switch3Pos stbdFuelCock("STBD_FUEL_COCK", 
                                           FUEL_COCK_STBD_MAIN_PIN, 
                                           FUEL_COCK_STBD_OUTER_PIN);

void setup() {
    DcsBios::EasyMode::setup();
}

void loop() {
    DcsBios::EasyMode::loop();
}
