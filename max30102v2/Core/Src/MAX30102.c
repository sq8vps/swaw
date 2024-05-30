#include "main.h"
#include "stm32l4xx_hal.h"

#include "MAX30102.h"
#include "algorithm.h"
#include "stdint.h"
#define I2C_TIMEOUT	1

I2C_HandleTypeDef *i2c_max30102;

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

typedef enum
{
	MAX30102_STATE_BEGIN,
	MAX30102_STATE_CALIBRATE,
	MAX30102_STATE_CALCULATE_HR,
	MAX30102_STATE_COLLECT_NEXT_PORTION
}MAX30102_STATE;
	MAX30102_STATE StateMachine;


static void Max30102_Write_Reg(uint8_t uch_addr, uint8_t uch_data){

	HAL_I2C_Mem_Write(i2c_max30102, MAX30102_ADDRESS, uch_addr, 1, &uch_data, 1, I2C_TIMEOUT);
}

static void Max30102_Read_Reg(uint8_t uch_addr, uint8_t *puch_data){

	HAL_I2C_Mem_Read(i2c_max30102, MAX30102_ADDRESS, uch_addr, 1, puch_data, 1, I2C_TIMEOUT);
}

static void Max30102_WriteRegisterBit(uint8_t Register, uint8_t Bit, uint8_t Value){

	uint8_t tmp;
    Max30102_Read_Reg(Register, &tmp);
		
	tmp &= ~(1<<Bit);
	tmp |= (Value&0x01)<<Bit;
	Max30102_Write_Reg(Register, tmp);
}

static void Max30102_FifoWritePointer(uint8_t Address){

	Max30102_Write_Reg(FIFO_WRITE_POINTER,(Address & 0x1F));  //FIFO_WR_PTR[4:0]
}

static void Max30102_FifoOverflowCounter(uint8_t Address){

	Max30102_Write_Reg(OVERFLOW_COUNTER,(Address & 0x1F));  //OVF_COUNTER[4:0]
}

static void Max30102_FifoReadPointer(uint8_t Address){

	Max30102_Write_Reg(FIFO_READ_POINTER,(Address & 0x1F));  //FIFO_RD_PTR[4:0]
}

static void Max30102_FifoSampleAveraging(uint8_t Value) {

	uint8_t tmp;
	Max30102_Read_Reg(FIFO_CONFIGURATION, &tmp);
		
	tmp &= ~(0x07);
	tmp |= (Value&0x07)<<5;
	Max30102_Write_Reg(FIFO_CONFIGURATION, tmp);
}

static void Max30102_FifoRolloverEnable(uint8_t Enable){

	Max30102_WriteRegisterBit(FIFO_CONFIGURATION, FIFO_CONF_FIFO_ROLLOVER_EN_BIT, (Enable & 0x01));
}

static void  Max30102_FifoAlmostFullValue(uint8_t Value){

	if(Value < 17) Value = 17;
	if(Value > 32) Value = 32;
	Value = 32 - Value;
	uint8_t tmp;
	Max30102_Read_Reg(FIFO_CONFIGURATION, &tmp);
		
	tmp &= ~(0x0F);
	tmp |= (Value & 0x0F);
	Max30102_Write_Reg(FIFO_CONFIGURATION, tmp);
}

static void Max30102_SetMode(uint8_t Mode){

	uint8_t tmp;
	Max30102_Read_Reg(MODE_CONFIGURATION, &tmp);
		
	tmp &= ~(0x07);
	tmp |= (Mode & 0x07);
	Max30102_Write_Reg(MODE_CONFIGURATION, tmp);
}

static void Max30102_SpO2AdcRange(uint8_t Value){

	uint8_t tmp;
	Max30102_Read_Reg(SPO2_CONFIGURATION, &tmp);

	tmp &= ~(0x03);
	tmp |= ((Value & 0x03) << 5);
    Max30102_Write_Reg(SPO2_CONFIGURATION, tmp);
}

static void Max30102_SpO2SampleRate(uint8_t Value){

	uint8_t tmp;
	Max30102_Read_Reg(SPO2_CONFIGURATION, &tmp);

	tmp &= ~(0x07);
	tmp |= ((Value & 0x07) << 2);
	Max30102_Write_Reg(SPO2_CONFIGURATION, tmp);
}

static void Max30102_SpO2LedPulseWidth(uint8_t Value){

	uint8_t tmp;
	Max30102_Read_Reg(SPO2_CONFIGURATION, &tmp);
	
	tmp &= ~(0x03);
	tmp |= (Value & 0x03);
	Max30102_Write_Reg(SPO2_CONFIGURATION, tmp);
		
}

static void Max30102_SetIntAlmostFullEnabled(uint8_t Enable){

	return Max30102_WriteRegisterBit(INTERRUPT_ENABLE_1, INT_A_FULL_BIT, Enable);
}

static void Max30102_SetIntFifoDataReadyEnabled(uint8_t Enable){

	return Max30102_WriteRegisterBit(INTERRUPT_ENABLE_1, INT_PPG_RDY_BIT, Enable);
}


/* LEDs Pulse Amplitute Configuration	LED Current = Value * 0.2 mA */	
static void Max30102_Led1PulseAmplitude(uint8_t Value)
{
	Max30102_Write_Reg(LED_1_PA, Value);

}

static void Max30102_Led2PulseAmplitude(uint8_t Value)
{
	Max30102_Write_Reg(LED_2_PA, Value);
}


/* Usage Functions */

int32_t Max30102_GetHeartRate(void)
{
	return HeartRate;
}


int32_t Max30102_GetSpO2Value(void)
{
	return Sp02Value;
}


MAX30102_STATUS Max30102_ReadFifo(volatile uint32_t *pun_red_led, volatile uint32_t *pun_ir_led)
{
	uint32_t un_temp;
	*pun_red_led=0;
	*pun_ir_led=0;
	uint8_t ach_i2c_data[6];

	if(HAL_I2C_Mem_Read(i2c_max30102, MAX30102_ADDRESS, FIFO_DATA_REGISTER, 1, ach_i2c_data, 6, I2C_TIMEOUT) != HAL_OK)
	{
		return MAX30102_ERROR;
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

	return MAX30102_OK;
}


static void  Max30102_ReadInterruptStatus(uint8_t *Status){
	
	uint8_t tmp;
	*Status = 0;
	Max30102_Read_Reg(INTERRUPT_STATUS_1, &tmp);
	*Status |= tmp & 0xE1; // 3 highest bits
}


void Max30102_InterruptCallback(void)
{
	uint8_t Status;
	Max30102_ReadInterruptStatus(&Status);

	// Almost Full FIFO Interrupt handle
	if(Status & (1<<INT_A_FULL_BIT))
	{
		for(uint8_t i = 0; i < MAX30102_FIFO_ALMOST_FULL_SAMPLES; i++)
		{
			while(MAX30102_OK != Max30102_ReadFifo((RedBuffer+BufferHead), (IrBuffer+BufferHead)));
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
	if(Status & (1<<INT_PPG_RDY_BIT))
	{
		while(MAX30102_OK != Max30102_ReadFifo((RedBuffer+BufferHead), (IrBuffer+BufferHead)));
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

	//  Ambient Light Cancellation Overflow Interrupt handle
	if(Status & (1<<INT_ALC_OVF_BIT))
	{

	}

	// Power Ready Interrupt handle
	if(Status & (1<<INT_PWR_RDY_BIT))
	{
	}

}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	Max30102_InterruptCallback();
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

				Max30102_Led1PulseAmplitude(MAX30102_RED_LED_CURRENT_HIGH);
				Max30102_Led2PulseAmplitude(MAX30102_IR_LED_CURRENT_HIGH);
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
					Max30102_Led1PulseAmplitude(MAX30102_RED_LED_CURRENT_LOW);
					Max30102_Led2PulseAmplitude(MAX30102_IR_LED_CURRENT_LOW);
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
				Max30102_Led1PulseAmplitude(MAX30102_RED_LED_CURRENT_LOW);
				Max30102_Led2PulseAmplitude(MAX30102_IR_LED_CURRENT_LOW);
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
				Max30102_Led1PulseAmplitude(MAX30102_RED_LED_CURRENT_LOW);
				Max30102_Led2PulseAmplitude(MAX30102_IR_LED_CURRENT_LOW);

				StateMachine = MAX30102_STATE_BEGIN;
			}
			break;
	}
}













void Max30102_Init(I2C_HandleTypeDef *i2c){
	uint8_t uch_dummy;
	i2c_max30102 = i2c;
 	Max30102_Write_Reg(MODE_CONFIGURATION,0x40);	/// Reset Register's 		
	Max30102_Read_Reg(0,&uch_dummy);
	Max30102_FifoWritePointer(0x00);				

	Max30102_FifoOverflowCounter(0x00);				
	
	Max30102_FifoReadPointer(0x00);					
	
	Max30102_FifoSampleAveraging(FIFO_SMP_AVE_1);	
		
    Max30102_FifoRolloverEnable(0);

	Max30102_FifoAlmostFullValue(MAX30102_FIFO_ALMOST_FULL_SAMPLES);
	
	Max30102_SetMode(MODE_SPO2_MODE);
	
	Max30102_SpO2AdcRange(SPO2_ADC_RGE_4096);

	Max30102_SpO2SampleRate(SPO2_SAMPLE_RATE);

	Max30102_SpO2LedPulseWidth(SPO2_PULSE_WIDTH_411);

	Max30102_Led1PulseAmplitude(MAX30102_RED_LED_CURRENT_LOW);

	Max30102_Led2PulseAmplitude(MAX30102_IR_LED_CURRENT_LOW);
	
	Max30102_SetIntAlmostFullEnabled(1);
	
	Max30102_SetIntFifoDataReadyEnabled(1);

	StateMachine = MAX30102_STATE_BEGIN;
}
