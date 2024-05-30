/** \file max30102.h ******************************************************

*	Website: https://msalamon.pl/palec-mi-pulsuje-pulsometr-max30102-pod-kontrola-stm32/
*	GitHub:  https://github.com/lamik/MAX30102_STM32_HAL
*
*/
#ifndef MAX30102_H_
#define MAX30102_H_

#define MAX30102_MEASUREMENT_SECONDS 	  	5	/// 
#define MAX30102_SAMPLES_PER_SECOND		  	50	/// 50, 100, 200, 400, 800, 100, 1600, 3200 sample rating
#define MAX30102_FIFO_ALMOST_FULL_SAMPLES 	17	///

#define MAX30102_BUFFER_LENGTH	((MAX30102_MEASUREMENT_SECONDS + 1) * MAX30102_SAMPLES_PER_SECOND)	/// 

/*--------------------REGISTER MAPS AND DESCRIPTIONS--------------------*/  

/* STATUS */
#define INTERRUPT_STATUS_1          0x00    /// Read
#define INTERRUPT_STATUS_2          0x01    /// Read
#define INTERRUPT_ENABLE_1          0x02    /// Read
#define INTERRUPT_ENABLE_2          0x03    /// Read/Write 

/* FIFO */
#define FIFO_WRITE_POINTER          0x04    /// Points to the location where the MAX30102 writes the next sample (advances for each sample pushed on to the FIFO), can be changed when MODE[2:0]. 
#define OVERFLOW_COUNTER            0x05    /// Read/Write 
#define FIFO_READ_POINTER           0x06    /// Read/Write 
#define FIFO_DATA_REGISTER          0x07    /// Read/Write 

/* CONFIGURATION */
#define FIFO_CONFIGURATION          0x08    /// Read/Write 
#define MODE_CONFIGURATION          0x09    /// Read/Write 
#define SPO2_CONFIGURATION          0x0A    /// Read/Write 

#define LED_1_PA                    0x0C    /// LED 1 Pulse Amplitude Read/Write Read/Writ
#define LED_2_PA                    0x0D    /// LED 2 Pulse Amplitude Read/Write 

#define M_LED_CTRL_REG_1            0x11    /// Multi-LED Mode Control Register 1     Read/Write 
#define M_LED_CTRL_REG_2            0x12    /// Multi-LED Mode Control Register 2     Read/Write 

/* DIE TEMPERATURE */
#define DIE_TEMP_INT                0x1F    /// Die Temperature Integer Read
#define DIE_TEMP_FRAC               0x20    /// Die Temperature Fraction Read
#define DIE_TEMP_CONFIG             0x21    /// Die Temperature Config Read/Write 

/* PART ID */
#define REVISION_ID                 0xFE    /// 
#define PART_ID                     0xFF    /// 

/* SLAVE ID */
#define MAX30102_ADDRESS 			0xAE	/// (0x57<<1)

/*--------------------CALIBRATION--------------------*/
#define MAX30102_IR_LED_CURRENT_LOW				0x01	///
#define MAX30102_RED_LED_CURRENT_LOW			0x00	///
#define MAX30102_IR_LED_CURRENT_HIGH			0x24	///
#define MAX30102_RED_LED_CURRENT_HIGH			0x24	///
#define MAX30102_IR_VALUE_FINGER_ON_SENSOR 		1600	///
#define MAX30102_IR_VALUE_FINGER_OUT_SENSOR 	50000	/// 


#define	INT_A_FULL_BIT					7	///	
#define	INT_PPG_RDY_BIT					6	///
#define	INT_ALC_OVF_BIT					5	///
#define	INT_DIE_TEMP_RDY_BIT			1	///
#define	INT_PWR_RDY_BIT					0	/// Only STATUS register

/*--------------------FIFO CONFIGURATION--------------------*/
#define FIFO_CONF_SMP_AVE_BIT 			7
#define FIFO_CONF_SMP_AVE_LENGHT 		3
#define FIFO_CONF_FIFO_ROLLOVER_EN_BIT 	4
#define FIFO_CONF_FIFO_A_FULL_BIT 		3
#define FIFO_CONF_FIFO_A_FULL_LENGHT 	4

#define FIFO_SMP_AVE_1					0	/// No. Of Samples Averaged Per Fifo Sample = 1 (No Averaging)	
#define FIFO_SMP_AVE_2					1	/// No. Of Samples Averaged Per Fifo Sample = 2 
#define FIFO_SMP_AVE_4					2 	/// No. Of Samples Averaged Per Fifo Sample = 4 
#define FIFO_SMP_AVE_8					3 	/// No. Of Samples Averaged Per Fifo Sample = 8 
#define FIFO_SMP_AVE_16					4 	/// No. Of Samples Averaged Per Fifo Sample = 16 
#define FIFO_SMP_AVE_32					5 	/// No. Of Samples Averaged Per Fifo Sample = 32

/*--------------------MODE CONFIGURATION--------------------*/
#define MODE_HEART_RATE_MODE	2
#define MODE_SPO2_MODE			3
#define MODE_MULTI_LED_MODE		7

/*--------------------SPO2 CONFIGURATION-------------------- */
#define SPO2_CONF_ADC_RGE_BIT		6
#define SPO2_CONF_ADC_RGE_LENGTH	2
#define SPO2_CONF_SR_BIT			4
#define SPO2_CONF_SR_LENGTH			3
#define SPO2_CONF_LED_PW_BIT		1
#define SPO2_CONF_LED_PW_LENGTH		2

#define	SPO2_ADC_RGE_2048	0
#define	SPO2_ADC_RGE_4096	1
#define	SPO2_ADC_RGE_8192	2
#define	SPO2_ADC_RGE_16384	3

#define	SPO2_SAMPLE_RATE_50		0
#define	SPO2_SAMPLE_RATE_100	1
#define	SPO2_SAMPLE_RATE_200	2
#define	SPO2_SAMPLE_RATE_400	3
#define	SPO2_SAMPLE_RATE_800	4
#define	SPO2_SAMPLE_RATE_1000	5
#define	SPO2_SAMPLE_RATE_1600	6
#define	SPO2_SAMPLE_RATE_3200	7	

#if(MAX30102_SAMPLES_PER_SECOND == 50)
#define SPO2_SAMPLE_RATE SPO2_SAMPLE_RATE_50
#elif((MAX30102_SAMPLES_PER_SECOND == 100))
#define SPO2_SAMPLE_RATE SPO2_SAMPLE_RATE_100
#elif((MAX30102_SAMPLES_PER_SECOND == 200))
#define SPO2_SAMPLE_RATE SPO2_SAMPLE_RATE_200
#elif((MAX30102_SAMPLES_PER_SECOND == 400))
#define SPO2_SAMPLE_RATE SPO2_SAMPLE_RATE_400
#elif((MAX30102_SAMPLES_PER_SECOND == 800))
#define SPO2_SAMPLE_RATE SPO2_SAMPLE_RATE_800
#elif((MAX30102_SAMPLES_PER_SECOND == 1000))
#define SPO2_SAMPLE_RATE SPO2_SAMPLE_RATE_1000
#elif((MAX30102_SAMPLES_PER_SECOND == 1600))
#define SPO2_SAMPLE_RATE SPO2_SAMPLE_RATE_1600
#elif((MAX30102_SAMPLES_PER_SECOND == 3200))
#define SPO2_SAMPLE_RATE SPO2_SAMPLE_RATE_3200
#else
#error "Wrong Sample Rate value. Use 50, 100, 200, 400, 800, 1000, 1600 or 3200."
#endif

#define	SPO2_PULSE_WIDTH_69			0
#define	SPO2_PULSE_WIDTH_118		1
#define	SPO2_PULSE_WIDTH_215		2
#define	SPO2_PULSE_WIDTH_411		3


typedef enum{
	MAX30102_ERROR 	= 0,
	MAX30102_OK 	= 1
} MAX30102_STATUS;


/*--------------------FUNCTION DEFINITIONS--------------------*/

void Max30102_Init(I2C_HandleTypeDef *i2c);
void Max30102_Task(void);
int32_t Max30102_GetHeartRate(void);
int32_t Max30102_GetSpO2Value(void);

#endif /* MAX30102_H_ */
