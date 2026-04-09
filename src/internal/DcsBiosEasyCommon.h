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

class EasyModeRefreshableInputBase {
private:
    bool refreshEnabled_ = false;
    EasyModeRefreshableInputBase* nextRefreshableInput_ = nullptr;

    static EasyModeRefreshableInputBase*& firstRefreshableInput() {
        static EasyModeRefreshableInputBase* first = nullptr;
        return first;
    }

protected:
    EasyModeRefreshableInputBase() {
        nextRefreshableInput_ = firstRefreshableInput();
        firstRefreshableInput() = this;
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
