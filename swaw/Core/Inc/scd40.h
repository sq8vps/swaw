#ifndef SCD40_H
#define SCD40_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx.h"

// SCD40 Sensor address
#define SCD40_ADDRESS 0b1100010 << 1
// Used command (MemAddr)
#define START_PERIODIC_MEASURMENT_COMMAND 0x21b1
#define STOP_PERIODIC_MEASURMENT_COMMAND 0x3f86
#define READ_MEASURMENT_COMMAND 0xec05
#define GET_DATA_READY_STATUS_COMMAND 0xe4b8

// Structure represent read value from SCD40 sensor
typedef struct{
    uint16_t co2_level;
    int16_t temperature;
    uint16_t humidity;
}SCD40;

/**
 * @brief Start measurement
 * @param Pointer to I2C_handle_Structure
 * @return Return 0 on success, 1 on failure
*/
uint8_t start_periodic_measurement(I2C_HandleTypeDef *hi2c2);

/**
 * @brief Stop measurement
 * @param Pointer to I2C_handle_Structure
 * @return Return 0 on success, 1 on failure
*/
uint8_t stop_periodic_measurement(I2C_HandleTypeDef *hi2c2);

/**
 * @brief Read GET_DATA_READY_STATUS (Check if the data is ready to read)
 * @param Pointer to I2C_handle_Structure
 * @param Pointer to receive buffer
 * @return Return 0 on success, 1 on failure
*/
uint8_t check_data_ready(I2C_HandleTypeDef *hi2c2, uint8_t* buffer);

/**
 * @brief Read data from sensor
 * @param Pointer to I2C_handle_Structure
 * @param Pointer to receive buffer
 * @return Return 0 on success, 1 on failure
*/
uint8_t read_data(I2C_HandleTypeDef *hi2c2, uint8_t* buffer);

/**
 * @brief Check CRC and save read data to SCD40 sensor structure
 * @param Pointer to SCD40 sensor structure
 * @param Pointer to receive buffer
*/
void check_read_data(SCD40* sensor, uint8_t* buffer);



#endif //SCD40_H
