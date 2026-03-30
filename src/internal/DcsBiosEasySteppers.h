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

private:
    enum HomeState {
        HOME_NONE,
        HOME_SEEK_SWITCH,
        HOME_RELEASE_SWITCH,
        HOME_DONE
    };

    AccelStepper stepper_;

    bool continuous_;
    float minAngleDeg_;
    float maxAngleDeg_;
    float trimDeg_;
    bool reverse_;

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

    static long roundToLong(float value) {
        return (long)lroundf(value);
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
        float outMin = minAngleDeg_ + trimDeg_;
        float outMax = maxAngleDeg_ + trimDeg_;

        if (reverse_) {
            float temp = outMin;
            outMin = outMax;
            outMax = temp;
        }

        float targetAngleDeg = outMin + ((outMax - outMin) * ((float)raw / 65535.0f));
        return angleDegToSteps(targetAngleDeg) + zeroOffsetSteps();
    }

    long rawToContinuousSteps(unsigned int raw) const {
        float targetAngleDeg = ((float)raw / 65535.0f) * 360.0f;

        if (reverse_) targetAngleDeg = -targetAngleDeg;
        targetAngleDeg += trimDeg_;

        long normalizedTarget = positiveModulo(
            angleDegToSteps(targetAngleDeg) + zeroOffsetSteps(),
            ProfileT::kStepsPerOutputRev
        );

        return chooseNearestEquivalent(
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
        float zeroOffsetDeg
    ) {
        continuous_ = continuous;
        minAngleDeg_ = minAngleDeg;
        maxAngleDeg_ = maxAngleDeg;
        trimDeg_ = trimDeg;
        reverse_ = reverse;
        zeroPin_ = zeroPin;
        zeroActiveLow_ = zeroActiveLow;
        homeDirection_ = (homeDirection < 0) ? -1 : 1;
        zeroOffsetDeg_ = zeroOffsetDeg;

        stepper_.setMaxSpeed(rpmToStepsPerSecond(maxRpm));
        stepper_.setAcceleration(accelRpmPerSecToStepsPerSec2(accelRpmPerSec));
        stepper_.setCurrentPosition(0);

        if (zeroPin_ == PIN_NONE) {
            homeState_ = HOME_DONE;
        } else {
            pinMode(zeroPin_, zeroActiveLow_ ? INPUT_PULLUP : INPUT);
            homeState_ = isZeroActive() ? HOME_RELEASE_SWITCH : HOME_SEEK_SWITCH;
        }
    }

public:
    /*
     * Easy continuous stepper output.
     *
     * Use this for gauges that can rotate forever, such as a compass or clock.
     * There is no maximum angle. The class automatically chooses the nearest
     * equivalent revolution so the pointer/card crosses zero smoothly.
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
        float zeroOffsetDeg = 0.0f               // Zero Offset Degrees: fine adjustment after homing
    ) : Int16Buffer(address),
        stepper_(
            ProfileT::kInterface,
            pin1,
            ProfileT::kSwapMiddlePins ? pin3 : pin2,
            ProfileT::kSwapMiddlePins ? pin2 : pin3,
            pin4
        ) {
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
            zeroOffsetDeg
        );
    }

    /*
     * Easy bounded stepper output.
     *
     * Use this for gauges that have a start angle and an end angle, such as
     * an altimeter needle or other instrument that does not rotate forever.
     *
     * minAngleDeg and maxAngleDeg set the size of the sweep used by the gauge.
     * trimDeg shifts that whole sweep around the dial face.
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
        float zeroOffsetDeg = 0.0f               // Zero Offset Degrees: fine adjustment after homing
    ) : Int16Buffer(address),
        stepper_(
            ProfileT::kInterface,
            pin1,
            ProfileT::kSwapMiddlePins ? pin3 : pin2,
            ProfileT::kSwapMiddlePins ? pin2 : pin3,
            pin4
        ) {
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
            zeroOffsetDeg
        );
    }

    virtual void loop() override {
        if (homeState_ != HOME_DONE) {
            runHoming();
            return;
        }

        if (hasUpdatedData()) {
            if (continuous_) {
                stepper_.moveTo(rawToContinuousSteps(getData()));
            } else {
                stepper_.moveTo(rawToBoundedSteps(getData()));
            }
        }

        stepper_.run();
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

    void setZeroOffsetDeg(float zeroOffsetDeg) {
        zeroOffsetDeg_ = zeroOffsetDeg;
    }

    void setMaxRpm(float maxRpm) {
        stepper_.setMaxSpeed(rpmToStepsPerSecond(maxRpm));
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

using Easy28Byj48Output = EasyStepperOutputT<Stepper28Byj48Profile>;

} // namespace DcsBios

#endif
