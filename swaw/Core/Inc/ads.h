
#ifndef INC_ADS_H_
#define INC_ADS_H_

#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx.h"

/**
 * @brief Protocol data ID
 */
#define ADS_PROTO_ID "EEG "

/**
 * @brief Number of ADS1299 channels
 */
#define ADS_CHANNEL_COUNT 4

/**
 * @brief ADS1299 data structure as sent to the PC
 */
struct AdsData
{
	uint8_t status[3];
	struct  __attribute__((packed))
	{
		uint8_t byte[3];
	} channel[ADS_CHANNEL_COUNT];
}  __attribute__ ((packed));

/**
 * @brief Data rate (effective sampling frequency) values
 */
enum AdsDataRate
{
	ADS_16K = 0x0,
	ADS_8K = 0x1,
	ADS_4K = 0x2,
	ADS_2K = 0x3,
	ADS_1K = 0x4,
	ADS_500 = 0x5,
	ADS_250 = 0x6
};

/**
 * @brief Lead-off detection comparator thresholds
 */
enum AdsLeadOffThreshold
{
	ADS_LO_P95_N5 = 0x0,
	ADS_LO_P92_N7 = 0x1,
	ADS_LO_P90_N10 = 0x2,
	ADS_LO_P87_N12 = 0x03,
	ADS_LO_P85_N15 = 0x04,
	ADS_LO_P80_N20 = 0x05,
	ADS_LO_P75_N25 = 0x06,
	ADS_LO_P70_N30 = 0x07
};

/**
 * @brief Lead-off detection current values
 */
enum AdsLeadOffCurrent
{
	ADS_LO_6NA = 0x0,
	ADS_LO_24NA = 0x1,
	ADS_LO_6UA = 0x2,
	ADS_LO_24UA = 0x3,
};

/**
 * @brief Lead-off detection frequency values
 */
enum AdsLeadOffFrequency
{
	ADS_LO_DC = 0x0,
	ADS_LO_7 = 0x1,
	ADS_LO_31 = 0x2,
	ADS_LO_DR = 0x03
};

/**
 * @brief Channel gain values
 */
enum AdsChannelGain
{
	ADS_GAIN_1 = 0x0,
	ADS_GAIN_2 = 0x1,
	ADS_GAIN_4 = 0x2,
	ADS_GAIN_6 = 0x3,
	ADS_GAIN_8 = 0x4,
	ADS_GAIN_12 = 0x5,
	ADS_GAIN_24 = 0x6
};

/**
 * @brief Channel modes
 */
enum AdsChannelMode
{
	ADS_CHANNEL_NORMAL = 0x0,
	ADS_CHANNEL_SHORTED = 0x1, //in+ and in- shorted to each other and to vref
	ADS_CHANNEL_BIAS_MEASUREMENT = 0x2,
	ADS_CHANNEL_SUPPLY_MEASUREMENT = 0x3,
	ADS_CHANNEL_TEMPERATURE = 0x4,
	ADS_CHANNEL_TEST = 0x5,
	ADS_CHANNEL_BIAS_POS = 0x6, //positive electrode as bias output
	ADS_CHANNEL_BIAS_NEG = 0x7, //negative elecrode as bias outptu
};

/**
 * @brief Reference input-derived bias gain values
 */
enum AdsBiasGain
{
	ADS_BIAS_GAIN_0 = 0,
	ADS_BIAS_GAIN_5,
	ADS_BIAS_GAIN_9,
	ADS_BIAS_GAIN_14,
	ADS_BIAS_GAIN_18,

};

/**
 * @brief Initialize ADS1299 with default configuration and start
 * @param *hspi SPI handler
 * @param *hDmaRx SPI RX DMA handler
 */
void AdsInit(SPI_HandleTypeDef *hspi, DMA_HandleTypeDef *hDmaRx);

/**
 * @brief Process ADS1299 events
 * @attention Run this function is the main loop
 */
void AdsProcess(void);

/**
 * @brief ADS1299 data ready callback
 * @attention Call this function in external interrupt handler
 */
void AdsReadyCallback(uint16_t GPIO_Pin);

/**
 * @brief Check bias lead connection state
 * @return Bias lead connection state: true if connected, false otherwise
 */
bool AdsCheckBiasLeadState(void);

/**
 * @brief Set input channel configuration
 * @param channel Channel number (starting from 0)
 * @param enable Enable or disable channel
 * @param mode Channel operation mode
 * @param gain Channel gain
 * @param biasDerivation Enable bias derivation from this channel
 * @param leadOffDetection Enable lead-off detection current source
 */
void AdsSetChannelConfig(uint8_t channel, bool enable, enum AdsChannelMode mode, enum AdsChannelGain gain, bool biasDerivation, bool leadOffDetection);

/**
 * @brief Set lead-off detection configuration
 * @param level Comparator threshold
 * @param current Current value
 * @param freq Detection frequency
 */
void AdsSetLeadOffConfig(enum AdsLeadOffThreshold level, enum AdsLeadOffCurrent current, enum AdsLeadOffFrequency freq);

/**
 * @brief Set reference input-derived bias gain
 * @param gain Bias gain
 */
void AdsSetBiasGain(enum AdsBiasGain gain);

/**
 * @brief Set data rate (effective sampling frequency)
 * @param freq Data rate
 */
void AdsSetDataRate(enum AdsDataRate freq);

/**
 * @brief Read device ID and other information
 * @param *channels Number of channels (output)
 * @param *deviceId Device ID (output)
 * @param *revisionId Revision ID (output)
 */
void AdsReadId(uint8_t *channels, uint8_t *deviceId, uint8_t *revisionId);

#endif /* INC_ADS_H_ */
