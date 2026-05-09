#include <string.h>

#include "eeprom_metadata.h"
#include "eeprom_emulation_type_a.h"

#define EEPROM_METADATA_MAGIC            (0x4155444Fu)
#define EEPROM_METADATA_VERSION          (0x00020001u)
#define EEPROM_METADATA_FLAG_VALID       (0x00000001u)
#define EEPROM_METADATA_FLAG_FILTER_ON   (0x00000002u)
#define EEPROM_METADATA_CHECKSUM_SEED    (0x5A5AA5A5u)

enum {
    EEPROM_METADATA_WORD_MAGIC = 0,
    EEPROM_METADATA_WORD_VERSION,
    EEPROM_METADATA_WORD_FLAGS,
    EEPROM_METADATA_WORD_RECORDING_BYTES,
    EEPROM_METADATA_WORD_DURATION_SECONDS,
    EEPROM_METADATA_WORD_SAMPLE_RATE_HZ,
    EEPROM_METADATA_WORD_CHECKSUM,
    EEPROM_METADATA_WORD_CHECKSUM_INV,
    EEPROM_METADATA_WORD_ADC_MINMAX,      /* 新增: adc_max<<16 | adc_min */
    EEPROM_METADATA_WORD_COUNT
};

#if defined(__clang__) || defined(__GNUC__) || defined(__TI_ARM_CLANG_COMPILER_VERSION__)
#define EEPROM_METADATA_ALIGN8      __attribute__((aligned(8)))
#else
#define EEPROM_METADATA_ALIGN8
#endif

static uint32_t sEEPROMEmulationState;
static uint32_t sEEPROMEmulationBuffer[EEPROM_EMULATION_DATA_SIZE / sizeof(uint32_t)]
    EEPROM_METADATA_ALIGN8;
static bool sEEPROMInitialized;

static void EEPROMMetadata_ResetTypeAState(void)
{
    gActiveRecordAddress = EEPROM_EMULATION_ADDRESS;
    gNextRecordAddress = EEPROM_EMULATION_ADDRESS;
    gActiveRecordNum = 0u;
    gActiveSectorNum = EEPROM_EMULATION_ACTIVE_SECTOR_NUM_MIN;
    gEEPROMTypeASearchFlag = false;
    gEEPROMTypeAEraseFlag = false;
    gEEPROMTypeAFormatErrorFlag = false;
}

static void EEPROMMetadata_Reset(EEPROMRecordingMetadata *metadata)
{
    metadata->hasRecording = false;
    metadata->filterEnabled = false;
    metadata->recordingBytes = 0u;
    metadata->durationSeconds = 0u;
    metadata->sampleRateHz = AUDIO_DEFAULT_SAMPLE_RATE_HZ;
    metadata->globalAdcMin = 0u;
    metadata->globalAdcMax = 4095u;
}

static uint32_t EEPROMMetadata_BytesPerSecond(uint32_t sampleRateHz)
{
    if ((sampleRateHz == 0u) ||
        (sampleRateHz > (UINT32_MAX / AUDIO_SAMPLE_BYTES))) {
        return 0u;
    }

    return sampleRateHz * AUDIO_SAMPLE_BYTES;
}

static uint32_t EEPROMMetadata_CalcChecksum(uint32_t flags,
    uint32_t recordingBytes, uint32_t durationSeconds,
    uint32_t sampleRateHz, uint32_t adcMinMax)
{
    return EEPROM_METADATA_MAGIC ^ EEPROM_METADATA_VERSION ^ flags ^
           recordingBytes ^ durationSeconds ^ sampleRateHz ^
           adcMinMax ^ EEPROM_METADATA_CHECKSUM_SEED;
}

static bool EEPROMMetadata_EnsureInitialized(uint32_t *initState)
{
    if (!sEEPROMInitialized) {
        memset(sEEPROMEmulationBuffer, 0xFF, sizeof(sEEPROMEmulationBuffer));
        sEEPROMEmulationState = EEPROM_TypeA_init(sEEPROMEmulationBuffer);
        if (initState != NULL) {
            *initState = sEEPROMEmulationState;
        }
        if (sEEPROMEmulationState != EEPROM_EMULATION_INIT_OK) {
            return false;
        }
        sEEPROMInitialized = true;
    } else if (initState != NULL) {
        *initState = EEPROM_EMULATION_INIT_OK;
    }

    return true;
}

static void EEPROMMetadata_RefreshBuffer(void)
{
    if (gEEPROMTypeASearchFlag == 1) {
        EEPROM_TypeA_readData(sEEPROMEmulationBuffer);
    } else {
        memset(sEEPROMEmulationBuffer, 0xFF, sizeof(sEEPROMEmulationBuffer));
    }
}

static void EEPROMMetadata_Pack(const EEPROMRecordingMetadata *metadata)
{
    uint32_t flags = 0u;
    uint32_t checksum;
    uint32_t recordingBytes = 0u;
    uint32_t durationSeconds = 0u;
    uint32_t sampleRateHz = AUDIO_DEFAULT_SAMPLE_RATE_HZ;

    if (metadata->hasRecording) {
        flags |= EEPROM_METADATA_FLAG_VALID;
        if (metadata->filterEnabled) {
            flags |= EEPROM_METADATA_FLAG_FILTER_ON;
        }
        recordingBytes = metadata->recordingBytes;
        durationSeconds = metadata->durationSeconds;
        sampleRateHz = metadata->sampleRateHz;
    }

    uint32_t adcMinMax = ((uint32_t)metadata->globalAdcMax << 16) |
                         (uint32_t)metadata->globalAdcMin;

    checksum = EEPROMMetadata_CalcChecksum(
        flags, recordingBytes, durationSeconds, sampleRateHz, adcMinMax);

    memset(sEEPROMEmulationBuffer, 0xFF, sizeof(sEEPROMEmulationBuffer));
    sEEPROMEmulationBuffer[EEPROM_METADATA_WORD_MAGIC] = EEPROM_METADATA_MAGIC;
    sEEPROMEmulationBuffer[EEPROM_METADATA_WORD_VERSION] =
        EEPROM_METADATA_VERSION;
    sEEPROMEmulationBuffer[EEPROM_METADATA_WORD_FLAGS] = flags;
    sEEPROMEmulationBuffer[EEPROM_METADATA_WORD_RECORDING_BYTES] =
        recordingBytes;
    sEEPROMEmulationBuffer[EEPROM_METADATA_WORD_DURATION_SECONDS] =
        durationSeconds;
    sEEPROMEmulationBuffer[EEPROM_METADATA_WORD_SAMPLE_RATE_HZ] =
        sampleRateHz;
    sEEPROMEmulationBuffer[EEPROM_METADATA_WORD_ADC_MINMAX] = adcMinMax;
    sEEPROMEmulationBuffer[EEPROM_METADATA_WORD_CHECKSUM] = checksum;
    sEEPROMEmulationBuffer[EEPROM_METADATA_WORD_CHECKSUM_INV] = ~checksum;
}

static bool EEPROMMetadata_Unpack(const uint32_t *buffer,
    EEPROMRecordingMetadata *metadata)
{
    uint32_t flags;
    uint32_t recordingBytes;
    uint32_t durationSeconds;
    uint32_t sampleRateHz;
    uint32_t adcMinMax;
    uint32_t byteRate;
    uint32_t checksum;

    EEPROMMetadata_Reset(metadata);

    if (buffer[EEPROM_METADATA_WORD_MAGIC] != EEPROM_METADATA_MAGIC) {
        return false;
    }

    if (buffer[EEPROM_METADATA_WORD_VERSION] != EEPROM_METADATA_VERSION) {
        return false;
    }

    flags = buffer[EEPROM_METADATA_WORD_FLAGS];
    recordingBytes = buffer[EEPROM_METADATA_WORD_RECORDING_BYTES];
    durationSeconds = buffer[EEPROM_METADATA_WORD_DURATION_SECONDS];

    sampleRateHz = buffer[EEPROM_METADATA_WORD_SAMPLE_RATE_HZ];
    if (EEPROMMetadata_BytesPerSecond(sampleRateHz) == 0u) {
        return false;
    }

    adcMinMax = buffer[EEPROM_METADATA_WORD_ADC_MINMAX];

    checksum = EEPROMMetadata_CalcChecksum(
        flags, recordingBytes, durationSeconds, sampleRateHz, adcMinMax);

    if ((buffer[EEPROM_METADATA_WORD_CHECKSUM] != checksum) ||
        (buffer[EEPROM_METADATA_WORD_CHECKSUM_INV] != ~checksum)) {
        return false;
    }

    if ((flags & EEPROM_METADATA_FLAG_VALID) == 0u) {
        if ((recordingBytes != 0u) || (durationSeconds != 0u)) {
            return false;
        }
        metadata->sampleRateHz = sampleRateHz;
        return true;
    }

    byteRate = EEPROMMetadata_BytesPerSecond(sampleRateHz);
    if ((byteRate == 0u) ||
        (durationSeconds != (recordingBytes / byteRate))) {
        return false;
    }

    metadata->hasRecording = true;
    metadata->filterEnabled =
        ((flags & EEPROM_METADATA_FLAG_FILTER_ON) != 0u);
    metadata->recordingBytes = recordingBytes;
    metadata->durationSeconds = durationSeconds;
    metadata->sampleRateHz = sampleRateHz;
    metadata->globalAdcMin = (uint16_t)(adcMinMax & 0xFFFFu);
    metadata->globalAdcMax = (uint16_t)(adcMinMax >> 16);
    return true;
}

static bool EEPROMMetadata_LoadInternal(EEPROMRecordingMetadata *metadata,
    uint32_t *initState)
{
    EEPROMMetadata_Reset(metadata);

    if (!EEPROMMetadata_EnsureInitialized(initState)) {
        return false;
    }

    EEPROMMetadata_RefreshBuffer();
    if (!EEPROMMetadata_Unpack(sEEPROMEmulationBuffer, metadata)) {
        EEPROMMetadata_Reset(metadata);
    }

    return true;
}

static bool EEPROMMetadata_WriteInternal(const EEPROMRecordingMetadata *metadata,
    uint32_t *initState, uint32_t *writeState)
{
    if (!EEPROMMetadata_EnsureInitialized(initState)) {
        if (writeState != NULL) {
            *writeState = EEPROM_EMULATION_WRITE_ERROR;
        }
        return false;
    }

    if (gEEPROMTypeAEraseFlag) {
        if (!EEPROM_TypeA_eraseLastSector()) {
            if (writeState != NULL) {
                *writeState = EEPROM_EMULATION_WRITE_ERROR;
            }
            return false;
        }
        gEEPROMTypeAEraseFlag = false;
    }

    EEPROMMetadata_Pack(metadata);
    sEEPROMEmulationState = EEPROM_TypeA_writeData(sEEPROMEmulationBuffer);
    if (writeState != NULL) {
        *writeState = sEEPROMEmulationState;
    }
    return (sEEPROMEmulationState == EEPROM_EMULATION_WRITE_OK);
}

static bool EEPROMMetadata_IsSame(const EEPROMRecordingMetadata *left,
    const EEPROMRecordingMetadata *right)
{
    return (left->hasRecording == right->hasRecording) &&
           (left->filterEnabled == right->filterEnabled) &&
           (left->recordingBytes == right->recordingBytes) &&
           (left->durationSeconds == right->durationSeconds) &&
           (left->sampleRateHz == right->sampleRateHz) &&
           (left->globalAdcMin == right->globalAdcMin) &&
           (left->globalAdcMax == right->globalAdcMax);
}

bool EEPROMMetadata_Load(EEPROMRecordingMetadata *metadata)
{
    if (metadata == NULL) {
        return false;
    }

    return EEPROMMetadata_LoadInternal(metadata, NULL);
}

bool EEPROMMetadata_SaveRecording(uint32_t recordingBytes,
    uint32_t sampleRateHz, bool filterEnabled,
    uint16_t globalAdcMin, uint16_t globalAdcMax)
{
    EEPROMRecordingMetadata metadata;
    uint32_t byteRate;

    EEPROMMetadata_Reset(&metadata);
    if (recordingBytes > 0u) {
        byteRate = EEPROMMetadata_BytesPerSecond(sampleRateHz);
        if (byteRate == 0u) {
            return false;
        }

        metadata.hasRecording = true;
        metadata.filterEnabled = filterEnabled;
        metadata.recordingBytes = recordingBytes;
        metadata.durationSeconds = recordingBytes / byteRate;
        metadata.sampleRateHz = sampleRateHz;
        metadata.globalAdcMin = globalAdcMin;
        metadata.globalAdcMax = globalAdcMax;
    }

    return EEPROMMetadata_WriteInternal(&metadata, NULL, NULL);
}

bool EEPROMMetadata_Clear(void)
{
    EEPROMRecordingMetadata metadata;

    EEPROMMetadata_Reset(&metadata);
    return EEPROMMetadata_WriteInternal(&metadata, NULL, NULL);
}

bool EEPROMMetadata_EraseAll(void)
{
    sEEPROMInitialized = false;
    if (!EEPROM_TypeA_eraseAllSectors()) {
        return false;
    }

    memset(sEEPROMEmulationBuffer, 0xFF, sizeof(sEEPROMEmulationBuffer));
    EEPROMMetadata_ResetTypeAState();
    sEEPROMEmulationState = EEPROM_EMULATION_INIT_OK;
    return true;
}

bool EEPROMMetadata_Test(uint32_t *initState, uint32_t *writeState,
    uint32_t *writtenValue, uint32_t *readValue)
{
    EEPROMRecordingMetadata metadata;
    EEPROMRecordingMetadata readBack;

    *initState = EEPROM_EMULATION_INIT_OK;
    *writeState = EEPROM_EMULATION_WRITE_OK;
    *writtenValue = 0u;
    *readValue = 0u;

    if (!EEPROMMetadata_LoadInternal(&metadata, initState)) {
        return false;
    }

    *writtenValue = metadata.recordingBytes;
    if (!EEPROMMetadata_WriteInternal(&metadata, initState, writeState)) {
        return false;
    }

    if (!EEPROMMetadata_Load(&readBack)) {
        return false;
    }

    *readValue = readBack.recordingBytes;
    return EEPROMMetadata_IsSame(&metadata, &readBack);
}