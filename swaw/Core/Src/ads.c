#include "ads.h"
#include <string.h>
#include "proto.h"
#include "main.h"
#include <stdbool.h>
#include "adsdef.h"

static struct AdsData AdsData[2];
static int handle = -1;
static SPI_HandleTypeDef *AdsSpi;
static DMA_HandleTypeDef *AdsDmaRx;


struct AdsState
{
	bool readMode;
	bool inProgress;
	bool send;
	uint8_t index;

} static AdsState = {.readMode = false, .inProgress = false, .send = false, .index = 0};

static uint8_t AdsDummyTxData[sizeof(struct AdsState)] = {0};

#define CHIP_SELECT() HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET)
#define CHIP_DESELECT() HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET)

#define START() HAL_GPIO_WritePin(ADS_START_GPIO_Port, ADS_START_Pin, GPIO_PIN_SET)
#define STOP() HAL_GPIO_WritePin(ADS_START_GPIO_Port, ADS_START_Pin, GPIO_PIN_RESET)

static void AdsSendCommand(enum AdsCommand command);
static void AdsWriteRegister(enum AdsRegister reg, uint8_t val);
static uint8_t AdsReadRegister(enum AdsRegister reg);
static bool AdsStopConversion(void);
static void AdsRestartConversion(bool previous);

static void AdsRequestRxCallback(void *buffer, size_t size)
{

}


void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	CHIP_DESELECT();

	AdsState.send = true;
	AdsState.index ^= 1; //switch to the other buffer
}

void AdsReadyCallback(uint16_t GPIO_Pin)
{
	if(AdsState.readMode)
	{
		AdsState.inProgress = true;
		CHIP_SELECT();
		HAL_SPI_TransmitReceive_DMA(AdsSpi, AdsDummyTxData, (uint8_t*)&(AdsData[AdsState.index]), sizeof(AdsData[0]));
	}
	else
	{
		STOP();
		AdsState.inProgress = false;
	}
}

static bool AdsStopConversion(void)
{
	bool previous = AdsState.readMode;
	while(true == AdsState.inProgress)
		;

	if(previous)
		AdsSendCommand(ADS_SDATAC);
	return previous;
}

static void AdsRestartConversion(bool previous)
{
	AdsState.readMode = previous;
	if(previous)
	{
		START();
		AdsSendCommand(ADS_RDATAC);
	}
}

static void AdsSendCommand(enum AdsCommand command)
{
	bool previous;
	if((ADS_SDATAC != command) && (ADS_RDATAC != command))
		previous = AdsStopConversion();

	uint8_t d = command;
	CHIP_SELECT();
	HAL_SPI_Transmit(AdsSpi, &d, 1, 20);
	CHIP_DESELECT();

	if((ADS_SDATAC != command) && (ADS_RDATAC != command))
		AdsRestartConversion(previous);
}

static void AdsWriteRegister(enum AdsRegister reg, uint8_t val)
{
	uint8_t data[3];
	data[0] = ADS_WREG_BITS | (reg & ADS_RREG_WREG_MASK);
	data[1] = 0; //count of bytes - 1
	data[2] = val;

	CHIP_SELECT();
	HAL_SPI_Transmit(AdsSpi, data, 3, 20);
	CHIP_DESELECT();
}

static uint8_t AdsReadRegister(enum AdsRegister reg)
{
	bool previous = AdsStopConversion();

	uint8_t data[3], dataRx[3];
	data[0] = ADS_RREG_BITS | (reg & ADS_RREG_WREG_MASK);
	data[1] = 0; //count of bytes - 1
	data[2] = 0; //dummy

	CHIP_SELECT();
	HAL_SPI_TransmitReceive(AdsSpi, data, dataRx, 3, 20);
	CHIP_DESELECT();

	AdsRestartConversion(previous);

	return dataRx[2];
}

bool AdsCheckBiasLeadState(void)
{
	return !(AdsReadRegister(CONFIG3) & ADS_CONFIG3_BIAS_STAT);
}

void AdsSetChannelConfig(uint8_t channel, bool enable, enum AdsChannelMode mode, enum AdsChannelGain gain, bool biasDerivation, bool leadOffDetection)
{
	if(channel >= ADS_CHANNEL_COUNT)
		return;

	mode &= 0x7;
	gain &= 0x7;

	bool previous = AdsStopConversion();

	AdsWriteRegister(ADS_CHnSET(channel), ((!enable) << 7) | (gain << 4) | mode);

	uint8_t d = 0;
	d = AdsReadRegister(BIAS_SENSP);
	if(biasDerivation)
		d |= ADS_BIAS_SENSx_BIT(channel);
	else
		d &= ~ADS_BIAS_SENSx_BIT(channel);
	AdsWriteRegister(BIAS_SENSP, d);

	d = AdsReadRegister(LOFF_SENSP);
	if(leadOffDetection)
		d |= ADS_LOFF_SENSx_BIT(channel);
	else
		d &= ~ADS_LOFF_SENSx_BIT(channel);
	AdsWriteRegister(LOFF_SENSP, d);

	AdsRestartConversion(previous);
}


void AdsSetLeadOffConfig(enum AdsLeadOffThreshold level, enum AdsLeadOffCurrent current, enum AdsLeadOffFrequency freq)
{
	level &= 0x7;
	current &= 0x3;
	freq &= 0x3;

	AdsWriteRegister(LOFF, ADS_LOFF_RSVD | (level << 5) | (current << 2) | (freq));
}

void AdsSetBiasGain(enum AdsBiasGain gain)
{
	uint8_t d = 0;
	switch(gain)
	{
		//gain=18 requires all 4 bits to be set, gain=14 requires 3 bits, etc.
		case ADS_BIAS_GAIN_18:
			d |= ADS_BIAS_SENSx_BIT(3);
		case ADS_BIAS_GAIN_14:
			d |= ADS_BIAS_SENSx_BIT(2);
		case ADS_BIAS_GAIN_9:
			d |= ADS_BIAS_SENSx_BIT(1);
		case ADS_BIAS_GAIN_5:
			d |= ADS_BIAS_SENSx_BIT(0);
			break;
		case ADS_BIAS_GAIN_0:
		default:
			break;

	}
	AdsWriteRegister(BIAS_SENSN, d);
}

void AdsSetDataRate(enum AdsDataRate freq)
{
	freq &= 0x7;
	AdsWriteRegister(CONFIG1, ADS_CONFIG1_RSVD | freq);
}

void AdsReadId(uint8_t *channels, uint8_t *deviceId, uint8_t *revisionId)
{
	uint8_t d = AdsReadRegister(ID);
	*channels = d & 0x3;
	*deviceId = (d >> 2) & 0x3;
	*revisionId = (d >> 5) & 0x7;
}

void AdsProcess(void)
{
	if(AdsState.send)
	{
		AdsState.send = false;
		ProtoSend(handle, &(AdsData[AdsState.index ^ 1]), sizeof(AdsData[0]));
	}

	//HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, AdsState.readMode ? SET : RESET);
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, (GPIOB->IDR & (1 << 8)) ? RESET : SET);
}

void AdsInit(SPI_HandleTypeDef *hspi, DMA_HandleTypeDef *hDmaRx)
{
	AdsSpi = hspi;
	AdsDmaRx = hDmaRx;
	CHIP_DESELECT();
	STOP();
	//ADS reset
	HAL_GPIO_WritePin(ADS_RESET_GPIO_Port, ADS_RESET_Pin, GPIO_PIN_RESET);
	HAL_Delay(100);
	HAL_GPIO_WritePin(ADS_RESET_GPIO_Port, ADS_RESET_Pin, GPIO_PIN_SET);
	HAL_Delay(300); //150 ms typical startup time

	AdsSendCommand(ADS_SDATAC); //stop data read
	//set output data rate to 1 ksps
	AdsSetDataRate(ADS_1K);
	//enable internal bias reference, bias buffer, bias lead off detection and internal reference buffer
	AdsWriteRegister(CONFIG3, ADS_CONFIG3_RSVD | ADS_CONFIG3_BIASREF_INT | ADS_CONFIG3_PD_BIAS | ADS_CONFIG3_BIAS_LOFF_SENS | ADS_CONFIG3_PD_REFBUF);
	//set initial lead off current sources config to 6 nA DC and the comparator threshold to 95%/5%
	AdsSetLeadOffConfig(ADS_LO_P95_N5, ADS_LO_6NA, ADS_LO_DC);
	//enable lead off comparator
	AdsWriteRegister(CONFIG4, ADS_CONFIG4_RSVD | ADS_CONFIG4_PD_LOFF_COMP);
	//enable SRB1, which is the reference electrode input
	AdsWriteRegister(MISC1, ADS_MISC1_RSVD | ADS_MISC1_SRB1_BIT);
	//enable internal test signal generation
	AdsWriteRegister(CONFIG2, ADS_CONFIG2_RSVD | ADS_CONFIG2_INT_CAL);
	//set reference derived bias gain to 5 (one amplifier)
	AdsSetBiasGain(ADS_BIAS_GAIN_5);

	//set initial channel config - normal mode, 1x gain, no bias derivation, enable lead off detection (current source)
	for(uint8_t i = 0; i < ADS_CHANNEL_COUNT; i++)
		AdsSetChannelConfig(i, true, ADS_CHANNEL_NORMAL, ADS_GAIN_1, false, true);

	handle = ProtoRegister(ADS_PROTO_ID, AdsRequestRxCallback);

	AdsRestartConversion(true);
}
