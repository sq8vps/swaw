
#include "scd40.h"
#include "i2c.h"
#include "proto.h"
#include "stm32f1xx.h"

//#define SCD40_DISABLE_AUTOCALIBRATION

#define CRC8_POLYNOMIAL 0x31
#define CRC8_INIT 0xFF

#define SCD40_ADDRESS 0x62 << 1

enum Scd40Command
{
	SCD40_START_PERIODIC_MEASUREMENT = 0x21b1,
	SCD40_STOP_PERIODIC_MEASUREMENT = 0x3f86,
	SCD40_READ_MEASUREMENT = 0xec05,
	SCD40_GET_DATA_READY_STATUS = 0xe4b8,
	SCD40_GET_SERIAL_NUMBER = 0x3682,
	SCD40_SET_AUTOMATIC_SELF_CALIBRATION_ENABLED = 0x2416,
};

static I2C_HandleTypeDef *ScdI2c = NULL;

static struct Scd40Data Scd40Data;

volatile static bool Scd40DataReady = false;

static int handle = -1;

volatile static uint8_t Scd40Buffer[9];
enum Scd40State
{
	SCD40_IDLE = 0,
	SCD40_CHECK_READY,
	SCD40_READ,
} volatile static Scd40State = SCD40_IDLE;

static uint32_t Scd40Timer = 0;

/**
 * @brief Calculate CRC8 for SCD40
 * @param *data Input data
 * @param count Byte count
 * @return Calculated CRC
 * @note This function comes directly from the datasheet
 */
static uint8_t Scd40CRC8(const uint8_t *data, uint16_t count)
{
    uint16_t current_byte;
    uint8_t crc = CRC8_INIT;
    uint8_t crc_bit;
    /* calculates 8-Bit checksum with given polynomial */
    for (current_byte = 0; current_byte < count; ++current_byte)
    {
        crc ^= (data[current_byte]);
        for (crc_bit = 8; crc_bit > 0; --crc_bit)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ CRC8_POLYNOMIAL;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}

/**
 * @brief Parse received data and fill the structure
 */
static void Scd40Parse(void)
{
	// Scd40Buffer contains: co2_MSB, co2_LSB, co2_CRC, temp_MSB, temp_LSB, temp_CRC, hum_MSB, hum_LSB, hum_CRC
    if(Scd40Buffer[2] == Scd40CRC8((uint8_t*)&Scd40Buffer[0], 2))
    {
        Scd40Data.co2 = (Scd40Buffer[0] << 8) | Scd40Buffer[1];
    }
    if(Scd40Buffer[5] == Scd40CRC8((uint8_t*)&Scd40Buffer[3], 2))
    {
        int32_t t = (Scd40Buffer[3] << 8) | Scd40Buffer[4];
        Scd40Data.temperature = ((175 * t) / 65535) - 45;
    }
    if(Scd40Buffer[8] == Scd40CRC8((uint8_t*)&Scd40Buffer[6], 2))
    {
        int32_t t = (Scd40Buffer[6] << 8) | Scd40Buffer[7];
        Scd40Data.humidity = (100 * t) / 65535;
    }
}

/**
 * @brief Send SCD40 command in blocking mode
 * @param cmd Command to send
 */
static void Scd40SendCommandBlocking(enum Scd40Command cmd)
{
	Scd40Buffer[0] = (cmd >> 8);
	Scd40Buffer[1] = cmd & 0xFF;
	HAL_I2C_Master_Transmit(ScdI2c, SCD40_ADDRESS, (uint8_t*)Scd40Buffer, 2, HAL_MAX_DELAY);
}

/**
 * @brief Read SCD40 serial number (blocking)
 * @return Serial number or 0 on failure
 */
static uint64_t Scd40ReadSerial(void)
{
	bool crcOk = true;
	HAL_I2C_Mem_Read(ScdI2c, SCD40_ADDRESS, SCD40_GET_SERIAL_NUMBER, I2C_MEMADD_SIZE_16BIT, (uint8_t*)Scd40Buffer, 9, HAL_MAX_DELAY);
	if(Scd40CRC8((uint8_t*)&(Scd40Buffer[0]), 2) != Scd40Buffer[2])
		crcOk = false;
	if(Scd40CRC8((uint8_t*)&(Scd40Buffer[3]), 2) != Scd40Buffer[5])
		crcOk = false;
	if(Scd40CRC8((uint8_t*)&(Scd40Buffer[6]), 2) != Scd40Buffer[8])
		crcOk = false;

	if(crcOk)
	{
		uint64_t serial = 0;
		serial |= Scd40Buffer[0];
		serial <<= 8;
		serial |= Scd40Buffer[1];
		serial <<= 8;
		serial |= Scd40Buffer[3];
		serial <<= 8;
		serial |= Scd40Buffer[4];
		serial <<= 8;
		serial |= Scd40Buffer[6];
		serial <<= 8;
		serial |= Scd40Buffer[7];
		return serial;
	}
	else
		return 0;
}

static void Scd40DisableAutoCalibration(void)
{
	Scd40Buffer[0] = 0;
	Scd40Buffer[1] = 0;
	Scd40Buffer[2] = Scd40CRC8((uint8_t*)Scd40Buffer, 2);
	HAL_I2C_Mem_Write(ScdI2c, SCD40_ADDRESS, SCD40_SET_AUTOMATIC_SELF_CALIBRATION_ENABLED, I2C_MEMADD_SIZE_16BIT, (uint8_t*)Scd40Buffer, 3, HAL_MAX_DELAY);
}

void Scd40Init(I2C_HandleTypeDef *hi2c)
{
	ScdI2c = hi2c;
	handle = ProtoRegister(SCD40_PROTO_ID, NULL);

	while(!I2cCheckFree())
		;
	I2cLock();

#ifdef SCD40_DISABLE_AUTOCALIBRATION
	Scd40SendCommandBlocking(SCD40_STOP_PERIODIC_MEASUREMENT);
	HAL_Delay(600); //500ms delay after stop command according to the datasheet

	Scd40DisableAutoCalibration();
#endif
	Scd40SendCommandBlocking(SCD40_START_PERIODIC_MEASUREMENT);

	I2cUnlock();
	Scd40Timer = HAL_GetTick() + SCD40_INTERVAL;
}

void Scd40HandleInterrupt(void)
{
	switch(Scd40State)
	{
		case SCD40_IDLE:
			break;
		case SCD40_CHECK_READY:
			if(((Scd40Buffer[0] & 0x07) == 0) && (Scd40Buffer[1] == 0))
			{
				I2cUnlock();
				Scd40State = SCD40_IDLE;
				//not ready, wait for the next measurement cycle
			}
			else
			{
				Scd40State = SCD40_READ;
				HAL_I2C_Mem_Read_IT(ScdI2c, SCD40_ADDRESS, SCD40_READ_MEASUREMENT, I2C_MEMADD_SIZE_16BIT, (uint8_t*)Scd40Buffer, 9);
			}
			break;
		case SCD40_READ:
			I2cUnlock();
			Scd40Parse();
			Scd40DataReady = true;
			Scd40State = SCD40_IDLE;
			break;
		default:
			break;
	}
}

void Scd40Process(void)
{
	if(Scd40DataReady)
	{
		ProtoSend(handle, &Scd40Data, sizeof(Scd40Data));
		Scd40DataReady = false;
	}

	if((HAL_GetTick() > Scd40Timer) && (Scd40Timer != 0))
	{
		if(I2cCheckFree())
		{
			I2cLock();
			Scd40Timer = HAL_GetTick() + SCD40_INTERVAL;
			Scd40State = SCD40_CHECK_READY;
			HAL_I2C_Mem_Read_IT(ScdI2c, SCD40_ADDRESS, SCD40_GET_DATA_READY_STATUS, I2C_MEMADD_SIZE_16BIT, (uint8_t*)Scd40Buffer, 3);
		}

	}
}

