# Altimeter Zero Detect System Requirements

## Implemented Milestone: Optional Fine Zero Detect

Some mechanically linked altimeters have three needles:

- 100-foot needle driving 10:1 into the 1,000-foot needle
- 1,000-foot needle driving 10:1 into the 10,000-foot needle
- 10,000-foot needle carrying a coarse zero lobe

The 10,000-foot shaft zero detector is useful for coarse homing, but backlash in the gear train can leave the 100-foot needle several turns away from its precise reference. The stepper API therefore supports an optional second zero detector on the 100-foot shaft.

Constructor wiring order:

```cpp
DcsBios::EasyMode::Stepper_28BYJ48 altimeterNeedle(
    telemetrySource,
    pin1,
    pin2,
    pin3,
    pin4,
    coarseZeroPin,
    coarseZeroActiveState,
    fineZeroPin,
    fineZeroActiveState
);
```

Homing behavior:

1. Seek the coarse detector to find the approximate altitude phase.
2. Reverse upward/clockwise until the coarse detector releases.
3. Continue upward/clockwise by the configured clearance distance.
4. If the fine detector is already active, continue upward/clockwise until it releases.
5. Seek downward/counter-clockwise at fine homing speed until the fine detector is found.
6. Use the fine detector edge as the final zero reference.

If the fine zero pin is omitted or set to `DcsBios::EasyMode::NoPin`, the fine pass falls back to the coarse detector. This preserves the single-detector behavior.
