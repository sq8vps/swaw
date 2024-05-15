#include "ads.h"
#include "stm32f1xx.h"
#include <string.h>
#include "proto.h"

#define ADS_PROTO_ID "EEG "

static struct AdsData AdsData;
static int handle = -1;

static void rxCallback(void *buffer, size_t size)
{

}

void AdsInit(void)
{
	handle = ProtoRegister(ADS_PROTO_ID, rxCallback);
	GPIOB->CRL |= GPIO_CRL_MODE7_0; //medium speed output - START
	GPIOB->CRL &= ~GPIO_CRL_CNF7;

	GPIOB->CRH &= ~GPIO_CRH_MODE8; //input - DATA READY
	GPIOB->CRH &= ~GPIO_CRH_CNF8_0; //pull up
	GPIOB->CRH |= GPIO_CRH_CNF8_1;
	GPIOB->BSRR = GPIO_BSRR_BS8;

	GPIOB->CRH |= GPIO_CRH_MODE9_0; //medium speed output - RESET
	GPIOB->CRH &= ~GPIO_CRH_CNF9;
}

void AdsSendData(void)
{
	ProtoSend(handle, &AdsData, sizeof(AdsData));
}
