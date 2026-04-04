#ifndef __DCSBIOS_EASY_INPUTS_H
#define __DCSBIOS_EASY_INPUTS_H

#ifndef __DCSBIOS_EASY_MODE_H
#error Do not call DcsBiosEasyInputs.h directly. Include DcsBiosEasyMode.h instead.
#endif

namespace DcsBios {

class EasyModeFirstPacketListener : public ExportStreamListener {
private:
    volatile bool hasSeenPacket_ = false;

public:
    EasyModeFirstPacketListener() : ExportStreamListener(0x0000, 0xFFFF) {}

    virtual void onDcsBiosWrite(unsigned int, unsigned int) override {
        hasSeenPacket_ = true;
    }

    bool hasSeenPacket() const {
        return hasSeenPacket_;
    }
};

inline EasyModeFirstPacketListener& easyModeFirstPacketListener() {
    static EasyModeFirstPacketListener listener;
    return listener;
}

template <unsigned long pollIntervalMs = POLL_EVERY_TIME>
class EasyModeSwitch2PosT : PollingInput, public ResettableInput {
private:
    const char* msg_;
    char pin_;
    char lastState_;
    char steadyState_;
    bool reverse_;
    unsigned long debounceDelay_;
    unsigned long lastDebounceTime_ = 0;

    bool initialSyncDone_ = false;

    char readState() const {
        char state = digitalRead(pin_);
        if (reverse_) state = !state;
        return state;
    }

    void resetState() {
        lastState_ = (lastState_ == 0) ? -1 : 0;
        steadyState_ = lastState_;
    }

    void pollInput() {
        if (!initialSyncDone_ && easyModeFirstPacketListener().hasSeenPacket()) {
            char hardwareState = readState();
            if (tryToSendDcsBiosMessage(msg_, hardwareState == HIGH ? "0" : "1")) {
                steadyState_ = hardwareState;
                lastState_ = hardwareState;
                initialSyncDone_ = true;
            }
            return;
        }

        char state = readState();
        unsigned long now = millis();
        if (state != lastState_) {
            lastDebounceTime_ = now;
        }

        if ((now - lastDebounceTime_) >= debounceDelay_) {
            if (state != steadyState_) {
                if (tryToSendDcsBiosMessage(msg_, state == HIGH ? "0" : "1")) {
                    steadyState_ = state;
                }
            }
        }

        lastState_ = state;
    }

public:
    EasyModeSwitch2PosT(
        const char* msg,
        char pin,
        bool reverse = false,
        unsigned long debounceDelay = 50
    ) :
        PollingInput(pollIntervalMs),
        msg_(msg),
        pin_(pin),
        reverse_(reverse),
        debounceDelay_(debounceDelay)
    {
        pinMode(pin_, INPUT_PULLUP);
        lastState_ = readState();
        steadyState_ = lastState_;
        (void)easyModeFirstPacketListener();
    }

    void SetControl(const char* msg) {
        msg_ = msg;
    }

    void resetThisState() {
        this->resetState();
    }

};

using EasyModeSwitch2Pos = EasyModeSwitch2PosT<>;

template <unsigned long pollIntervalMs = POLL_EVERY_TIME>
class EasyModeSwitch3PosT : PollingInput, public ResettableInput {
private:
    const char* msg_;
    char pinA_;
    char pinB_;
    char lastState_;
    char debounceSteadyState_;
    unsigned long debounceDelay_;
    unsigned long lastDebounceTime_ = 0;

    bool initialSyncDone_ = false;

    char readState() const {
        if (digitalRead(pinA_) == LOW) return 0;
        if (digitalRead(pinB_) == LOW) return 2;
        return 1;
    }

    void resetState() {
        lastState_ = (lastState_ == 0) ? -1 : 0;
    }

    void pollInput() {
        if (!initialSyncDone_ && easyModeFirstPacketListener().hasSeenPacket()) {
            char hardwareState = readState();
            const char* value = hardwareState == 0 ? "0" : (hardwareState == 1 ? "1" : "2");
            if (tryToSendDcsBiosMessage(msg_, value)) {
                lastState_ = hardwareState;
                debounceSteadyState_ = hardwareState;
                initialSyncDone_ = true;
            }
            return;
        }

        char state = readState();
        unsigned long now = millis();
        if (state != debounceSteadyState_) {
            lastDebounceTime_ = now;
            debounceSteadyState_ = state;
        }

        if ((now - lastDebounceTime_) >= debounceDelay_) {
            if (state != lastState_) {
                if (state == 0) {
                    if (tryToSendDcsBiosMessage(msg_, "0")) lastState_ = state;
                } else if (state == 1) {
                    if (tryToSendDcsBiosMessage(msg_, "1")) lastState_ = state;
                } else if (state == 2) {
                    if (tryToSendDcsBiosMessage(msg_, "2")) lastState_ = state;
                }
            }
        }
    }

public:
    EasyModeSwitch3PosT(
        const char* msg,
        char pinA,
        char pinB,
        unsigned long debounceDelay = 50
    ) :
        PollingInput(pollIntervalMs),
        msg_(msg),
        pinA_(pinA),
        pinB_(pinB),
        lastState_(0),
        debounceSteadyState_(0),
        debounceDelay_(debounceDelay)
    {
        pinMode(pinA_, INPUT_PULLUP);
        pinMode(pinB_, INPUT_PULLUP);
        lastState_ = readState();
        debounceSteadyState_ = lastState_;
        (void)easyModeFirstPacketListener();
    }

    void SetControl(const char* msg) {
        msg_ = msg;
    }

    void resetThisState() {
        this->resetState();
    }

};

using EasyModeSwitch3Pos = EasyModeSwitch3PosT<>;

template <unsigned long pollIntervalMs = POLL_EVERY_TIME>
class EasyModeSwitchMultiPosT : PollingInput, public ResettableInput {
private:
    const char* msg_;
    const byte* pins_;
    char numberOfPins_;
    char lastState_;
    bool reverse_;

    bool initialSyncDone_ = false;

    char readState() const {
        unsigned char ncPinIdx = lastState_;
        for (unsigned char i = 0; i < numberOfPins_; i++) {
            if (pins_[i] == PIN_NC) {
                ncPinIdx = i;
            } else if (!reverse_ && digitalRead(pins_[i]) == LOW) {
                return i;
            } else if (reverse_ && digitalRead(pins_[i]) == HIGH) {
                return i;
            }
        }
        return ncPinIdx;
    }

    void resetState() {
        lastState_ = (lastState_ == 0) ? -1 : 0;
    }

    void pollInput() {
        if (!initialSyncDone_ && easyModeFirstPacketListener().hasSeenPacket()) {
            char hardwareState = readState();
            char buf[7];
            utoa(hardwareState, buf, 10);
            if (tryToSendDcsBiosMessage(msg_, buf)) {
                lastState_ = hardwareState;
                initialSyncDone_ = true;
            }
            return;
        }

        char state = readState();
        if (state != lastState_) {
            char buf[7];
            utoa(state, buf, 10);
            if (tryToSendDcsBiosMessage(msg_, buf)) {
                lastState_ = state;
            }
        }
    }

public:
    EasyModeSwitchMultiPosT(
        const char* msg,
        const byte* pins,
        char numberOfPins,
        bool reverse = false
    ) :
        PollingInput(pollIntervalMs),
        msg_(msg),
        pins_(pins),
        numberOfPins_(numberOfPins),
        lastState_(0),
        reverse_(reverse)
    {
        for (unsigned char i = 0; i < numberOfPins_; i++) {
            if (pins_[i] != PIN_NC) {
                pinMode(pins_[i], INPUT_PULLUP);
            }
        }
        lastState_ = readState();
        (void)easyModeFirstPacketListener();
    }

    void SetControl(const char* msg) {
        msg_ = msg;
    }

    void resetThisState() {
        this->resetState();
    }

};

using EasyModeSwitchMultiPos = EasyModeSwitchMultiPosT<>;

template <unsigned long pollIntervalMs = POLL_EVERY_TIME>
class EasyModeAnalogMultiPosT : PollingInput, public ResettableInput {
private:
    const char* msg_;
    char pin_;
    unsigned char numOfSteps_;
    unsigned char lastState_;
    unsigned long periodMs_ = 750;
    unsigned long lastPollMs_ = 0;

    bool initialSyncDone_ = false;

    unsigned char readState() const {
        return map(analogRead(pin_), 0, 1023, 0, numOfSteps_);
    }

    void resetState() {
        lastState_ = (lastState_ == 0) ? -1 : 0;
    }

    void pollInput() {
        if (!initialSyncDone_ && easyModeFirstPacketListener().hasSeenPacket()) {
            unsigned char hardwareState = readState();
            char cstr[5];
            itoa(hardwareState, cstr, 10);
            if (tryToSendDcsBiosMessage(msg_, cstr)) {
                lastState_ = hardwareState;
                initialSyncDone_ = true;
            }
            return;
        }

        unsigned long now = millis();
        if (now <= lastPollMs_ + periodMs_) return;

        unsigned char state = readState();
        lastPollMs_ = now;
        if (state != lastState_) {
            char cstr[5];
            itoa(state, cstr, 10);
            if (tryToSendDcsBiosMessage(msg_, cstr)) {
                lastState_ = state;
            }
        }
    }

public:
    EasyModeAnalogMultiPosT(
        const char* msg,
        char pin,
        char numOfSteps
    ) :
        PollingInput(pollIntervalMs),
        msg_(msg),
        pin_(pin),
        numOfSteps_(numOfSteps),
        lastState_(0)
    {
        lastState_ = readState();
        (void)easyModeFirstPacketListener();
    }

    void SetControl(const char* msg) {
        msg_ = msg;
    }

    void resetThisState() {
        this->resetState();
    }

};

using EasyModeAnalogMultiPos = EasyModeAnalogMultiPosT<>;

template <unsigned long pollIntervalMs = POLL_EVERY_TIME, StepsPerDetent stepsPerDetent = ONE_STEP_PER_DETENT>
class EasyModeRotarySwitchT : PollingInput, public ResettableInput {
private:
    const char* msg_;
    char pinA_;
    char pinB_;
    signed char switchValue_;
    signed char maxSwitchValue_;
    char lastState_;
    signed char delta_;

    bool initialSyncDone_ = false;

    char readState() const {
        return (digitalRead(pinA_) << 1) | digitalRead(pinB_);
    }

    void resetState() {
        lastState_ = (lastState_ == 0) ? -1 : 0;
    }

    void pollInput() {
        if (!initialSyncDone_ && easyModeFirstPacketListener().hasSeenPacket()) {
            char buf[7];
            utoa(switchValue_, buf, 10);
            if (tryToSendDcsBiosMessage(msg_, buf)) {
                initialSyncDone_ = true;
            }
            return;
        }

        char state = readState();
        switch (lastState_) {
            case 0:
                if (state == 2) delta_--;
                if (state == 1) delta_++;
                break;
            case 1:
                if (state == 0) delta_--;
                if (state == 3) delta_++;
                break;
            case 2:
                if (state == 3) delta_--;
                if (state == 0) delta_++;
                break;
            case 3:
                if (state == 1) delta_--;
                if (state == 2) delta_++;
                break;
        }
        lastState_ = state;

        if (delta_ >= stepsPerDetent) {
            switchValue_ = min((signed char)(switchValue_ + 1), maxSwitchValue_);
            char buf[7];
            utoa(switchValue_, buf, 10);
            if (tryToSendDcsBiosMessage(msg_, buf)) {
                delta_ -= stepsPerDetent;
            }
        }
        if (delta_ <= -stepsPerDetent) {
            switchValue_ = max((signed char)(switchValue_ - 1), (signed char)0);
            char buf[7];
            utoa(switchValue_, buf, 10);
            if (tryToSendDcsBiosMessage(msg_, buf)) {
                delta_ += stepsPerDetent;
            }
        }
    }

public:
    EasyModeRotarySwitchT(
        const char* msg,
        char pinA,
        char pinB,
        signed char maxSwitchValue
    ) :
        PollingInput(pollIntervalMs),
        msg_(msg),
        pinA_(pinA),
        pinB_(pinB),
        switchValue_(0),
        maxSwitchValue_(maxSwitchValue),
        lastState_(0),
        delta_(0)
    {
        pinMode(pinA_, INPUT_PULLUP);
        pinMode(pinB_, INPUT_PULLUP);
        lastState_ = readState();
        (void)easyModeFirstPacketListener();
    }

    void SetControl(const char* msg) {
        msg_ = msg;
    }

    void resetThisState() {
        this->resetState();
    }

};

template <unsigned long pollIntervalMs = POLL_EVERY_TIME, StepsPerDetent stepsPerDetent = ONE_STEP_PER_DETENT>
using EasyModeRotarySwitch = EasyModeRotarySwitchT<pollIntervalMs, stepsPerDetent>;

template <unsigned long pollIntervalMs = POLL_EVERY_TIME, bool invert = false>
using EasyModeRotarySyncingPotentiometerT = RotarySyncingPotentiometerEWMA<pollIntervalMs, invert>;

using EasyModeRotarySyncingPotentiometer = EasyModeRotarySyncingPotentiometerT<>;
using EasyModeInvertedRotarySyncingPotentiometer = EasyModeRotarySyncingPotentiometerT<POLL_EVERY_TIME, true>;

} // namespace DcsBios

#endif
