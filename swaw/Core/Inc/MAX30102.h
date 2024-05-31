
/*
 * Original author's notice:
* Website: https://msalamon.pl/palec-mi-pulsuje-pulsometr-max30102-pod-kontrola-stm32/
*	GitHub:  https://github.com/lamik/MAX30102_STM32_HAL
*
**************************************************************************************
*
*	Modified heavily by the WZW Team for the Sleep Monitor with non-blocking and shared I2C.
*/

/**
 * @file max30102.h
 * @brief MAX30102 library header file
 */

#ifndef MAX30102_H_
#define MAX30102_H_

#include "stm32f1xx.h"
#include <stdint.h>

/**
 * @brief Protocol data ID
 */
#define MAX30102_PROTO_ID "SPO2"

/**
 * @brief Data send interval in ms
 */
#define MAX30102_INTERVAL 1000

/**
 * @brief Warm-up time in seconds
 */
#define MAX30102_WARMUP_TIME 5

/**
 * @brief Sample rate
 */
#define MAX30102_SAMPLES_PER_SECOND 50

/**
 * @brief Structure used to send data to the PC
 */
struct Max30102Results
{
	uint8_t fingerDetected;
	int32_t spo2;
	uint8_t spo2Valid;
	int32_t hr;
	uint8_t hrValid;
} __attribute__ ((packed));

/**
 * @brief Buffer length calculation macro
 */
#define MAX30102_BUFFER_LENGTH	((MAX30102_WARMUP_TIME + 1) * MAX30102_SAMPLES_PER_SECOND)

/**
 * @brief Handle I2C interrupts
 */
void Max30102HandleI2cInterrupt(void);

/**
 * @brief Handle EXTI (INT pin) interrupts
 */
void Max30102HandleExtiCallback(void);

/**
 * @brief Initialize MAX30102 module
 */
void Max30102Init(I2C_HandleTypeDef *i2c);

/**
 * @brief Process MAX30102 events
 * @attention Call this function in the main loop
 */
void Max30102Process(void);

#endif /* MAX30102_H_ */
