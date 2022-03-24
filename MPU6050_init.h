/*
 * MPU6050_init.h
 *
 *  Created on: Sep 15, 2021
 *      Author: Samo Novak
 */

#ifndef INC_MPU6050_INIT_H_
#define INC_MPU6050_INIT_H_

#include "stm32l4xx_hal.h"
#include "string.h"

extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim1;

#define MPU6050_ADDR 0x68<<1	    //i2c address

#define SMPLRT_DIV_REG 0x19		    //sample rate divider
#define GYRO_CONFIG_REG 0x1B	    //gyro configurations
#define ACCEL_CONFIG_REG 0x1C	    //acc configurations
#define GYRO_ZOUT_H_REG 0x47	    //gyro z data
#define TEMP_OUT_H_REG 0x41		    //temp data
#define PWR_MGMT_1_REG 0x6B		    //power management register
#define WHO_AM_I_REG 0x75		      //check I2C address

#define DLPF_CFG 0x1A			        //DLPF enable
#define DLPF_20Hz 0x04			      //DLPF 20Hz

#define INT_PIN_CFG 0x37		      //interrupt configurations
#define INT_ENABLE 0x38			      //interrupt enable register
#define DATA_EN_INT 0x01		      //data ready interrupt enable

int16_t Accel_X_RAW;
int16_t Accel_Y_RAW;
int16_t Accel_Z_RAW;

int16_t Gyro_X_RAW;
int16_t Gyro_Y_RAW;
int16_t Gyro_Z_RAW;

int16_t Temperature;

float Gz;
uint16_t dt;
float yaw;

void MPU6050_Init (void);
void MPU6050_Read_Gyro (void);


#endif /* INC_MPU6050_INIT_H_ */
