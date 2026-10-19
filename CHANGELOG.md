10/18/2026
Adding motorized switch/levers via `DcsBios::EasyMode::SyncingSwitch3Pos`
This can be a switch/lever that is manually moved back to match DCS
or via a spring pulling back to the default middle state ("MID") and
solenoids holding a switch as "UP" or "DOWN".
e.g. Hydrolic levers that return to middle after hitting end stops
     - Mosquito, Bomb Bay Door, Landing Gear, Flaps
Solenoids are optional via an extended constructor.
- Added `examples/63_Mosquito_Levers` to demonstrate this.

8/22/2026
Moved the Watchdog Timer reboot() into DCSBIOSEasyMode

8/22/2026
Updated the Arduino Nano pin documentation

8/22/2026
Implemented saving of calibration mode to EEPROM from a sketch into DCSBIOSEasyMode

8/22/2026
Implemented the integration of the calibration mode from a sketch into DCSBIOSEasyMode

What changed:
- Added internal calibration registry in `src/internal/DcsBiosEasyCommon.h`.
- Added new internal EEPROM persistence helper: `src/internal/DcsBiosEasyEEPROM.h`.
- Added public EasyMode wrappers in `src/DcsBiosEasyMode.h`:
  `loadCalibration`, `beginCalibration`, `updateCalibration`, `saveCalibration`, `saveCalibrationIfChanged`, `calibrationIsValid`.
- Updated analog constructors to use ADC span instead of constructor min/max.
- Analog inputs now suppress output when calibration has been explicitly cleared/loaded invalid.
- Migrated `examples/61_Mosquito_Throttle_Quadrant`.
- Updated `documentation/Example_Guide.md` around the analog calibration section.
- Added keyword entries for the new public API.

8/22/2026
Authentikit Mosquito Throttle Quadrant v1
- DCS-BIOS (not Joystick) implementation using Arduino Nano
- First use of EasyMode Potentiometer & AnalogMultiPos use of min/max 
  calibration data so that the physical limits of the input map to the 
  0..64k range DCS expects. DcsBios just assumes 0..1023 ADC range.
- First use of in-sketch EEPROM saving of min/max calibration data
