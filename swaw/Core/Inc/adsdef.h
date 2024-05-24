#ifndef INC_ADSDEF_H_
#define INC_ADSDEF_H_

enum AdsCommand
{
	ADS_WAKEUP = 0x02,
	ADS_STANDBY = 0x04,
	ADS_RESET = 0x06,
	ADS_START = 0x08,
	ADS_STOP = 0x0A,
	ADS_RDATAC = 0x10,
	ADS_SDATAC = 0x11,
	ADS_RDATA = 0x12
};

#define ADS_RREG_BITS (0x20)
#define ADS_WREG_BITS (0x40)
#define ADS_RREG_WREG_MASK (0x1F)
#define ADS_RREG_WREG_SIZE_MASK (0x1F)

enum AdsRegister
{
	//read-only id register
	ID = 0x0,

	//global settings
	CONFIG1 = 0x1,
	CONFIG2 = 0x2,
	CONFIG3 = 0x3,
	LOFF = 0x4,

	//channel specific
	CH1SET = 0x5,
	CH2SET = 0x6,
	CH3SET = 0x7,
	CH4SET = 0x8,
	BIAS_SENSP = 0xD,
	BIAS_SENSN = 0xE,
	LOFF_SENSP = 0xF,
	LOFF_SENSN = 0x10,
	LOFF_FLIP = 0x11,

	//read-only
	LOFF_STATP = 0x12,
	LOFF_STATN = 0x13,

	//gpio and other
	GPIO = 0x14,
	MISC1 = 0x15,
	MISC2 = 0x16,
	CONFIG4 = 0x17
};

#define ADS_CHnSET(n) (CH1SET + n)

#define ADS_CONFIG1_RSVD 0b10010000

#define ADS_CONFIG2_RSVD 0b11000000

#define ADS_CONFIG3_RSVD 0b01100000
#define ADS_CONFIG3_BIASREF_INT 0x08
#define ADS_CONFIG3_PD_BIAS 0x04
#define ADS_CONFIG3_BIAS_LOFF_SENS 0x02
#define ADS_CONFIG3_BIAS_STAT 0x01

#define ADS_LOFF_RSVD 0

#define ADS_BIAS_SENSx_BIT(n) (1 << (n))

#define ADS_LOFF_SENSx_BIT(n) (1 << (n))

#define ADS_MISC1_RSVD 0
#define ADS_MISC1_SRB1_BIT 0b00100000

#define ADS_CONFIG4_RSVD 0
#define ADS_CONFIG4_PD_LOFF_COMP 0x1

#endif
