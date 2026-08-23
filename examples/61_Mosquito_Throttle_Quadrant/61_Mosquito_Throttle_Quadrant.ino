#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBiosEasyMode.h>
/*
 * Mosquito Throttle Quadrant.
 */

#if defined(ARDUINO_AVR_NANO)
/*
 * Arduino Nano pin usage for this panel:
 *
 *                                 +-----------+
 *                           TX1 --|    ...    |-- VIN
 *                           RX0 --|    ...    |-- GND  << USE THIS
 *                           RST --|    ISP    |-- RST
 *               USE THIS >> GND --|           |-- +5V  << USE THIS
 *                             2 --|           |-- A7
 *            PIN_SUPERCHARGER 3 --|           |-- A6
 *           PIN_RKT_FIRING_SW 4 --|   NANO    |-- A5
 *    CALIBRATION_MODE_BUTTON* 5 --|           |-- A4 PIN_MIXTURE
 *                             6 --|           |-- A3 PIN_THROTTLE_CONTROL_L
 *                             7 --|           |-- A2 PIN_THROTTLE_CONTROL_R
 *                             8 --|           |-- A1 PIN_THROTTLE_PROP_CONTROL_L
 *                             9 --|           |-- A0 PIN_THROTTLE_PROP_CONTROL_R
 *                            10 --|           |-- REF┐ See NOTE 1
 *                            11 --|  +-----+  |-- 3V3┘ 3.3v as ADC REF (Optional)
 *                            12 --|  | USB |  |-- 13
 *                                 +--+_____+--+
 * NOTE 1 - Strap REF to 3v3 if your adc inputs are 3.3v (eg some Hall Sensors are 3.3v). 
 *          If the inputs are 5V, DO NOT CONNECT REF TO ANYTHING
 *          If you have a mix of 3.3v and 5v inputs, set ADC REF to 5v and expect
 *          the 3.3v ADC input channels to range from 0..~700 instead of 0..1023
 * 
 * CALIBRATION_MODE_BUTTON* is just a suggestion. 
 * This example looks at the PIN_RKT_FIRING_SW input at boot time otherwise normal use
 */

 // Analog inputs
#define PIN_THROTTLE_PROP_CONTROL_R     A0
#define PIN_THROTTLE_PROP_CONTROL_L     A1
#define PIN_THROTTLE_CONTROL_R          A2
#define PIN_THROTTLE_CONTROL_L          A3
#define PIN_MIXTURE                     A4

// Digital inputs
#define PIN_SUPERCHARGER                3
#define PIN_RKT_FIRING_SW               4

// Set the digital input to check on bootup to go into Calibration Mode
//#define CALIBRATION_MODE_BUTTON       5    // Dedicated pin (see diagram above)
#define CALIBRATION_MODE_BUTTON         PIN_RKT_FIRING_SW // Use the Rocket Button


#else
#error "Unsupported board - Please use an Arduino Nano or implement your own"
#endif

// ======================================================================
// DCS-BIOS Input Devices
DcsBios::EasyMode::Potentiometer throttleControlL("THROTTLE_CONTROL_L",
                                        PIN_THROTTLE_CONTROL_L,
                                        true,     // reverse
                                        3);       // adc hysteresis
DcsBios::EasyMode::Potentiometer throttleControlR("THROTTLE_CONTROL_R",
                                        PIN_THROTTLE_CONTROL_R,
                                        false,    // reverse
                                        3);       // adc hysteresis
DcsBios::EasyMode::Potentiometer propControlL("PROP_CONTROL_L",
                                        PIN_THROTTLE_PROP_CONTROL_L,
                                        true,     // reverse
                                        3);       // adc hysteresis
DcsBios::EasyMode::Potentiometer propControlR("PROP_CONTROL_R",
                                        PIN_THROTTLE_PROP_CONTROL_R,
                                        false,    // reverse
                                        3);       // adc hysteresis

DcsBios::EasyMode::AnalogMultiPos mixture("MIXTURE",
                                        PIN_MIXTURE,
                                        1,        // positions (0,1)
                                        3,        // adc hysteresis
                                        true);    // fast polling

DcsBios::EasyMode::Switch2Pos rocketFiring("RKT_FIRING_SW",
                                        PIN_RKT_FIRING_SW,
                                        false);  // reverse
DcsBios::EasyMode::Switch2Pos superCharger("SUPERCHARGER",
                                        PIN_SUPERCHARGER,
                                        true);  // reverse

// ======================================================================
// HELPER ROUTINES
// ======================================================================

// ======================================================================
// Debounced button helper
bool buttonPressedDebounced(byte pin, bool level) {
    bool i,j;
    i = digitalRead(pin);
    delay(25);
    j = digitalRead(pin);
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

    pinMode(LED_BUILTIN, OUTPUT);   // Used to show operating mode

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

    /* If the Rocket Firing button is pressed during bootup go into
     * Calibration Mode to set the ranges of each analog input.
     *
     * Particularly important for Hall Effect sensors since they are 
     * no where near the full 0..5v analog range ([0..1023]) */
    pinMode(CALIBRATION_MODE_BUTTON, INPUT_PULLUP);
    calibrationMode = digitalRead(CALIBRATION_MODE_BUTTON) == LOW;

    if(calibrationMode) {
        // CALIBRATION MODE
        digitalWrite(LED_BUILTIN, HIGH);
        Serial.begin(250000);
        Serial.println("Entering Calibration Mode");
        DcsBios::EasyMode::beginCalibration();
    }
    else {
        // NORMAL DCS BIOS MODE
        digitalWrite(LED_BUILTIN, LOW);
        DcsBios::EasyMode::loadCalibration();
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

        throttleControlL.refresh(true);
        throttleControlR.refresh(true);
        propControlL.refresh(true);
        propControlR.refresh(true);

        mixture.refresh(true);

        rocketFiring.refresh(true);
        superCharger.refresh(true);
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

        DcsBios::EasyMode::serviceCalibration(Serial);
        delay(1000); // Wait 1 second before looking for new values again - no point thrashing the EEPROM

        /* If the rocket firing switch has been released (HIGH), 
         * reboot (using a watchdog timeout) into normal operation */
        if(buttonPressedDebounced(CALIBRATION_MODE_BUTTON, HIGH)) {
            Serial.println("Rebooting");
            DcsBios::EasyMode::reboot();
        }
    }
    else
    {
        // ====================
        // NORMAL DCS BIOS MODE
        // ====================
        
        DcsBios::EasyMode::loop();
    }
}
