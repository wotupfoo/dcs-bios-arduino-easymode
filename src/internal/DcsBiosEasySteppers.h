#ifndef __DCSBIOS_EASY_STEPPERS_H
#define __DCSBIOS_EASY_STEPPERS_H

#include <math.h>

namespace DcsBios {

template<long STEPS_PER_OUTPUT_REV,
         uint8_t ACCELSTEPPER_INTERFACE,
         int DEFAULT_MAX_RPM_X10,
         int DEFAULT_ACCEL_RPM_PER_SEC_X10,
         int DEFAULT_HOMING_RPM_X10,
         bool SWAP_MIDDLE_PINS = false>
struct StepperProfile {
    static constexpr long kStepsPerOutputRev = STEPS_PER_OUTPUT_REV;
    static constexpr uint8_t kInterface = ACCELSTEPPER_INTERFACE;
    static constexpr float kDefaultMaxRpm = DEFAULT_MAX_RPM_X10 / 10.0f;
    static constexpr float kDefaultAccelRpmPerSec = DEFAULT_ACCEL_RPM_PER_SEC_X10 / 10.0f;
    static constexpr float kDefaultHomingRpm = DEFAULT_HOMING_RPM_X10 / 10.0f;
    static constexpr bool kSwapMiddlePins = SWAP_MIDDLE_PINS;
};

// Generic directly-driven 4-wire stepper defaults.
// This is a sensible baseline for hobby steppers without gearbox-specific
// assumptions. More specific hardware can expose their own flat aliases.
using GenericStepperProfile = StepperProfile<
    200,
    AccelStepper::FULL4WIRE,
    60,   // 6.0 RPM normal running speed
    120,  // 12.0 RPM/sec acceleration
    10,   // 1.0 RPM homing speed
    false
>;

// 28BYJ-48 on a ULN2003 board.
// 4096 half-steps per output revolution is the common hobby value.
using Stepper28Byj48Profile = StepperProfile<
    4096,
    AccelStepper::HALF4WIRE,
    80,   // 8.0 RPM normal running speed
    200,  // 20.0 RPM/sec acceleration
    10,   // 1.0 RPM homing speed
    true  // swap the middle pins for AccelStepper
>;

template<typename ProfileT>
class EasyStepperOutputT : public Int16Buffer {
public:
    static constexpr uint8_t PIN_NONE = 0xFF;
    typedef void (*FaultCallback)(
        unsigned int address,
        unsigned long serviceGapUs,
        unsigned long allowedGapUs
    );

private:
    enum HomeState {
        HOME_NONE,
        HOME_SEEK_SWITCH,
        HOME_RELEASE_SWITCH,
        HOME_DONE
    };

    AccelStepper stepper_;
    unsigned int address_;
    float maxRpm_;
    FaultCallback faultCallback_;
    bool timingFaultLatched_;
    float faultToleranceMultiplier_;
    unsigned long lastServiceUs_;
    unsigned long lastExpectedStepIntervalUs_;

    unsigned int mask_;
    unsigned char shift_;
    bool continuous_;
    bool continuousUseModulo_;
    bool continuousUseShortestPath_;
    bool continuousInputIsAngle_;
    float minAngleDeg_;
    float maxAngleDeg_;
    float trimDeg_;
    bool reverse_;
    unsigned int inputMaxValue_;
    bool inputZeroCentered_;

    uint8_t zeroPin_;
    bool zeroActiveLow_;
    int8_t homeDirection_;
    float zeroOffsetDeg_;
    HomeState homeState_;

    static float rpmToStepsPerSecond(float rpm) {
        return (rpm * (float)ProfileT::kStepsPerOutputRev) / 60.0f;
    }

    static float accelRpmPerSecToStepsPerSec2(float accelRpmPerSec) {
        return (accelRpmPerSec * (float)ProfileT::kStepsPerOutputRev) / 60.0f;
    }

    static unsigned long stepsPerSecondToIntervalUs(float stepsPerSecond) {
        float magnitude = fabsf(stepsPerSecond);
        if (magnitude < 0.001f) return 0UL;

        float intervalUs = 1000000.0f / magnitude;
        if (intervalUs <= 1.0f) return 1UL;
        return (unsigned long)lroundf(intervalUs);
    }

    static long roundToLong(float value) {
        return (long)lroundf(value);
    }

    float rawToCenteredFraction(unsigned int raw) const {
        unsigned int midLower = inputMaxValue_ / 2U;
        unsigned int midUpper = (inputMaxValue_ + 1U) / 2U;

        if (raw <= midLower) {
            if (midLower == 0U) return 0.0f;
            return ((float)raw / (float)midLower) - 1.0f;
        }

        if (raw < midUpper) return 0.0f;

        unsigned int positiveSpan = inputMaxValue_ - midUpper;
        if (positiveSpan == 0U) return 0.0f;
        return (float)(raw - midUpper) / (float)positiveSpan;
    }

    static long positiveModulo(long value, long modulus) {
        long out = value % modulus;
        if (out < 0) out += modulus;
        return out;
    }

    static long chooseNearestEquivalent(long currentPosition, long normalizedTarget, long modulus) {
        long turn = (long)lround((double)(currentPosition - normalizedTarget) / (double)modulus);
        long candidate = normalizedTarget + (turn * modulus);

        long best = candidate;
        long bestDistance = labs(candidate - currentPosition);

        long candidateMinus = candidate - modulus;
        long distanceMinus = labs(candidateMinus - currentPosition);
        if (distanceMinus < bestDistance) {
            best = candidateMinus;
            bestDistance = distanceMinus;
        }

        long candidatePlus = candidate + modulus;
        long distancePlus = labs(candidatePlus - currentPosition);
        if (distancePlus < bestDistance) {
            best = candidatePlus;
        }

        return best;
    }

    static long chooseDirectionalEquivalent(long currentPosition, long normalizedTarget, long modulus) {
        long currentNormalized = positiveModulo(currentPosition, modulus);
        long currentTurnBase = currentPosition - currentNormalized;
        long candidate = currentTurnBase + normalizedTarget;

        if (currentNormalized < normalizedTarget && candidate < currentPosition) {
            candidate += modulus;
        } else if (currentNormalized > normalizedTarget && candidate > currentPosition) {
            candidate -= modulus;
        } else if (currentNormalized == normalizedTarget) {
            candidate = currentPosition;
        }

        return candidate;
    }

    unsigned int readSourceValue() {
        return ((this->Int16Buffer::getData()) & mask_) >> shift_;
    }

    bool isZeroActive() const {
        if (zeroPin_ == PIN_NONE) return false;
        int value = digitalRead(zeroPin_);
        return zeroActiveLow_ ? (value == LOW) : (value == HIGH);
    }

    long angleDegToSteps(float angleDeg) const {
        return roundToLong((angleDeg / 360.0f) * (float)ProfileT::kStepsPerOutputRev);
    }

    long zeroOffsetSteps() const {
        return angleDegToSteps(zeroOffsetDeg_);
    }

    long rawToBoundedSteps(unsigned int raw) const {
        if (inputZeroCentered_) {
            float centeredFraction = rawToCenteredFraction(raw);
            float targetAngleDeg = (centeredFraction < 0.0f)
                ? (minAngleDeg_ * -centeredFraction)
                : (maxAngleDeg_ * centeredFraction);

            if (reverse_) targetAngleDeg = -targetAngleDeg;
            targetAngleDeg += trimDeg_;
            return angleDegToSteps(targetAngleDeg) + zeroOffsetSteps();
        }

        float outMin = minAngleDeg_ + trimDeg_;
        float outMax = maxAngleDeg_ + trimDeg_;

        if (reverse_) {
            float temp = outMin;
            outMin = outMax;
            outMax = temp;
        }

        float targetAngleDeg = outMin + ((outMax - outMin) * ((float)raw / (float)inputMaxValue_));
        return angleDegToSteps(targetAngleDeg) + zeroOffsetSteps();
    }

    long rawToContinuousSteps(unsigned int raw) const {
        float targetAngleDeg = 0.0f;

        if (continuousInputIsAngle_) {
            targetAngleDeg = inputZeroCentered_
                ? (rawToCenteredFraction(raw) * ((float)inputMaxValue_ / 2.0f))
                : (float)raw;
        } else {
            targetAngleDeg = inputZeroCentered_
                ? (rawToCenteredFraction(raw) * 180.0f)
                : (((float)raw / (float)inputMaxValue_) * 360.0f);
        }

        if (reverse_) targetAngleDeg = -targetAngleDeg;
        targetAngleDeg += trimDeg_;

        long targetSteps = angleDegToSteps(targetAngleDeg) + zeroOffsetSteps();
        if (!continuousUseModulo_) return targetSteps;

        long normalizedTarget = positiveModulo(targetSteps, ProfileT::kStepsPerOutputRev);
        if (continuousUseShortestPath_) {
            return chooseNearestEquivalent(
                stepper_.currentPosition(),
                normalizedTarget,
                ProfileT::kStepsPerOutputRev
            );
        }

        return chooseDirectionalEquivalent(
            stepper_.currentPosition(),
            normalizedTarget,
            ProfileT::kStepsPerOutputRev
        );
    }

    void runHoming() {
        if (homeState_ == HOME_DONE || homeState_ == HOME_NONE) return;

        float homingSpeed = rpmToStepsPerSecond(ProfileT::kDefaultHomingRpm);

        if (homeState_ == HOME_SEEK_SWITCH) {
            if (isZeroActive()) {
                homeState_ = HOME_RELEASE_SWITCH;
            } else {
                stepper_.setSpeed((homeDirection_ < 0) ? -homingSpeed : homingSpeed);
                stepper_.runSpeed();
                return;
            }
        }

        if (homeState_ == HOME_RELEASE_SWITCH) {
            if (isZeroActive()) {
                stepper_.setSpeed((homeDirection_ < 0) ? homingSpeed : -homingSpeed);
                stepper_.runSpeed();
                return;
            }

            stepper_.setCurrentPosition(zeroOffsetSteps());
            stepper_.moveTo(zeroOffsetSteps());
            homeState_ = HOME_DONE;
        }
    }

    unsigned long allowedServiceGapUs() const {
        if (lastExpectedStepIntervalUs_ == 0UL) return 0UL;

        float allowedGapUs = (float)lastExpectedStepIntervalUs_ * faultToleranceMultiplier_;
        if (allowedGapUs <= 1.0f) return 1UL;
        return (unsigned long)lroundf(allowedGapUs);
    }

    void checkTimingFault(unsigned long nowUs) {
        if (timingFaultLatched_) return;
        if (lastServiceUs_ == 0UL) return;
        if (lastExpectedStepIntervalUs_ == 0UL) return;

        unsigned long allowedGapUs = allowedServiceGapUs();
        unsigned long serviceGapUs = nowUs - lastServiceUs_;
        if (allowedGapUs == 0UL || serviceGapUs <= allowedGapUs) return;

        timingFaultLatched_ = true;
        if (faultCallback_ != nullptr) {
            faultCallback_(address_, serviceGapUs, allowedGapUs);
        }
    }

    void updateExpectedStepIntervalUs() {
        if (homeState_ != HOME_DONE && homeState_ != HOME_NONE) {
            lastExpectedStepIntervalUs_ = stepsPerSecondToIntervalUs(
                rpmToStepsPerSecond(ProfileT::kDefaultHomingRpm)
            );
            return;
        }

        if (stepper_.distanceToGo() == 0L) {
            lastExpectedStepIntervalUs_ = 0UL;
            return;
        }

        lastExpectedStepIntervalUs_ = stepsPerSecondToIntervalUs(stepper_.speed());
    }

    void commonInit(
        bool continuous,
        float minAngleDeg,
        float maxAngleDeg,
        bool reverse,
        float trimDeg,
        float maxRpm,
        float accelRpmPerSec,
        uint8_t zeroPin,
        bool zeroActiveLow,
        int8_t homeDirection,
        float zeroOffsetDeg,
        unsigned int inputMaxValue
    ) {
        continuous_ = continuous;
        continuousUseModulo_ = continuous;
        continuousUseShortestPath_ = continuous;
        continuousInputIsAngle_ = false;
        minAngleDeg_ = minAngleDeg;
        maxAngleDeg_ = maxAngleDeg;
        trimDeg_ = trimDeg;
        reverse_ = reverse;
        inputMaxValue_ = inputMaxValue ? inputMaxValue : 65535;
        zeroPin_ = zeroPin;
        zeroActiveLow_ = zeroActiveLow;
        homeDirection_ = (homeDirection < 0) ? -1 : 1;
        zeroOffsetDeg_ = zeroOffsetDeg;
        inputZeroCentered_ = false;
        maxRpm_ = maxRpm;
        faultCallback_ = nullptr;
        timingFaultLatched_ = false;
        faultToleranceMultiplier_ = 1.5f;
        lastServiceUs_ = micros();
        lastExpectedStepIntervalUs_ = 0UL;

        stepper_.setMaxSpeed(rpmToStepsPerSecond(maxRpm_));
        stepper_.setAcceleration(accelRpmPerSecToStepsPerSec2(accelRpmPerSec));
        stepper_.setCurrentPosition(0);

        if (zeroPin_ == PIN_NONE) {
            homeState_ = HOME_DONE;
        } else {
            pinMode(zeroPin_, zeroActiveLow_ ? INPUT_PULLUP : INPUT);
            homeState_ = isZeroActive() ? HOME_RELEASE_SWITCH : HOME_SEEK_SWITCH;
        }
    }

protected:
    void configureContinuousBehavior(bool useModulo, bool useShortestPath, bool inputIsAngle) {
        continuous_ = true;
        continuousUseModulo_ = useModulo;
        continuousUseShortestPath_ = useShortestPath;
        continuousInputIsAngle_ = inputIsAngle;
        minAngleDeg_ = 0.0f;
        maxAngleDeg_ = 360.0f;
    }

public:
    /*
     * Easy continuous stepper output.
     *
     * Use this for gauges that can rotate forever, such as a compass or clock.
     * There is no maximum angle. With modulus enabled, the class automatically
     * chooses the nearest equivalent revolution so the pointer/card crosses
     * zero smoothly, for example 355 -> 361 instead of 355 -> 1 backwards.
     *
     * trimDeg rotates the whole repeating scale around the dial face.
     * zeroOffsetDeg is mainly for homing systems: it moves the defined zero
     * point by a small amount after the switch or opto sensor is found.
     */
    EasyStepperOutputT(
        unsigned int address,                    // DCS World: memory address with the value
        uint8_t pin1,                            // Stepper driver input pin 1
        uint8_t pin2,                            // Stepper driver input pin 2
        uint8_t pin3,                            // Stepper driver input pin 3
        uint8_t pin4,                            // Stepper driver input pin 4
        bool reverse = false,                    // Reverse the direction (true or false)
        float trimDeg = 0.0f,                    // Trim Degrees: rotate the whole repeating scale around the dial face
        float maxRpm = ProfileT::kDefaultMaxRpm, // Maximum Speed in Revolutions Per Minute (RPM)
        float accelRpmPerSec = ProfileT::kDefaultAccelRpmPerSec, // Maximum Acceleration in RPM per second
        uint8_t zeroPin = PIN_NONE,              // zeroPin: optional microswitch or opto detector input pin
        bool zeroActiveLow = true,               // zeroPin Active LOW (active when the signal is pulled to Ground)
        int8_t homeDirection = -1,               // Homing direction: -1 or +1 while seeking zero
        float zeroOffsetDeg = 0.0f,              // Zero Offset Degrees: fine adjustment after homing
        unsigned int inputMaxValue = 65535,      // Maximum incoming DCS-BIOS value for this source
        bool inputZeroCentered = false           // True if the middle of the DCS-BIOS range should map to 0 degrees
    ) : Int16Buffer(address),
        stepper_(
            ProfileT::kInterface,
            pin1,
            ProfileT::kSwapMiddlePins ? pin3 : pin2,
            ProfileT::kSwapMiddlePins ? pin2 : pin3,
            pin4
        ),
        address_(address),
        mask_(0xFFFF),
        shift_(0) {
        commonInit(
            true,
            0.0f,
            0.0f,
            reverse,
            trimDeg,
            maxRpm,
            accelRpmPerSec,
            zeroPin,
            zeroActiveLow,
            homeDirection,
            zeroOffsetDeg,
            inputMaxValue
        );
        inputZeroCentered_ = inputZeroCentered;
    }

    EasyStepperOutputT(
        unsigned int address,                    // DCS World: memory address with the value
        unsigned int mask,                       // Bit mask for packed integer fields
        unsigned char shift,                     // Right shift for packed integer fields
        uint8_t pin1,                            // Stepper driver input pin 1
        uint8_t pin2,                            // Stepper driver input pin 2
        uint8_t pin3,                            // Stepper driver input pin 3
        uint8_t pin4,                            // Stepper driver input pin 4
        bool reverse = false,                    // Reverse the direction (true or false)
        float trimDeg = 0.0f,                    // Trim Degrees: rotate the whole repeating scale around the dial face
        float maxRpm = ProfileT::kDefaultMaxRpm, // Maximum Speed in Revolutions Per Minute (RPM)
        float accelRpmPerSec = ProfileT::kDefaultAccelRpmPerSec, // Maximum Acceleration in RPM per second
        uint8_t zeroPin = PIN_NONE,              // zeroPin: optional microswitch or opto detector input pin
        bool zeroActiveLow = true,               // zeroPin Active LOW (active when the signal is pulled to Ground)
        int8_t homeDirection = -1,               // Homing direction: -1 or +1 while seeking zero
        float zeroOffsetDeg = 0.0f,              // Zero Offset Degrees: fine adjustment after homing
        unsigned int inputMaxValue = 65535,      // Maximum incoming DCS-BIOS value for this source
        bool inputZeroCentered = false           // True if the middle of the DCS-BIOS range should map to 0 degrees
    ) : Int16Buffer(address),
        stepper_(
            ProfileT::kInterface,
            pin1,
            ProfileT::kSwapMiddlePins ? pin3 : pin2,
            ProfileT::kSwapMiddlePins ? pin2 : pin3,
            pin4
        ),
        address_(address),
        mask_(mask),
        shift_(shift) {
        commonInit(
            true,
            0.0f,
            0.0f,
            reverse,
            trimDeg,
            maxRpm,
            accelRpmPerSec,
            zeroPin,
            zeroActiveLow,
            homeDirection,
            zeroOffsetDeg,
            inputMaxValue
        );
        inputZeroCentered_ = inputZeroCentered;
    }

    /*
     * Easy bounded stepper output.
     *
     * Use this for gauges that have a start angle and an end angle, such as
     * an altimeter needle or other instrument that does not rotate forever.
     *
     * minAngleDeg and maxAngleDeg set the size of the sweep used by the gauge.
     * trimDeg shifts that whole sweep around the dial face.
     *
     * If inputZeroCentered is true, the middle of the incoming DCS-BIOS range
     * maps to 0 degrees instead of the midpoint between minAngleDeg and maxAngleDeg.
     */
    EasyStepperOutputT(
        unsigned int address,                    // DCS World: memory address with the value
        uint8_t pin1,                            // Stepper driver input pin 1
        uint8_t pin2,                            // Stepper driver input pin 2
        uint8_t pin3,                            // Stepper driver input pin 3
        uint8_t pin4,                            // Stepper driver input pin 4
        float minAngleDeg,                       // Minimum needle angle in degrees for the lowest DCS-BIOS value
        float maxAngleDeg,                       // Maximum needle angle in degrees for the highest DCS-BIOS value
        bool reverse = false,                    // Reverse the direction (true or false)
        float trimDeg = 0.0f,                    // Trim Degrees: rotate the whole scale to match the printed dial face
        float maxRpm = ProfileT::kDefaultMaxRpm, // Maximum Speed in Revolutions Per Minute (RPM)
        float accelRpmPerSec = ProfileT::kDefaultAccelRpmPerSec, // Maximum Acceleration in RPM per second
        uint8_t zeroPin = PIN_NONE,              // zeroPin: optional microswitch or opto detector input pin
        bool zeroActiveLow = true,               // zeroPin Active LOW (active when the signal is pulled to Ground)
        int8_t homeDirection = -1,               // Homing direction: -1 or +1 while seeking zero
        float zeroOffsetDeg = 0.0f,              // Zero Offset Degrees: fine adjustment after homing
        unsigned int inputMaxValue = 65535,      // Maximum incoming DCS-BIOS value for this source
        bool inputZeroCentered = false           // True if the middle of the DCS-BIOS range should map to 0 degrees
    ) : Int16Buffer(address),
        stepper_(
            ProfileT::kInterface,
            pin1,
            ProfileT::kSwapMiddlePins ? pin3 : pin2,
            ProfileT::kSwapMiddlePins ? pin2 : pin3,
            pin4
        ),
        address_(address),
        mask_(0xFFFF),
        shift_(0) {
        commonInit(
            false,
            minAngleDeg,
            maxAngleDeg,
            reverse,
            trimDeg,
            maxRpm,
            accelRpmPerSec,
            zeroPin,
            zeroActiveLow,
            homeDirection,
            zeroOffsetDeg,
            inputMaxValue
        );
        inputZeroCentered_ = inputZeroCentered;
    }

    EasyStepperOutputT(
        unsigned int address,                    // DCS World: memory address with the value
        unsigned int mask,                       // Bit mask for packed integer fields
        unsigned char shift,                     // Right shift for packed integer fields
        uint8_t pin1,                            // Stepper driver input pin 1
        uint8_t pin2,                            // Stepper driver input pin 2
        uint8_t pin3,                            // Stepper driver input pin 3
        uint8_t pin4,                            // Stepper driver input pin 4
        float minAngleDeg,                       // Minimum needle angle in degrees for the lowest DCS-BIOS value
        float maxAngleDeg,                       // Maximum needle angle in degrees for the highest DCS-BIOS value
        bool reverse = false,                    // Reverse the direction (true or false)
        float trimDeg = 0.0f,                    // Trim Degrees: rotate the whole scale to match the printed dial face
        float maxRpm = ProfileT::kDefaultMaxRpm, // Maximum Speed in Revolutions Per Minute (RPM)
        float accelRpmPerSec = ProfileT::kDefaultAccelRpmPerSec, // Maximum Acceleration in RPM per second
        uint8_t zeroPin = PIN_NONE,              // zeroPin: optional microswitch or opto detector input pin
        bool zeroActiveLow = true,               // zeroPin Active LOW (active when the signal is pulled to Ground)
        int8_t homeDirection = -1,               // Homing direction: -1 or +1 while seeking zero
        float zeroOffsetDeg = 0.0f,              // Zero Offset Degrees: fine adjustment after homing
        unsigned int inputMaxValue = 65535,      // Maximum incoming DCS-BIOS value for this source
        bool inputZeroCentered = false           // True if the middle of the DCS-BIOS range should map to 0 degrees
    ) : Int16Buffer(address),
        stepper_(
            ProfileT::kInterface,
            pin1,
            ProfileT::kSwapMiddlePins ? pin3 : pin2,
            ProfileT::kSwapMiddlePins ? pin2 : pin3,
            pin4
        ),
        address_(address),
        mask_(mask),
        shift_(shift) {
        commonInit(
            false,
            minAngleDeg,
            maxAngleDeg,
            reverse,
            trimDeg,
            maxRpm,
            accelRpmPerSec,
            zeroPin,
            zeroActiveLow,
            homeDirection,
            zeroOffsetDeg,
            inputMaxValue
        );
        inputZeroCentered_ = inputZeroCentered;
    }

    virtual void loop() override {
        unsigned long nowUs = micros();
        checkTimingFault(nowUs);
        lastServiceUs_ = nowUs;

        if (homeState_ != HOME_DONE) {
            runHoming();
            updateExpectedStepIntervalUs();
            return;
        }

        if (hasUpdatedData()) {
            unsigned int sourceValue = readSourceValue();
            if (continuous_) {
                stepper_.moveTo(rawToContinuousSteps(sourceValue));
            } else {
                stepper_.moveTo(rawToBoundedSteps(sourceValue));
            }
        }

        stepper_.run();
        updateExpectedStepIntervalUs();
    }

    void startHoming() {
        if (zeroPin_ == PIN_NONE) return;
        homeState_ = isZeroActive() ? HOME_RELEASE_SWITCH : HOME_SEEK_SWITCH;
    }

    bool isHomed() const {
        return (homeState_ == HOME_DONE);
    }

    void setTrimDeg(float trimDeg) {
        trimDeg_ = trimDeg;
    }

    void setMinAngle(float angleDeg) {
        minAngleDeg_ = angleDeg;
        continuous_ = false;
    }

    void setMaxAngle(float angleDeg) {
        maxAngleDeg_ = angleDeg;
        continuous_ = false;
    }

    void setDirection(EasyModeDir direction) {
        reverse_ = (direction == EasyModeDir::CCW);
    }

    void setInputMaxValue(unsigned int inputMaxValue) {
        inputMaxValue_ = inputMaxValue ? inputMaxValue : 65535;
    }

    // Treat the incoming DCS-BIOS value as a centered range where the
    // midpoint is the instrument's 0-degree position.
    void setInputZeroCentered(bool inputZeroCentered) {
        inputZeroCentered_ = inputZeroCentered;
    }

    void setInputZeroInMiddleOfRange(bool inputZeroCentered) {
        setInputZeroCentered(inputZeroCentered);
    }

    // Enable 0..360 wrapping for continuous gauges. When enabled, movement
    // uses the nearest equivalent target across the modulus boundary.
    void setModulusEnabled(bool modulusEnabled) {
        continuousUseModulo_ = modulusEnabled;
    }

    // Backward-compatible alias.
    void setUseModulo(bool useModulo) {
        setModulusEnabled(useModulo);
    }

    void setZeroOffsetDeg(float zeroOffsetDeg) {
        zeroOffsetDeg_ = zeroOffsetDeg;
    }

    void setFaultCallback(FaultCallback faultCallback) {
        faultCallback_ = faultCallback;
    }

    void clearTimingFault() {
        timingFaultLatched_ = false;
        lastServiceUs_ = micros();
    }

    bool hasTimingFault() const {
        return timingFaultLatched_;
    }

    void setFaultToleranceMultiplier(float faultToleranceMultiplier) {
        faultToleranceMultiplier_ = (faultToleranceMultiplier < 1.0f) ? 1.0f : faultToleranceMultiplier;
    }

    void setMaxRpm(float maxRpm) {
        maxRpm_ = maxRpm;
        stepper_.setMaxSpeed(rpmToStepsPerSecond(maxRpm_));
    }

    float getMaxRpm() const {
        return maxRpm_;
    }

    void setAccelerationRpmPerSec(float accelRpmPerSec) {
        stepper_.setAcceleration(accelRpmPerSecToStepsPerSec2(accelRpmPerSec));
    }

    void setCurrentPositionDeg(float angleDeg) {
        stepper_.setCurrentPosition(angleDegToSteps(angleDeg));
    }

    long currentPositionSteps() const {
        return stepper_.currentPosition();
    }

    long targetPositionSteps() const {
        return stepper_.targetPosition();
    }

    long distanceToGoSteps() const {
        return stepper_.distanceToGo();
    }
};

// Public naming scheme for snippet generators and no-code users:
//   EasyStepper                         -> legacy flexible stepper
//   EasyStepper_Bounded                -> generic bounded sweep stepper
//   EasyStepper_Continuous             -> generic continuous angle stepper
//   EasyStepper_28BYJ48                -> legacy flexible 28BYJ-48 stepper
//   EasyStepper_28BYJ48_Bounded        -> 28BYJ-48 bounded sweep stepper
//   EasyStepper_28BYJ48_Continuous     -> 28BYJ-48 continuous angle stepper
using EasyStepper = EasyStepperOutputT<GenericStepperProfile>;

class EasyStepper_Bounded : public EasyStepperOutputT<GenericStepperProfile> {
public:
    EasyStepper_Bounded(
        unsigned int address,
        uint8_t pin1,
        uint8_t pin2,
        uint8_t pin3,
        uint8_t pin4,
        uint8_t zeroPin = EasyStepperOutputT<GenericStepperProfile>::PIN_NONE,
        bool inputZeroCentered = false
    ) : EasyStepperOutputT<GenericStepperProfile>(
        address,
        pin1,
        pin2,
        pin3,
        pin4,
        inputZeroCentered ? -180.0f : 0.0f,
        inputZeroCentered ? 180.0f : 360.0f,
        false,
        0.0f,
        GenericStepperProfile::kDefaultMaxRpm,
        GenericStepperProfile::kDefaultAccelRpmPerSec,
        zeroPin,
        true,
        -1,
        0.0f,
        65535,
        inputZeroCentered
    ) {
    }

    EasyStepper_Bounded(
        unsigned int address,
        unsigned int mask,
        unsigned char shift,
        uint8_t pin1,
        uint8_t pin2,
        uint8_t pin3,
        uint8_t pin4,
        uint8_t zeroPin = EasyStepperOutputT<GenericStepperProfile>::PIN_NONE,
        bool inputZeroCentered = false
    ) : EasyStepperOutputT<GenericStepperProfile>(
        address,
        mask,
        shift,
        pin1,
        pin2,
        pin3,
        pin4,
        inputZeroCentered ? -180.0f : 0.0f,
        inputZeroCentered ? 180.0f : 360.0f,
        false,
        0.0f,
        GenericStepperProfile::kDefaultMaxRpm,
        GenericStepperProfile::kDefaultAccelRpmPerSec,
        zeroPin,
        true,
        -1,
        0.0f,
        65535,
        inputZeroCentered
    ) {
    }
};

class EasyStepper_Continuous : public EasyStepperOutputT<GenericStepperProfile> {
public:
    EasyStepper_Continuous(
        unsigned int address,
        uint8_t pin1,
        uint8_t pin2,
        uint8_t pin3,
        uint8_t pin4,
        uint8_t zeroPin = EasyStepperOutputT<GenericStepperProfile>::PIN_NONE,
        bool inputZeroCentered = false
    ) : EasyStepperOutputT<GenericStepperProfile>(
        address,
        pin1,
        pin2,
        pin3,
        pin4,
        false,
        0.0f,
        GenericStepperProfile::kDefaultMaxRpm,
        GenericStepperProfile::kDefaultAccelRpmPerSec,
        zeroPin,
        true,
        -1,
        0.0f,
        360,
        inputZeroCentered
    ) {
        this->configureContinuousBehavior(true, true, true);
    }

    EasyStepper_Continuous(
        unsigned int address,
        unsigned int mask,
        unsigned char shift,
        uint8_t pin1,
        uint8_t pin2,
        uint8_t pin3,
        uint8_t pin4,
        uint8_t zeroPin = EasyStepperOutputT<GenericStepperProfile>::PIN_NONE,
        bool inputZeroCentered = false
    ) : EasyStepperOutputT<GenericStepperProfile>(
        address,
        mask,
        shift,
        pin1,
        pin2,
        pin3,
        pin4,
        false,
        0.0f,
        GenericStepperProfile::kDefaultMaxRpm,
        GenericStepperProfile::kDefaultAccelRpmPerSec,
        zeroPin,
        true,
        -1,
        0.0f,
        360,
        inputZeroCentered
    ) {
        this->configureContinuousBehavior(true, true, true);
    }
};

class EasyStepper_28BYJ48 : public EasyStepperOutputT<Stepper28Byj48Profile> {
public:
    EasyStepper_28BYJ48(
        unsigned int address,
        uint8_t pin1,
        uint8_t pin2,
        uint8_t pin3,
        uint8_t pin4
    ) : EasyStepperOutputT<Stepper28Byj48Profile>(address, pin1, pin2, pin3, pin4) {
    }

    EasyStepper_28BYJ48(
        unsigned int address,
        unsigned int mask,
        unsigned char shift,
        uint8_t pin1,
        uint8_t pin2,
        uint8_t pin3,
        uint8_t pin4
    ) : EasyStepperOutputT<Stepper28Byj48Profile>(address, mask, shift, pin1, pin2, pin3, pin4) {
    }
};

class EasyStepper_28BYJ48_Bounded : public EasyStepperOutputT<Stepper28Byj48Profile> {
public:
    EasyStepper_28BYJ48_Bounded(
        unsigned int address,
        uint8_t pin1,
        uint8_t pin2,
        uint8_t pin3,
        uint8_t pin4,
        uint8_t zeroPin = EasyStepperOutputT<Stepper28Byj48Profile>::PIN_NONE,
        bool inputZeroCentered = false
    ) : EasyStepperOutputT<Stepper28Byj48Profile>(
        address,
        pin1,
        pin2,
        pin3,
        pin4,
        inputZeroCentered ? -180.0f : 0.0f,
        inputZeroCentered ? 180.0f : 360.0f,
        false,
        0.0f,
        Stepper28Byj48Profile::kDefaultMaxRpm,
        Stepper28Byj48Profile::kDefaultAccelRpmPerSec,
        zeroPin,
        true,
        -1,
        0.0f,
        65535,
        inputZeroCentered
    ) {
    }

    EasyStepper_28BYJ48_Bounded(
        unsigned int address,
        unsigned int mask,
        unsigned char shift,
        uint8_t pin1,
        uint8_t pin2,
        uint8_t pin3,
        uint8_t pin4,
        uint8_t zeroPin = EasyStepperOutputT<Stepper28Byj48Profile>::PIN_NONE,
        bool inputZeroCentered = false
    ) : EasyStepperOutputT<Stepper28Byj48Profile>(
        address,
        mask,
        shift,
        pin1,
        pin2,
        pin3,
        pin4,
        inputZeroCentered ? -180.0f : 0.0f,
        inputZeroCentered ? 180.0f : 360.0f,
        false,
        0.0f,
        Stepper28Byj48Profile::kDefaultMaxRpm,
        Stepper28Byj48Profile::kDefaultAccelRpmPerSec,
        zeroPin,
        true,
        -1,
        0.0f,
        65535,
        inputZeroCentered
    ) {
    }
};

class EasyStepper_28BYJ48_Continuous : public EasyStepperOutputT<Stepper28Byj48Profile> {
public:
    EasyStepper_28BYJ48_Continuous(
        unsigned int address,
        uint8_t pin1,
        uint8_t pin2,
        uint8_t pin3,
        uint8_t pin4,
        uint8_t zeroPin = EasyStepperOutputT<Stepper28Byj48Profile>::PIN_NONE,
        bool inputZeroCentered = false
    ) : EasyStepperOutputT<Stepper28Byj48Profile>(
        address,
        pin1,
        pin2,
        pin3,
        pin4,
        false,
        0.0f,
        Stepper28Byj48Profile::kDefaultMaxRpm,
        Stepper28Byj48Profile::kDefaultAccelRpmPerSec,
        zeroPin,
        true,
        -1,
        0.0f,
        360,
        inputZeroCentered
    ) {
        this->configureContinuousBehavior(true, true, true);
    }

    EasyStepper_28BYJ48_Continuous(
        unsigned int address,
        unsigned int mask,
        unsigned char shift,
        uint8_t pin1,
        uint8_t pin2,
        uint8_t pin3,
        uint8_t pin4,
        uint8_t zeroPin = EasyStepperOutputT<Stepper28Byj48Profile>::PIN_NONE,
        bool inputZeroCentered = false
    ) : EasyStepperOutputT<Stepper28Byj48Profile>(
        address,
        mask,
        shift,
        pin1,
        pin2,
        pin3,
        pin4,
        false,
        0.0f,
        Stepper28Byj48Profile::kDefaultMaxRpm,
        Stepper28Byj48Profile::kDefaultAccelRpmPerSec,
        zeroPin,
        true,
        -1,
        0.0f,
        360,
        inputZeroCentered
    ) {
        this->configureContinuousBehavior(true, true, true);
    }
};

// Backwards-compatible aliases.
using EasyStepperOutput = EasyStepper;
using Easy28Byj48Output = EasyStepperOutputT<Stepper28Byj48Profile>;

} // namespace DcsBios

#endif
