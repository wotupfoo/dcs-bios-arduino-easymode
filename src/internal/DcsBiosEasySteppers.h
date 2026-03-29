#ifndef __DCSBIOS_EASY_STEPPERS_H
#define __DCSBIOS_EASY_STEPPERS_H

#include <Arduino.h>
#include <AccelStepper.h>
#include "DcsBios.h"

namespace DcsBios {

template<long STEPS_PER_OUTPUT_REV,
         uint8_t ACCELSTEPPER_INTERFACE,
         float DEFAULT_MAX_SPEED_STEPS_PER_SEC,
         float DEFAULT_ACCEL_STEPS_PER_SEC2>
struct StepperProfile {
    static constexpr long  kStepsPerOutputRev = STEPS_PER_OUTPUT_REV;
    static constexpr uint8_t kInterface = ACCELSTEPPER_INTERFACE;
    static constexpr float kDefaultMaxSpeed = DEFAULT_MAX_SPEED_STEPS_PER_SEC;
    static constexpr float kDefaultAccel = DEFAULT_ACCEL_STEPS_PER_SEC2;
};

// Common 28BYJ-48 easy-mode profile.
// Using 4096 half-steps/output rev as the familiar hobby default.
using Stepper28Byj48Profile = StepperProfile<
    4096,
    AccelStepper::HALF4WIRE,
    600.0f,
    300.0f
>;

template<typename ProfileT>
class EasyStepperOutputT : public Int16Buffer {
public:
    static constexpr uint8_t PIN_NONE = 0xFF;

private:
    AccelStepper stepper_;

    float minAngleDeg_;
    float maxAngleDeg_;
    float trimDeg_;
    bool reverse_;

    uint8_t zeroPin_;
    bool zeroActiveLow_;
    bool homingEnabled_;
    bool homed_;
    bool homeBackoffDone_;
    float homingSpeedStepsPerSec_;
    long homingBackoffSteps_;
    long zeroOffsetSteps_;

    bool isZeroActive() const {
        if (zeroPin_ == PIN_NONE) return false;
        int v = digitalRead(zeroPin_);
        return zeroActiveLow_ ? (v == LOW) : (v == HIGH);
    }

    long angleDegToSteps(float angleDeg) const {
        return lroundf((angleDeg / 360.0f) * (float)ProfileT::kStepsPerOutputRev);
    }

    float rawToAngleDeg(unsigned int raw) const {
        float outMin = minAngleDeg_ + trimDeg_;
        float outMax = maxAngleDeg_ + trimDeg_;

        if (reverse_) {
            float t = outMin;
            outMin = outMax;
            outMax = t;
        }

        return outMin + ((outMax - outMin) * ((float)raw / 65535.0f));
    }

    void runHoming() {
        if (!homingEnabled_ || homed_) return;

        // First seek the switch slowly in the negative direction.
        if (!isZeroActive() && !homeBackoffDone_) {
            stepper_.setSpeed(-fabs(homingSpeedStepsPerSec_));
            stepper_.runSpeed();
            return;
        }

        // Once the switch hits, back off until it releases.
        if (isZeroActive() && !homeBackoffDone_) {
            homeBackoffDone_ = true;
            stepper_.setSpeed(fabs(homingSpeedStepsPerSec_));
            stepper_.runSpeed();
            return;
        }

        if (homeBackoffDone_) {
            if (isZeroActive()) {
                stepper_.setSpeed(fabs(homingSpeedStepsPerSec_));
                stepper_.runSpeed();
                return;
            }

            // Switch just released: define this as zero reference, plus optional offset.
            stepper_.setCurrentPosition(zeroOffsetSteps_);
            stepper_.moveTo(zeroOffsetSteps_);
            homed_ = true;
        }
    }

public:
    // Generic constructor.
    //
    // For HALF4WIRE motors like 28BYJ-48 on ULN2003, pass pins in natural order:
    // in1, in2, in3, in4
    //
    // The constructor internally swaps pin2/pin3 for AccelStepper when using HALF4WIRE,
    // because that's the common 28BYJ-48/ULN2003 quirk.
    EasyStepperOutputT(
        unsigned int address,
        uint8_t pin1,
        uint8_t pin2,
        uint8_t pin3,
        uint8_t pin4,
        float minAngleDeg = 0.0f,
        float maxAngleDeg = 360.0f,
        bool reverse = false,
        float trimDeg = 0.0f,
        float maxSpeedStepsPerSec = ProfileT::kDefaultMaxSpeed,
        float accelStepsPerSec2 = ProfileT::kDefaultAccel,
        uint8_t zeroPin = PIN_NONE,
        bool zeroActiveLow = true,
        float homingSpeedStepsPerSec = 150.0f,
        long homingBackoffSteps = 16,
        long zeroOffsetSteps = 0
    )
        : Int16Buffer(address),
          stepper_(
              ProfileT::kInterface,
              pin1,
              (ProfileT::kInterface == AccelStepper::HALF4WIRE) ? pin3 : pin2,
              (ProfileT::kInterface == AccelStepper::HALF4WIRE) ? pin2 : pin3,
              pin4
          ),
          minAngleDeg_(minAngleDeg),
          maxAngleDeg_(maxAngleDeg),
          trimDeg_(trimDeg),
          reverse_(reverse),
          zeroPin_(zeroPin),
          zeroActiveLow_(zeroActiveLow),
          homingEnabled_(zeroPin != PIN_NONE),
          homed_(zeroPin == PIN_NONE),
          homeBackoffDone_(false),
          homingSpeedStepsPerSec_(homingSpeedStepsPerSec),
          homingBackoffSteps_(homingBackoffSteps),
          zeroOffsetSteps_(zeroOffsetSteps)
    {
        stepper_.setMaxSpeed(maxSpeedStepsPerSec);
        stepper_.setAcceleration(accelStepsPerSec2);
        stepper_.setCurrentPosition(0);

        if (zeroPin_ != PIN_NONE) {
            pinMode(zeroPin_, zeroActiveLow_ ? INPUT_PULLUP : INPUT);
        }
    }

    virtual void loop() override {
        if (!homed_) {
            runHoming();
            return;
        }

        if (hasUpdatedData()) {
            float targetAngleDeg = rawToAngleDeg(getData());
            long targetSteps = angleDegToSteps(targetAngleDeg) + zeroOffsetSteps_;
            stepper_.moveTo(targetSteps);
        }

        stepper_.run();
    }

    void startHoming() {
        if (!homingEnabled_) return;
        homed_ = false;
        homeBackoffDone_ = false;
    }

    bool isHomed() const { return homed_; }

    void setZeroOffsetSteps(long zeroOffsetSteps) { zeroOffsetSteps_ = zeroOffsetSteps; }
    void setTrimDeg(float trimDeg) { trimDeg_ = trimDeg; }
    void setMaxSpeedStepsPerSec(float s) { stepper_.setMaxSpeed(s); }
    void setAccelerationStepsPerSec2(float a) { stepper_.setAcceleration(a); }

    void setCurrentPositionSteps(long pos) { stepper_.setCurrentPosition(pos); }
    long currentPositionSteps() const { return stepper_.currentPosition(); }
    long targetPositionSteps() const { return stepper_.targetPosition(); }
    long distanceToGoSteps() const { return stepper_.distanceToGo(); }
};

// Named easy-mode type for 28BYJ-48
using Easy28Byj48Output = EasyStepperOutputT<Stepper28Byj48Profile>;

} // namespace DcsBios

#endif
