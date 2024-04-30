#include "spi.h"
#include "stm32f1xx.h"

void SpiInit(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

	GPIOB->CRL |= GPIO_CRL_MODE3; //hi speed output - SCK
	GPIOB->CRL &= ~GPIO_CRL_CNF3_0;
	GPIOB->CRL |= GPIO_CRL_CNF3_1;

	GPIOB->CRL &= ~GPIO_CRL_MODE4; //input - MISO
	GPIOB->CRL &= ~GPIO_CRL_CNF4_1;
	GPIOB->CRL |= GPIO_CRL_CNF4_0;

	GPIOB->CRL |= GPIO_CRL_MODE5; //hi speed output - MOSI
	GPIOB->CRL |= GPIO_CRL_CNF5_0;
	GPIOB->CRL |= GPIO_CRL_CNF5_1;

	GPIOB->CRL |= GPIO_CRL_MODE6_0; //medium speed output - CS
	GPIOB->CRL &= ~GPIO_CRL_CNF6;

	AFIO->MAPR |= AFIO_MAPR_SPI1_REMAP; //remap SPI to PB3,4,5


    SPI1->CR1 &= ~SPI_CR1_BR_2; //baudrate control: 0b011 = /16, which is 4.5 MHz
    SPI1->CR1 |= SPI_CR1_BR_1;
    SPI1->CR1 |= SPI_CR1_BR_0;

    SPI1->CR1 &= ~SPI_CR1_DFF; //8-bit frame
    SPI1->CR1 |= SPI_CR1_MSTR; //master mode
    SPI1->CR2 &= ~SPI_CR2_SSOE; //disable slave management
    SPI1->CR1 |= SPI_CR1_CPHA; //mode 3
    SPI1->CR1 &= ~SPI_CR1_CPOL;

    SPI1->CR1 |= SPI_CR1_SPE;

    //in progress
}
