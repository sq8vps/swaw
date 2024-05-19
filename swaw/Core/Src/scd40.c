
#include "scd40.h"
#define CRC8_POLYNOMIAL 0x31
#define CRC8_INIT 0xFF

// Count CRC from data
uint8_t crc_generate(const uint8_t* data, uint16_t count);

uint8_t start_periodic_measurement(I2C_HandleTypeDef *hi2c2){
    if(HAL_OK == HAL_I2C_Mem_Write_IT(hi2c2, SCD40_ADDRESS, START_PERIODIC_MEASURMENT_COMMAND, I2C_MEMADD_SIZE_16BIT, NULL, 0)){
    	return 0;
    }
    return 1;
}

uint8_t stop_periodic_measurement(I2C_HandleTypeDef *hi2c2){
	if(HAL_OK == HAL_I2C_Mem_Write_IT(hi2c2, SCD40_ADDRESS, STOP_PERIODIC_MEASURMENT_COMMAND, I2C_MEMADD_SIZE_16BIT, NULL, 0)){
		return 0;
    }
    return 1;
}

uint8_t check_data_ready(I2C_HandleTypeDef *hi2c2, uint8_t* buffer){
	if(HAL_OK == HAL_I2C_Mem_Read_IT(hi2c2, SCD40_ADDRESS, GET_DATA_READY_STATUS_COMMAND, I2C_MEMADD_SIZE_16BIT, buffer, 2)){
		return 0;
    }
    return 1;
}

uint8_t read_data(I2C_HandleTypeDef *hi2c2, uint8_t* buffer){
	if(HAL_OK == HAL_I2C_Mem_Read_IT(hi2c2, SCD40_ADDRESS, READ_MEASURMENT_COMMAND, I2C_MEMADD_SIZE_16BIT, buffer, 9)){
		return 0;
    }
    return 1;
}


void check_read_data(SCD40* sensor, uint8_t* buffer){
	// Buffer contains: co2_MSB, co2_LSB, co2_CRC, temp_MSB, temp_LSB, temp_CRC, hum_MSB, hum_LSB, hum_CRC
    if(*(buffer+2) == crc_generate(buffer, 2)){
        sensor->co2_level = *(buffer) << 8 | *(buffer+1);
    }
    if(*(buffer+5) == crc_generate(buffer+3, 2)){
        sensor->temperature = *(buffer+3) << 8 | *(buffer+4);
    }
    if(*(buffer+8) == crc_generate(buffer+6, 2)){
        sensor->humidity = *(buffer+6) << 8 | *(buffer+7);
    }
}

uint8_t crc_generate(const uint8_t* data, uint16_t count){
    uint16_t current_byte;
    uint8_t crc = CRC8_INIT;
    uint8_t crc_bit;
    /* calculates 8-Bit checksum with given polynomial */
    for (current_byte = 0; current_byte < count; ++current_byte) {
        crc ^= (data[current_byte]);
        for (crc_bit = 8; crc_bit > 0; --crc_bit) {
            if (crc & 0x80)
                crc = (crc << 1) ^ CRC8_POLYNOMIAL;
            else
                crc = (crc << 1);
            }
    }
    return crc;
}


