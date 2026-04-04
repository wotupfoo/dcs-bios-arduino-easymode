#ifndef __DCSBIOS_EASY_SERVOS_H
#define __DCSBIOS_EASY_SERVOS_H

#ifndef __DCSBIOS_EASY_MODE_H
#error Do not call DcsBiosEasyServos.h directly. Include DcsBiosEasyMode.h instead.
#endif

#include <math.h>

#ifdef ARDUINO_ARCH_ESP32
#include <ESP32Servo.h>
#else
#include <Servo.h>
#endif

namespace DcsBios {

template<int DEFAULT_MIN_PULSE_US,
         int DEFAULT_MAX_PULSE_US,
         int DEFAULT_SERVO_TRAVEL_DEG>
struct ServoProfile {
    static constexpr int kMinPulseUs = DEFAULT_MIN_PULSE_US;
    static constexpr int kMaxPulseUs = DEFAULT_MAX_PULSE_US;
    static constexpr int kTravelDeg  = DEFAULT_SERVO_TRAVEL_DEG;
};

// Generic servo defaults.
// These intentionally track the common Arduino Servo defaults so the plain
// EasyServo name feels like the natural first choice.
using GenericServoProfile = ServoProfile<544, 2400, 180>;

// SG90-style defaults.
// Travel varies a little from unit to unit, but 500..2400us and 180 degrees
// are good easy-mode defaults for cockpit instruments.
using Sg90Profile = ServoProfile<500, 2400, 180>;

template<typename ProfileT>
class EasyServoOutputT : public Int16Buffer {
private:
    Servo servo_;
    unsigned int mask_;
    unsigned char shift_;
    char pin_;
    int minAngleDeg_;
    int maxAngleDeg_;
    int trimDeg_;
    bool reverse_;
    unsigned int inputMaxValue_;

    static long clampLong(long v, long lo, long hi) {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    static int roundToInt(float value) {
        return (int)lroundf(value);
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

    unsigned int readSourceValue() {
        return ((this->Int16Buffer::getData()) & mask_) >> shift_;
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
        int maxAngleDeg = ProfileT::kTravelDeg, // Maximum needle angle in degrees for the highest DCS-BIOS value
        bool reverse = false,      // Reverse the direction (true or false)
        int trimDeg = 0,           // Trim Degrees: rotate the whole scale to match the printed dial face
        unsigned int inputMaxValue = 65535 // Maximum incoming DCS-BIOS value for this source
    ) : Int16Buffer(address),
        mask_(0xFFFF),
        shift_(0),
        pin_(pin),
        minAngleDeg_(minAngleDeg),
        maxAngleDeg_(maxAngleDeg),
        trimDeg_(trimDeg),
        reverse_(reverse),
        inputMaxValue_(inputMaxValue ? inputMaxValue : 65535) {
    }

    EasyServoOutputT(
        unsigned int address,      // DCS World: memory address with the value
        unsigned int mask,         // Bit mask for packed integer fields
        unsigned char shift,       // Right shift for packed integer fields
        char pin,                  // Arduino pin connected to the servo signal wire
        int minAngleDeg = 0,       // Minimum needle angle in degrees for the lowest DCS-BIOS value
        int maxAngleDeg = ProfileT::kTravelDeg, // Maximum needle angle in degrees for the highest DCS-BIOS value
        bool reverse = false,      // Reverse the direction (true or false)
        int trimDeg = 0,           // Trim Degrees: rotate the whole scale to match the printed dial face
        unsigned int inputMaxValue = 65535 // Maximum incoming DCS-BIOS value for this source
    ) : Int16Buffer(address),
        mask_(mask),
        shift_(shift),
        pin_(pin),
        minAngleDeg_(minAngleDeg),
        maxAngleDeg_(maxAngleDeg),
        trimDeg_(trimDeg),
        reverse_(reverse),
        inputMaxValue_(inputMaxValue ? inputMaxValue : 65535) {
    }

    virtual void loop() override {
Serial.println("EasyServo loop");
        if (!servo_.attached()) {
            Serial.println("EasyServo attaching");
            servo_.attach(pin_, ProfileT::kMinPulseUs, ProfileT::kMaxPulseUs);
        }

        if (hasUpdatedData()) {
Serial.println("EasyServo has updated data");
            long outMin = minAngleDeg_ + trimDeg_;
            long outMax = maxAngleDeg_ + trimDeg_;

            if (reverse_) {
                long t = outMin;
                outMin = outMax;
                outMax = t;
            }

            long angleDeg = map((long)readSourceValue(), 0L, (long)inputMaxValue_, outMin, outMax);
            Serial.println("EasyServo has updated data");
Serial.print("Setting servo angle to: "); Serial.println(angleDeg);
            servo_.writeMicroseconds(angleToPulse(angleDeg));
        }
    }

    void setMinAngle(float angleDeg) { minAngleDeg_ = roundToInt(angleDeg); }

    void setMaxAngle(float angleDeg) { maxAngleDeg_ = roundToInt(angleDeg); }

    void setDirection(EasyModeDir direction) { reverse_ = (direction == EasyModeDir::CCW); }

    void setDirection(int direction) { reverse_ = (direction == -1); }

    void setInputMaxValue(unsigned int inputMaxValue) {
        inputMaxValue_ = inputMaxValue ? inputMaxValue : 65535;
    }

    void setTrimDeg(int trimDeg) { trimDeg_ = trimDeg; }
};

// Public naming scheme for snippet generators and no-code users:
//   EasyServo        -> generic default servo
//   EasyServo_SG90   -> SG90-specific defaults
using EasyServo = EasyServoOutputT<GenericServoProfile>;

class EasyServo_SG90 : public EasyServoOutputT<Sg90Profile> {
public:
    EasyServo_SG90(
        unsigned int address,
        char pin
    ) : EasyServoOutputT<Sg90Profile>(address, pin) {
    }

    EasyServo_SG90(
        unsigned int address,
        unsigned int mask,
        unsigned char shift,
        char pin
    ) : EasyServoOutputT<Sg90Profile>(address, mask, shift, pin) {
    }
};

// Backwards-compatible aliases.
using EasyServoOutput = EasyServo;
using EasySg90ServoOutput = EasyServoOutputT<Sg90Profile>;

} // namespace DcsBios

#endif
