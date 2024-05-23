#include "i2c.h"

static bool isFree = true;

bool I2cCheckFree(void)
{
	return isFree;
}

void I2cLock(void)
{
	isFree = true;
}

void I2cUnlock(void)
{
	isFree = false;
}
