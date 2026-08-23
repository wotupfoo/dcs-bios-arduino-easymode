#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBiosEasyMode.h>
/*
 * Mosquito Throttle Quadrant.
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
    *                             2 --|           |-- A7
    *            PIN_SUPERCHARGER 3 --|           |-- A6
    *           PIN_RKT_FIRING_SW 4 --|   NANO    |-- A5
    *                             5 --|           |-- A4 PIN_MIXTURE
    *                             6 --|           |-- A3 PIN_THROTTLE_CONTROL_L
    *                             7 --|           |-- A2 PIN_THROTTLE_CONTROL_R
    *                             8 --|           |-- A1 PIN_THROTTLE_PROP_CONTROL_L
    *                             9 --|           |-- A0 PIN_THROTTLE_PROP_CONTROL_R
    *                            10 --|           |-- REF┐
    *                            11 --|  +-----+  |-- 3V3┘ Use 3.3v as ADC REF
    *                            12 --|  | USB |  |-- 13
    *                                 +--+_____+--+
    *
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
//#define CALIBRATION_MODE_BUTTON       5    // Dedicated pin
#define CALIBRATION_MODE_BUTTON         PIN_RKT_FIRING_SW // Use the Rocket Button

/*
// Set the Nano ADC to use the clean 3.3v as the reference before globals.
// It has to be here in a global otherwise the other global won't have it
// configured before adcRead() is called in the class constructor.

struct EXTERNAL_AREF_AS_GLOBAL_VAR {
  External_Aref_Before_Globals() { analogReference(EXTERNAL); }
} External_Aref_Before_Globals;
*/

#else
#error "Unsupported board - Please use an Arduino Nano or implement your own"
#endif

// ======================================================================
// EEPROM holds each input's Minimum and Maximum value between [0...1023]
#include <EEPROM.h>
typedef struct {
    uint16_t l;
    uint16_t h;
} LO_HI;

typedef struct {
    LO_HI throttleL;
    LO_HI throttleR;
    LO_HI propL;
    LO_HI propR;
    LO_HI mix;
} QUADRANT_LO_HI;

// MANDATORY DEFINITION OF THE EEPROM LAYOUT, Address, Magic Number and Version
const int EEPROM_ADDR = 0;
const uint16_t EEPROM_MAGIC = 0xD511;
const uint8_t EEPROM_VERSION = 1;

struct EEPROM_BLOCK {
    uint16_t magic;         // MANDATORY FIELD
    uint8_t  version;       // MANDATORY FIELD
    QUADRANT_LO_HI lh;      // USER DEFINED
} eeprom;

// Allowable Min/Max is more than 256 value separation (512 +/- 64) either 
//                      ---VALID RANGE---
// side of 512 [0..448] [512-64...512+64] [576..1023]
// Min can be in the range [0..448]
// Max can be in the range [576..1023]
static const uint16_t  ADC_SPAN = 1024;  // 10bit ADC on Nano
static const uint16_t  ADC_MID = (ADC_SPAN>>1); // Midpoint
static const uint16_t  MIN_VALID_ADC_SPAN = 64; // +/- from ADC_MID

// CALIBRATION STUFF
void resetValues(LO_HI& lh) {
    lh.l = ADC_MID - MIN_VALID_ADC_SPAN;
    lh.h = ADC_MID + MIN_VALID_ADC_SPAN;
}

void resetBlock() {
    resetValues(eeprom.lh.throttleL);
    resetValues(eeprom.lh.throttleR);
    resetValues(eeprom.lh.propL);
    resetValues(eeprom.lh.propR);
    resetValues(eeprom.lh.mix);
}

bool doUpdateEEPROM = false;        // Update the EEPROM because values changed
void updateMinMax(LO_HI& lh,uint16_t raw) {
    if (raw < lh.l) {
        lh.l = raw;
        doUpdateEEPROM = true;
    }
    if (raw > lh.h) {
        lh.h = raw;
        doUpdateEEPROM = true;
    }
}
// ======================================================================

// ======================================================================
// DCS-BIOS Input Devices
DcsBios::EasyMode::Potentiometer throttleControlL("THROTTLE_CONTROL_L",
                                        PIN_THROTTLE_CONTROL_L,
                                        true,   // reverse
                                        0,      // adc min
                                        1023,   // adc max
                                        3);     // adc hysterisis
DcsBios::EasyMode::Potentiometer throttleControlR("THROTTLE_CONTROL_R",
                                        PIN_THROTTLE_CONTROL_R,
                                        false,  // reverse
                                        0,      // adc min
                                        1023,   // adc max
                                        3);     // adc hysterisis
DcsBios::EasyMode::Potentiometer propControlL("PROP_CONTROL_L",
                                        PIN_THROTTLE_PROP_CONTROL_L,
                                        true,   // reverse
                                        0,      // adc min
                                        1023,   // adc max
                                        3);     // adc hysterisis
DcsBios::EasyMode::Potentiometer propControlR("PROP_CONTROL_R",
                                        PIN_THROTTLE_PROP_CONTROL_R,
                                        false,  // reverse
                                        0,      // adc min
                                        1023,   // adc max
                                        3);     // adc hysterisis

DcsBios::EasyMode::AnalogMultiPos mixture("MIXTURE",
                                        PIN_MIXTURE,
                                        1,      // positions (0,1)
                                        0,      // adc min
                                        1023,   // adc max
                                        3);     // adc hysterisis

DcsBios::EasyMode::Switch2Pos rocketFiring("RKT_FIRING_SW",
                                        PIN_RKT_FIRING_SW,
                                        false);  // reverse
DcsBios::EasyMode::Switch2Pos superCharger("SUPERCHARGER",
                                        PIN_SUPERCHARGER,
                                        true);  // reverse

// ======================================================================
// Current ADC input minimum and maximum ranges
struct MOSQUITO_THROTTLE_QUADRANT_INPUTS {
    uint16_t throttleL;
    uint16_t throttleR;
    uint16_t propL;
    uint16_t propR;
    uint16_t mix;
    bool     mixb;
    bool     rocket_fire;
    bool     supercharger;
} quadrant;

// ======================================================================
// HELPER ROUTINES
void applyCalibration() {
    // Apply the EEPROM values to each of the input readers
    throttleControlL.setMin(eeprom.lh.throttleL.l);   throttleControlL.setMax(eeprom.lh.throttleL.h);
    throttleControlR.setMin(eeprom.lh.throttleR.l);   throttleControlR.setMax(eeprom.lh.throttleR.h);
    propControlL.setMin(eeprom.lh.propL.l);           propControlL.setMax(eeprom.lh.propL.h);
    propControlR.setMin(eeprom.lh.propR.l);           propControlR.setMax(eeprom.lh.propR.h);
    mixture.setMin(eeprom.lh.mix.l);                  mixture.setMax(eeprom.lh.mix.h);
}

// Debounced button helper
bool buttonPressedDebounced(byte pin, bool level) {
    bool i,j;
    i = digitalRead(pin);
    delay(25);
    j = digitalRead(pin);
    return (i == level && j == level);
}

// Way to reboot the board to exit Calibration mode
// Make sure the WatchDog Time is disabled as soon as possible in setup()
#include <avr/wdt.h>
void reboot() {
  wdt_enable(WDTO_15MS);
  while (true);
}

// A flag to run in Calibration Mode or Normal Mode
bool calibrationMode;
static char buf[150];   // Current string length is 140+1

// ======================================================================
// SETUP
// ======================================================================
void setup() {
    // Disable the Watchdog Timer (used to reboot out of Calibration mode)
    wdt_disable();
    Serial.begin(250000);

    pinMode(LED_BUILTIN, OUTPUT);   // Used to show operating mode

    // Get the Min and Max input values for each ADC from EEPROM storage
    EEPROM.get(EEPROM_ADDR, eeprom);    // .get method understands the data type size

    // Populate the values if they don't exist in the EEPROM
    if(eeprom.magic != EEPROM_MAGIC || eeprom.version != EEPROM_VERSION) {
        resetBlock();
        eeprom.magic = EEPROM_MAGIC;
        eeprom.version = EEPROM_VERSION;
        EEPROM.put(EEPROM_ADDR, eeprom);
    }
    // Load EEPROM values into the Input Readers
    applyCalibration(); 

    // Wait a second before looking at the fire button to see
    // if we are to go into Calibration Mode (vs Normal DCS-BIOS mode)
    delay(1000);    
    pinMode(CALIBRATION_MODE_BUTTON, INPUT_PULLUP);

    // If the Rocket Firing button is pressed during bootup go into
    // Calibration Mode to set the ranges of each analog input
    // Particularly important for Hall Effect sensors since they are 
    // no where near the full 0.0...3.3v analog range ([0..1023])
    calibrationMode = digitalRead(CALIBRATION_MODE_BUTTON) == LOW;

    if(calibrationMode) {
        // CALIBRATION MODE
        digitalWrite(LED_BUILTIN, HIGH);

        Serial.println("Resetting EEPROM to defaults");
        eeprom.magic = EEPROM_MAGIC;
        eeprom.version = EEPROM_VERSION;
        resetBlock();
        Serial.println("Writing EEPROM");
        EEPROM.put(EEPROM_ADDR, eeprom);

        Serial.println("Entering Calibration Mode");
    }
    else {
        // NORMAL DCS BIOS MODE
        digitalWrite(LED_BUILTIN, LOW);
        DcsBios::EasyMode::setup();

        // Update and Send the state of the hardware every 5 seconds
        // Changes to outputs will be sent immediately
        DcsBios::EasyMode::refreshInterval(5000);
/*
        throttleControlL.refresh(true);
        throttleControlR.refresh(true);
        propControlL.refresh(true);
        propControlR.refresh(true);
*/
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

        // Process the analog stuff
        quadrant.throttleL = throttleControlL.getRawValue();
        quadrant.throttleR = throttleControlR.getRawValue();
        quadrant.propL = propControlL.getRawValue();
        quadrant.propR = propControlR.getRawValue();
        quadrant.mix = mixture.getRawValue();

        // Process the digital stuff
        uint16_t mix_span = eeprom.lh.mix.h - eeprom.lh.mix.l;
        uint16_t mix_mid = eeprom.lh.mix.l + mix_span/2;
        quadrant.mixb = quadrant.mix > mix_mid;

        quadrant.rocket_fire = digitalRead(PIN_RKT_FIRING_SW) == LOW;
        quadrant.supercharger = digitalRead(PIN_SUPERCHARGER) == LOW;

        // Print what we have before changing it so that there is an output of the EEPROM state
        snprintf(buf,
                sizeof(buf),
                "Throttle %04u|%04u|%04u  %04u|%04u|%04u  Prop %04u|%04u|%04u  %04u|%04u|%04u  Mix %04u|%04u|%04u %d  RocketFiring %d  SuperCharger %d  Update %d",
                eeprom.lh.throttleL.l,   quadrant.throttleL, eeprom.lh.throttleL.h,
                eeprom.lh.throttleR.l,   quadrant.throttleR, eeprom.lh.throttleR.h,
                eeprom.lh.propL.l,       quadrant.propL,     eeprom.lh.propL.h,
                eeprom.lh.propR.l,       quadrant.propR,     eeprom.lh.propR.h,
                eeprom.lh.mix.l,         quadrant.mix,       eeprom.lh.mix.h,     quadrant.mixb,
                quadrant.rocket_fire,
                quadrant.supercharger,
                doUpdateEEPROM
        );
        Serial.println(buf);

        // Push the new Min/Max values to the EEPROM storage
        // These will set doUpdateEEPROM if any values were changed
        updateMinMax(eeprom.lh.throttleL, quadrant.throttleL);
        updateMinMax(eeprom.lh.throttleR, quadrant.throttleR);
        updateMinMax(eeprom.lh.propL,     quadrant.propL);
        updateMinMax(eeprom.lh.propR,     quadrant.propR);
        updateMinMax(eeprom.lh.mix,       quadrant.mix);

        // Push the updated EEPROM Min/Max values out to the Input Readers
        applyCalibration();

        // Decide to stop and/or write the updated value to EEPROM
        if(doUpdateEEPROM) {
            eeprom.magic = EEPROM_MAGIC;
            eeprom.version = EEPROM_VERSION;
            Serial.println("Writing EEPROM");
            EEPROM.put(EEPROM_ADDR, eeprom);
            doUpdateEEPROM = false;
        }
        delay(1000); // Wait 1 second before looking for new values again - no point thrashing the EEPROM

        // If the rocket firing switch has been released (HIGH), reboot (using a watchdog timeout) into normal operation
        if(buttonPressedDebounced(CALIBRATION_MODE_BUTTON, HIGH)) {
            Serial.println("Rebooting");
            reboot();
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
