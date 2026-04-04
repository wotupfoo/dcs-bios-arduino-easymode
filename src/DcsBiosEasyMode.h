#ifndef __DCSBIOS_EASY_MODE_H
#define __DCSBIOS_EASY_MODE_H

#include <Arduino.h>

#include <AccelStepper.h>
#include <DcsBios.h>

#include "internal/DcsBiosEasyCommon.h"
#include "internal/DcsBiosEasyInputs.h"
#include "internal/DcsBiosEasyServos.h"
#include "internal/DcsBiosEasySteppers.h"

namespace DcsBios {
namespace EasyMode {

inline void setup() {
    DcsBios::setup();
}

inline void loop() {
    DcsBios::loop();
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

using ActionButton = DcsBios::ActionButton;
using ToggleButton = DcsBios::ToggleButton;
using MatActionButton = DcsBios::MatActionButton;
using MatActionButtonToggle = DcsBios::MatActionButtonToggle;
using MatActionButtonSet = DcsBios::MatActionButtonSet;
using DualModeButton = DcsBios::DualModeButton;

using Switch2Pos = EasyModeSwitch2Pos;
using Switch3Pos = EasyModeSwitch3Pos;
using SwitchMultiPos = EasyModeSwitchMultiPos;
using SwitchWithCover2Pos = DcsBios::SwitchWithCover2Pos;
#if defined(USE_MATRIX_SWITCHES) || defined(DCSBIOS_USE_MATRIX_SWITCHES)
using Matrix2Pos = DcsBios::Matrix2Pos;
using Matrix3Pos = DcsBios::Matrix3Pos;
using MatSwitch2Pos = DcsBios::MatSwitch2Pos;
using MatSwitch3Pos = DcsBios::MatSwitch3Pos;
#endif

using AnalogMultiPos = EasyModeAnalogMultiPos;
using RotarySwitch = EasyModeRotarySwitch<>;
using Potentiometer = DcsBios::Potentiometer;
using RotaryEncoder = DcsBios::RotaryEncoder;
using RotaryAcceleratedEncoder = DcsBios::RotaryAcceleratedEncoder;
using MatRotaryEncoder = DcsBios::MatRotaryEncoder;
using EmulatedConcentricRotaryEncoder = DcsBios::EmulatedConcentricRotaryEncoder;
using RotarySyncingPotentiometer = EasyModeRotarySyncingPotentiometer;
using InvertedRotarySyncingPotentiometer = EasyModeInvertedRotarySyncingPotentiometer;
using BcdWheel = DcsBios::BcdWheel;
using RadioPreset = DcsBios::RadioPreset;

using IntegerBuffer = DcsBios::IntegerBuffer;
template <unsigned int LENGTH>
using StringBuffer = DcsBios::StringBuffer<LENGTH>;
using LED = DcsBios::LED;
using Dimmer = DcsBios::Dimmer;

using ServoSG90 = EasyServo_SG90;
using ServoOutput = DcsBios::ServoOutput;

using Stepper_Bounded = EasyStepper_Bounded;
using Stepper_Continuous = EasyStepper_Continuous;
using Stepper_28BYJ48_Bounded = EasyStepper_28BYJ48_Bounded;
using Stepper_28BYJ48_Continuous = EasyStepper_28BYJ48_Continuous;
using Stepper_Manual_28BYJ48 = EasyStepper_Manual_28BYJ48;
using StepperOutput = EasyStepperOutput;
using Stepper28Byj48Output = Easy28Byj48Output;

} // namespace EasyMode
} // namespace DcsBios

#endif
