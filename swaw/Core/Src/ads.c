

#include "stm32f1xx.h"

void AdsInit(void)
{
	GPIOB->CRL |= GPIO_CRL_MODE7_0; //medium speed output - START
	GPIOB->CRL &= ~GPIO_CRL_CNF7;

	GPIOB->CRH &= ~GPIO_CRH_MODE8; //input - DATA READY
	GPIOB->CRH &= ~GPIO_CRH_CNF8_0; //pull up
	GPIOB->CRH |= GPIO_CRH_CNF8_1;
	GPIOB->BSRR = GPIO_BSRR_BS8;

	GPIOB->CRH |= GPIO_CRH_MODE9_0; //medium speed output - RESET
	GPIOB->CRH &= ~GPIO_CRH_CNF9;
}
