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

template <unsigned long pollIntervalMs = POLL_EVERY_TIME, unsigned int defaultHysteresis = 2>
class EasyModeAnalogMultiPosT : PollingInput, public ResettableInput, public EasyModeCalibratableInputBase {
private:
    const char* msg_;
    char pin_;
    unsigned char numOfSteps_;
    unsigned int inputMin_;
    unsigned int inputMax_;
    unsigned int adcSpan_;
    unsigned int minimumCalibrationSpan_;
    bool hasCalibrationSample_ = true;
    bool calibrationValid_ = true;
    unsigned int hysteresis_;
    unsigned char lastState_ = 0;
    bool hasLastState_ = false;
    unsigned long periodMs_ = 750;
    unsigned long lastPollMs_ = 0;

    void resetState() {
        hasLastState_ = false;
    }

    void pollInput() {
        if (!calibrationValid_) return;
        unsigned long now = millis();
        if (now > lastPollMs_ + periodMs_) {
            unsigned int raw = getRawValue();
            unsigned char state = 0;

            if (inputMax_ <= inputMin_ || raw <= inputMin_) {
                state = 0;
            } else if (raw >= inputMax_) {
                state = numOfSteps_;
            } else {
                unsigned long span = (unsigned long)(inputMax_ - inputMin_) + 1UL;
                unsigned long mapped = ((unsigned long)(raw - inputMin_) * ((unsigned long)numOfSteps_ + 1UL)) / span;
                if (mapped > numOfSteps_) mapped = numOfSteps_;
                state = (unsigned char)mapped;

                if (hasLastState_ && state != lastState_ && hysteresis_ > 0 && numOfSteps_ > 0) {
                    unsigned long divisor = (unsigned long)numOfSteps_ + 1UL;
                    if (state > lastState_ && lastState_ < numOfSteps_) {
                        unsigned long boundary = (unsigned long)inputMin_ + ((((unsigned long)lastState_ + 1UL) * span) + divisor - 1UL) / divisor;
                        unsigned long threshold = boundary + (unsigned long)hysteresis_;
                        if (threshold > inputMax_) threshold = inputMax_;
                        if ((unsigned long)raw < threshold) state = lastState_;
                    } else if (state < lastState_ && lastState_ > 0) {
                        unsigned long boundary = (unsigned long)inputMin_ + (((unsigned long)lastState_ * span) + divisor - 1UL) / divisor;
                        unsigned long threshold = (boundary > (unsigned long)hysteresis_) ? boundary - (unsigned long)hysteresis_ : 0UL;
                        if (threshold < inputMin_) threshold = inputMin_;
                        if ((unsigned long)raw > threshold) state = lastState_;
                    }
                }
            }

            lastPollMs_ = now;
            if (!hasLastState_ || state != lastState_) {
                char cstr[5];
                itoa(state, cstr, 10);
                if (tryToSendDcsBiosMessage(msg_, cstr)) {
                    lastState_ = state;
                    hasLastState_ = true;
                }
            }
        }
    }

public:
    EasyModeAnalogMultiPosT(
        const char* msg,
        char pin,
        char numOfSteps,
        unsigned int adcSpan = EASYMODE_DEFAULT_ADC_SPAN,
        unsigned int hysteresis = defaultHysteresis
    ) :
        PollingInput(pollIntervalMs),
        msg_(msg),
        pin_(pin),
        numOfSteps_(numOfSteps),
        inputMin_(0),
        inputMax_(adcSpan == 0 ? EASYMODE_DEFAULT_ADC_SPAN - 1 : adcSpan - 1),
        adcSpan_(adcSpan == 0 ? EASYMODE_DEFAULT_ADC_SPAN : adcSpan),
        minimumCalibrationSpan_(EASYMODE_DEFAULT_MIN_CALIBRATION_SPAN),
        hysteresis_(hysteresis)
    {
        pinMode(pin_, INPUT);
        (void)easyModeFirstPacketListener();
    }

    void SetControl(const char* msg) {
        msg_ = msg;
    }

    void setMin(unsigned int value) {
        inputMin_ = value;
        hasCalibrationSample_ = true;
        calibrationValid_ = inputMax_ > inputMin_ && (inputMax_ - inputMin_) >= minimumCalibrationSpan_;
        resetState();
    }

    void setMax(unsigned int value) {
        inputMax_ = value;
        hasCalibrationSample_ = true;
        calibrationValid_ = inputMax_ > inputMin_ && (inputMax_ - inputMin_) >= minimumCalibrationSpan_;
        resetState();
    }

    unsigned int getRawValue() const {
        analogRead(pin_);
        return analogRead(pin_);
    }

    virtual const char* calibrationName() const override {
        return msg_;
    }

    virtual unsigned int calibrationAdcSpan() const override {
        return adcSpan_;
    }

    virtual unsigned int calibrationMin() const override {
        return inputMin_;
    }

    virtual unsigned int calibrationMax() const override {
        return inputMax_;
    }

    virtual bool calibrationIsValid() const override {
        return calibrationValid_;
    }

    virtual void clearCalibration() override {
        inputMin_ = 0;
        inputMax_ = 0;
        hasCalibrationSample_ = false;
        calibrationValid_ = false;
        resetState();
    }

    virtual bool learnCalibrationSample() override {
        unsigned int raw = getRawValue();
        if (adcSpan_ > 0 && raw >= adcSpan_) raw = adcSpan_ - 1;

        if (!hasCalibrationSample_) {
            inputMin_ = raw;
            inputMax_ = raw;
            hasCalibrationSample_ = true;
            calibrationValid_ = false;
            resetState();
            return true;
        }

        bool changed = false;
        if (raw < inputMin_) {
            inputMin_ = raw;
            changed = true;
        }
        if (raw > inputMax_) {
            inputMax_ = raw;
            changed = true;
        }

        bool newValid = inputMax_ > inputMin_ && (inputMax_ - inputMin_) >= minimumCalibrationSpan_;
        if (newValid != calibrationValid_) {
            calibrationValid_ = newValid;
            changed = true;
        }
        if (changed) resetState();
        return changed;
    }

    virtual bool applyCalibration(unsigned int minValue, unsigned int maxValue, unsigned int adcSpan) override {
        if (adcSpan == 0) adcSpan = adcSpan_;
        if (maxValue >= adcSpan) return false;
        if (maxValue <= minValue) return false;
        if ((maxValue - minValue) < minimumCalibrationSpan_) return false;

        adcSpan_ = adcSpan;
        inputMin_ = minValue;
        inputMax_ = maxValue;
        hasCalibrationSample_ = true;
        calibrationValid_ = true;
        resetState();
        return true;
    }

    virtual void printCalibrationStatus(Print& out) const override {
        out.print(msg_);
        out.print(" ");
        out.print(inputMin_);
        out.print("|");
        out.print(getRawValue());
        out.print("|");
        out.print(inputMax_);
    }
    void resetThisState() {
        this->resetState();
    }

};

using EasyModeAnalogMultiPos = EasyModeAnalogMultiPosT<>;

template <unsigned long pollIntervalMs = 5, unsigned int defaultRawHysteresis = 2, unsigned int ewmaDivisor = 5>
class EasyModePotentiometerT : PollingInput, public ResettableInput, public EasyModeCalibratableInputBase {
private:
    const char* msg_;
    char pin_;
    bool reverse_;
    unsigned int inputMin_;
    unsigned int inputMax_;
    unsigned int adcSpan_;
    unsigned int minimumCalibrationSpan_;
    bool hasCalibrationSample_ = true;
    bool calibrationValid_ = true;
    unsigned int rawHysteresis_;
    unsigned int lastState_ = 0;
    float accumulator_ = 0.0f;
    bool hasLastState_ = false;
    void resetState() {
        hasLastState_ = false;
    }

    void resetCalibrationMapping() {
        resetState();
    }

    void pollInput() {
        if (!calibrationValid_) return;
        unsigned int state;
        unsigned int value = getRawValue();

        if (inputMax_ <= inputMin_) value = inputMin_;
        else if (value < inputMin_) value = inputMin_;
        else if (value > inputMax_) value = inputMax_;

        if (inputMax_ <= inputMin_) {
            state = 0;
        } else if (reverse_) {
            state = map(value, inputMin_, inputMax_, 65535, 0);
        } else {
            state = map(value, inputMin_, inputMax_, 0, 65535);
        }

        accumulator_ += ((float)state - accumulator_) / (float)ewmaDivisor;
        state = (unsigned int)accumulator_;

        unsigned int hysteresis = rawHysteresis_;
        if (inputMax_ > inputMin_) {
            unsigned long mappedHysteresis = ((unsigned long)rawHysteresis_ * 65535UL) / (unsigned long)(inputMax_ - inputMin_);
            if (rawHysteresis_ > 0 && mappedHysteresis == 0) mappedHysteresis = 1;
            if (mappedHysteresis > 65535UL) mappedHysteresis = 65535UL;
            hysteresis = (unsigned int)mappedHysteresis;
        }

        if (!hasLastState_
        || ((lastState_ > state && (lastState_ - state > hysteresis)))
        || ((state > lastState_) && (state - lastState_ > hysteresis))
        || ((state > (65535 - hysteresis) && state > lastState_))
        || ((state < hysteresis && state < lastState_))
        ) {
            char buf[6];
            utoa(state, buf, 10);
            if (tryToSendDcsBiosMessage(msg_, buf)) {
                lastState_ = state;
                hasLastState_ = true;
            }
        }
    }

public:
    EasyModePotentiometerT(
        const char* msg,
        char pin,
        bool reverse = false,
        unsigned int adcSpan = EASYMODE_DEFAULT_ADC_SPAN,
        unsigned int rawHysteresis = defaultRawHysteresis
    ) :
        PollingInput(pollIntervalMs),
        msg_(msg),
        pin_(pin),
        reverse_(reverse),
        inputMin_(0),
        inputMax_(adcSpan == 0 ? EASYMODE_DEFAULT_ADC_SPAN - 1 : adcSpan - 1),
        adcSpan_(adcSpan == 0 ? EASYMODE_DEFAULT_ADC_SPAN : adcSpan),
        minimumCalibrationSpan_(EASYMODE_DEFAULT_MIN_CALIBRATION_SPAN),
        rawHysteresis_(rawHysteresis)
    {
        pinMode(pin_, INPUT);
        (void)easyModeFirstPacketListener();
    }

    void SetControl(const char* msg) {
        msg_ = msg;
    }

    void setMin(unsigned int value) {
        inputMin_ = value;
        hasCalibrationSample_ = true;
        calibrationValid_ = inputMax_ > inputMin_ && (inputMax_ - inputMin_) >= minimumCalibrationSpan_;
        resetCalibrationMapping();
    }

    void setMax(unsigned int value) {
        inputMax_ = value;
        hasCalibrationSample_ = true;
        calibrationValid_ = inputMax_ > inputMin_ && (inputMax_ - inputMin_) >= minimumCalibrationSpan_;
        resetCalibrationMapping();
    }

    unsigned int getRawValue() const {
        analogRead(pin_);
        return analogRead(pin_);
    }

    virtual const char* calibrationName() const override {
        return msg_;
    }

    virtual unsigned int calibrationAdcSpan() const override {
        return adcSpan_;
    }

    virtual unsigned int calibrationMin() const override {
        return inputMin_;
    }

    virtual unsigned int calibrationMax() const override {
        return inputMax_;
    }

    virtual bool calibrationIsValid() const override {
        return calibrationValid_;
    }

    virtual void clearCalibration() override {
        inputMin_ = 0;
        inputMax_ = 0;
        hasCalibrationSample_ = false;
        calibrationValid_ = false;
        resetState();
    }

    virtual bool learnCalibrationSample() override {
        unsigned int raw = getRawValue();
        if (adcSpan_ > 0 && raw >= adcSpan_) raw = adcSpan_ - 1;

        if (!hasCalibrationSample_) {
            inputMin_ = raw;
            inputMax_ = raw;
            hasCalibrationSample_ = true;
            calibrationValid_ = false;
            resetState();
            return true;
        }

        bool changed = false;
        if (raw < inputMin_) {
            inputMin_ = raw;
            changed = true;
        }
        if (raw > inputMax_) {
            inputMax_ = raw;
            changed = true;
        }

        bool newValid = inputMax_ > inputMin_ && (inputMax_ - inputMin_) >= minimumCalibrationSpan_;
        if (newValid != calibrationValid_) {
            calibrationValid_ = newValid;
            changed = true;
        }
        if (changed) resetState();
        return changed;
    }

    virtual bool applyCalibration(unsigned int minValue, unsigned int maxValue, unsigned int adcSpan) override {
        if (adcSpan == 0) adcSpan = adcSpan_;
        if (maxValue >= adcSpan) return false;
        if (maxValue <= minValue) return false;
        if ((maxValue - minValue) < minimumCalibrationSpan_) return false;

        adcSpan_ = adcSpan;
        inputMin_ = minValue;
        inputMax_ = maxValue;
        hasCalibrationSample_ = true;
        calibrationValid_ = true;
        resetState();
        return true;
    }

    virtual void printCalibrationStatus(Print& out) const override {
        out.print(msg_);
        out.print(" ");
        out.print(inputMin_);
        out.print("|");
        out.print(getRawValue());
        out.print("|");
        out.print(inputMax_);
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
class EasyModeRotarySyncingPotentiometerT : PollingInput, Int16Buffer, public ResettableInput, public EasyModeCalibratableInputBase {
private:
    const char* msg_;
    char pin_;
    unsigned int inputMin_;
    unsigned int inputMax_;
    unsigned int adcSpan_;
    unsigned int minimumCalibrationSpan_;
    bool hasCalibrationSample_ = true;
    bool calibrationValid_ = true;
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
        if (!calibrationValid_) return;
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
        unsigned int adcSpan = EASYMODE_DEFAULT_ADC_SPAN
    ) :
        PollingInput(pollIntervalMs),
        Int16Buffer(syncToAddress),
        msg_(msg),
        pin_(pin),
        inputMin_(0),
        inputMax_(adcSpan == 0 ? EASYMODE_DEFAULT_ADC_SPAN - 1 : adcSpan - 1),
        adcSpan_(adcSpan == 0 ? EASYMODE_DEFAULT_ADC_SPAN : adcSpan),
        minimumCalibrationSpan_(EASYMODE_DEFAULT_MIN_CALIBRATION_SPAN),
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
        hasCalibrationSample_ = true;
        calibrationValid_ = inputMax_ > inputMin_ && (inputMax_ - inputMin_) >= minimumCalibrationSpan_;
        resetState();
    }

    void setMax(unsigned int value) {
        inputMax_ = value;
        hasCalibrationSample_ = true;
        calibrationValid_ = inputMax_ > inputMin_ && (inputMax_ - inputMin_) >= minimumCalibrationSpan_;
        resetState();
    }

    unsigned int getRawValue() const {
        analogRead(pin_);
        return analogRead(pin_);
    }

    virtual const char* calibrationName() const override {
        return msg_;
    }

    virtual unsigned int calibrationAdcSpan() const override {
        return adcSpan_;
    }

    virtual unsigned int calibrationMin() const override {
        return inputMin_;
    }

    virtual unsigned int calibrationMax() const override {
        return inputMax_;
    }

    virtual bool calibrationIsValid() const override {
        return calibrationValid_;
    }

    virtual void clearCalibration() override {
        inputMin_ = 0;
        inputMax_ = 0;
        hasCalibrationSample_ = false;
        calibrationValid_ = false;
        resetState();
    }

    virtual bool learnCalibrationSample() override {
        unsigned int raw = getRawValue();
        if (adcSpan_ > 0 && raw >= adcSpan_) raw = adcSpan_ - 1;

        if (!hasCalibrationSample_) {
            inputMin_ = raw;
            inputMax_ = raw;
            hasCalibrationSample_ = true;
            calibrationValid_ = false;
            resetState();
            return true;
        }

        bool changed = false;
        if (raw < inputMin_) {
            inputMin_ = raw;
            changed = true;
        }
        if (raw > inputMax_) {
            inputMax_ = raw;
            changed = true;
        }

        bool newValid = inputMax_ > inputMin_ && (inputMax_ - inputMin_) >= minimumCalibrationSpan_;
        if (newValid != calibrationValid_) {
            calibrationValid_ = newValid;
            changed = true;
        }
        if (changed) resetState();
        return changed;
    }

    virtual bool applyCalibration(unsigned int minValue, unsigned int maxValue, unsigned int adcSpan) override {
        if (adcSpan == 0) adcSpan = adcSpan_;
        if (maxValue >= adcSpan) return false;
        if (maxValue <= minValue) return false;
        if ((maxValue - minValue) < minimumCalibrationSpan_) return false;

        adcSpan_ = adcSpan;
        inputMin_ = minValue;
        inputMax_ = maxValue;
        hasCalibrationSample_ = true;
        calibrationValid_ = true;
        resetState();
        return true;
    }

    virtual void printCalibrationStatus(Print& out) const override {
        out.print(msg_);
        out.print(" ");
        out.print(inputMin_);
        out.print("|");
        out.print(getRawValue());
        out.print("|");
        out.print(inputMax_);
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

template <unsigned long pollIntervalMs = 5, unsigned int defaultDeadband = 512, unsigned long commandIntervalMs = 100>
class EasyModeAnalogSyncingRockerT : PollingInput, Int16Buffer, public ResettableInput, public EasyModeCalibratableInputBase {
private:
    const char* msg_;
    char pin_;
    unsigned int feedbackMin_;
    unsigned int feedbackMax_;
    unsigned int inputMin_;
    unsigned int inputMax_;
    unsigned int adcSpan_;
    unsigned int minimumCalibrationSpan_;
    bool hasCalibrationSample_ = true;
    bool calibrationValid_ = true;
    unsigned int deadband_;
    bool reverseDirection_;

    unsigned int targetState_ = 0;
    bool hasTargetState_ = false;
    unsigned int feedbackState_ = 0;
    bool hasFeedbackState_ = false;

    unsigned int mask_;
    unsigned char shift_;
    unsigned char lastCommandState_ = 1;
    bool hasLastCommandState_ = false;
    unsigned long lastPollTime_ = 0;
    unsigned long lastSendTime_ = 0;

    unsigned int mapRawToFeedbackState(unsigned int raw) const {
        if (inputMax_ <= inputMin_) return feedbackMin_;
        if (raw <= inputMin_) return feedbackMin_;
        if (raw >= inputMax_) return feedbackMax_;

        unsigned long inputSpan = (unsigned long)(inputMax_ - inputMin_);
        unsigned long feedbackSpan = (feedbackMax_ >= feedbackMin_) ?
            (unsigned long)(feedbackMax_ - feedbackMin_) :
            (unsigned long)(feedbackMin_ - feedbackMax_);
        unsigned long offset = ((unsigned long)(raw - inputMin_) * feedbackSpan) / inputSpan;

        if (feedbackMax_ >= feedbackMin_) return feedbackMin_ + offset;
        return feedbackMin_ - offset;
    }

    unsigned int readTargetState() const {
        return mapRawToFeedbackState(getRawValue());
    }

    void updateTargetState(unsigned long now) {
        if (!calibrationValid_) return;
        if (pollIntervalMs != POLL_EVERY_TIME && (unsigned long)(now - lastPollTime_) < pollIntervalMs) return;

        targetState_ = readTargetState();
        hasTargetState_ = true;
        lastPollTime_ = now;
    }

    unsigned char commandForDelta(long delta) const {
        if (delta > (long)deadband_) return reverseDirection_ ? 0 : 2;
        if (delta < -(long)deadband_) return reverseDirection_ ? 2 : 0;
        return 1;
    }

    void resetState() {
        hasLastCommandState_ = false;
    }

    void resetTargetMapping() {
        hasTargetState_ = false;
        resetState();
    }

    void pollInput() {
        updateTargetState(millis());
    }

    bool updateFeedbackState() {
        if (!this->hasUpdatedData()) return hasFeedbackState_;

        feedbackState_ = getData();
        hasFeedbackState_ = true;
        return true;
    }

    const char* rockerStateArg(unsigned char state) const {
        if (state == 0) return "0";
        if (state == 2) return "2";
        return "1";
    }

public:
    EasyModeAnalogSyncingRockerT(
        const char* msg,
        char pin,
        unsigned int feedbackAddress,
        unsigned int feedbackMask,
        unsigned char feedbackShift,
        unsigned int feedbackMin = 0,
        unsigned int feedbackMax = 65535,
        unsigned int adcSpan = EASYMODE_DEFAULT_ADC_SPAN,
        unsigned int deadband = defaultDeadband,
        bool reverseDirection = false
    ) :
        PollingInput(pollIntervalMs),
        Int16Buffer(feedbackAddress),
        msg_(msg),
        pin_(pin),
        feedbackMin_(feedbackMin),
        feedbackMax_(feedbackMax),
        inputMin_(0),
        inputMax_(adcSpan == 0 ? EASYMODE_DEFAULT_ADC_SPAN - 1 : adcSpan - 1),
        adcSpan_(adcSpan == 0 ? EASYMODE_DEFAULT_ADC_SPAN : adcSpan),
        minimumCalibrationSpan_(EASYMODE_DEFAULT_MIN_CALIBRATION_SPAN),
        deadband_(deadband),
        reverseDirection_(reverseDirection),
        mask_(feedbackMask),
        shift_(feedbackShift),
        lastSendTime_(millis())
    {
        pinMode(pin_, INPUT);
        (void)easyModeFirstPacketListener();
    }

    void SetControl(const char* msg) {
        msg_ = msg;
    }

    void setMin(unsigned int value) {
        inputMin_ = value;
        hasCalibrationSample_ = true;
        calibrationValid_ = inputMax_ > inputMin_ && (inputMax_ - inputMin_) >= minimumCalibrationSpan_;
        resetTargetMapping();
    }

    void setMax(unsigned int value) {
        inputMax_ = value;
        hasCalibrationSample_ = true;
        calibrationValid_ = inputMax_ > inputMin_ && (inputMax_ - inputMin_) >= minimumCalibrationSpan_;
        resetTargetMapping();
    }
    void setFeedbackMin(unsigned int value) {
        feedbackMin_ = value;
        resetTargetMapping();
    }

    void setFeedbackMax(unsigned int value) {
        feedbackMax_ = value;
        resetTargetMapping();
    }

    void setDeadband(unsigned int value) {
        deadband_ = value;
        resetState();
    }

    void setReverseDirection(bool value) {
        reverseDirection_ = value;
        resetState();
    }

    unsigned int getRawValue() const {
        analogRead(pin_);
        return analogRead(pin_);
    }

    virtual const char* calibrationName() const override {
        return msg_;
    }

    virtual unsigned int calibrationAdcSpan() const override {
        return adcSpan_;
    }

    virtual unsigned int calibrationMin() const override {
        return inputMin_;
    }

    virtual unsigned int calibrationMax() const override {
        return inputMax_;
    }

    virtual bool calibrationIsValid() const override {
        return calibrationValid_;
    }

    virtual void clearCalibration() override {
        inputMin_ = 0;
        inputMax_ = 0;
        hasCalibrationSample_ = false;
        calibrationValid_ = false;
        resetTargetMapping();
    }

    virtual bool learnCalibrationSample() override {
        unsigned int raw = getRawValue();
        if (adcSpan_ > 0 && raw >= adcSpan_) raw = adcSpan_ - 1;

        if (!hasCalibrationSample_) {
            inputMin_ = raw;
            inputMax_ = raw;
            hasCalibrationSample_ = true;
            calibrationValid_ = false;
            resetTargetMapping();
            return true;
        }

        bool changed = false;
        if (raw < inputMin_) {
            inputMin_ = raw;
            changed = true;
        }
        if (raw > inputMax_) {
            inputMax_ = raw;
            changed = true;
        }

        bool newValid = inputMax_ > inputMin_ && (inputMax_ - inputMin_) >= minimumCalibrationSpan_;
        if (newValid != calibrationValid_) {
            calibrationValid_ = newValid;
            changed = true;
        }
        if (changed) resetTargetMapping();
        return changed;
    }

    virtual bool applyCalibration(unsigned int minValue, unsigned int maxValue, unsigned int adcSpan) override {
        if (adcSpan == 0) adcSpan = adcSpan_;
        if (maxValue >= adcSpan) return false;
        if (maxValue <= minValue) return false;
        if ((maxValue - minValue) < minimumCalibrationSpan_) return false;

        adcSpan_ = adcSpan;
        inputMin_ = minValue;
        inputMax_ = maxValue;
        hasCalibrationSample_ = true;
        calibrationValid_ = true;
        resetTargetMapping();
        return true;
    }
    virtual void printCalibrationStatus(Print& out) const override {
        out.print(msg_);
        out.print(" ");
        out.print(inputMin_);
        out.print("|");
        out.print(getRawValue());
        out.print("|");
        out.print(inputMax_);
    }
    void resetThisState() {
        this->resetState();
    }

    unsigned int getData() {
        return ((this->Int16Buffer::getData()) & mask_) >> shift_;
    }

    virtual void loop() {
        unsigned long now = millis();
        updateTargetState(now);
        if (!hasTargetState_ || !updateFeedbackState()) return;

        unsigned char desiredState = commandForDelta((long)targetState_ - (long)feedbackState_);
        if (hasLastCommandState_ && desiredState == lastCommandState_) return;
        if ((unsigned long)(now - lastSendTime_) < commandIntervalMs) return;

        if (tryToSendDcsBiosMessage(msg_, rockerStateArg(desiredState))) {
            lastCommandState_ = desiredState;
            hasLastCommandState_ = true;
            lastSendTime_ = now;
        }
    }
};

using EasyModeAnalogSyncingRocker = EasyModeAnalogSyncingRockerT<>;

} // namespace DcsBios

#endif
