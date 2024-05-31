#ifndef SCD40_H
#define SCD40_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx.h"

/**
 * @brief Data send interval in ms
 */
#define SCD40_INTERVAL 10000

/**
 * @brief Protocol data ID
 */
#define SCD40_PROTO_ID "CO2 "

/**
 * @brief SCD40 data structure as sent to the PC
 */
struct Scd40Data
{
    uint16_t co2;
    int16_t temperature;
    uint16_t humidity;
} __attribute__ ((packed));

/**
 * @brief Initialize SCD40 CO2 sensor
 * @param *hi2c I2C handle
 */
void Scd40Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief Handle I2C interrupt
 * @attention Must be called in I2C interrupt handlers
 */
void Scd40HandleInterrupt(void);

/**
 * @brief Process SCD40 events
 * @attention Run this function is the main loop
 */
void Scd40Process(void);

#endif //SCD40_H
