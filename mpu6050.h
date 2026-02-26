#ifndef __MPU6050_H
#define __MPU6050_H

#include "ti_msp_dl_config.h"
#include <math.h>

typedef struct {
    int16_t AccX;
    int16_t AccY;
    int16_t AccZ;
    int16_t Temp;
    int16_t GyroX;
    int16_t GyroY;
    int16_t GyroZ;
} MPU6050_RawData;

typedef struct {
    float AccX;   // 加速度，单位: g
    float AccY;   
    float AccZ;   
    float Temp;   // 温度，单位: °C
    float GyroX;  // 角速度，单位: °/s
    float GyroY;  
    float GyroZ;  
} MPU6050_RealData;

void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data);
uint8_t MPU6050_ReadReg(uint8_t RegAddress);
void MPU6050_Init(void);
uint8_t MPU6050_GetID(void);
void MPU6050_GetData(MPU6050_RawData *raw_data);
MPU6050_RealData MPU6050_Read(MPU6050_RawData raw_data);
void ComputeEulerAngles(MPU6050_RealData *real_data, float *pitch, float *roll, float *yaw);

#endif