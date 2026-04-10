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
 * On boards like the Arduino Mega2560, the standard Arduino Servo library uses
 * hardware timers to schedule the pulse timing, then toggles the chosen digital
 * output pin in software. That means servo outputs do not need to be on hardware
 * PWM pins for this example. The Arduino Servo library can handle a LOT of Servos.
 * Typically 12 per hardware timer with 4 or more timers per chip. As such there
 * really isn't a practical limit to how many Servos you can put in a single sketch.
 * That's not true for Stepper Motors.
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
 * enough pins to run the entire RAF Mosquito/Spitfire Blind Panel with all gauges and
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
    * - Notes: Pins 0/1 are Serial RX/TX; pins 20/21 are I2C; pins 50-53 are SPI.
    *   This example intentionally keeps the whole panel on D22-D45, the dual-row
    *   header at the end of the board.
    *
    * Mega2560 pin usage for this panel:
    * Rotated CCW 90 degrees so the D22-D45 header is on the right.
    *
    *                           +== TOP SIDE HEADER ====================+||
    *                       +-----+                                      ||-- 45 TURN_SERVO
    *                       | USB |                                      ||-- 44 SIDESLIP_SERVO
    *                       +-----+                                      ||-- 43 DI_ZERO
    *                           |                                        ||-- 42 DI_STEPPER_4
    *                           |                                        ||-- 41 DI_STEPPER_3
    *                           |                                        ||-- 40 DI_STEPPER_2
    *                           |            ARDUINO MEGA2560            ||-- 39 DI_STEPPER_1
    *                           |                                        ||-- 38 ALTIMETER_ZERO
    *                           |                                        ||-- 37 ALTIMETER_STEPPER_4
    *                           |                                        ||-- 36 ALTIMETER_STEPPER_3
    *                           |                                        ||-- 35 ALTIMETER_STEPPER_2
    *                           |                                        ||-- 34 ALTIMETER_STEPPER_1
    *                           |                                        ||-- 33 ROCLIMB_ZERO
    *                           |                                        ||-- 32 ROCLIMB_STEPPER_4
    *                           |                                        ||-- 31 ROCLIMB_STEPPER_3
    *                           |                                        ||-- 30 ROCLIMB_STEPPER_2
    *                           |                                        ||-- 29 ROCLIMB_STEPPER_1
    *                           |                                        ||-- 28 AHORIZON_PITCH_SERVO
    *                           |                                        ||-- 27 AHORIZON_BANK_SERVO
    *                           |                                        ||-- 26 AIRSPEED_ZERO
    *                           |                                        ||-- 25 AIRSPEED_STEPPER_4
    *                           |                                        ||-- 24 AIRSPEED_STEPPER_3
    *                           |                                        ||-- 23 AIRSPEED_STEPPER_2
    *                           |                                        ||-- 22 AIRSPEED_STEPPER_1
    *                           +== BOTTOM SIDE HEADER =================+||
    */
    // Pin assignments (ordered by gauge position on panel)
    // Air Speed Indicated (ASI)
    #define AIRSPEED_STEPPER_PIN1     22
    #define AIRSPEED_STEPPER_PIN2     23
    #define AIRSPEED_STEPPER_PIN3     24
    #define AIRSPEED_STEPPER_PIN4     25
    #define AIRSPEED_ZERO_PIN         26

    // Artificial Horizon
    #define AHORIZON_BANK_SERVO_PIN   27
    #define AHORIZON_PITCH_SERVO_PIN  28

    // Rate of Climb Indicator
    #define ROCLIMB_STEPPER_PIN1      29
    #define ROCLIMB_STEPPER_PIN2      30
    #define ROCLIMB_STEPPER_PIN3      31
    #define ROCLIMB_STEPPER_PIN4      32
    #define ROCLIMB_ZERO_PIN          33

    // Altimeter
    #define ALTIMETER_STEPPER_PIN1    34
    #define ALTIMETER_STEPPER_PIN2    35
    #define ALTIMETER_STEPPER_PIN3    36
    #define ALTIMETER_STEPPER_PIN4    37
    #define ALTIMETER_ZERO_PIN        38

    // Gyroscopic Directional Indicator (DI)
    #define DI_STEPPER_PIN1           39
    #define DI_STEPPER_PIN2           40
    #define DI_STEPPER_PIN3           41
    #define DI_STEPPER_PIN4           42
    #define DI_ZERO_PIN               43

    // Side Slip and Turn Gauge
    #define SIDESLIP_SERVO_PIN        44
    #define TURN_SERVO_PIN            45

#elif defined(ARDUINO_SAM_DUE)
    /*
    * Arduino Due Pin Availability:
    * - Digital Pins: 0-53
    * - Analog Pins: A0-A11
    * - Notes: Pins 0/1 are Serial RX/TX; pins 20/21 are I2C.
    * - The Due is largely Mega2560-pin-compatible mechanically, but uses 3.3V logic.
    *   Do not connect 5V-only signals directly to Due GPIO pins.
    * - This example intentionally keeps the whole panel on D22-D45, the dual-row
    *   header at the end of the board.
    *
    * Due pin usage for this panel:
    * Rotated CCW 90 degrees so the D22-D45 header is on the right.
    *
    *                              +== TOP SIDE HEADER ====================+||
    *                          +-----+                                      ||-- 45 TURN_SERVO
    *                Native----| USB |                                      ||-- 44 SIDESLIP_SERVO
    *                          +-----+                                      ||-- 43 DI_ZERO
    *                              |                                        ||-- 42 DI_STEPPER_4
    *                              |                                        ||-- 41 DI_STEPPER_3
    *                          +-----+                                      ||-- 40 DI_STEPPER_2
    *           Programming----| USB |           ARDUINO DUE 3.3V           ||-- 39 DI_STEPPER_1
    *                          +-----+                                      ||-- 38 ALTIMETER_ZERO
    *                              |                                        ||-- 37 ALTIMETER_STEPPER_4
    *                              |                                        ||-- 36 ALTIMETER_STEPPER_3
    *                              |                                        ||-- 35 ALTIMETER_STEPPER_2
    *                              |                                        ||-- 34 ALTIMETER_STEPPER_1
    *                              |                                        ||-- 33 ROCLIMB_ZERO
    *                              |                                        ||-- 32 ROCLIMB_STEPPER_4
    *                              |                                        ||-- 31 ROCLIMB_STEPPER_3
    *                              |                                        ||-- 30 ROCLIMB_STEPPER_2
    *                              |                                        ||-- 29 ROCLIMB_STEPPER_1
    *                              |                                        ||-- 28 AHORIZON_PITCH_SERVO
    *                              |                                        ||-- 27 AHORIZON_BANK_SERVO
    *                              |                                        ||-- 26 AIRSPEED_ZERO
    *                              |                                        ||-- 25 AIRSPEED_STEPPER_4
    *                              |                                        ||-- 24 AIRSPEED_STEPPER_3
    *                              |                                        ||-- 23 AIRSPEED_STEPPER_2
    *                              |                                        ||-- 22 AIRSPEED_STEPPER_1
    *                              +== BOTTOM SIDE HEADER =================+||
    */
    // Pin assignments (ordered by gauge position on panel)
    // Air Speed Indicated (ASI)
    #define AIRSPEED_STEPPER_PIN1     22
    #define AIRSPEED_STEPPER_PIN2     23
    #define AIRSPEED_STEPPER_PIN3     24
    #define AIRSPEED_STEPPER_PIN4     25
    #define AIRSPEED_ZERO_PIN         26

    // Artificial Horizon
    #define AHORIZON_BANK_SERVO_PIN   27
    #define AHORIZON_PITCH_SERVO_PIN  28

    // Rate of Climb Indicator
    #define ROCLIMB_STEPPER_PIN1      29
    #define ROCLIMB_STEPPER_PIN2      30
    #define ROCLIMB_STEPPER_PIN3      31
    #define ROCLIMB_STEPPER_PIN4      32
    #define ROCLIMB_ZERO_PIN          33

    // Altimeter
    #define ALTIMETER_STEPPER_PIN1    34
    #define ALTIMETER_STEPPER_PIN2    35
    #define ALTIMETER_STEPPER_PIN3    36
    #define ALTIMETER_STEPPER_PIN4    37
    #define ALTIMETER_ZERO_PIN        38

    // Gyroscopic Directional Indicator (DI)
    #define DI_STEPPER_PIN1           39
    #define DI_STEPPER_PIN2           40
    #define DI_STEPPER_PIN3           41
    #define DI_STEPPER_PIN4           42
    #define DI_ZERO_PIN               43

    // Side Slip and Turn Gauge
    #define SIDESLIP_SERVO_PIN        44
    #define TURN_SERVO_PIN            45
#else
    #error "Unsupported board. Add a pin mapping for this FQBN define."
#endif

#endif
