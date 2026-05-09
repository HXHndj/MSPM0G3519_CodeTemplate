#ifndef EEPROM_METADATA_H
#define EEPROM_METADATA_H

#include <stdbool.h>
#include <stdint.h>

#include "audio_format.h"

typedef struct {
    bool hasRecording;
    bool filterEnabled;
    uint32_t recordingBytes;
    uint32_t durationSeconds;
    uint32_t sampleRateHz;
    uint16_t globalAdcMin;      /* 录音全程 ADC 最小值（用于播放时归一化拉伸） */
    uint16_t globalAdcMax;      /* 录音全程 ADC 最大值 */
} EEPROMRecordingMetadata;

bool EEPROMMetadata_Load(EEPROMRecordingMetadata *metadata);
bool EEPROMMetadata_SaveRecording(uint32_t recordingBytes,
    uint32_t sampleRateHz, bool filterEnabled,
    uint16_t globalAdcMin, uint16_t globalAdcMax);
bool EEPROMMetadata_Clear(void);
bool EEPROMMetadata_EraseAll(void);

bool EEPROMMetadata_Test(uint32_t *initState, uint32_t *writeState,
    uint32_t *writtenValue, uint32_t *readValue);

#endif