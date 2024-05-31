/** \file max30102.h ******************************************************

*	Website: https://msalamon.pl/palec-mi-pulsuje-pulsometr-max30102-pod-kontrola-stm32/
*	GitHub:  https://github.com/lamik/MAX30102_STM32_HAL
*
*/
#ifndef MAX30102_H_
#define MAX30102_H_

#include "stm32f1xx.h"
#include <stdint.h>

#define MAX30102_MEASUREMENT_SECONDS 	  	5	/// 
#define MAX30102_FIFO_ALMOST_FULL_SAMPLES 	17	///

#define MAX30102_SAMPLES_PER_SECOND 50

#define MAX30102_BUFFER_LENGTH	((MAX30102_MEASUREMENT_SECONDS + 1) * MAX30102_SAMPLES_PER_SECOND)	///



/*--------------------FUNCTION DEFINITIONS--------------------*/

void Max30102HandleI2cInterrupt(void);
void Max30102_InterruptCallback(void);
void Max30102_Init(I2C_HandleTypeDef *i2c);
void Max30102_Task(void);
int32_t Max30102_GetHeartRate(void);
int32_t Max30102_GetSpO2Value(void);

#endif /* MAX30102_H_ */
