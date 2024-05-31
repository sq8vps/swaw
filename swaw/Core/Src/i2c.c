#include <max30102.h>
#include "i2c.h"
#include "scd40.h"

static bool isFree = true;

bool I2cCheckFree(void)
{
	return isFree;
}

void I2cLock(void)
{
	isFree = false;
}

void I2cUnlock(void)
{
	isFree = true;
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	Scd40HandleInterrupt();
	Max30102HandleI2cInterrupt();
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	Scd40HandleInterrupt();
	Max30102HandleI2cInterrupt();
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	Scd40HandleInterrupt();
}
