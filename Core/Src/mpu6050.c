#include "main.h"
#include "gpio.h"
#include "mpu6050_Reg.h"
#include "i2c.h"
#include "OLED.h"
#include "Key.h"
#include "menu.h"

// 陀螺仪零偏校准值
static int16_t Gyro_Offset_X = 0;
static int16_t Gyro_Offset_Y = 0;
static int16_t Gyro_Offset_Z = 0;
static uint8_t MPU6050_Initialized = 0;

// I2C总线复位 - 当I2C被锁死时调用
static void I2C_Bus_Reset(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 先配置为GPIO开漏输出
    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11; // PB10=SCL, PB11=SDA
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // 发送9个时钟脉冲
    for (int i = 0; i < 9; i++)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
        HAL_Delay(1);
    }

    // 发送停止条件
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
    HAL_Delay(1);

    // 重新初始化I2C
    HAL_I2C_DeInit(&hi2c2);
    HAL_I2C_Init(&hi2c2);
}

HAL_StatusTypeDef MPU6050_WriteReg(uint16_t WriteAddress, uint8_t pData)
{
    HAL_StatusTypeDef status;
    status = HAL_I2C_Mem_Write(&hi2c2, (0x68 << 1), WriteAddress, I2C_MEMADD_SIZE_8BIT, &pData, 1, 20);
    if (status != HAL_OK)
    {
        I2C_Bus_Reset(); // 失败时复位总线
    }
    return status;
}

HAL_StatusTypeDef MPU6050_ReadReg(uint16_t ReadAddress, uint8_t *pData)
{
    HAL_StatusTypeDef status;
    status = HAL_I2C_Mem_Read(&hi2c2, (0x68 << 1), ReadAddress, I2C_MEMADD_SIZE_8BIT, pData, 1, 20);
    if (status != HAL_OK)
    {
        I2C_Bus_Reset(); // 失败时复位总线
    }
    return status;
}

uint8_t MPU6050_ReadReg_Simple(uint16_t ReadAddress)
{
    uint8_t Data = 0;
    MPU6050_ReadReg(ReadAddress, &Data);
    return Data;
}


uint8_t MPU6050_GetID(void)
{
    return MPU6050_ReadReg_Simple(MPU6050_WHO_AM_I);
}

// 带错误检查的读取函数
static HAL_StatusTypeDef MPU6050_ReadData(uint16_t RegAddr, int16_t *Data)
{
    uint8_t DataH, DataL;
    HAL_StatusTypeDef status;

    status = MPU6050_ReadReg(RegAddr, &DataH);
    if (status != HAL_OK) return status;

    status = MPU6050_ReadReg(RegAddr + 1, &DataL);
    if (status != HAL_OK) return status;

    *Data = (int16_t)((DataH << 8) | DataL);
    return HAL_OK;
}

// 陀螺仪零偏校准 - 简化为20次采样
void MPU6050_CalibrateGyro(void)
{
    int32_t sum_x = 0, sum_y = 0, sum_z = 0;
    int16_t gx, gy, gz;
    HAL_StatusTypeDef status;
    uint8_t i;
    uint8_t success_count = 0;

    // 采集20次数据求平均（减少初始化时间）
    for (i = 0; i < 20; i++)
    {
        status = MPU6050_ReadData(MPU6050_GYRO_XOUT_H, &gx);
        if (status != HAL_OK) continue;

        status = MPU6050_ReadData(MPU6050_GYRO_YOUT_H, &gy);
        if (status != HAL_OK) continue;

        status = MPU6050_ReadData(MPU6050_GYRO_ZOUT_H, &gz);
        if (status != HAL_OK) continue;

        sum_x += gx;
        sum_y += gy;
        sum_z += gz;
        success_count++;
        HAL_Delay(2); // 缩短延时
    }

    if (success_count > 0)
    {
        Gyro_Offset_X = (int16_t)(sum_x / success_count);
        Gyro_Offset_Y = (int16_t)(sum_y / success_count);
        Gyro_Offset_Z = (int16_t)(sum_z / success_count);
    }
}

HAL_StatusTypeDef MPU6050_Init(void)
{
    HAL_StatusTypeDef status;

    MX_GPIO_Init();

    status = MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x01);
    if (status != HAL_OK) return status;

    status = MPU6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00);
    if (status != HAL_OK) return status;

    status = MPU6050_WriteReg(MPU6050_SMPLRT_DIV, 0x04);
    if (status != HAL_OK) return status;

    status = MPU6050_WriteReg(MPU6050_CONFIG, 0x06);
    if (status != HAL_OK) return status;

    status = MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x18);
    if (status != HAL_OK) return status;

    status = MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x18);
    if (status != HAL_OK) return status;

    // 等待传感器稳定
    HAL_Delay(100);

    // 执行陀螺仪零偏校准
    MPU6050_CalibrateGyro();

    MPU6050_Initialized = 1;
    return HAL_OK;
}

// 获取校准后的陀螺仪零偏值
void MPU6050_GetGyroOffset(int16_t *offset_x, int16_t *offset_y, int16_t *offset_z)
{
    *offset_x = Gyro_Offset_X;
    *offset_y = Gyro_Offset_Y;
    *offset_z = Gyro_Offset_Z;
}

HAL_StatusTypeDef MPU6050_GetData(int16_t *ax, int16_t *gx, int16_t *ay, int16_t *gy, int16_t *az, int16_t *gz)
{
    HAL_StatusTypeDef status;

    if (!MPU6050_Initialized)
    {
        return HAL_ERROR;
    }

    status = MPU6050_ReadData(MPU6050_ACCEL_XOUT_H, ax);
    if (status != HAL_OK) return status;

    status = MPU6050_ReadData(MPU6050_GYRO_XOUT_H, gx);
    if (status != HAL_OK) return status;

    status = MPU6050_ReadData(MPU6050_ACCEL_YOUT_H, ay);
    if (status != HAL_OK) return status;

    status = MPU6050_ReadData(MPU6050_GYRO_YOUT_H, gy);
    if (status != HAL_OK) return status;

    status = MPU6050_ReadData(MPU6050_ACCEL_ZOUT_H, az);
    if (status != HAL_OK) return status;

    status = MPU6050_ReadData(MPU6050_GYRO_ZOUT_H, gz);
    if (status != HAL_OK) return status;

    // 减去零偏校准值
    *gx -= Gyro_Offset_X;
    *gy -= Gyro_Offset_Y;
    *gz -= Gyro_Offset_Z;

    return HAL_OK;
}



