#ifndef __DCSBIOS_EASY_COMMON_H
#define __DCSBIOS_EASY_COMMON_H

#ifndef __DCSBIOS_EASY_MODE_H
#error Do not call DcsBiosEasyCommon.h directly. Include DcsBiosEasyMode.h instead.
#endif

namespace DcsBios {

enum class EasyModeDir {
    CW,
    CCW
};

static const unsigned int EASYMODE_DEFAULT_ADC_SPAN = 1024;
static const unsigned int EASYMODE_DEFAULT_MIN_CALIBRATION_SPAN = 64;

class EasyModeRefreshableInputBase {
private:
    bool refreshEnabled_ = false;
    EasyModeRefreshableInputBase* nextRefreshableInput_ = nullptr;

    static EasyModeRefreshableInputBase*& firstRefreshableInput() {
        static EasyModeRefreshableInputBase* first = nullptr;
        return first;
    }

    static EasyModeRefreshableInputBase*& lastRefreshableInput() {
        static EasyModeRefreshableInputBase* last = nullptr;
        return last;
    }

protected:
    EasyModeRefreshableInputBase() {
        if (firstRefreshableInput() == nullptr) {
            firstRefreshableInput() = this;
        } else {
            lastRefreshableInput()->nextRefreshableInput_ = this;
        }
        lastRefreshableInput() = this;
    }

    virtual void resetForRefresh() = 0;

public:
    void applyRefreshReset() {
        resetForRefresh();
    }

    void refresh(bool enabled = true) {
        refreshEnabled_ = enabled;
    }

    bool isRefreshEnabled() const {
        return refreshEnabled_;
    }

    EasyModeRefreshableInputBase* nextRefreshableInput() const {
        return nextRefreshableInput_;
    }

    static EasyModeRefreshableInputBase* first() {
        return firstRefreshableInput();
    }
};

class EasyModeCalibratableInputBase {
private:
    EasyModeCalibratableInputBase* nextCalibratableInput_ = nullptr;

    static EasyModeCalibratableInputBase*& firstCalibratableInput() {
        static EasyModeCalibratableInputBase* first = nullptr;
        return first;
    }

    static EasyModeCalibratableInputBase*& lastCalibratableInput() {
        static EasyModeCalibratableInputBase* last = nullptr;
        return last;
    }

protected:
    EasyModeCalibratableInputBase() {
        if (firstCalibratableInput() == nullptr) {
            firstCalibratableInput() = this;
        } else {
            lastCalibratableInput()->nextCalibratableInput_ = this;
        }
        lastCalibratableInput() = this;
    }

public:
    EasyModeCalibratableInputBase* nextCalibratableInput() const {
        return nextCalibratableInput_;
    }

    static EasyModeCalibratableInputBase* first() {
        return firstCalibratableInput();
    }

    virtual const char* calibrationName() const = 0;
    virtual unsigned int calibrationAdcSpan() const = 0;
    virtual unsigned int calibrationMin() const = 0;
    virtual unsigned int calibrationMax() const = 0;
    virtual bool calibrationIsValid() const = 0;
    virtual void clearCalibration() = 0;
    virtual bool learnCalibrationSample() = 0;
    virtual bool applyCalibration(unsigned int minValue, unsigned int maxValue, unsigned int adcSpan) = 0;
    virtual void printCalibrationStatus(Print& out) const = 0;
};

inline unsigned int easyModeCalibrationNameHash(const char* name) {
    unsigned long hash = 2166136261UL;
    while (name != nullptr && *name != '\0') {
        hash ^= (unsigned char)(*name++);
        hash *= 16777619UL;
    }
    return (unsigned int)((hash >> 16) ^ (hash & 0xFFFFUL));
}

inline bool& easyModeCalibrationDirty() {
    static bool dirty = false;
    return dirty;
}

inline void beginEasyModeCalibration() {
    EasyModeCalibratableInputBase* input = EasyModeCalibratableInputBase::first();
    while (input != nullptr) {
        input->clearCalibration();
        input = input->nextCalibratableInput();
    }
    easyModeCalibrationDirty() = true;
}

inline bool updateEasyModeCalibration() {
    bool changed = false;
    EasyModeCalibratableInputBase* input = EasyModeCalibratableInputBase::first();
    while (input != nullptr) {
        if (input->learnCalibrationSample()) changed = true;
        input = input->nextCalibratableInput();
    }
    if (changed) easyModeCalibrationDirty() = true;
    return changed;
}

inline bool easyModeCalibrationIsValid() {
    EasyModeCalibratableInputBase* input = EasyModeCalibratableInputBase::first();
    while (input != nullptr) {
        if (!input->calibrationIsValid()) return false;
        input = input->nextCalibratableInput();
    }
    return true;
}

inline unsigned long& easyModeRefreshIntervalMs() {
    static unsigned long intervalMs = 0;
    return intervalMs;
}

inline unsigned long& easyModeLastRefreshMs() {
    static unsigned long lastRefreshMs = 0;
    return lastRefreshMs;
}

inline void setEasyModeRefreshInterval(unsigned long intervalMs) {
    if (easyModeRefreshIntervalMs() != intervalMs) {
        easyModeRefreshIntervalMs() = intervalMs;
        easyModeLastRefreshMs() = millis() - intervalMs;
    }
}

inline unsigned long getEasyModeRefreshInterval() {
    return easyModeRefreshIntervalMs();
}

inline void serviceEasyModeRefreshes() {
    unsigned long intervalMs = easyModeRefreshIntervalMs();
    if (intervalMs == 0) return;

    unsigned long now = millis();
    if ((unsigned long)(now - easyModeLastRefreshMs()) < intervalMs) return;

    easyModeLastRefreshMs() = now;

    EasyModeRefreshableInputBase* input = EasyModeRefreshableInputBase::first();
    while (input != nullptr) {
        if (input->isRefreshEnabled()) {
            input->applyRefreshReset();
        }
        input = input->nextRefreshableInput();
    }
}

template <typename BaseT>
class EasyModeRefreshableInputT : public BaseT, public EasyModeRefreshableInputBase {
private:
    template <typename T>
    static auto requestRefresh(T* input, int) -> decltype(input->resetThisState(), void()) {
        input->resetThisState();
    }

    template <typename T>
    static void requestRefresh(T*, long) {
        // Some edge-triggered helpers do not expose resetThisState(). For those
        // inputs refresh() is accepted but does not force a resend.
    }

protected:
    virtual void resetForRefresh() override {
        requestRefresh(static_cast<BaseT*>(this), 0);
    }

public:
    using BaseT::BaseT;
};

} // namespace DcsBios

#endif
