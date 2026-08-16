#define DCSBIOS_DEFAULT_SERIAL
//#define DCSBIOS_IRQ_SERIAL
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
    *           PIN_RKT_FIRING_SW 3 --|           |-- A6
    *           PIN_SUPERCHARGER  4 --|   NANO    |-- A5
    *                             5 --|           |-- A4 PIN_MIXTURE
    *                             6 --|           |-- A3 PIN_THROTTLE_PROP_CONTROL_R
    *                             7 --|           |-- A2 PIN_THROTTLE_PROP_CONTROL_L
    *                             8 --|           |-- A1 PIN_THROTTLE_CONTROL_R
    *                             9 --|           |-- A0 PIN_THROTTLE_CONTROL_L
    *                            10 --|           |-- REF
    *                            11 --|  +-----+  |-- 3V3
    *                            12 --|  | USB |  |-- 13
    *                                 +--+_____+--+
    *
    */
// Analog inputs
#define PIN_THROTTLE_CONTROL_L          A0
#define PIN_THROTTLE_CONTROL_R          A1
#define PIN_THROTTLE_PROP_CONTROL_L     A2
#define PIN_THROTTLE_PROP_CONTROL_R     A3
#define PIN_MIXTURE                     A4

// Digital inputs
#define PIN_RKT_FIRING_SW               3
#define PIN_SUPERCHARGER                4

#else
#error "Unsupported board - Please use an Arduino Nano or implement your own"
#endif

DcsBios::EasyMode::Potentiometer throttleControlL("THROTTLE_CONTROL_L", 
                                                PIN_THROTTLE_CONTROL_L,
                                                false,
                                                100,    //min
                                                900);   //max
DcsBios::EasyMode::Potentiometer throttleControlR("THROTTLE_CONTROL_R", 
                                                PIN_THROTTLE_CONTROL_R,
                                                false,
                                                100,    //min
                                                900);   //max
DcsBios::EasyMode::Potentiometer throttlePropControlL("THROTTLE_CONTROL_PROP_L", 
                                                PIN_THROTTLE_PROP_CONTROL_L,
                                                false,
                                                100,    //min
                                                900);   //max
DcsBios::EasyMode::Potentiometer throttlePropControlR("THROTTLE_CONTROL_PROP_R", 
                                                PIN_THROTTLE_PROP_CONTROL_R,
                                                false,
                                                100,    //min
                                                900);   //max

DcsBios::EasyMode::AnalogMultiPos Mixture("MIXTURE", 
                                        PIN_MIXTURE,
                                        1);

DcsBios::EasyMode::Switch2Pos RocketFiring("RKT_FIRING_SW", 
                                        PIN_RKT_FIRING_SW);
DcsBios::EasyMode::Switch2Pos Supercharger("SUPERCHARGER", 
                                        PIN_SUPERCHARGER);

void setup() {
    DcsBios::EasyMode::setup();

    // Send the state of the hardware every 2 seconds
    DcsBios::EasyMode::refreshInterval(2000);
    throttleControlL.refresh(true);
    throttleControlR.refresh(true);
    throttlePropControlL.refresh(true);
    throttlePropControlR.refresh(true);

    Mixture.refresh(true);

    RocketFiring.refresh(true);
    Supercharger.refresh(true);
}

struct input_pins {
    unsigned int throttleL;
    unsigned int throttleR;
    unsigned int propL;
    unsigned int propR;
    unsigned int mix;
    bool         rocketfiring;
    bool         supercharger;
} pins;

void loop() {
    static char buffer[150];
//    DcsBios::EasyMode::loop();
    pins.throttleL = analogRead(PIN_THROTTLE_CONTROL_L);
    pins.throttleR = analogRead(PIN_THROTTLE_CONTROL_R);
    pins.propL = analogRead(PIN_THROTTLE_PROP_CONTROL_L);
    pins.propR = analogRead(PIN_THROTTLE_PROP_CONTROL_R);
    pins.mix = analogRead(PIN_MIXTURE);
    pins.rocketfiring = (bool)digitalRead(PIN_RKT_FIRING_SW);
    pins.supercharger = (bool)digitalRead(PIN_SUPERCHARGER);
    snprintf(buffer,
            150,
            "Throttle %04u,%04u Prop %04u,%04u Mix %04u RocketFiring %d SuperCharger %d\r\n",
            pins.throttleL,
            pins.throttleR,
            pins.propL,
            pins.propR,
            pins.mix,
            pins.rocketfiring,
            pins.supercharger);
    Serial.print(buffer);
}
