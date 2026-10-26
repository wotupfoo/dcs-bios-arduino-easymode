#include <Arduino.h>
#define __DCSBIOS_EASY_MODE_WATCHDOG_CPP
#include "DcsBiosEasyWatchdog.h"

// Watchdog Timer/Reboot mechanism for each architecture
// DO NOT put in the DcsBios or DcsBios::EasyMode namespace
#if defined(__AVR__)                        // nano, uno, mega, leonardo etc
// Nothing
#elif defined(ARDUINO_ARCH_SAM)             // Arduino Due
// Override the core's weak startup hook. Do not consume the watchdog mode
// register's single write before EasyMode::reboot_setup() configures it.
void watchdogSetup() {}

#elif defined(ARDUINO_GENERIC_STM32F103C)   // Bluepill STM32F10C
// Nothing
#else
#error "Unsupported board - Please use an Arduino Nano, Leonardo, Uno or Due or implement your own"
#endif
