#ifndef __MPU6050_H
#define __MPU6050_H
#include "main.h"

HAL_StatusTypeDef MPU6050_WriteReg(uint16_t WriteAddress, uint8_t pData);
HAL_StatusTypeDef MPU6050_ReadReg(uint16_t ReadAddress, uint8_t *pData);
uint8_t MPU6050_ReadReg_Simple(uint16_t ReadAddress);
uint8_t MPU6050_GetID(void);
void MPU6050_CalibrateGyro(void);
HAL_StatusTypeDef MPU6050_Init(void);
void MPU6050_GetGyroOffset(int16_t *offset_x, int16_t *offset_y, int16_t *offset_z);
HAL_StatusTypeDef MPU6050_GetData(int16_t *ax, int16_t *gx, int16_t *ay, int16_t *gy, int16_t *az, int16_t *gz);

#endif
