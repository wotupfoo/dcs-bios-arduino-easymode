#ifndef __DCSBIOS_EASY_SERVOS_H
#define __DCSBIOS_EASY_SERVOS_H

#include <Arduino.h>

#ifdef ARDUINO_ARCH_ESP32
#include <ESP32Servo.h>
#else
#include <Servo.h>
#endif

#include "DcsBios.h"

namespace DcsBios {

template<int DEFAULT_MIN_PULSE_US,
         int DEFAULT_MAX_PULSE_US,
         int DEFAULT_SERVO_TRAVEL_DEG>
struct ServoProfile {
    static constexpr int kMinPulseUs = DEFAULT_MIN_PULSE_US;
    static constexpr int kMaxPulseUs = DEFAULT_MAX_PULSE_US;
    static constexpr int kTravelDeg  = DEFAULT_SERVO_TRAVEL_DEG;
};

// Common SG90-style defaults.
// Actual travel varies by unit, but this is a practical easy-mode profile.
using Sg90Profile = ServoProfile<500, 2400, 180>;

template<typename ProfileT>
class EasyServoOutputT : public Int16Buffer {
private:
    Servo servo_;
    char pin_;
    int minAngleDeg_;
    int maxAngleDeg_;
    int trimDeg_;
    bool reverse_;

    static long clampLong(long v, long lo, long hi) {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    unsigned int angleToPulse(long angleDeg) const {
        angleDeg = clampLong(angleDeg, 0, ProfileT::kTravelDeg);
        return (unsigned int)map(
            angleDeg,
            0,
            ProfileT::kTravelDeg,
            ProfileT::kMinPulseUs,
            ProfileT::kMaxPulseUs
        );
    }

public:
    // Easiest form:
    // address, pin, gauge min angle, gauge max angle, reverse, trim
    EasyServoOutputT(
        unsigned int address,
        char pin,
        int minAngleDeg = 0,
        int maxAngleDeg = 120,
        bool reverse = false,
        int trimDeg = 0
    ) : Int16Buffer(address),
        pin_(pin),
        minAngleDeg_(minAngleDeg),
        maxAngleDeg_(maxAngleDeg),
        trimDeg_(trimDeg),
        reverse_(reverse) {
    }

    virtual void loop() override {
        if (!servo_.attached()) {
            servo_.attach(pin_, ProfileT::kMinPulseUs, ProfileT::kMaxPulseUs);
        }

        if (hasUpdatedData()) {
            long outMin = minAngleDeg_ + trimDeg_;
            long outMax = maxAngleDeg_ + trimDeg_;

            if (reverse_) {
                long t = outMin;
                outMin = outMax;
                outMax = t;
            }

            long angleDeg = map((long)getData(), 0L, 65535L, outMin, outMax);
            servo_.writeMicroseconds(angleToPulse(angleDeg));
        }
    }

    void setTrimDeg(int trimDeg) { trimDeg_ = trimDeg; }
};

// Named easy-mode types
using EasySg90ServoOutput = EasyServoOutputT<Sg90Profile>;

} // namespace DcsBios

#endif
