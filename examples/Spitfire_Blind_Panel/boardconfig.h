#ifndef SPITFIRE_BLIND_PANEL_BOARDCONFIG_H
#define SPITFIRE_BLIND_PANEL_BOARDCONFIG_H

/* In this file you can configure the pinnouts based on the board you are using.
 *
 * A NOTE ON BOARD CHOICE:
 * Running Stepper motors smoothly requires a significant amount of cpu time.
 * While this example puts 4 Steppers and 4 Servos on a single board, the Steppers
 * may not keep up with rapidly changing telemetry values. If you find that the 
 * Steppers are jittery or not keeping up, you can break the panel into smaller groups
 * of gauges and run each group on a separate Arduino. (See Below)
 *
 * A NOTE ON SERVO MOTORS
 * Note also that pins used for servo motor control should be PWM-capable pins to 
 * ensure smooth servo movement. PWM (Pulse Width Modulation) is a piece of hardware
 * that controls the pin output independently of the main program loop, allowing for 
 * more precise and consistent control of the servo position. Using a non-PWM pin for 
 * a servo can lead to jittery or unresponsive behavior because the servo relies on 
 * precise timing of the signal to determine its position. The Arduino Servo library can
 * handle a LOT of Servos. Typically 12 per hardware timer with 4 or more per chip.
 * As such there really isn't a practical limit to how many Servos you can put in a
 * single sketch. That's not true for Stepper Motors.
 * 
 * A NOTE ON STEPPER MOTORS
 * Note that pins used for stepper motor control should be ordinary digital 
 * pins (not Analog, PWM, I2C, SPI, or Serial pins) to ensure clean operation.
 * 
 * Unfortunately, unlike Servos using the hardware timers to automate the flipping
 * of the digital pins, there is no such way to have the hardware control the Stepper
 * Motors. The pins need to be flipped in the main program loop, so using pins that
 * have other hardware functions can lead to timing conflicts and erratic behavior 
 * of the stepper motor.
 * Being under cpu control also limits the number of stepper motors you can run because 
 * you may not have enough cpu speed to keep up.
 * Either split your gauges into multiple boards, or use a more powerful Arduino device
 * such as the STM32 or ESP32 which have much faster cpu speeds.
 *
 * A NOTE ON BOARD CHOICES:
 * The example code is set up for Arduino Mega2560 and Arduino Due because they have
 * enough pins to run the entire Spitfire Blind Panel with all gauges and
 * features. However, you can modify the pin assignments to run on smaller boards
 * by prioritizing which gauges and features you want to include and ensuring you
 * have enough pins for those. The pin assignments are ordered by gauge position 
 * on the panel to make it easier to follow and modify as needed.
 * It is recommended that you break a panel of gauges into smaller groups.
 * 
 * Either have one Arduino per gauge for maximum flexibility, or group them by 
 * row (top row of 3 gauges on one Arduino and bottom row of 3 gauges on another 
 * Arduino). USB 2.0 hubs are cheap and can easily be put behind the panel with the
 * USB cable of each Arduino kept short with one long cable used to go to the PC.
 * 
 * The examples for Arduino Uno and Nano are intentionally left out because they 
 * don't have enough pins to run the full panel, but you can use the pin mapping 
 * style in this file to set up a pared-down version of the panel on those boards 
 * if desired. If you want to use one of those boards we recommend you copy this
 * example into two independent sketches. In only only keep the top row and in the 
 * other the bottom row.
 * 
 * If you are using Arduino Mega2560 or Arduino Due (a mostly pin compatible 
 * 3.3v (vs Mega at 5v) version of the Mega2560 board the following code will
 * automatically use the right set of pin definitions because the Arduino IDE 
 * defines the board type in the build process.
 */ 

#if defined(ARDUINO_AVR_MEGA2560)
    /*
    * Mega2560 Pin Availability:
    * - Digital Pins: 0-53
    * - Analog Pins: A0-A15 (usable as digital pins 54-69 if needed)
    * - Notes: Pins 0/1 are Serial RX/TX; avoid for stepper/servos. Pins 2-13 have PWM. Pins 50-53 are SPI.
    */
    // Pin assignments (ordered by gauge position on panel)
    // Air Speed Indicated (ASI)
    #define AIRSPEED_STEPPER_PIN1     2
    #define AIRSPEED_STEPPER_PIN2     3
    #define AIRSPEED_STEPPER_PIN3     4
    #define AIRSPEED_STEPPER_PIN4     5
    #define AIRSPEED_ZERO_PIN         20

    // Artificial Horizon
    #define AHORIZON_BANK_SERVO_PIN   6
    #define AHORIZON_PITCH_SERVO_PIN  7

    // Rate of Climb Indicator
    #define ROCLIMB_STEPPER_PIN1      16
    #define ROCLIMB_STEPPER_PIN2      17
    #define ROCLIMB_STEPPER_PIN3      18
    #define ROCLIMB_STEPPER_PIN4      19
    #define ROCLIMB_ZERO_PIN          21

    // Altimeter
    #define ALTIMETER_STEPPER_PIN1    8
    #define ALTIMETER_STEPPER_PIN2    9
    #define ALTIMETER_STEPPER_PIN3    10
    #define ALTIMETER_STEPPER_PIN4    11
    #define ALTIMETER_ZERO_PIN        21

    // Gyroscopic Directional Indicator (DI)
    #define DI_STEPPER_PIN1           12
    #define DI_STEPPER_PIN2           13
    #define DI_STEPPER_PIN3           14
    #define DI_STEPPER_PIN4           15
    #define DI_ZERO_PIN               22

    // Side Slip and Turn Gauge
    #define SIDESLIP_SERVO_PIN        44
    #define TURN_SERVO_PIN            45

#elif defined(ARDUINO_SAM_DUE)
    /*
    * Arduino Due Pin Availability:
    * - Digital Pins: 0-53
    * - Analog Pins: A0-A11
    * - Notes: Pins 0/1 are Serial RX/TX; pins 20/21 are I2C. Keep the panel on
    *   ordinary digital pins so it works cleanly on either Due USB port choice.
    */
    // Pin assignments (ordered by gauge position on panel)
    // Air Speed Indicated (ASI)
    #define AIRSPEED_STEPPER_PIN1     22
    #define AIRSPEED_STEPPER_PIN2     23
    #define AIRSPEED_STEPPER_PIN3     24
    #define AIRSPEED_STEPPER_PIN4     25
    #define AIRSPEED_ZERO_PIN         26

    // Artificial Horizon
    #define AHORIZON_BANK_SERVO_PIN   6
    #define AHORIZON_PITCH_SERVO_PIN  7

    // Rate of Climb Indicator
    #define ROCLIMB_STEPPER_PIN1      27
    #define ROCLIMB_STEPPER_PIN2      28
    #define ROCLIMB_STEPPER_PIN3      29
    #define ROCLIMB_STEPPER_PIN4      30
    #define ROCLIMB_ZERO_PIN          31

    // Altimeter
    #define ALTIMETER_STEPPER_PIN1    32
    #define ALTIMETER_STEPPER_PIN2    33
    #define ALTIMETER_STEPPER_PIN3    34
    #define ALTIMETER_STEPPER_PIN4    35
    #define ALTIMETER_ZERO_PIN        36

    // Gyroscopic Directional Indicator (DI)
    #define DI_STEPPER_PIN1           37
    #define DI_STEPPER_PIN2           38
    #define DI_STEPPER_PIN3           39
    #define DI_STEPPER_PIN4           40
    #define DI_ZERO_PIN               41

    // Side Slip and Turn Gauge
    #define SIDESLIP_SERVO_PIN        8
    #define TURN_SERVO_PIN            9
#else
    #error "Unsupported board. Add a pin mapping for this FQBN define."
#endif

#endif
