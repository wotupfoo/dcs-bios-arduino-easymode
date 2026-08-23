#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBiosEasyMode.h>

/*
 * Mosquito trim panel.
 * Using an Arduino Nano with three analog Hall sensors.
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
    *                             2 --|           |-- A7
    *                             3 --|           |-- A6
    *         PIN_CALIBRATION_BTN 4 --|   NANO    |-- A5
    *                             5 --|           |-- A4
    *                             6 --|           |-- A3
    *                             7 --|           |-- A2 PIN_ELEVATOR_TRIM
    *                             8 --|           |-- A1 PIN_RUDDER_TRIM
    *                             9 --|           |-- A0 PIN_AILERON_TRIM
    *                            10 --|           |-- REF
    *                            11 --|  +-----+  |-- 3V3 Optional ADC reference
    *                            12 --|  | USB |  |-- 13
    *                                 +--+_____+--+
    */
#define PIN_AILERON_TRIM       A0
#define PIN_RUDDER_TRIM        A1
#define PIN_ELEVATOR_TRIM      A2
#define PIN_CALIBRATION_BTN    4

#else
#error "Unsupported board - Please use an Arduino Nano or implement your own"
#endif

// These names are placeholders for future continuous DCS-BIOS trim exports.
// The current Mosquito trim definitions only export rocker state 0/1/2.
#ifndef Mosquito_AILERON_TRIM_GAUGE
#define Mosquito_AILERON_TRIM_GAUGE 0xFFFF, 0xFFFF, 0
#endif

#ifndef Mosquito_RUDDER_TRIM_GAUGE
#define Mosquito_RUDDER_TRIM_GAUGE 0xFFFF, 0xFFFF, 0
#endif

#ifndef Mosquito_ELEVATOR_TRIM_GAUGE
#define Mosquito_ELEVATOR_TRIM_GAUGE 0xFFFF, 0xFFFF, 0
#endif

// ======================================================================
// Arduino Nano analogRead() returns values in the 0..1023 ADC domain.
static const uint16_t ADC_SPAN = 1024;
static const uint16_t TRIM_GAUGE_MIN = 0;
static const uint16_t TRIM_GAUGE_MAX = 65535;
static const uint16_t TRIM_DEADBAND = 512;
// ======================================================================

// ======================================================================
// DCS-BIOS Input Devices
DcsBios::EasyMode::AnalogSyncingRocker aileronTrim(
                                        "AILERON_TRIM",
                                        PIN_AILERON_TRIM,
                                        Mosquito_AILERON_TRIM_GAUGE,
                                        TRIM_GAUGE_MIN,
                                        TRIM_GAUGE_MAX,
                                        ADC_SPAN,
                                        TRIM_DEADBAND);
DcsBios::EasyMode::AnalogSyncingRocker rudderTrim(
                                        "RUDDER_TRIM",
                                        PIN_RUDDER_TRIM,
                                        Mosquito_RUDDER_TRIM_GAUGE,
                                        TRIM_GAUGE_MIN,
                                        TRIM_GAUGE_MAX,
                                        ADC_SPAN,
                                        TRIM_DEADBAND);
DcsBios::EasyMode::AnalogSyncingRocker elevatorTrim(
                                        "ELEVATOR_TRIM",
                                        PIN_ELEVATOR_TRIM,
                                        Mosquito_ELEVATOR_TRIM_GAUGE,
                                        TRIM_GAUGE_MIN,
                                        TRIM_GAUGE_MAX,
                                        ADC_SPAN,
                                        TRIM_DEADBAND);
// ======================================================================



bool buttonPressedDebounced(byte pin, bool level) {
    bool i = digitalRead(pin);
    delay(25);
    bool j = digitalRead(pin);
    return (i == level && j == level);
}


bool calibrationMode;

void setup() {
    DcsBios::EasyMode::reboot_disable();

    // If AREF is wired to the Nano 3V3 pin, uncomment this before any ADC reads.
    // analogReference(EXTERNAL);

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PIN_CALIBRATION_BTN, INPUT_PULLUP);

    delay(1000);
    calibrationMode = digitalRead(PIN_CALIBRATION_BTN) == LOW;

    if (calibrationMode) {
        Serial.begin(250000);
        digitalWrite(LED_BUILTIN, HIGH);
    } else {
        digitalWrite(LED_BUILTIN, LOW);
    }


    if (calibrationMode) {
        Serial.println("Entering Calibration Mode");
        DcsBios::EasyMode::beginCalibration();
    } else {
        DcsBios::EasyMode::loadCalibration();
        DcsBios::EasyMode::setup();

        DcsBios::EasyMode::refreshInterval(1000);
        aileronTrim.refresh(true);
        rudderTrim.refresh(true);
        elevatorTrim.refresh(true);
    }
}

void loop() {
    if (calibrationMode) {
        DcsBios::EasyMode::serviceCalibration(Serial);
        delay(1000);

        if (buttonPressedDebounced(PIN_CALIBRATION_BTN, HIGH)) {
            Serial.println("Rebooting");
            DcsBios::EasyMode::reboot();
        }
    } else {
        DcsBios::EasyMode::loop();
    }
}
