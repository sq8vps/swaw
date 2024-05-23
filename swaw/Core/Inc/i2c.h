
#ifndef INC_I2C_H_
#define INC_I2C_H_

#include <stdbool.h>

/**
 * @file i2c.h
 * @brief A very simple thread unsafe I2C arbitrator
 */

/**
 * @brief Check if I2C is free to use
 * @return True if free, false if not
 */
bool I2cCheckFree(void);

/**
 * @brief Lock I2C
 */
void I2cLock(void);

/**
 * @brief Unlock I2C
 */
void I2cUnlock(void);

#endif /* INC_I2C_H_ */
