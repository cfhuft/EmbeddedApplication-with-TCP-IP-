/*
 * MPU6050_init.c
 *
 *  Created on: Sep 15, 2021
 *      Author: Samo Novak
 */

#include <MPU6050_init.h>

void MPU6050_Init (void)
{
	uint8_t check;
	uint8_t Data;


	HAL_I2C_Mem_Read (&hi2c1, MPU6050_ADDR, WHO_AM_I_REG,1, &check, 1, 1000);

	if (check == 104) //0x68
	{
		Data = 0x00;
		HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, PWR_MGMT_1_REG, 1,&Data, 1, 1000);  	//wake the sensor up
		Data = 0x00;
		HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, ACCEL_CONFIG_REG, 1, &Data, 1, 1000);	//± 2g accelerometer scale (16384 LSB/g sensitivity) Ax = Accel_X_RAW/16384.0;
		Data = 0x00;
		HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, GYRO_CONFIG_REG, 1, &Data, 1, 1000);	//± 250 °/s gyroscope scale (131 LSB/°/s sensitivity) Gx = Gyro_X_RAW/131.0;
		Data = DLPF_20Hz;
		HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, DLPF_CFG, 1, &Data, 1, 1000);			//20Hz LPF, 1kHz gyro data rate
		Data = 0x00;
		HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, SMPLRT_DIV_REG, 1, &Data, 1, 1000);		//data rate 8kHz (changed to 1kHz because of LPF enabled)
		Data = 0x00;
		HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, INT_PIN_CFG, 1, &Data, 1, 1000);		//interrupt configurations
		Data = DATA_EN_INT;
		HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, INT_ENABLE, 1, &Data, 1, 1000);			//enable 'data ready' interrupt
	}
}

void MPU6050_Read_Gyro (void)
{
		uint8_t Data[2];
		HAL_I2C_Mem_Read (&hi2c1, MPU6050_ADDR, GYRO_ZOUT_H_REG, 1, Data, 2, 100);
		Gyro_Z_RAW = (int16_t)(Data[0] << 8 | Data [1]);

		Gz = Gyro_Z_RAW/131.0;
		if (Gz<0.2 && Gz>-0.2){Gz=0;}
		dt = (__HAL_TIM_GET_COUNTER(&htim6) - dt);
		yaw += (Gz*dt*0.000001*3);
		dt = __HAL_TIM_GET_COUNTER(&htim6);
}
