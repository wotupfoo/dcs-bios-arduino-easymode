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

} // namespace DcsBios

#endif
