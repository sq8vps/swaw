#include "ads.h"
#include <string.h>
#include "proto.h"
#include "main.h"
#include <stdbool.h>
#include "adsdef.h"

#define ADS_PROTO_ID "EEG "
#define ADS_INPUT_COUNT 4

static struct AdsData AdsData[2];
static int handle = -1;

struct AdsState
{
	bool readMode;
	bool inProgress;
	bool send;
	uint8_t index;

} static AdsState = {.readMode = false, .inProgress = false, .send = false, .index = 0};

static uint8_t AdsDummyTxData[sizeof(struct AdsState)] = {0};

extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef hdma_spi1_rx;

#define CHIP_SELECT() HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET)
#define CHIP_DESELECT() HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET)

#define START() HAL_GPIO_WritePin(ADS_START_GPIO_Port, ADS_START_Pin, GPIO_PIN_SET)
#define STOP() HAL_GPIO_WritePin(ADS_START_GPIO_Port, ADS_START_Pin, GPIO_PIN_RESET)

void AdsSendCommand(enum AdsCommand command);
static void AdsWriteRegister(enum AdsRegister reg, uint8_t val);
static void AdsWriteRegisterEx(enum AdsRegister startReg, uint8_t *data, uint8_t size);
static uint8_t AdsReadRegister(enum AdsRegister reg);
static void AdsReadRegisterEx(enum AdsRegister startReg, uint8_t *data, uint8_t size);
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
		HAL_SPI_TransmitReceive_DMA(&hspi1, AdsDummyTxData, (uint8_t*)&(AdsData[AdsState.index]), sizeof(AdsData[0]));
	}
	else
	{
		STOP();
		AdsState.inProgress = false;
	}
}


uint8_t AdsCheckInputLeadState(void)
{
	return ~(AdsReadRegister(LOFF_STATP) & ((1 << ADS_INPUT_COUNT) - 1));
}

bool AdsCheckBiasLeadState(void)
{
	return !(AdsReadRegister(CONFIG3) & ADS_CONFIG3_BIAS_STAT);
}

void AdsSetChannelConfig(uint8_t channel, bool enable, enum AdsChannelMode mode, enum AdsChannelGain gain, bool biasDerivation, bool leadOffDetection)
{
	if(channel >= ADS_INPUT_COUNT)
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
	if(biasDerivation)
		d |= ADS_LOFF_SENSx_BIT(channel);
	else
		d &= ~ADS_LOFF_SENSx_BIT(channel);
	AdsWriteRegister(LOFF_SENSP, d);

	AdsRestartConversion(previous);
}


/**
 * @brief Set lead-off detection configuration
 * @param level Comparator threshold
 * @param current Current magnitude
 * @param frequency Frequency
 */
void AdsSetLeadOffConfig(enum AdsLeadOffThreshold level, enum AdsLeadOffCurrent current, enum AdsLeadOffFrequency freq)
{
	level &= 0x7;
	current &= 0x3;
	freq &= 0x3;

	AdsWriteRegister(LOFF, ADS_LOFF_RSVD | (level << 5) | (current << 2) | (freq));
}

/**
 * @brief Set output data rate
 * @param freq Data rate
 */
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

void AdsSendCommand(enum AdsCommand command)
{
	bool previous;
	if((ADS_SDATAC != command) && (ADS_RDATAC != command))
		previous = AdsStopConversion();

	uint8_t d = command;
	CHIP_SELECT();
	HAL_SPI_Transmit(&hspi1, &d, 1, 20);
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
	HAL_SPI_Transmit(&hspi1, data, 3, 20);
	CHIP_DESELECT();
}

//static void AdsWriteRegisterEx(enum AdsRegister startReg, uint8_t *data, uint8_t size)
//{
//	uint8_t header[2];
//	header[0] = ADS_WREG_BITS | (startReg & ADS_RREG_WREG_MASK);
//	header[1] = (size - 1) & ADS_RREG_WREG_SIZE_MASK;
//	CHIP_SELECT();
//	HAL_SPI_Transmit(&hspi1, header, 2, 20);
//	HAL_SPI_Transmit(&hspi1, data, size, size * 10 + 10);
//	CHIP_DESELECT();
//}

static uint8_t AdsReadRegister(enum AdsRegister reg)
{
	bool previous = AdsStopConversion();

	uint8_t data[3], dataRx[3];
	data[0] = ADS_RREG_BITS | (reg & ADS_RREG_WREG_MASK);
	data[1] = 0; //count of bytes - 1
	data[2] = 0; //dummy

	CHIP_SELECT();
	HAL_SPI_TransmitReceive(&hspi1, data, dataRx, 3, 20);
	CHIP_DESELECT();

	AdsRestartConversion(previous);

	return dataRx[2];
}

//static void AdsReadRegisterEx(enum AdsRegister startReg, uint8_t *data, uint8_t size)
//{
//	bool previous = AdsStopConversion();
//
//	uint8_t header[2];
//	header[0] = ADS_RREG_BITS | (startReg & ADS_RREG_WREG_MASK);
//	header[1] = (size - 1) & ADS_RREG_WREG_SIZE_MASK;
//	CHIP_SELECT();
//	HAL_SPI_Transmit(&hspi1, header, 2, 20);
//	HAL_SPI_Receive(&hspi1, data, size, size * 10 + 10);
//	CHIP_DESELECT();
//
//	AdsRestartConversion(previous);
//}

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

void AdsInit(void)
{

	CHIP_DESELECT();
	STOP();
	//ADS reset
	HAL_GPIO_WritePin(ADS_RESET_GPIO_Port, ADS_RESET_Pin, GPIO_PIN_RESET);
	HAL_Delay(100);
	HAL_GPIO_WritePin(ADS_RESET_GPIO_Port, ADS_RESET_Pin, GPIO_PIN_SET);
	HAL_Delay(300); //150 ms typical startup time

//	AdsSendCommand(ADS_SDATAC);
//
//	AdsWriteRegister(CONFIG3, ADS_CONFIG3_RSVD | ADS_CONFIG3_PD_BIAS);
//	AdsSetDataRate(ADS_250);
//	AdsWriteRegister(CONFIG2, ADS_CONFIG2_RSVD);
//	AdsWriteRegister(ADS_CHnSET(0), 1);
//	AdsWriteRegister(ADS_CHnSET(1), 1);
//	AdsWriteRegister(ADS_CHnSET(2), 1);
//	AdsWriteRegister(ADS_CHnSET(3), 1);
//
//	START();
//	AdsSendCommand(ADS_RDATAC);
//	AdsState.readMode = true;

	AdsSendCommand(ADS_SDATAC); //stop data read
	//set output data rate to 1 ksps
	AdsSetDataRate(ADS_1K);
	//enable internal bias reference, bias buffer and bias lead off detection
	AdsWriteRegister(CONFIG3, ADS_CONFIG3_RSVD | ADS_CONFIG3_BIASREF_INT | ADS_CONFIG3_PD_BIAS | ADS_CONFIG3_BIAS_LOFF_SENS);
	//set initial lead off config
	AdsSetLeadOffConfig(ADS_LO_P95_N5, ADS_LO_6NA, ADS_LO_DC);
	//enable SRB1, which is the reference electrode input
	AdsWriteRegister(MISC1, ADS_MISC1_RSVD | ADS_MISC1_SRB1_BIT);
	//enable lead off comparator
	AdsWriteRegister(CONFIG4, ADS_CONFIG4_RSVD | ADS_CONFIG4_PD_LOFF_COMP);
	//enable internal test signal generation
	AdsWriteRegister(CONFIG2, ADS_CONFIG2_RSVD | ADS_CONFIG2_INT_CAL);

	//set initial channel config - normal mode, gain 8x, bias derivation and lead off detection
	for(uint8_t i = 0; i < ADS_INPUT_COUNT; i++)
		AdsSetChannelConfig(i, true, ADS_CHANNEL_SHORTED, ADS_GAIN_24, true, true);

	handle = ProtoRegister(ADS_PROTO_ID, AdsRequestRxCallback);

	AdsRestartConversion(true);
}
