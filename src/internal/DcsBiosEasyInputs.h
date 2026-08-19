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
    unsigned int inputMin_;
    unsigned int inputMax_;
    unsigned char lastState_ = 0;
    bool hasLastState_ = false;
    unsigned long periodMs_ = 750;
    unsigned long lastPollMs_ = 0;

    bool initialSyncDone_ = false;

    unsigned char readState() const {
        unsigned int raw = getRawValue();

        if (inputMax_ <= inputMin_ || raw <= inputMin_) return 0;
        if (raw >= inputMax_) return numOfSteps_;

        unsigned long span = (unsigned long)(inputMax_ - inputMin_) + 1UL;
        unsigned long mapped = ((unsigned long)(raw - inputMin_) * ((unsigned long)numOfSteps_ + 1UL)) / span;
        if (mapped > numOfSteps_) mapped = numOfSteps_;
        return (unsigned char)mapped;
    }

    void resetState() {
        hasLastState_ = false;
    }

    void pollInput() {
        unsigned long now = millis();
        if (initialSyncDone_ && now <= lastPollMs_ + periodMs_) return;

        unsigned char state = readState();

        if (!initialSyncDone_ && !easyModeFirstPacketListener().hasSeenPacket()) {
            lastState_ = state;
            hasLastState_ = true;
            lastPollMs_ = now;
            return;
        }

        if (!initialSyncDone_ || !hasLastState_ || state != lastState_) {
            char cstr[5];
            itoa(state, cstr, 10);
            if (tryToSendDcsBiosMessage(msg_, cstr)) {
                lastState_ = state;
                hasLastState_ = true;
                initialSyncDone_ = true;
            }
        }

        lastPollMs_ = now;
    }

public:
    EasyModeAnalogMultiPosT(
        const char* msg,
        char pin,
        char numOfSteps,
        unsigned int inputMin = 0,
        unsigned int inputMax = 1023
    ) :
        PollingInput(pollIntervalMs),
        msg_(msg),
        pin_(pin),
        numOfSteps_(numOfSteps),
        inputMin_(inputMin),
        inputMax_(inputMax)
    {
        pinMode(pin_, INPUT);
        (void)easyModeFirstPacketListener();
    }

    void SetControl(const char* msg) {
        msg_ = msg;
    }

    void setMin(unsigned int value) {
        inputMin_ = value;
        resetState();
    }

    void setMax(unsigned int value) {
        inputMax_ = value;
        resetState();
    }

    unsigned int getRawValue() const {
        analogRead(pin_);
        return analogRead(pin_);
    }

    void resetThisState() {
        this->resetState();
    }

};

using EasyModeAnalogMultiPos = EasyModeAnalogMultiPosT<>;

template <unsigned long pollIntervalMs = 5, unsigned int hysteresis = 128, unsigned int ewmaDivisor = 5>
class EasyModePotentiometerT : PollingInput, public ResettableInput {
private:
    const char* msg_;
    char pin_;
    bool reverse_;
    unsigned int inputMin_;
    unsigned int inputMax_;
    unsigned int lastState_ = 0;
    unsigned int filteredState_ = 0;
    bool hasLastState_ = false;
    bool filterInitialized_ = false;
    bool initialSyncDone_ = false;

    unsigned int mapRawToState(unsigned int raw) const {
        if (inputMax_ <= inputMin_) return 0;

        if (raw <= inputMin_) {
            return reverse_ ? 65535U : 0U;
        }

        if (raw >= inputMax_) {
            return reverse_ ? 0U : 65535U;
        }

        unsigned long mapped = ((unsigned long)(raw - inputMin_) * 65535UL) / (unsigned long)(inputMax_ - inputMin_);
        if (reverse_) mapped = 65535UL - mapped;
        return (unsigned int)mapped;
    }

    unsigned int readState() {
        unsigned int state = mapRawToState(getRawValue());

        if (state == 0U || state == 65535U || !filterInitialized_ || ewmaDivisor <= 1) {
            filteredState_ = state;
            filterInitialized_ = true;
            return filteredState_;
        }

        filteredState_ = (unsigned int)((((unsigned long)filteredState_ * (ewmaDivisor - 1)) + state) / ewmaDivisor);
        return filteredState_;
    }

    bool shouldSendState(unsigned int state) const {
        if (!hasLastState_) return true;
        if (state == 0U || state == 65535U) return state != lastState_;
        return abs((long)state - (long)lastState_) >= (long)hysteresis;
    }

    void resetState() {
        hasLastState_ = false;
    }

    void resetCalibrationMapping() {
        filterInitialized_ = false;
        resetState();
    }

    void pollInput() {
        unsigned int state = readState();

        if (!initialSyncDone_ && !easyModeFirstPacketListener().hasSeenPacket()) {
            lastState_ = state;
            hasLastState_ = true;
            return;
        }

        if (!initialSyncDone_ || shouldSendState(state)) {
            char buf[7];
            utoa(state, buf, 10);
            if (tryToSendDcsBiosMessage(msg_, buf)) {
                lastState_ = state;
                hasLastState_ = true;
                initialSyncDone_ = true;
            }
        }
    }

public:
    EasyModePotentiometerT(
        const char* msg,
        char pin,
        bool reverse = false,
        unsigned int inputMin = 0,
        unsigned int inputMax = 1023
    ) :
        PollingInput(pollIntervalMs),
        msg_(msg),
        pin_(pin),
        reverse_(reverse),
        inputMin_(inputMin),
        inputMax_(inputMax)
    {
        pinMode(pin_, INPUT);
        (void)easyModeFirstPacketListener();
    }

    void SetControl(const char* msg) {
        msg_ = msg;
    }

    void setMin(unsigned int value) {
        inputMin_ = value;
        resetCalibrationMapping();
    }

    void setMax(unsigned int value) {
        inputMax_ = value;
        resetCalibrationMapping();
    }

    unsigned int getRawValue() const {
        analogRead(pin_);
        return analogRead(pin_);
    }

    void resetThisState() {
        this->resetState();
    }

};

using EasyModePotentiometer = EasyModePotentiometerT<>;

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
class EasyModeRotarySyncingPotentiometerT : PollingInput, Int16Buffer, public ResettableInput {
private:
    const char* msg_;
    char pin_;
    unsigned int inputMin_;
    unsigned int inputMax_;
    unsigned int lastState_ = 0;
    bool hasLastState_ = false;

    unsigned int mask_;
    unsigned char shift_;
    unsigned int lastDcsData_ = 0;
    bool hasDcsData_ = false;
    unsigned long lastSendTime_ = 0;

    int (*mapperCallback_)(unsigned int, unsigned int);

    unsigned int mapRawToState(unsigned int raw) const {
        if (inputMax_ <= inputMin_) return 0;

        if (raw <= inputMin_) {
            return invert ? 65535U : 0U;
        }

        if (raw >= inputMax_) {
            return invert ? 0U : 65535U;
        }

        unsigned long mapped = ((unsigned long)(raw - inputMin_) * 65535UL) / (unsigned long)(inputMax_ - inputMin_);
        if (invert) mapped = 65535UL - mapped;
        return (unsigned int)mapped;
    }

    unsigned int readState() const {
        return mapRawToState(getRawValue());
    }

    void resetState() {
        hasLastState_ = false;
    }

    void pollInput() {
        lastState_ = readState();
        hasLastState_ = true;
    }

    bool updateDcsData() {
        if (!this->hasUpdatedData()) return hasDcsData_;

        lastDcsData_ = getData();
        hasDcsData_ = true;
        return true;
    }

public:
    EasyModeRotarySyncingPotentiometerT(
        const char* msg,
        char pin,
        unsigned int syncToAddress,
        unsigned int syncToMask,
        unsigned char syncToShift,
        int (*mapperCallback)(unsigned int, unsigned int),
        unsigned int inputMin = 0,
        unsigned int inputMax = 1023
    ) :
        PollingInput(pollIntervalMs),
        Int16Buffer(syncToAddress),
        msg_(msg),
        pin_(pin),
        inputMin_(inputMin),
        inputMax_(inputMax),
        mask_(syncToMask),
        shift_(syncToShift),
        lastSendTime_(millis()),
        mapperCallback_(mapperCallback)
    {
        pinMode(pin_, INPUT);
        (void)easyModeFirstPacketListener();
    }

    void SetControl(const char* msg) {
        msg_ = msg;
    }

    void setMin(unsigned int value) {
        inputMin_ = value;
        resetState();
    }

    void setMax(unsigned int value) {
        inputMax_ = value;
        resetState();
    }

    unsigned int getRawValue() const {
        analogRead(pin_);
        return analogRead(pin_);
    }

    void resetThisState() {
        this->resetState();
    }

    unsigned int getData() {
        return ((this->Int16Buffer::getData()) & mask_) >> shift_;
    }

    virtual void loop() {
        if (!hasLastState_ || !updateDcsData()) return;

        int requiredAdjustment = mapperCallback_(lastState_, lastDcsData_);
        if (requiredAdjustment == 0) return;
        if (millis() - lastSendTime_ <= 100) return;

        char buff[7];
        sprintf(buff, "%+d", requiredAdjustment);
        if (tryToSendDcsBiosMessage(msg_, buff)) {
            lastSendTime_ = millis();
        }
    }
};

using EasyModeRotarySyncingPotentiometer = EasyModeRotarySyncingPotentiometerT<>;
using EasyModeInvertedRotarySyncingPotentiometer = EasyModeRotarySyncingPotentiometerT<POLL_EVERY_TIME, true>;

} // namespace DcsBios

#endif
