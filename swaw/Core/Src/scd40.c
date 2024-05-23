
#include "scd40.h"
#include "i2c.h"
#include "proto.h"

#define CRC8_POLYNOMIAL 0x31
#define CRC8_INIT 0xFF


#define SCD40_INTERVAL 10000 //ms
// SCD40 Sensor address
#define SCD40_ADDRESS 0b1100010 << 1
// Used command (MemAddr)
#define START_PERIODIC_MEASURMENT_COMMAND 0x21b1
#define STOP_PERIODIC_MEASURMENT_COMMAND 0x3f86
#define READ_MEASURMENT_COMMAND 0xec05
#define GET_DATA_READY_STATUS_COMMAND 0xe4b8

extern I2C_HandleTypeDef hi2c2;

// Structure represent read value from SCD40 sensor
struct SCD40
{
    uint16_t co2_level;
    int16_t temperature;
    uint16_t humidity;
} static Scd40Data;

static bool Scd40DataReady = false;

static int handle = -1;

static uint8_t Scd40Buffer[9];
enum Scd40State
{
	SCD40_IDLE = 0,
	SCD40_START = 1,
	SCD40_CHECK_READY = 2,
	SCD40_READ = 3,
	SCD40_STOP = 4
} static Scd40State = SCD40_IDLE;

static uint32_t Scd40Timer = 0;

static uint8_t crc_generate(const uint8_t* data, uint16_t count)
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

static uint8_t start_periodic_measurement(void)
{
    if(HAL_OK == HAL_I2C_Mem_Write_IT(&hi2c2, SCD40_ADDRESS, START_PERIODIC_MEASURMENT_COMMAND, I2C_MEMADD_SIZE_16BIT, NULL, 0))
    {
    	return 0;
    }
    return 1;
}

static uint8_t stop_periodic_measurement(void)
{
	if(HAL_OK == HAL_I2C_Mem_Write_IT(&hi2c2, SCD40_ADDRESS, STOP_PERIODIC_MEASURMENT_COMMAND, I2C_MEMADD_SIZE_16BIT, NULL, 0))
	{
		return 0;
    }
    return 1;
}

static uint8_t check_data_ready(void)
{
	if(HAL_OK == HAL_I2C_Mem_Read_IT(&hi2c2, SCD40_ADDRESS, GET_DATA_READY_STATUS_COMMAND, I2C_MEMADD_SIZE_16BIT, Scd40Buffer, 2))
	{
		return 0;
    }
    return 1;
}

static uint8_t read_data(void)
{
	if(HAL_OK == HAL_I2C_Mem_Read_IT(&hi2c2, SCD40_ADDRESS, READ_MEASURMENT_COMMAND, I2C_MEMADD_SIZE_16BIT, Scd40Buffer, 9))
	{
		return 0;
    }
    return 1;
}


static void parse()
{
	// Scd40Buffer contains: co2_MSB, co2_LSB, co2_CRC, temp_MSB, temp_LSB, temp_CRC, hum_MSB, hum_LSB, hum_CRC
    if(Scd40Buffer[2] == crc_generate(&Scd40Buffer[0], 2))
    {
        sensor->co2_level = (Scd40Buffer[0] << 8) | Scd40Buffer[1];
    }
    if(Scd40Buffer[5] == crc_generate(&Scd40Buffer[3], 2))
    {
        int32_t t = (Scd40Buffer[3] << 8) | Scd40Buffer[4];
        sensor->temperature = ((175 * t) / 65535) - 45;
    }
    if(Scd40Buffer[8] == crc_generate(&Scd40Buffer[6], 2))
    {
        int32_t t = (Scd40Buffer[6] << 8) | Scd40Buffer[7];
        sensor->humidity = (100 * t) / 65535;
    }
}

void Scd40Init(void)
{
	handle = ProtoRegister("CO2 ", NULL);

	while(!I2cCheckFree())
		;
	I2cLock();
	Scd40State = SCD40_START;
	start_periodic_measurement();
}

void Scd40HandleInterrupt(void)
{
	switch(Scd40State)
	{
		case SCD40_IDLE:
			break;
		case SCD40_START:
			I2cUnlock();
			Scd40Timer = HAL_GetTick() + SCD40_INTERVAL;
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
				read_data();
			}
			break;
		case SCD40_READ:
			I2cUnlock();
			parse_data();
			Scd40DataReady = true;
			Scd40State = SCD40_IDLE;
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
			check_data_ready();
		}

	}
}

