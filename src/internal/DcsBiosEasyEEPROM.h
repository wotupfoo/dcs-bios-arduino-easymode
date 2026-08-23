#ifndef __DCSBIOS_EASY_EEPROM_H
#define __DCSBIOS_EASY_EEPROM_H

#ifndef __DCSBIOS_EASY_MODE_H
#error Do not call DcsBiosEasyEEPROM.h directly. Include DcsBiosEasyMode.h instead.
#endif

#include <EEPROM.h>

namespace DcsBios {

static const unsigned int EASYMODE_EEPROM_MAGIC = 0xD5E1;
static const unsigned char EASYMODE_EEPROM_VERSION = 1;
static const int EASYMODE_EEPROM_DEFAULT_ADDR = 0;

struct EasyModeEepromHeader {
    unsigned int magic;
    unsigned char version;
    unsigned char count;
};

struct EasyModeEepromCalibrationRecord {
    unsigned int id;
    unsigned int adcSpan;
    unsigned int minValue;
    unsigned int maxValue;
    unsigned char flags;
    unsigned char checksum;
};

inline unsigned char easyModeEepromRecordChecksum(const EasyModeEepromCalibrationRecord& record) {
    unsigned char checksum = 0xA5;
    checksum ^= (unsigned char)(record.id & 0xFF);
    checksum ^= (unsigned char)(record.id >> 8);
    checksum ^= (unsigned char)(record.adcSpan & 0xFF);
    checksum ^= (unsigned char)(record.adcSpan >> 8);
    checksum ^= (unsigned char)(record.minValue & 0xFF);
    checksum ^= (unsigned char)(record.minValue >> 8);
    checksum ^= (unsigned char)(record.maxValue & 0xFF);
    checksum ^= (unsigned char)(record.maxValue >> 8);
    checksum ^= record.flags;
    return checksum;
}


inline bool loadEasyModeCalibrationFromEEPROM(int address = EASYMODE_EEPROM_DEFAULT_ADDR) {
    EasyModeEepromHeader header;
    EEPROM.get(address, header);

    EasyModeCalibratableInputBase* input = EasyModeCalibratableInputBase::first();
    while (input != nullptr) {
        input->clearCalibration();
        input = input->nextCalibratableInput();
    }

    if (header.magic != EASYMODE_EEPROM_MAGIC || header.version != EASYMODE_EEPROM_VERSION) {
        easyModeCalibrationDirty() = false;
        return false;
    }

    int recordAddress = address + sizeof(EasyModeEepromHeader);
    for (unsigned char i = 0; i < header.count; ++i) {
        EasyModeEepromCalibrationRecord record;
        EEPROM.get(recordAddress, record);
        recordAddress += sizeof(EasyModeEepromCalibrationRecord);

        if (record.checksum != easyModeEepromRecordChecksum(record)) continue;

        EasyModeCalibratableInputBase* recordInput = EasyModeCalibratableInputBase::first();
        while (recordInput != nullptr) {
            if (easyModeCalibrationNameHash(recordInput->calibrationName()) == record.id) break;
            recordInput = recordInput->nextCalibratableInput();
        }
        if (recordInput == nullptr) continue;

        if ((record.flags & 0x01) == 0) continue;
        recordInput->applyCalibration(record.minValue, record.maxValue, record.adcSpan);
    }

    easyModeCalibrationDirty() = false;
    return easyModeCalibrationIsValid();
}

inline void saveEasyModeCalibrationToEEPROM(int address = EASYMODE_EEPROM_DEFAULT_ADDR) {
    EasyModeEepromHeader header;
    header.magic = EASYMODE_EEPROM_MAGIC;
    header.version = EASYMODE_EEPROM_VERSION;
    header.count = 0;

    EasyModeCalibratableInputBase* input = EasyModeCalibratableInputBase::first();
    while (input != nullptr && header.count < 255) {
        ++header.count;
        input = input->nextCalibratableInput();
    }

    EEPROM.put(address, header);

    int recordAddress = address + sizeof(EasyModeEepromHeader);
    input = EasyModeCalibratableInputBase::first();
    while (input != nullptr) {
        EasyModeEepromCalibrationRecord record;
        record.id = easyModeCalibrationNameHash(input->calibrationName());
        record.adcSpan = input->calibrationAdcSpan();
        record.minValue = input->calibrationMin();
        record.maxValue = input->calibrationMax();
        record.flags = input->calibrationIsValid() ? 0x01 : 0x00;
        record.checksum = easyModeEepromRecordChecksum(record);
        EEPROM.put(recordAddress, record);

        recordAddress += sizeof(EasyModeEepromCalibrationRecord);
        input = input->nextCalibratableInput();
    }

    easyModeCalibrationDirty() = false;
}

inline bool saveEasyModeCalibrationToEEPROMIfChanged(int address = EASYMODE_EEPROM_DEFAULT_ADDR) {
    if (!easyModeCalibrationDirty()) return false;
    saveEasyModeCalibrationToEEPROM(address);
    return true;
}

inline bool serviceEasyModeCalibration(Print& out, int address = EASYMODE_EEPROM_DEFAULT_ADDR) {
    bool changed = updateEasyModeCalibration();

    EasyModeCalibratableInputBase* input = EasyModeCalibratableInputBase::first();
    bool first = true;
    while (input != nullptr) {
        if (!first) out.print("  ");
        input->printCalibrationStatus(out);
        first = false;
        input = input->nextCalibratableInput();
    }

    out.print("  Valid ");
    out.print(easyModeCalibrationIsValid() ? 1 : 0);
    out.print("  Update ");
    out.println(changed ? 1 : 0);

    if (changed) {
        out.println("Writing EEPROM");
        saveEasyModeCalibrationToEEPROMIfChanged(address);
    }

    return changed;
}

} // namespace DcsBios

#endif