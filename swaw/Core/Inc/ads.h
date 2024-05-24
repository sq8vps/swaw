
#ifndef INC_ADS_H_
#define INC_ADS_H_

#include <stdint.h>

#define ADS_CHANNEL_COUNT 4

struct AdsData
{
	uint8_t status[3];
	struct  __attribute__((packed))
	{
		uint8_t byte[3];
	}
	channel[ADS_CHANNEL_COUNT];
}  __attribute__ ((packed));

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

enum AdsLeadOffCurrent
{
	ADS_LO_6NA = 0x0,
	ADS_LO_24NA = 0x1,
	ADS_LO_6UA = 0x2,
	ADS_LO_24UA = 0x3,
};

enum AdsLeadOffFrequency
{
	ADS_LO_DC = 0x0,
	ADS_LO_7 = 0x1,
	ADS_LO_31 = 0x2,
	ADS_LO_DR = 0x03
};

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

void AdsInit(void);

void AdsProcess(void);

#endif /* INC_ADS_H_ */
