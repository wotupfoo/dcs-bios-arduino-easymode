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

// Analog inputs
#define PIN_ELEVATOR_TRIM      A2
#define PIN_RUDDER_TRIM        A1
#define PIN_AILERON_TRIM       A0

// Digital inputs
#define PIN_CALIBRATION_BTN    4

// Set the digital input to check on bootup to go into Calibration Mode
#define CALIBRATION_MODE_BUTTON         PIN_CALIBRATION_BTN

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
                                        TRIM_DEADBAND);
DcsBios::EasyMode::AnalogSyncingRocker rudderTrim(
                                        "RUDDER_TRIM",
                                        PIN_RUDDER_TRIM,
                                        Mosquito_RUDDER_TRIM_GAUGE,
                                        TRIM_GAUGE_MIN,
                                        TRIM_GAUGE_MAX,
                                        TRIM_DEADBAND);
DcsBios::EasyMode::AnalogSyncingRocker elevatorTrim(
                                        "ELEVATOR_TRIM",
                                        PIN_ELEVATOR_TRIM,
                                        Mosquito_ELEVATOR_TRIM_GAUGE,
                                        TRIM_GAUGE_MIN,
                                        TRIM_GAUGE_MAX,
                                        TRIM_DEADBAND);

// ======================================================================
// HELPER ROUTINES
// ======================================================================

// ======================================================================
// Debounced button helper
bool buttonPressedDebounced(byte pin, bool level) {
    bool i = digitalRead(pin);
    delay(25);
    bool j = digitalRead(pin);
    return (i == level && j == level);
}

// A flag to run in Calibration Mode or Normal Mode
bool calibrationMode;

// ======================================================================
// SETUP
// ======================================================================
void setup() {
    // Disable the Watchdog Timer (used to reboot out of Calibration mode)
    DcsBios::EasyMode::reboot_disable();

#if defined(ARDUINO_AVR_NANO)
    /* Optionally set the Nano ADC to use a clean external reference 
     * voltage for the analog to digital (adc) converter.
     * Joining 3.3v to REF is how you get the full adc range when you 
     * have 3.3v inputs. */
    // analogReference(EXTERNAL);
#endif

    /* Wait a second before looking at the fire button to see
     * if we are to go into Calibration Mode (vs Normal DCS-BIOS mode) */
    delay(1000);

    /* If the Calibration Mode button is pressed during bootup go into
     * Calibration Mode to set the ranges of each analog input.
     *
     * Particularly important for Hall Effect sensors since they are 
     * no where near the full 0..5v analog range ([0..1023]) */
    pinMode(CALIBRATION_MODE_BUTTON, INPUT_PULLUP);
    calibrationMode = buttonPressedDebounced(CALIBRATION_MODE_BUTTON, LOW);

    if(calibrationMode) {
        // CALIBRATION MODE
        DcsBios::EasyMode::setupCalibration();
    }
    else {
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
        DcsBios::EasyMode::refreshInterval(5000);

        aileronTrim.refresh(true);
        rudderTrim.refresh(true);
        elevatorTrim.refresh(true);
    }
}

// ======================================================================
// LOOP
// ======================================================================
void loop() {
    if(calibrationMode)
    {
        // ====================
        // CALIBRATION MODE
        // ====================

        /* Move all inputs through the full range of travel to find the 
         * minimum and maximum values. */

        DcsBios::EasyMode::loopCalibration(Serial);

        /* If the calibration mode switch has been released (HIGH), 
         * reboot (using a watchdog timeout) into normal operation */
        if(buttonPressedDebounced(CALIBRATION_MODE_BUTTON, HIGH)) {
            DcsBios::EasyMode::reboot();
        }

        /* Wait 1 second before looking for new limits of range again.
         * There is little point to be saving the new values more 
         * frequently than that. */
        delay(1000); 
    }
    else
    {
        // ====================
        // NORMAL DCS BIOS MODE
        // ====================

        DcsBios::EasyMode::loop();
    }
}
