#ifndef __DCSBIOS_EASY_MODE_WATCHDOG_H
#define __DCSBIOS_EASY_MODE_WATCHDOG_H

#if !defined(__DCSBIOS_EASY_MODE_H) && !defined(__DCSBIOS_EASY_MODE_WATCHDOG_CPP)
#error Do not call DcsBiosEasyWatchdog.h directly (except DcsBiosEasyWatchdog.cpp). Include DcsBiosEasyMode.h instead.
#endif

// Watchdog Timer/Reboot mechanism for each architecture
// DO NOT put in the DcsBios or DcsBios::EasyMode namespace
#if defined(__AVR__)                        // nano, uno, mega, leonardo etc
#include <avr/wdt.h>
#elif defined(ARDUINO_ARCH_SAM)             // Arduino Due
#include <watchdog.h>
#elif defined(ARDUINO_GENERIC_STM32F103C)   // Bluepill STM32F10C
#include <libmaple/iwdg.h>
#endif

#endif
