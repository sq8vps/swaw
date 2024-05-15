
#ifndef INC_ADS_H_
#define INC_ADS_H_

#include <stdint.h>

#define ADS_CHANNEL_COUNT 4
#define ADS_SAMPLE_COUNT 32

struct AdsData
{
	struct
	{
		struct  __attribute__((packed))
		{
			uint8_t byte[3];
		}
		channel[ADS_CHANNEL_COUNT];
	}
	buffer[ADS_SAMPLE_COUNT];
} __attribute__ ((packed));


#endif /* INC_ADS_H_ */
