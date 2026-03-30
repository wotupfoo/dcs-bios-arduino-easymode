#ifndef __DCSBIOS_EASY_MODE_H
#define __DCSBIOS_EASY_MODE_H

#include <Arduino.h>

#ifdef ARDUINO_ARCH_ESP32
#include <ESP32Servo.h>
#else
#include <Servo.h>
#endif

#include <AccelStepper.h>
#include <DcsBios.h>

#include "internal/DcsBiosEasyCommon.h"
#include "internal/DcsBiosEasyServos.h"
#include "internal/DcsBiosEasySteppers.h"

#endif
