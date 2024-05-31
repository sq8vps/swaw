
#ifndef INC_I2C_H_
#define INC_I2C_H_

#include <stdbool.h>
#include "stm32f1xx.h"

/**
 * @file i2c.h
 * @brief A very simple thread unsafe I2C arbiter
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

/**
 * @brief HAL I2C memory RX complete callback
 */
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c);

/**
 * @brief HAL I2C memory TX complete callback
 */
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c);

/**
 * @brief HAL I2C master TX complete callback
 */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c);

#endif /* INC_I2C_H_ */
