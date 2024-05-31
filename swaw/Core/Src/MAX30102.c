#include "MAX30102.h"
#include "max30102def.h"
#include "algorithm.h"
#include <stdbool.h>

#define MAX30102_ADDRESS (0x57 << 1)

I2C_HandleTypeDef *Max30102I2c = NULL;

volatile uint32_t IrBuffer[MAX30102_BUFFER_LENGTH]; /// IR LED sensor data
volatile uint32_t RedBuffer[MAX30102_BUFFER_LENGTH]; /// Red LED sensor data
volatile uint32_t BufferHead;
volatile uint32_t BufferTail;
volatile uint32_t CollectedSamples;
volatile uint8_t IsFingerOnScreen;
int32_t Sp02Value;
int8_t Sp02IsValid;
int32_t HeartRate;
int8_t IsHrValid;

enum Max30102MachineState
{
	MAX30102_STATE_BEGIN,
	MAX30102_STATE_CALIBRATE,
	MAX30102_STATE_CALCULATE_HR,
	MAX30102_STATE_COLLECT_NEXT_PORTION
};

struct
{
	enum Max30102MachineState state;
	bool waitingForI2cInterrupt;

	bool fingerDetected;
}
static Max30102State;

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

static void Max30102SetLedsEnabled(bool state)
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
	HAL_I2C_Mem_Write_IT(Max30102I2c, MAX30102_ADDRESS, LED1_PULSE_AMPLITUDE, I2C_MEMADD_SIZE_8BIT, (uint8_t*)data, 2);
}


int32_t Max30102_GetHeartRate(void)
{
	return HeartRate;
}


int32_t Max30102_GetSpO2Value(void)
{
	return Sp02Value;
}


void Max30102_ReadFifo(volatile uint32_t *pun_red_led, volatile uint32_t *pun_ir_led)
{
	uint32_t un_temp;
	*pun_red_led=0;
	*pun_ir_led=0;
	uint8_t ach_i2c_data[6];

	if(HAL_I2C_Mem_Read(Max30102I2c, MAX30102_ADDRESS, FIFO_DATA_REGISTER, 1, ach_i2c_data, 6, HAL_MAX_DELAY) != HAL_OK)
	{
		return;
	}
	un_temp=(unsigned char) ach_i2c_data[0];
	un_temp<<=16;
	*pun_red_led+=un_temp;
	un_temp=(unsigned char) ach_i2c_data[1];
	un_temp<<=8;
	*pun_red_led+=un_temp;
	un_temp=(unsigned char) ach_i2c_data[2];
	*pun_red_led+=un_temp;

	un_temp=(unsigned char) ach_i2c_data[3];
	un_temp<<=16;
	*pun_ir_led+=un_temp;
	un_temp=(unsigned char) ach_i2c_data[4];
	un_temp<<=8;
	*pun_ir_led+=un_temp;
	un_temp=(unsigned char) ach_i2c_data[5];
	*pun_ir_led+=un_temp;
	*pun_red_led&=0x03FFFF;  //Mask MSB [23:18]
	*pun_ir_led&=0x03FFFF;  //Mask MSB [23:18]
}

void Max30102HandleI2cInterrupt(void)
{

}

void Max30102_InterruptCallback(void)
{
	uint8_t status = Max30102ReadRegister(INTERRUPT_STATUS_1);

	// Almost Full FIFO Interrupt handle
	if(status & MAX30102_INTERRUPT_STATUS_1_A_FULL)
	{
		for(uint8_t i = 0; i < MAX30102_FIFO_ALMOST_FULL_SAMPLES; i++)
		{
			Max30102_ReadFifo((RedBuffer+BufferHead), (IrBuffer+BufferHead));
			if(IsFingerOnScreen)
			{
				if(IrBuffer[BufferHead] < MAX30102_IR_VALUE_FINGER_OUT_SENSOR) IsFingerOnScreen = 0;
			}
			else
			{
				if(IrBuffer[BufferHead] > MAX30102_IR_VALUE_FINGER_ON_SENSOR) IsFingerOnScreen = 1;
			}
			BufferHead = (BufferHead + 1) % MAX30102_BUFFER_LENGTH;
			CollectedSamples++;
		}
	}

	// New FIFO Data Ready Interrupt handle
	if(status & MAX30102_INTERRUPT_STATUS_1_PPG_RDY)
	{
		Max30102_ReadFifo((RedBuffer+BufferHead), (IrBuffer+BufferHead));
		if(IsFingerOnScreen)
		{
			if(IrBuffer[BufferHead] < MAX30102_IR_VALUE_FINGER_OUT_SENSOR) IsFingerOnScreen = 0;
		}
		else
		{
			if(IrBuffer[BufferHead] > MAX30102_IR_VALUE_FINGER_ON_SENSOR) IsFingerOnScreen = 1;
		}
		BufferHead = (BufferHead + 1) % MAX30102_BUFFER_LENGTH;
		CollectedSamples++;
	}
}


void Max30102_Task(void)
{
	switch(StateMachine)
	{
		case MAX30102_STATE_BEGIN:
			HeartRate = 0;
			Sp02Value = 0;
			if(IsFingerOnScreen)
			{
				CollectedSamples = 0;
				BufferTail = BufferHead;

				Max30102SetLedsEnabled(true);
				StateMachine = MAX30102_STATE_CALIBRATE;
			}
			break;
		case MAX30102_STATE_CALIBRATE:
				if(IsFingerOnScreen)
				{
					if(CollectedSamples > (MAX30102_BUFFER_LENGTH-MAX30102_SAMPLES_PER_SECOND))
					{
						StateMachine = MAX30102_STATE_CALCULATE_HR;
					}
				}
				else
				{
					Max30102SetLedsEnabled(false);
					StateMachine = MAX30102_STATE_BEGIN;
				}
			break;

		case MAX30102_STATE_CALCULATE_HR:
			if(IsFingerOnScreen)
			{
				maxim_heart_rate_and_oxygen_saturation(IrBuffer, RedBuffer, MAX30102_BUFFER_LENGTH-MAX30102_SAMPLES_PER_SECOND, BufferTail, &Sp02Value, &Sp02IsValid, &HeartRate, &IsHrValid);
				BufferTail = (BufferTail + MAX30102_SAMPLES_PER_SECOND) % MAX30102_BUFFER_LENGTH;
				CollectedSamples = 0;
				StateMachine = MAX30102_STATE_COLLECT_NEXT_PORTION;
			}
			else
			{
				Max30102SetLedsEnabled(false);
				StateMachine = MAX30102_STATE_BEGIN;
			}
			break;

		case MAX30102_STATE_COLLECT_NEXT_PORTION:
			if(IsFingerOnScreen)
			{
				if(CollectedSamples > MAX30102_SAMPLES_PER_SECOND)
				{
					StateMachine = MAX30102_STATE_CALCULATE_HR;
				}
			}
			else
			{
				Max30102SetLedsEnabled(false);

				StateMachine = MAX30102_STATE_BEGIN;
			}
			break;
	}
}


void Max30102_Init(I2C_HandleTypeDef *hi2c)
{
	Max30102I2c = hi2c;

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

	Max30102SetSpO2SampleRate(SPO2_SAMPLE_RATE_50);

	Max30102SetSpO2LedPulseWidth(SPO2_PULSE_WIDTH_411);

	Max30102WriteRegister(LED1_PULSE_AMPLITUDE, MAX30102_RED_LED_CURRENT_LOW);
	Max30102WriteRegister(LED2_PULSE_AMPLITUDE, MAX30102_IR_LED_CURRENT_LOW);
	
	Max30102WriteRegisterMasked(INTERRUPT_ENABLE_1, MAX30102_INTERRUPT_ENABLE_1_A_FULL_EN, true);
	Max30102WriteRegisterMasked(INTERRUPT_ENABLE_1, MAX30102_INTERRUPT_ENABLE_1_PPG_RDY_EN, true);

	StateMachine = MAX30102_STATE_BEGIN;
}
