#ifndef __DCSBIOS_EASY_MODE_H
#define __DCSBIOS_EASY_MODE_H

#include <Arduino.h>

#if defined(__AVR__)
#include <avr/wdt.h>
#endif

#include <AccelStepper.h>
#include <DcsBios.h>

#include "internal/DcsBiosEasyCommon.h"
#include "internal/DcsBiosEasyEEPROM.h"
#include "internal/DcsBiosEasyInputs.h"
#include "internal/DcsBiosEasyServos.h"
#include "internal/DcsBiosEasySteppers.h"

namespace DcsBios {
namespace EasyMode {

// Use -1 for optional pins that are not connected, such as an unused zero switch.
// The constructors accept this and map it to the library's internal "no pin" value.
static constexpr int NoPin = -1;

inline void refreshInterval(unsigned long intervalMs) {
    DcsBios::setEasyModeRefreshInterval(intervalMs);
}

inline unsigned long getRefreshInterval() {
    return DcsBios::getEasyModeRefreshInterval();
}

inline bool tryToSendDcsBiosMessage(const char* msg, const char* arg) {
    return DcsBios::tryToSendDcsBiosMessage(msg, arg);
}

inline bool sendDcsBiosMessage(const char* msg, const char* arg) {
    return DcsBios::sendDcsBiosMessage(msg, arg);
}

inline void resetAllStates() {
    DcsBios::resetAllStates();
}

// ==============================
// Calibration helpers
// ==============================
inline bool loadCalibration(int eepromAddress = DcsBios::EASYMODE_EEPROM_DEFAULT_ADDR) {
    return DcsBios::loadEasyModeCalibrationFromEEPROM(eepromAddress);
}

inline bool updateCalibration() {
    return DcsBios::updateEasyModeCalibration();
}

inline void saveCalibration(int eepromAddress = DcsBios::EASYMODE_EEPROM_DEFAULT_ADDR) {
    DcsBios::saveEasyModeCalibrationToEEPROM(eepromAddress);
}

inline bool saveCalibrationIfChanged(int eepromAddress = DcsBios::EASYMODE_EEPROM_DEFAULT_ADDR) {
    return DcsBios::saveEasyModeCalibrationToEEPROMIfChanged(eepromAddress);
}

inline bool calibrationIsValid() {
    return DcsBios::easyModeCalibrationIsValid();
}

// ==============================
// Reboot helpers
inline void reboot_disable() {
#if defined(__AVR__)
    wdt_disable();
#endif
}

inline void reboot() {
    Serial.println("Rebooting");
#if defined(__AVR__)
    wdt_enable(WDTO_15MS);
    while (true) {}
#endif
}

// ==============================
// Called by sketch in setup() in Calibration Mode
inline void setupCalibration() {
    Serial.begin(250000);
    Serial.println("Entering Calibration Mode");
    DcsBios::beginEasyModeCalibration();
}

// ==============================
// called in sketch setup() in Normal Mode
inline void setup(int eepromAddress = DcsBios::EASYMODE_EEPROM_DEFAULT_ADDR) {
    DcsBios::EasyMode::loadCalibration(eepromAddress);
    DcsBios::setup();
}

// ==============================
// Called by sketch in loop() in Calibration Mode
inline bool loopCalibration(Print& out, int eepromAddress = DcsBios::EASYMODE_EEPROM_DEFAULT_ADDR) {
    return DcsBios::serviceEasyModeCalibration(out, eepromAddress);
}

// ==============================
// called in sketch loop() in Normal Mode
inline void loop() {
    DcsBios::serviceEasyModeRefreshes();
    DcsBios::loop();
}

// Only maintained-state inputs participate in periodic refreshes.
// Momentary buttons and relative encoders are intentionally left as plain aliases.
using ActionButton = DcsBios::ActionButton;
using ToggleButton = DcsBios::ToggleButton;
using MatActionButton = DcsBios::MatActionButton;
using MatActionButtonToggle = DcsBios::MatActionButtonToggle;
using MatActionButtonSet = DcsBios::MatActionButtonSet;
using DualModeButton = DcsBios::DualModeButton;

using Switch2Pos = EasyModeRefreshableInputT<DcsBios::Switch2Pos>;
using Switch3Pos = EasyModeRefreshableInputT<DcsBios::Switch3Pos>;
using SwitchMultiPos = EasyModeRefreshableInputT<DcsBios::SwitchMultiPos>;
using SwitchWithCover2Pos = EasyModeRefreshableInputT<DcsBios::SwitchWithCover2Pos>;
#if defined(USE_MATRIX_SWITCHES) || defined(DCSBIOS_USE_MATRIX_SWITCHES)
using Matrix2Pos = EasyModeRefreshableInputT<DcsBios::Matrix2Pos>;
using Matrix3Pos = EasyModeRefreshableInputT<DcsBios::Matrix3Pos>;
using MatSwitch2Pos = EasyModeRefreshableInputT<DcsBios::MatSwitch2Pos>;
using MatSwitch3Pos = EasyModeRefreshableInputT<DcsBios::MatSwitch3Pos>;
#endif

using AnalogMultiPos = EasyModeRefreshableInputT<EasyModeAnalogMultiPos>;
using AnalogSyncingRocker = EasyModeRefreshableInputT<EasyModeAnalogSyncingRocker>;
using RotarySwitch = EasyModeRefreshableInputT<DcsBios::RotarySwitch>;
using Potentiometer = EasyModeRefreshableInputT<EasyModePotentiometer>;
using RotaryEncoder = DcsBios::RotaryEncoder;
using RotaryAcceleratedEncoder = DcsBios::RotaryAcceleratedEncoder;
using MatRotaryEncoder = DcsBios::MatRotaryEncoder;
using EmulatedConcentricRotaryEncoder = DcsBios::EmulatedConcentricRotaryEncoder;
using RotarySyncingPotentiometer = EasyModeRefreshableInputT<EasyModeRotarySyncingPotentiometer>;
using InvertedRotarySyncingPotentiometer = EasyModeRefreshableInputT<EasyModeInvertedRotarySyncingPotentiometer>;
using BcdWheel = EasyModeRefreshableInputT<DcsBios::BcdWheel>;
using RadioPreset = EasyModeRefreshableInputT<DcsBios::RadioPreset>;

using IntegerBuffer = DcsBios::IntegerBuffer;
template <unsigned int LENGTH>
using StringBuffer = DcsBios::StringBuffer<LENGTH>;
using LED = DcsBios::LED;
using Dimmer = DcsBios::Dimmer;

using Servo = EasyServo;
using Servo_SG90 = EasyServo_SG90;
using ServoOutput = DcsBios::ServoOutput;

using Stepper = EasyStepper;
using Stepper_Bounded = EasyStepper_Bounded;
using Stepper_Continuous = EasyStepper_Continuous;
using Stepper_Manual = EasyStepper_Manual<GenericStepperProfile>;
using Stepper_28BYJ48 = EasyStepper_28BYJ48;
using Stepper_28BYJ48_Bounded = EasyStepper_28BYJ48_Bounded;
using Stepper_28BYJ48_Continuous = EasyStepper_28BYJ48_Continuous;
using Stepper_Manual_28BYJ48 = EasyStepper_Manual<Stepper28Byj48Profile>;

} // namespace EasyMode
} // namespace DcsBios

#endif
