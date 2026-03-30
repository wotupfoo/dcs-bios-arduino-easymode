#ifndef __DCSBIOS_EASY_SERVOS_H
#define __DCSBIOS_EASY_SERVOS_H

namespace DcsBios {

template<int DEFAULT_MIN_PULSE_US,
         int DEFAULT_MAX_PULSE_US,
         int DEFAULT_SERVO_TRAVEL_DEG>
struct ServoProfile {
    static constexpr int kMinPulseUs = DEFAULT_MIN_PULSE_US;
    static constexpr int kMaxPulseUs = DEFAULT_MAX_PULSE_US;
    static constexpr int kTravelDeg  = DEFAULT_SERVO_TRAVEL_DEG;
};

// SG90-style defaults.
// Travel varies a little from unit to unit, but 500..2400us and 180 degrees
// are good easy-mode defaults for cockpit instruments.
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
    /*
     * Easy servo output.
     *
     * The goal is to think in instrument angles instead of PWM pulse widths.
     * You describe where the needle should point on the dial, and the class
     * handles the servo timing for you.
     *
     * minAngleDeg and maxAngleDeg set the size of the sweep used by the gauge.
     * trimDeg shifts that whole sweep around the dial face.
     *
     * trimDeg is useful when the servo's mechanical zero is not the same as
     * the instrument's printed zero.
     *
     * Example: a speedometer needle may physically rest near 6 o'clock when
     * the servo is at its natural zero, but the printed 0 mph mark on the dial
     * may be nearer 7 o'clock. In that case, keep the same sweep size and use
     * trimDeg to rotate the whole working range until the needle lines up with
     * the printed scale.
     *
     * Another common case is when the servo horn only fits on the spline a bit
     * off from the exact angle you wanted. trimDeg lets you correct that in
     * software without rebuilding the gauge.
     */
    EasyServoOutputT(
        unsigned int address,      // DCS World: memory address with the value
        char pin,                  // Arduino pin connected to the servo signal wire
        int minAngleDeg = 0,       // Minimum needle angle in degrees for the lowest DCS-BIOS value
        int maxAngleDeg = 120,     // Maximum needle angle in degrees for the highest DCS-BIOS value
        bool reverse = false,      // Reverse the direction (true or false)
        int trimDeg = 0            // Trim Degrees: rotate the whole scale to match the printed dial face
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

using EasySg90ServoOutput = EasyServoOutputT<Sg90Profile>;

} // namespace DcsBios

#endif
