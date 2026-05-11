#ifndef __DCSBIOS_EASY_STEPPERS_H
#define __DCSBIOS_EASY_STEPPERS_H

#ifndef __DCSBIOS_EASY_MODE_H
#error Do not call DcsBiosEasySteppers.h directly. Include DcsBiosEasyMode.h instead.
#endif

#include <math.h>

namespace DcsBios {

template<long STEPS_PER_OUTPUT_REV,
         uint8_t ACCELSTEPPER_INTERFACE,
         int DEFAULT_MAX_RPM_X10,
         int DEFAULT_ACCEL_RPM_PER_SEC_X10,
         int DEFAULT_HOMING_RPM_X10,
         bool SWAP_MIDDLE_PINS = false,
         int CLOCKWISE_STEP_SIGN = 1>
struct StepperProfile {
    static constexpr long kStepsPerOutputRev = STEPS_PER_OUTPUT_REV;
    static constexpr uint8_t kInterface = ACCELSTEPPER_INTERFACE;
    static constexpr float kDefaultMaxRpm = DEFAULT_MAX_RPM_X10 / 10.0f;
    static constexpr float kDefaultAccelRpmPerSec = DEFAULT_ACCEL_RPM_PER_SEC_X10 / 10.0f;
    static constexpr float kDefaultHomingRpm = DEFAULT_HOMING_RPM_X10 / 10.0f;
    static constexpr bool kSwapMiddlePins = SWAP_MIDDLE_PINS;
    static constexpr long kClockwiseStepSign = (CLOCKWISE_STEP_SIGN < 0) ? -1L : 1L;
    static constexpr int8_t kDefaultHomeDirection = (CLOCKWISE_STEP_SIGN < 0) ? 1 : -1;
};

// Generic directly-driven 4-wire stepper defaults.
// This is a sensible baseline for hobby steppers without gearbox-specific
// assumptions. More specific hardware can expose their own flat aliases.
using GenericStepperProfile = StepperProfile<
    200,
    AccelStepper::FULL4WIRE,
    60,   // 6.0 RPM normal running speed
    120,  // 12.0 RPM/sec acceleration
    30,   // 3.0 RPM homing speed fallback
    false
>;

// 28BYJ-48 on a ULN2003 board.
// 2048 full-steps per output revolution trades resolution for higher torque.
using Stepper28Byj48Profile = StepperProfile<
    2048,
    AccelStepper::FULL4WIRE,
    160,  // 16.0 RPM normal running speed
    200,  // 20.0 RPM/sec acceleration
    80,   // 8.0 RPM homing speed fallback
    true, // swap the middle pins for AccelStepper
    -1    // clockwise at the gearbox shaft is negative AccelStepper movement
>;

template<typename ProfileT>
class EasyStepperOutputT : public Int16Buffer {
public:
    static constexpr uint8_t PIN_NONE = 0xFF;
    static constexpr long kDefaultHomingBackoffSteps = 100L;
    typedef void (*FaultCallback)(
        unsigned int address,
        unsigned long serviceGapUs,
        unsigned long allowedGapUs
    );

private:
    enum HomeState {
        HOME_NONE,
        HOME_START_OFFSET,
        HOME_COARSE_SEEK_SWITCH,
        HOME_RELEASE_SWITCH,
        HOME_CLEAR_SWITCH,
        HOME_FINE_SEEK_SWITCH,
        HOME_STOP_AT_ZERO,
        HOME_FAILED,
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
    uint8_t zeroActiveState_;
    int8_t homeDirection_;
    float zeroOffsetDeg_;
    long homingStartOffsetSteps_;
    long homingBackoffSteps_;
    long homingReferencePosition_;
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
        return value == zeroActiveState_;
    }

    long angleDegToSteps(float angleDeg) const {
        return roundToLong((angleDeg / 360.0f) * (float)ProfileT::kStepsPerOutputRev);
    }

    long homingOffsetDegToSteps(float angleDeg) const {
        return angleDegToSteps(angleDeg) * ProfileT::kClockwiseStepSign;
    }

    float fineHomingRpm() const {
        float rpm = maxRpm_ * 0.5f;
        return (rpm > 0.0f) ? rpm : ProfileT::kDefaultHomingRpm;
    }

    long zeroOffsetSteps() const {
        return angleDegToSteps(zeroOffsetDeg_);
    }

    long signedHomeDirection() const {
        return (homeDirection_ < 0) ? -1L : 1L;
    }

    bool isCoarseZeroActive() const {
        return isZeroActive();
    }

    bool isFineZeroActive() const {
        return isZeroActive();
    }

    long homingSeekTravelSteps() const {
        return ProfileT::kStepsPerOutputRev * 10000L;
    }

    static long stepMagnitude(long steps) {
        return (steps < 0L) ? -steps : steps;
    }

    void setHomingMaxRpm(float rpm) {
        stepper_.setMaxSpeed(rpmToStepsPerSecond(rpm));
    }

    void moveInHomingDirection(long direction, long steps) {
        long distance = stepMagnitude(steps);
        long offset = (direction < 0L) ? -distance : distance;
        stepper_.moveTo(stepper_.currentPosition() + offset);
    }

    void moveTowardSwitch(long steps) {
        moveInHomingDirection(signedHomeDirection(), steps);
    }

    void moveAwayFromSwitch(long steps) {
        moveInHomingDirection(-signedHomeDirection(), steps);
    }

    void startHomingSeek() {
        setHomingMaxRpm(maxRpm_);
        if (isCoarseZeroActive()) {
            startReleaseFromSwitch();
        } else {
            moveTowardSwitch(homingSeekTravelSteps());
            homeState_ = HOME_COARSE_SEEK_SWITCH;
        }
    }

    void startReleaseFromSwitch() {
        setHomingMaxRpm(maxRpm_);
        moveAwayFromSwitch(homingSeekTravelSteps());
        homeState_ = HOME_RELEASE_SWITCH;
    }

    void startClearanceFromSwitch() {
        if (homingBackoffSteps_ <= 0L) {
            startFineSeekSwitch();
            return;
        }

        setHomingMaxRpm(maxRpm_);
        moveAwayFromSwitch(homingBackoffSteps_);
        homeState_ = HOME_CLEAR_SWITCH;
    }

    void startFineSeekSwitch() {
        setHomingMaxRpm(fineHomingRpm());
        moveTowardSwitch(homingSeekTravelSteps());
        homeState_ = HOME_FINE_SEEK_SWITCH;
    }

    void startStopAtZero() {
        homingReferencePosition_ = stepper_.currentPosition();
        stepper_.stop();
        homeState_ = HOME_STOP_AT_ZERO;
    }

    void finishHoming() {
        long stoppedDeltaSteps = stepper_.currentPosition() - homingReferencePosition_;
        stepper_.setCurrentPosition(zeroOffsetSteps() + stoppedDeltaSteps);
        setHomingMaxRpm(maxRpm_);
        homeState_ = HOME_DONE;
    }

    void failHoming() {
        stepper_.setSpeed(0.0f);
        stepper_.stop();
        homeState_ = HOME_FAILED;
    }

    void startHomingWithOffset(long startOffsetSteps) {
        if (zeroPin_ == PIN_NONE) return;
        if (startOffsetSteps == 0L) {
            startHomingSeek();
            return;
        }

        stepper_.move(startOffsetSteps);
        homeState_ = HOME_START_OFFSET;
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

    long rawToContinuousSteps(unsigned int raw) {
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
        if (homeState_ == HOME_DONE || homeState_ == HOME_NONE || homeState_ == HOME_FAILED) return;

        if (homeState_ == HOME_START_OFFSET) {
            if (stepper_.distanceToGo() != 0L) {
                stepper_.run();
                return;
            }

            startHomingSeek();
        }

        if (homeState_ == HOME_COARSE_SEEK_SWITCH) {
            if (isCoarseZeroActive()) {
                startReleaseFromSwitch();
            } else if (stepper_.distanceToGo() == 0L) {
                failHoming();
                return;
            } else {
                stepper_.run();
                return;
            }
        }

        if (homeState_ == HOME_RELEASE_SWITCH) {
            if (!isCoarseZeroActive()) {
                startClearanceFromSwitch();
            } else if (stepper_.distanceToGo() == 0L) {
                failHoming();
                return;
            } else {
                stepper_.run();
                return;
            }
        }

        if (homeState_ == HOME_CLEAR_SWITCH) {
            if (stepper_.distanceToGo() != 0L) {
                stepper_.run();
                return;
            }

            startFineSeekSwitch();
        }

        if (homeState_ == HOME_FINE_SEEK_SWITCH) {
            if (isFineZeroActive()) {
                startStopAtZero();
            } else if (stepper_.distanceToGo() == 0L) {
                failHoming();
                return;
            } else {
                stepper_.run();
                return;
            }
        }

        if (homeState_ == HOME_STOP_AT_ZERO) {
            if (stepper_.distanceToGo() != 0L) {
                stepper_.run();
                return;
            }

            finishHoming();
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
        if (homeState_ == HOME_FAILED) {
            lastExpectedStepIntervalUs_ = 0UL;
            return;
        }

        if (homeState_ != HOME_DONE && homeState_ != HOME_NONE) {
            float expectedSpeed = 0.0f;
            if (homeState_ == HOME_STOP_AT_ZERO) {
                expectedSpeed = stepper_.speed();
            } else {
                expectedSpeed = (homeState_ == HOME_FINE_SEEK_SWITCH)
                    ? rpmToStepsPerSecond(fineHomingRpm())
                    : rpmToStepsPerSecond(maxRpm_);
            }
            lastExpectedStepIntervalUs_ = stepsPerSecondToIntervalUs(
                expectedSpeed
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
        uint8_t zeroActiveState,
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
        zeroActiveState_ = (zeroActiveState == HIGH) ? HIGH : LOW;
        homeDirection_ = (homeDirection < 0) ? -1 : 1;
        zeroOffsetDeg_ = zeroOffsetDeg;
        homingStartOffsetSteps_ = 0L;
        homingBackoffSteps_ = kDefaultHomingBackoffSteps;
        homingReferencePosition_ = 0L;
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
            pinMode(zeroPin_, INPUT_PULLUP);
            homeState_ = HOME_NONE;
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
        uint8_t zeroActiveState = LOW,           // zeroPin is active when it reads LOW or HIGH
        int8_t homeDirection = ProfileT::kDefaultHomeDirection, // Homing direction while seeking the lowest physical angle
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
            zeroActiveState,
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
        bool reverse,                            // Reverse the direction (true or false)
        float trimDeg,                           // Trim Degrees: rotate the whole repeating scale around the dial face
        float maxRpm,                            // Maximum Speed in Revolutions Per Minute (RPM)
        float accelRpmPerSec,                    // Maximum Acceleration in RPM per second
        uint8_t zeroPin,                         // zeroPin: optional microswitch or opto detector input pin
        uint8_t zeroActiveState,                 // zeroPin is active when it reads LOW or HIGH
        int8_t homeDirection,                    // Homing direction: -1 or +1 while seeking zero
        float zeroOffsetDeg,                     // Zero Offset Degrees: fine adjustment after homing
        unsigned int inputMaxValue,              // Maximum incoming DCS-BIOS value for this source
        bool inputZeroCentered                   // True if the middle of the DCS-BIOS range should map to 0 degrees
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
            zeroActiveState,
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
        uint8_t zeroActiveState = LOW,           // zeroPin is active when it reads LOW or HIGH
        int8_t homeDirection = ProfileT::kDefaultHomeDirection, // Homing direction while seeking the lowest physical angle
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
            zeroActiveState,
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
        bool reverse,                            // Reverse the direction (true or false)
        float trimDeg,                           // Trim Degrees: rotate the whole scale to match the printed dial face
        float maxRpm,                            // Maximum Speed in Revolutions Per Minute (RPM)
        float accelRpmPerSec,                    // Maximum Acceleration in RPM per second
        uint8_t zeroPin,                         // zeroPin: optional microswitch or opto detector input pin
        uint8_t zeroActiveState,                 // zeroPin is active when it reads LOW or HIGH
        int8_t homeDirection,                    // Homing direction: -1 or +1 while seeking zero
        float zeroOffsetDeg,                     // Zero Offset Degrees: fine adjustment after homing
        unsigned int inputMaxValue,              // Maximum incoming DCS-BIOS value for this source
        bool inputZeroCentered                   // True if the middle of the DCS-BIOS range should map to 0 degrees
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
            zeroActiveState,
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
        startHomingWithOffset(homingStartOffsetSteps_);
    }

    // Backward-compatible alias for older sketches.
    void home() {
        startHoming();
    }

    void home(long startOffsetSteps) {
        startHomingWithOffset(startOffsetSteps);
    }

    void homeDeg(float startOffsetDeg) {
        home(homingOffsetDegToSteps(startOffsetDeg));
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

    // Backward-compatible helper for older sketches that explicitly switch
    // between bounded and continuous modes after construction.
    void configureBoundedBehavior(float minAngleDeg, float maxAngleDeg) {
        continuous_ = false;
        minAngleDeg_ = minAngleDeg;
        maxAngleDeg_ = maxAngleDeg;
    }

    void setDirection(EasyModeDir direction) {
        reverse_ = (direction == EasyModeDir::CCW);
    }

    void setDirection(bool direction) {
        reverse_ = (direction == 1);
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

    void zeroInMiddle() {
        inputZeroCentered_ = true;
        if (!continuous_) {
            minAngleDeg_ = -180.0f;
            maxAngleDeg_ = 180.0f;
        }
    }

    void zeroAtStart() {
        inputZeroCentered_ = false;
        if (!continuous_) {
            minAngleDeg_ = 0.0f;
            maxAngleDeg_ = 360.0f;
        }
    }

    void wrapAround() {
        configureContinuousBehavior(true, true, false);
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

    void setHomingBackoffSteps(long backoffSteps) {
        homingBackoffSteps_ = (backoffSteps < 0L) ? -backoffSteps : backoffSteps;
    }

    void setHomingBackoffDeg(float backoffDeg) {
        setHomingBackoffSteps(angleDegToSteps(backoffDeg));
    }

    void setHomingBackoffDegs(float backoffDeg) {
        setHomingBackoffDeg(backoffDeg);
    }

    void setHomingStartOffsetSteps(long startOffsetSteps) {
        homingStartOffsetSteps_ = startOffsetSteps;
    }

    void setHomingStartOffsetDeg(float startOffsetDeg) {
        setHomingStartOffsetSteps(homingOffsetDegToSteps(startOffsetDeg));
    }

    void setHomingStartOffsetDegs(float startOffsetDeg) {
        setHomingStartOffsetDeg(startOffsetDeg);
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

    bool hasHomingFault() const {
        return homeState_ == HOME_FAILED;
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

    // Backward-compatible alias for older sketches.
    void setAccelRpmPerSec(float accelRpmPerSec) {
        setAccelerationRpmPerSec(accelRpmPerSec);
    }

    void setCurrentPositionDeg(float angleDeg) {
        stepper_.setCurrentPosition(angleDegToSteps(angleDeg));
    }

    long currentPositionSteps() {
        return stepper_.currentPosition();
    }

    long targetPositionSteps() {
        return stepper_.targetPosition();
    }

    long distanceToGoSteps() {
        return stepper_.distanceToGo();
    }
};

// Public naming scheme for snippet generators and no-code users:
//   EasyStepper                     -> generic 4-wire stepper
//   EasyStepper_Bounded             -> generic bounded sweep stepper
//   EasyStepper_Continuous          -> generic continuous angle stepper
//   EasyStepper_28BYJ48             -> 28BYJ-48 / ULN2003 stepper
//   EasyStepper_28BYJ48_Bounded     -> 28BYJ-48 bounded sweep stepper
//   EasyStepper_28BYJ48_Continuous  -> 28BYJ-48 continuous angle stepper
class EasyStepper : public EasyStepperOutputT<GenericStepperProfile> {
public:
    EasyStepper(
        unsigned int address,
        uint8_t pin1,
        uint8_t pin2,
        uint8_t pin3,
        uint8_t pin4,
        uint8_t zeroPin = EasyStepperOutputT<GenericStepperProfile>::PIN_NONE,
        uint8_t zeroActiveState = LOW
    ) : EasyStepperOutputT<GenericStepperProfile>(
        address,
        pin1,
        pin2,
        pin3,
        pin4,
        0.0f,
        360.0f,
        false,
        0.0f,
        GenericStepperProfile::kDefaultMaxRpm,
        GenericStepperProfile::kDefaultAccelRpmPerSec,
        zeroPin,
        zeroActiveState,
        GenericStepperProfile::kDefaultHomeDirection,
        0.0f,
        65535U,
        false
    ) {
    }

    EasyStepper(
        unsigned int address,
        unsigned int mask,
        unsigned char shift,
        uint8_t pin1,
        uint8_t pin2,
        uint8_t pin3,
        uint8_t pin4,
        uint8_t zeroPin = EasyStepperOutputT<Stepper28Byj48Profile>::PIN_NONE,
        uint8_t zeroActiveState = LOW
    ) : EasyStepperOutputT<GenericStepperProfile>(
        address,
        mask,
        shift,
        pin1,
        pin2,
        pin3,
        pin4,
        0.0f,
        360.0f,
        false,
        0.0f,
        GenericStepperProfile::kDefaultMaxRpm,
        GenericStepperProfile::kDefaultAccelRpmPerSec,
        zeroPin,
        zeroActiveState,
        GenericStepperProfile::kDefaultHomeDirection,
        0.0f,
        65535U,
        false
    ) {
    }
};

class EasyStepper_Bounded : public EasyStepperOutputT<GenericStepperProfile> {
public:
    EasyStepper_Bounded(
        unsigned int address,
        uint8_t pin1,
        uint8_t pin2,
        uint8_t pin3,
        uint8_t pin4,
        uint8_t zeroPin = EasyStepperOutputT<GenericStepperProfile>::PIN_NONE,
        uint8_t zeroActiveState = LOW
    ) : EasyStepperOutputT<GenericStepperProfile>(
        address,
        pin1,
        pin2,
        pin3,
        pin4,
        0.0f,
        360.0f,
        false,
        0.0f,
        GenericStepperProfile::kDefaultMaxRpm,
        GenericStepperProfile::kDefaultAccelRpmPerSec,
        zeroPin,
        zeroActiveState,
        GenericStepperProfile::kDefaultHomeDirection,
        0.0f,
        65535,
        false
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
        uint8_t zeroPin = EasyStepperOutputT<Stepper28Byj48Profile>::PIN_NONE,
        uint8_t zeroActiveState = LOW
    ) : EasyStepperOutputT<GenericStepperProfile>(
        address,
        mask,
        shift,
        pin1,
        pin2,
        pin3,
        pin4,
        0.0f,
        360.0f,
        false,
        0.0f,
        GenericStepperProfile::kDefaultMaxRpm,
        GenericStepperProfile::kDefaultAccelRpmPerSec,
        zeroPin,
        zeroActiveState,
        GenericStepperProfile::kDefaultHomeDirection,
        0.0f,
        65535,
        false
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
        uint8_t zeroActiveState = LOW
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
        zeroActiveState,
        GenericStepperProfile::kDefaultHomeDirection,
        0.0f,
        360,
        false
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
        uint8_t zeroPin,
        uint8_t zeroActiveState
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
        zeroActiveState,
        GenericStepperProfile::kDefaultHomeDirection,
        0.0f,
        360,
        false
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
        uint8_t pin4,
        uint8_t zeroPin,
        uint8_t zeroActiveState
    ) : EasyStepperOutputT<Stepper28Byj48Profile>(
        address,
        pin1,
        pin2,
        pin3,
        pin4,
        0.0f,
        360.0f,
        false,
        0.0f,
        Stepper28Byj48Profile::kDefaultMaxRpm,
        Stepper28Byj48Profile::kDefaultAccelRpmPerSec,
        zeroPin,
        zeroActiveState,
        Stepper28Byj48Profile::kDefaultHomeDirection,
        0.0f,
        65535U,
        false
    ) {
    }

    EasyStepper_28BYJ48(
        unsigned int address,
        unsigned int mask,
        unsigned char shift,
        uint8_t pin1,
        uint8_t pin2,
        uint8_t pin3,
        uint8_t pin4,
        uint8_t zeroPin,
        uint8_t zeroActiveState
    ) : EasyStepperOutputT<Stepper28Byj48Profile>(
        address,
        mask,
        shift,
        pin1,
        pin2,
        pin3,
        pin4,
        0.0f,
        360.0f,
        false,
        0.0f,
        Stepper28Byj48Profile::kDefaultMaxRpm,
        Stepper28Byj48Profile::kDefaultAccelRpmPerSec,
        zeroPin,
        zeroActiveState,
        Stepper28Byj48Profile::kDefaultHomeDirection,
        0.0f,
        65535U,
        false
    ) {
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
        uint8_t zeroActiveState = LOW
    ) : EasyStepperOutputT<Stepper28Byj48Profile>(
        address,
        pin1,
        pin2,
        pin3,
        pin4,
        0.0f,
        360.0f,
        false,
        0.0f,
        Stepper28Byj48Profile::kDefaultMaxRpm,
        Stepper28Byj48Profile::kDefaultAccelRpmPerSec,
        zeroPin,
        zeroActiveState,
        Stepper28Byj48Profile::kDefaultHomeDirection,
        0.0f,
        65535,
        false
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
        uint8_t zeroPin,
        uint8_t zeroActiveState
    ) : EasyStepperOutputT<Stepper28Byj48Profile>(
        address,
        mask,
        shift,
        pin1,
        pin2,
        pin3,
        pin4,
        0.0f,
        360.0f,
        false,
        0.0f,
        Stepper28Byj48Profile::kDefaultMaxRpm,
        Stepper28Byj48Profile::kDefaultAccelRpmPerSec,
        zeroPin,
        zeroActiveState,
        Stepper28Byj48Profile::kDefaultHomeDirection,
        0.0f,
        65535,
        false
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
        uint8_t zeroActiveState = LOW
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
        zeroActiveState,
        Stepper28Byj48Profile::kDefaultHomeDirection,
        0.0f,
        360,
        false
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
        uint8_t zeroPin,
        uint8_t zeroActiveState
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
        zeroActiveState,
        Stepper28Byj48Profile::kDefaultHomeDirection,
        0.0f,
        360,
        false
    ) {
        this->configureContinuousBehavior(true, true, true);
    }
};

// Manual stepper class that doesn't subscribe to DCS-BIOS, allows manual position setting
template<typename ProfileT>
class EasyStepper_Manual {
public:
    static constexpr uint8_t PIN_NONE = 0xFF;
    static constexpr long kDefaultHomingBackoffSteps = 100L;
    typedef void (*FaultCallback)(
        unsigned int address,
        unsigned long serviceGapUs,
        unsigned long allowedGapUs
    );

private:
    enum HomeState {
        HOME_NONE,
        HOME_START_OFFSET,
        HOME_COARSE_SEEK_SWITCH,
        HOME_RELEASE_SWITCH,
        HOME_CLEAR_SWITCH,
        HOME_FINE_SEEK_SWITCH,
        HOME_STOP_AT_ZERO,
        HOME_FAILED,
        HOME_DONE
    };

    AccelStepper stepper_;
    float maxRpm_;
    FaultCallback faultCallback_;
    bool timingFaultLatched_;
    float faultToleranceMultiplier_;
    unsigned long lastServiceUs_;
    unsigned long lastExpectedStepIntervalUs_;

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
    uint8_t zeroActiveState_;
    int8_t homeDirection_;
    float zeroOffsetDeg_;
    long homingStartOffsetSteps_;
    long homingBackoffSteps_;
    long homingReferencePosition_;
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

    bool isZeroActive() const {
        if (zeroPin_ == PIN_NONE) return false;
        int value = digitalRead(zeroPin_);
        return value == zeroActiveState_;
    }

    long angleDegToSteps(float angleDeg) const {
        return roundToLong((angleDeg / 360.0f) * (float)ProfileT::kStepsPerOutputRev);
    }

    long homingOffsetDegToSteps(float angleDeg) const {
        return angleDegToSteps(angleDeg) * ProfileT::kClockwiseStepSign;
    }

    float fineHomingRpm() const {
        float rpm = maxRpm_ * 0.5f;
        return (rpm > 0.0f) ? rpm : ProfileT::kDefaultHomingRpm;
    }

    long zeroOffsetSteps() const {
        return angleDegToSteps(zeroOffsetDeg_);
    }

    long signedHomeDirection() const {
        return (homeDirection_ < 0) ? -1L : 1L;
    }

    bool isCoarseZeroActive() const {
        return isZeroActive();
    }

    bool isFineZeroActive() const {
        return isZeroActive();
    }

    long homingSeekTravelSteps() const {
        return ProfileT::kStepsPerOutputRev * 10000L;
    }

    static long stepMagnitude(long steps) {
        return (steps < 0L) ? -steps : steps;
    }

    void setHomingMaxRpm(float rpm) {
        stepper_.setMaxSpeed(rpmToStepsPerSecond(rpm));
    }

    void moveInHomingDirection(long direction, long steps) {
        long distance = stepMagnitude(steps);
        long offset = (direction < 0L) ? -distance : distance;
        stepper_.moveTo(stepper_.currentPosition() + offset);
    }

    void moveTowardSwitch(long steps) {
        moveInHomingDirection(signedHomeDirection(), steps);
    }

    void moveAwayFromSwitch(long steps) {
        moveInHomingDirection(-signedHomeDirection(), steps);
    }

    void startHomingSeek() {
        setHomingMaxRpm(maxRpm_);
        if (isCoarseZeroActive()) {
            startReleaseFromSwitch();
        } else {
            moveTowardSwitch(homingSeekTravelSteps());
            homeState_ = HOME_COARSE_SEEK_SWITCH;
        }
    }

    void startReleaseFromSwitch() {
        setHomingMaxRpm(maxRpm_);
        moveAwayFromSwitch(homingSeekTravelSteps());
        homeState_ = HOME_RELEASE_SWITCH;
    }

    void startClearanceFromSwitch() {
        if (homingBackoffSteps_ <= 0L) {
            startFineSeekSwitch();
            return;
        }

        setHomingMaxRpm(maxRpm_);
        moveAwayFromSwitch(homingBackoffSteps_);
        homeState_ = HOME_CLEAR_SWITCH;
    }

    void startFineSeekSwitch() {
        setHomingMaxRpm(fineHomingRpm());
        moveTowardSwitch(homingSeekTravelSteps());
        homeState_ = HOME_FINE_SEEK_SWITCH;
    }

    void startStopAtZero() {
        homingReferencePosition_ = stepper_.currentPosition();
        stepper_.stop();
        homeState_ = HOME_STOP_AT_ZERO;
    }

    void finishHoming() {
        long stoppedDeltaSteps = stepper_.currentPosition() - homingReferencePosition_;
        stepper_.setCurrentPosition(zeroOffsetSteps() + stoppedDeltaSteps);
        setHomingMaxRpm(maxRpm_);
        homeState_ = HOME_DONE;
    }

    void failHoming() {
        stepper_.setSpeed(0.0f);
        stepper_.stop();
        homeState_ = HOME_FAILED;
    }

    void startHomingWithOffset(long startOffsetSteps) {
        if (zeroPin_ == PIN_NONE) return;
        if (startOffsetSteps == 0L) {
            startHomingSeek();
            return;
        }

        stepper_.move(startOffsetSteps);
        homeState_ = HOME_START_OFFSET;
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
        } else {
            float targetAngleDeg = minAngleDeg_ + ((maxAngleDeg_ - minAngleDeg_) * ((float)raw / (float)inputMaxValue_));
            if (reverse_) targetAngleDeg = -targetAngleDeg;
            targetAngleDeg += trimDeg_;
            return angleDegToSteps(targetAngleDeg) + zeroOffsetSteps();
        }
    }

    long rawToContinuousSteps(unsigned int raw) const {
        float targetAngleDeg = ((float)raw / (float)inputMaxValue_) * 360.0f;
        if (reverse_) targetAngleDeg = -targetAngleDeg;
        targetAngleDeg += trimDeg_;
        return angleDegToSteps(targetAngleDeg) + zeroOffsetSteps();
    }

    long rawToSteps(unsigned int raw) const {
        if (continuous_) {
            return rawToContinuousSteps(raw);
        } else {
            return rawToBoundedSteps(raw);
        }
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
        uint8_t zeroActiveState,
        int8_t homeDirection,
        float zeroOffsetDeg,
        unsigned int inputMaxValue
    ) {
        continuous_ = continuous;
        minAngleDeg_ = minAngleDeg;
        maxAngleDeg_ = maxAngleDeg;
        trimDeg_ = trimDeg;
        reverse_ = reverse;
        inputMaxValue_ = inputMaxValue;
        inputZeroCentered_ = false; // For manual, assume not centered unless specified

        zeroPin_ = zeroPin;
        zeroActiveState_ = (zeroActiveState == HIGH) ? HIGH : LOW;
        homeDirection_ = (homeDirection < 0) ? -1 : 1;
        zeroOffsetDeg_ = zeroOffsetDeg;
        homingStartOffsetSteps_ = 0L;
        homingBackoffSteps_ = kDefaultHomingBackoffSteps;
        homingReferencePosition_ = 0L;
        homeState_ = (zeroPin_ == PIN_NONE) ? HOME_DONE : HOME_NONE;

        maxRpm_ = maxRpm;
        stepper_.setMaxSpeed(rpmToStepsPerSecond(maxRpm_));
        stepper_.setAcceleration(accelRpmPerSecToStepsPerSec2(accelRpmPerSec));

        faultCallback_ = nullptr;
        timingFaultLatched_ = false;
        faultToleranceMultiplier_ = 1.0f;
        lastServiceUs_ = 0UL;
        lastExpectedStepIntervalUs_ = 0UL;

        if (zeroPin_ != PIN_NONE) {
            pinMode(zeroPin_, INPUT_PULLUP);
        }

        setContinuousBehaviorFlags(false, false, false);
    }

    void setContinuousBehaviorFlags(bool useModulo, bool useShortestPath, bool inputIsAngle) {
        continuousUseModulo_ = useModulo;
        continuousUseShortestPath_ = useShortestPath;
        continuousInputIsAngle_ = inputIsAngle;
    }

    void serviceStepper() {
        unsigned long nowUs = micros();
        unsigned long intervalUs = nowUs - lastServiceUs_;
        lastServiceUs_ = nowUs;

        stepper_.run();

        if (faultCallback_ != nullptr && !timingFaultLatched_) {
            unsigned long expectedIntervalUs = stepsPerSecondToIntervalUs(stepper_.speed());
            if (expectedIntervalUs > 0UL) {
                lastExpectedStepIntervalUs_ = expectedIntervalUs;
                unsigned long allowedGapUs = (unsigned long)lroundf((float)expectedIntervalUs * faultToleranceMultiplier_);
                if (intervalUs > allowedGapUs) {
                    timingFaultLatched_ = true;
                    faultCallback_(0, intervalUs, allowedGapUs); // address 0 for manual
                }
            }
        }
    }

    void serviceHoming() {
        if (homeState_ == HOME_NONE || homeState_ == HOME_DONE || homeState_ == HOME_FAILED) return;

        if (homeState_ == HOME_START_OFFSET) {
            if (stepper_.distanceToGo() != 0L) {
                stepper_.run();
                return;
            }

            startHomingSeek();
        }

        if (homeState_ == HOME_COARSE_SEEK_SWITCH) {
            if (isCoarseZeroActive()) {
                startReleaseFromSwitch();
            } else if (stepper_.distanceToGo() == 0L) {
                failHoming();
                return;
            } else {
                stepper_.run();
                return;
            }
        }

        if (homeState_ == HOME_RELEASE_SWITCH) {
            if (!isCoarseZeroActive()) {
                startClearanceFromSwitch();
            } else if (stepper_.distanceToGo() == 0L) {
                failHoming();
                return;
            } else {
                stepper_.run();
                return;
            }
        }

        if (homeState_ == HOME_CLEAR_SWITCH) {
            if (stepper_.distanceToGo() != 0L) {
                stepper_.run();
                return;
            }

            startFineSeekSwitch();
        }

        if (homeState_ == HOME_FINE_SEEK_SWITCH) {
            if (isFineZeroActive()) {
                startStopAtZero();
            } else if (stepper_.distanceToGo() == 0L) {
                failHoming();
                return;
            } else {
                stepper_.run();
                return;
            }
        }

        if (homeState_ == HOME_STOP_AT_ZERO) {
            if (stepper_.distanceToGo() != 0L) {
                stepper_.run();
                return;
            }

            finishHoming();
        }
    }

public:
    EasyStepper_Manual(
        uint8_t pin1,
        uint8_t pin2,
        uint8_t pin3,
        uint8_t pin4,
        uint8_t zeroPin = PIN_NONE,
        uint8_t zeroActiveState = LOW,
        unsigned int inputMaxValue = 65535
    ) : stepper_(
        ProfileT::kInterface,
        pin1,
        ProfileT::kSwapMiddlePins ? pin3 : pin2,
        ProfileT::kSwapMiddlePins ? pin2 : pin3,
        pin4
    ) {
        commonInit(
            false, // bounded
            0.0f,
            360.0f,
            false,
            0.0f,
            ProfileT::kDefaultMaxRpm,
            ProfileT::kDefaultAccelRpmPerSec,
            zeroPin,
            zeroActiveState,
            ProfileT::kDefaultHomeDirection,
            0.0f,
            inputMaxValue
        );
    }

    void setPosition(unsigned int value) {
        if (homeState_ != HOME_DONE) return; // only set position after homing

        long targetSteps = rawToSteps(value);
        if (continuous_ && continuousUseShortestPath_) {
            long modulus = angleDegToSteps(360.0f);
            targetSteps = chooseNearestEquivalent(stepper_.currentPosition(), targetSteps, modulus);
        }
        stepper_.moveTo(targetSteps);
    }

    void loop() {
        serviceHoming();
        serviceStepper();
    }

    void setMaxRpm(float maxRpm) {
        maxRpm_ = maxRpm;
        stepper_.setMaxSpeed(rpmToStepsPerSecond(maxRpm_));
    }

    void setAccelRpmPerSec(float accelRpmPerSec) {
        stepper_.setAcceleration(accelRpmPerSecToStepsPerSec2(accelRpmPerSec));
    }

    void setReverse(bool reverse) {
        reverse_ = reverse;
    }

    void setTrimDeg(float trimDeg) {
        trimDeg_ = trimDeg;
    }

    void setFaultCallback(FaultCallback callback, float faultToleranceMultiplier = 1.0f) {
        faultCallback_ = callback;
        faultToleranceMultiplier_ = faultToleranceMultiplier;
        timingFaultLatched_ = false;
    }

    void home() {
        startHomingWithOffset(homingStartOffsetSteps_);
    }

    void home(long startOffsetSteps) {
        startHomingWithOffset(startOffsetSteps);
    }

    void homeDeg(float startOffsetDeg) {
        home(homingOffsetDegToSteps(startOffsetDeg));
    }

    void setHomingBackoffSteps(long backoffSteps) {
        homingBackoffSteps_ = (backoffSteps < 0L) ? -backoffSteps : backoffSteps;
    }

    void setHomingBackoffDeg(float backoffDeg) {
        setHomingBackoffSteps(angleDegToSteps(backoffDeg));
    }

    void setHomingBackoffDegs(float backoffDeg) {
        setHomingBackoffDeg(backoffDeg);
    }

    void setHomingStartOffsetSteps(long startOffsetSteps) {
        homingStartOffsetSteps_ = startOffsetSteps;
    }

    void setHomingStartOffsetDeg(float startOffsetDeg) {
        setHomingStartOffsetSteps(homingOffsetDegToSteps(startOffsetDeg));
    }

    void setHomingStartOffsetDegs(float startOffsetDeg) {
        setHomingStartOffsetDeg(startOffsetDeg);
    }

    bool isHomed() const {
        return homeState_ == HOME_DONE;
    }

    bool hasHomingFault() const {
        return homeState_ == HOME_FAILED;
    }

    void zeroInMiddle() {
        inputZeroCentered_ = true;
        if (!continuous_) {
            minAngleDeg_ = -180.0f;
            maxAngleDeg_ = 180.0f;
        }
    }

    void zeroAtStart() {
        inputZeroCentered_ = false;
        if (!continuous_) {
            minAngleDeg_ = 0.0f;
            maxAngleDeg_ = 360.0f;
        }
    }

    void wrapAround() {
        configureContinuousBehavior(true, true, false);
    }

    void configureContinuousBehavior(bool useModulo, bool useShortestPath, bool inputIsAngle) {
        continuous_ = true;
        setContinuousBehaviorFlags(useModulo, useShortestPath, inputIsAngle);
    }

    void configureBoundedBehavior(float minAngleDeg, float maxAngleDeg) {
        continuous_ = false;
        minAngleDeg_ = minAngleDeg;
        maxAngleDeg_ = maxAngleDeg;
    }
};

} // namespace DcsBios

#endif
