/*
* ------------------------------------------------------------------------- */
/*******************************************************************************
* Copyright (C) 2016 Maxim Integrated Products, Inc., All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
* OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*
* Except as contained in this notice, the name of Maxim Integrated
* Products, Inc. shall not be used except as stated in the Maxim Integrated
* Products, Inc. Branding Policy.
*
* The mere transfer of this software does not imply any licenses
* of trade secrets, proprietary technology, copyrights, patents,
* trademarks, maskwork rights, or any other form of intellectual
* property whatsoever. Maxim Integrated Products, Inc. retains all
* ownership rights.
*******************************************************************************
*  Modified original MAXIM source code on: 13.01.2019
*		Author: Mateusz Salamon
*		www.msalamon.pl
*		mateusz@msalamon.pl
*	Code is modified to work with STM32 HAL libraries.
*
*	Website: https://msalamon.pl/palec-mi-pulsuje-pulsometr-max30102-pod-kontrola-stm32/
*	GitHub:  https://github.com/lamik/MAX30102_STM32_HAL

**************************************************************************************
*
*	Modified heavily by the WZW Team for the Sleep Monitor with non-blocking and shared I2C.
*
*/

#include "maxim.h"
#include "max30102.h"
#include "max30102def.h"
#include <stdbool.h>
#include "i2c.h"
#include "proto.h"

#define MAX30102_ENABLE_AVERAGING

#define MAX30102_ADDRESS (0x57 << 1)
#define MAX30102_FIFO_ALMOST_FULL_SAMPLES 17 //number of samples to be accumulated for "FIFO almost full" interrupt

#ifdef MAX30102_ENABLE_AVERAGING
#define MAX30102_AVERAGING ((MAX30102_INTERVAL * MAX30102_SAMPLES_PER_SECOND) / 1000)
#endif

I2C_HandleTypeDef *Max30102I2c = NULL; //I2C peripheral handle

static int handle = -1; //protocol module handle

static uint32_t Max30102Timer = 0; //timer for sending data

enum Max30102MachineState
{
	MAX30102_STATE_BEGIN, //waiting for finger
	MAX30102_STATE_CALIBRATE, //warmup phase, accumulating samples to fill the buffer
	MAX30102_STATE_CALCULATE_HR, //calculating
	MAX30102_STATE_COLLECT_NEXT_PORTION //gathering the next sample portion
};

enum Max30102I2cState
{
	MAX30102_I2CSTATE_IDLE, //idle
	MAX30102_I2CSTATE_SWITCH_LED_MODE, //waiting for new LED mode to be set
	MAX30102_I2CSTATE_INTERRUPT_RECEIVED, //external interrupt received - actually a flag
	MAX30102_I2CSTATE_READ_INTERRUPT_STATUS, //waiting for interrupt status register read
	MAX30102_I2CSTATE_READ_FIFO, //waiting for FIFO read
};

struct
{
	enum Max30102MachineState state; //current state machine state
	enum Max30102I2cState i2cState; //current I2C state
	uint8_t buffer[MAX30102_FIFO_ALMOST_FULL_SAMPLES * 6]; //general I2C buffer
	uint32_t irBuffer[MAX30102_BUFFER_LENGTH]; //IR LED samples - circular buffer
	uint32_t redBuffer[MAX30102_BUFFER_LENGTH]; //red LED samples - circular buffer
	uint32_t bufferHead, bufferTail; //circular buffer pointer
	uint32_t sampleCount; //number of new samples in circular buffer
}
static volatile Max30102State = {.state = MAX30102_STATE_BEGIN, .i2cState = MAX30102_I2CSTATE_IDLE, .bufferHead = 0, .bufferTail = 0, .sampleCount = 0};

static struct Max30102Results Max30102Results = {.hrValid = 0, .spo2Valid = 0, .fingerDetected = 0};

static void Max30102WriteRegister(enum Max30102Register reg, uint8_t val)
{
	HAL_I2C_Mem_Write(Max30102I2c, MAX30102_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, HAL_MAX_DELAY);
}

static uint8_t Max30102ReadRegister(enum Max30102Register reg)
{
	uint8_t val = 0;
	HAL_I2C_Mem_Read(Max30102I2c, MAX30102_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, HAL_MAX_DELAY);
	return val;
}

static void Max30102WriteRegisterMasked(enum Max30102Register reg, uint8_t mask, bool state){

	uint8_t t = Max30102ReadRegister(reg);
	t &= ~mask;
	t |= mask;
	Max30102WriteRegister(reg, t);
}

static void Max30102SetFifoSampleAveraging(enum Max30102SampleAveraging val)
{
	val &= MAX30102_FIFO_CONFIGURATION_SMP_AVE_MASK;
	uint8_t t = Max30102ReadRegister(FIFO_CONFIGURATION);
	t &= ~(MAX30102_FIFO_CONFIGURATION_SMP_AVE_MASK << MAX30102_FIFO_CONFIGURATION_SMP_AVE_SHIFT);
	t |= (val << MAX30102_FIFO_CONFIGURATION_SMP_AVE_SHIFT);
	Max30102WriteRegister(FIFO_CONFIGURATION, t);
}

static void Max30102SetFifoAlmostFullValue(uint8_t val){

	if(val < 17)
		val = 17;
	else if(val > 32)
		val = 32;
	val = 32 - val;

	uint8_t t = Max30102ReadRegister(FIFO_CONFIGURATION);
	t &= ~(MAX30102_FIFO_CONFIGURATION_FIFO_A_FULL_MASK << MAX30102_FIFO_CONFIGURATION_FIFO_A_FULL_SHIFT);
	t |= (val << MAX30102_FIFO_CONFIGURATION_FIFO_A_FULL_SHIFT);
	Max30102WriteRegister(FIFO_CONFIGURATION, t);
}

static void Max30102SetMode(enum Max30102Mode mode)
{
	mode &= MAX30102_MODE_CONFIGURATION_MODE_MASK;
	uint8_t t = Max30102ReadRegister(MODE_CONFIGURATION);
	t &= ~(MAX30102_MODE_CONFIGURATION_MODE_MASK << MAX30102_MODE_CONFIGURATION_MODE_SHIFT);
	t |= (mode << MAX30102_MODE_CONFIGURATION_MODE_SHIFT);
	Max30102WriteRegister(MODE_CONFIGURATION, t);
}

static void Max30102SetSpO2AdcRange(enum Max30102Spo2Range val)
{
	val &= MAX30102_SPO2_CONFIGURATION_ADC_RGE_MASK;
	uint8_t t = Max30102ReadRegister(SPO2_CONFIGURATION);

	t &= ~(MAX30102_SPO2_CONFIGURATION_ADC_RGE_MASK << MAX30102_SPO2_CONFIGURATION_ADC_RGE_SHIFT);
	t |= (val << MAX30102_SPO2_CONFIGURATION_ADC_RGE_SHIFT);
    Max30102WriteRegister(SPO2_CONFIGURATION, t);
}

static void Max30102SetSpO2SampleRate(enum Max30102Spo2SampleRate val)
{
	val &= MAX30102_SPO2_CONFIGURATION_SR_MASK;
	uint8_t t = Max30102ReadRegister(SPO2_CONFIGURATION);

	t &= ~(MAX30102_SPO2_CONFIGURATION_SR_MASK << MAX30102_SPO2_CONFIGURATION_SR_SHIFT);
	t |= (val << MAX30102_SPO2_CONFIGURATION_SR_SHIFT);
    Max30102WriteRegister(SPO2_CONFIGURATION, t);
}

static void Max30102SetSpO2LedPulseWidth(enum Max30102Spo2LedPulseWidth val)
{
	val &= MAX30102_SPO2_CONFIGURATION_LED_PW_MASK;
	uint8_t t = Max30102ReadRegister(SPO2_CONFIGURATION);

	t &= ~(MAX30102_SPO2_CONFIGURATION_LED_PW_MASK << MAX30102_SPO2_CONFIGURATION_LED_PW_SHIFT);
	t |= (val << MAX30102_SPO2_CONFIGURATION_LED_PW_SHIFT);
    Max30102WriteRegister(SPO2_CONFIGURATION, t);
}

static void Max30102SetLedMeasurementMode(bool state)
{
	static volatile uint8_t data[2];
	if(state)
	{
		data[0] = MAX30102_RED_LED_CURRENT_HIGH;
		data[1] = MAX30102_IR_LED_CURRENT_HIGH;
	}
	else
	{
		data[0] = MAX30102_RED_LED_CURRENT_LOW;
		data[1] = MAX30102_IR_LED_CURRENT_LOW;
	}
	while(!I2cCheckFree())
		;
	I2cLock();
	Max30102State.i2cState = MAX30102_I2CSTATE_SWITCH_LED_MODE;
	HAL_I2C_Mem_Write_IT(Max30102I2c, MAX30102_ADDRESS, LED1_PULSE_AMPLITUDE, I2C_MEMADD_SIZE_8BIT, (uint8_t*)data, 2);
}


int32_t Max30102_GetHeartRate(void)
{
	return Max30102Results.hr;
}


int32_t Max30102_GetSpO2Value(void)
{
	return Max30102Results.spo2;
}


static void Max30102ParseFifo(void)
{
	for(uint8_t i = 0; i < MAX30102_FIFO_ALMOST_FULL_SAMPLES; i++)
	{
		Max30102State.redBuffer[Max30102State.bufferHead] = 0;
		Max30102State.irBuffer[Max30102State.bufferHead] = 0;
		for(uint8_t k = 0; k < 3; k++)
		{
			Max30102State.redBuffer[Max30102State.bufferHead] <<= 8;
			Max30102State.irBuffer[Max30102State.bufferHead] <<= 8;
			Max30102State.redBuffer[Max30102State.bufferHead] |= Max30102State.buffer[i * 6 + k];
			Max30102State.irBuffer[Max30102State.bufferHead] |= Max30102State.buffer[i * 6 + 3 + k];
		}
		Max30102State.redBuffer[Max30102State.bufferHead] &= 0x3FFFF; //limit to 18 bits
		Max30102State.irBuffer[Max30102State.bufferHead] &= 0x3FFFF; //limit to 18 bits

		if(Max30102Results.fingerDetected)
		{
			if(Max30102State.irBuffer[Max30102State.bufferHead] < MAX30102_IR_VALUE_FINGER_OUT_SENSOR)
				Max30102Results.fingerDetected = 0;
		}
		else
		{
			if(Max30102State.irBuffer[Max30102State.bufferHead] > MAX30102_IR_VALUE_FINGER_ON_SENSOR)
				Max30102Results.fingerDetected = 1;
		}
		Max30102State.bufferHead = (Max30102State.bufferHead + 1) % MAX30102_BUFFER_LENGTH;
		Max30102State.sampleCount++;
	}
}

void Max30102HandleI2cInterrupt(void)
{
	switch(Max30102State.i2cState)
	{
		//status register read previously requested
		case MAX30102_I2CSTATE_READ_INTERRUPT_STATUS:
			//check for interrupts
			if(Max30102State.buffer[0] & MAX30102_INTERRUPT_STATUS_1_A_FULL)
			{
				//FIFO almost full, read FIFO
				Max30102State.i2cState = MAX30102_I2CSTATE_READ_FIFO;
				HAL_I2C_Mem_Read_IT(Max30102I2c, MAX30102_ADDRESS, FIFO_DATA_REGISTER, I2C_MEMADD_SIZE_8BIT, (uint8_t*)Max30102State.buffer, MAX30102_FIFO_ALMOST_FULL_SAMPLES * 6);
			}
			break;
		//FIFO read finished
		case MAX30102_I2CSTATE_READ_FIFO:
			I2cUnlock();
			Max30102State.i2cState = MAX30102_I2CSTATE_IDLE;
			Max30102ParseFifo();
			break;
		//LED mode switch finished
		case MAX30102_I2CSTATE_SWITCH_LED_MODE:
			I2cUnlock();
			Max30102State.i2cState = MAX30102_I2CSTATE_IDLE;
			break;
		//states that are not handled in this interrupt
		case MAX30102_I2CSTATE_INTERRUPT_RECEIVED:
		case MAX30102_I2CSTATE_IDLE:
			break;
	}
}

void Max30102HandleExtiCallback(void)
{
	//wait for I2C task completion, which should be fast
	while(MAX30102_I2CSTATE_IDLE != Max30102State.i2cState)
		;
	//let the block state machine handle this interrupt
	Max30102State.i2cState = MAX30102_I2CSTATE_INTERRUPT_RECEIVED;
}

void Max30102Process(void)
{
	if(Max30102State.i2cState == MAX30102_I2CSTATE_INTERRUPT_RECEIVED)
	{
		while(!I2cCheckFree())
			;
		I2cLock();
		Max30102State.i2cState = MAX30102_I2CSTATE_READ_INTERRUPT_STATUS;
		HAL_I2C_Mem_Read_IT(Max30102I2c, MAX30102_ADDRESS, INTERRUPT_STATUS_1, I2C_MEMADD_SIZE_8BIT, (uint8_t*)Max30102State.buffer, 1);
	}

	switch(Max30102State.state)
	{
		case MAX30102_STATE_BEGIN:
			Max30102Results.hr = 0;
			Max30102Results.hrValid = 0;
			Max30102Results.spo2 = 0;
			Max30102Results.spo2Valid = 0;
			if(Max30102Results.fingerDetected)
			{
				Max30102State.sampleCount = 0;
				Max30102State.bufferTail = Max30102State.bufferHead;

				Max30102SetLedMeasurementMode(true);
				Max30102State.state = MAX30102_STATE_CALIBRATE;
			}
			break;
		case MAX30102_STATE_CALIBRATE:
				if(Max30102Results.fingerDetected)
				{
					if(Max30102State.sampleCount > (MAX30102_BUFFER_LENGTH - MAX30102_SAMPLES_PER_SECOND))
					{
						Max30102State.state = MAX30102_STATE_CALCULATE_HR;
					}
				}
				else
				{
					Max30102SetLedMeasurementMode(false);
					Max30102State.state = MAX30102_STATE_BEGIN;
				}
			break;

		case MAX30102_STATE_CALCULATE_HR:
			if(Max30102Results.fingerDetected)
			{
				int32_t hr, spo2;
				uint8_t hrValid, spo2Valid;
				MaximCalculate(Max30102State.irBuffer, Max30102State.redBuffer, MAX30102_BUFFER_LENGTH - MAX30102_SAMPLES_PER_SECOND,
						Max30102State.bufferTail, &spo2, &spo2Valid, &hr, &hrValid);
				Max30102Results.hr = hr;
				Max30102Results.spo2 = spo2;
				Max30102Results.hrValid = hrValid;
				Max30102Results.spo2Valid = spo2Valid;
				Max30102State.bufferTail = (Max30102State.bufferTail + MAX30102_SAMPLES_PER_SECOND) % MAX30102_BUFFER_LENGTH;
				Max30102State.sampleCount = 0;
				Max30102State.state = MAX30102_STATE_COLLECT_NEXT_PORTION;
			}
			else
			{
				Max30102SetLedMeasurementMode(false);
				Max30102State.state = MAX30102_STATE_BEGIN;
			}
			break;

		case MAX30102_STATE_COLLECT_NEXT_PORTION:
			if(Max30102Results.fingerDetected)
			{
				if(Max30102State.sampleCount > MAX30102_SAMPLES_PER_SECOND)
				{
					Max30102State.state = MAX30102_STATE_CALCULATE_HR;
				}
			}
			else
			{
				Max30102SetLedMeasurementMode(false);
				Max30102State.state = MAX30102_STATE_BEGIN;
			}
			break;
	}

	if((HAL_GetTick() > Max30102Timer))
	{
		Max30102Timer = HAL_GetTick() + MAX30102_INTERVAL;
		ProtoSend(handle, &Max30102Results, sizeof(Max30102Results));
	}
}


void Max30102Init(I2C_HandleTypeDef *hi2c)
{
	Max30102I2c = hi2c;

	handle = ProtoRegister(MAX30102_PROTO_ID, NULL);

	while(!I2cCheckFree())
		;

	I2cLock();

 	Max30102WriteRegister(MODE_CONFIGURATION, MAX30102_MODE_CONFIGURATION_RESET);

	Max30102ReadRegister(INTERRUPT_STATUS_1);
	
	Max30102WriteRegister(FIFO_WRITE_POINTER, 0);
	Max30102WriteRegister(FIFO_OVERFLOW_COUNTER, 0);
	Max30102WriteRegister(FIFO_READ_POINTER, 0);
	
	Max30102SetFifoSampleAveraging(FIFO_SMP_AVE_1);
		
	Max30102WriteRegisterMasked(FIFO_CONFIGURATION, MAX30102_FIFO_CONFIGURATION_ROLLOVER_EN, false);

	Max30102SetFifoAlmostFullValue(MAX30102_FIFO_ALMOST_FULL_SAMPLES);
	
	Max30102SetMode(MODE_SPO2);
	
	Max30102SetSpO2AdcRange(SPO2_RANGE_4096);

	Max30102SetSpO2SampleRate(SPO2_SAMPLE_RATE);

	Max30102SetSpO2LedPulseWidth(SPO2_PULSE_WIDTH_411);

	Max30102WriteRegister(LED1_PULSE_AMPLITUDE, MAX30102_RED_LED_CURRENT_LOW);
	Max30102WriteRegister(LED2_PULSE_AMPLITUDE, MAX30102_IR_LED_CURRENT_LOW);
	
	Max30102WriteRegisterMasked(INTERRUPT_ENABLE_1, MAX30102_INTERRUPT_ENABLE_1_A_FULL_EN, true);
	Max30102WriteRegisterMasked(INTERRUPT_ENABLE_1, MAX30102_INTERRUPT_ENABLE_1_PPG_RDY_EN, false);

	I2cUnlock();
}
