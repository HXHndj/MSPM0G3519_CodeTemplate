#include "MPU6050_Reg.h"
#include "mpu6050.h"


/* * MPU6050 地址定义 
 * AD0引脚接低电平时地址为 0xD0 (写) / 0xD1 (读)
 * DriverLib 的 API 需要 7位地址，即 0xD0 >> 1 = 0x68
 */
#define MPU6050_ADDRESS     0xD0
#define MPU6050_I2C_ADDR    (MPU6050_ADDRESS >> 1) 

/**
  * 函    数：MPU6050写寄存器
  * 参    数：RegAddress 寄存器地址
  * 参    数：Data 要写入的数据
  * 返 回 值：无
  */
void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data)
{
    /* 1. 等待I2C控制器空闲 (确保上一次传输已完全结束) */
    while (DL_I2C_getControllerStatus(I2C_0_INST) & DL_I2C_CONTROLLER_STATUS_BUSY);

    /* 2. 在填充数据前，清空 TX FIFO 以确保没有残留数据 */
    DL_I2C_flushControllerTXFIFO(I2C_0_INST);

    /* 3. 准备发送数据：[寄存器地址, 数据] */
    uint8_t txBuf[2] = {RegAddress, Data};
    
    /* 4. 填充 TX FIFO */
    DL_I2C_fillControllerTXFIFO(I2C_0_INST, txBuf, 2);

    /* 5. 启动写传输
     * 模式：标准模式 (自动发送 START 和 STOP)
     * 长度：2字节
     */
    DL_I2C_startControllerTransfer(I2C_0_INST, MPU6050_I2C_ADDR, 
                                   DL_I2C_CONTROLLER_DIRECTION_TX, 2);

    /* 6. 等待传输完成 */
    while (DL_I2C_getControllerStatus(I2C_0_INST) & DL_I2C_CONTROLLER_STATUS_BUSY);
    
    /* 7. 检查是否有错误 (如 NACK) 并清除，防止死锁 */
    if (DL_I2C_getControllerStatus(I2C_0_INST) & DL_I2C_CONTROLLER_STATUS_ERROR) {
        DL_I2C_clearInterruptStatus(I2C_0_INST, DL_I2C_INTERRUPT_CONTROLLER_NACK);
    }
}

/**
  * 函    数：MPU6050读寄存器 (单字节)
  */
uint8_t MPU6050_ReadReg(uint8_t RegAddress)
{
    uint8_t data = 0;

    while (DL_I2C_getControllerStatus(I2C_0_INST) & DL_I2C_CONTROLLER_STATUS_BUSY);
    DL_I2C_flushControllerTXFIFO(I2C_0_INST);

    /* 写入寄存器地址 */
    DL_I2C_fillControllerTXFIFO(I2C_0_INST, &RegAddress, 1);
    DL_I2C_startControllerTransferAdvanced(I2C_0_INST, MPU6050_I2C_ADDR, 
                                           DL_I2C_CONTROLLER_DIRECTION_TX, 1, 
                                           DL_I2C_CONTROLLER_START_ENABLE, 
                                           DL_I2C_CONTROLLER_STOP_DISABLE, 
                                           DL_I2C_CONTROLLER_ACK_DISABLE);

    while (DL_I2C_getControllerStatus(I2C_0_INST) & DL_I2C_CONTROLLER_STATUS_BUSY);

    if (DL_I2C_getControllerStatus(I2C_0_INST) & DL_I2C_CONTROLLER_STATUS_ERROR) {
        DL_I2C_clearInterruptStatus(I2C_0_INST, DL_I2C_INTERRUPT_CONTROLLER_NACK); 
        DL_I2C_startControllerTransfer(I2C_0_INST, MPU6050_I2C_ADDR, DL_I2C_CONTROLLER_DIRECTION_TX, 0); 
        return 0;
    }

    /* 1. 读之前先清空一下接收 FIFO，确保里面没有上次错过的垃圾数据 */
    DL_I2C_flushControllerRXFIFO(I2C_0_INST);
    
    /* 2. 发起读传输 */
    DL_I2C_startControllerTransfer(I2C_0_INST, MPU6050_I2C_ADDR, DL_I2C_CONTROLLER_DIRECTION_RX, 1);
    
    /* 3. 【核心修复】：死盯 FIFO，只要是空的就一直等 */
    while (DL_I2C_isControllerRXFIFOEmpty(I2C_0_INST)) {
        // 如果遇到错误跳出，防止卡死
        if (DL_I2C_getControllerStatus(I2C_0_INST) & DL_I2C_CONTROLLER_STATUS_ERROR) break;
    }

    /* 4. 从 FIFO 拿走我们要的 ID 数据 */
    if (!(DL_I2C_getControllerStatus(I2C_0_INST) & DL_I2C_CONTROLLER_STATUS_ERROR)) {
        data = DL_I2C_receiveControllerData(I2C_0_INST);
    }
    
    /* 5. 数据拿走后，再等待总线发送 STOP 并结束 */
    while (DL_I2C_getControllerStatus(I2C_0_INST) & DL_I2C_CONTROLLER_STATUS_BUSY);

    if (DL_I2C_getControllerStatus(I2C_0_INST) & DL_I2C_CONTROLLER_STATUS_ERROR) {
        DL_I2C_clearInterruptStatus(I2C_0_INST, DL_I2C_INTERRUPT_CONTROLLER_NACK);
    }

    return data;
}

/**
  * 函    数：MPU6050连续读取 (Burst)
  */
void MPU6050_ReadRegs_Burst(uint8_t RegAddress, uint8_t *Buffer, uint16_t Length)
{
    while (DL_I2C_getControllerStatus(I2C_0_INST) & DL_I2C_CONTROLLER_STATUS_BUSY);
    DL_I2C_flushControllerTXFIFO(I2C_0_INST);

    DL_I2C_fillControllerTXFIFO(I2C_0_INST, &RegAddress, 1);
    DL_I2C_startControllerTransferAdvanced(I2C_0_INST, MPU6050_I2C_ADDR, 
                                           DL_I2C_CONTROLLER_DIRECTION_TX, 1, 
                                           DL_I2C_CONTROLLER_START_ENABLE, 
                                           DL_I2C_CONTROLLER_STOP_DISABLE, 
                                           DL_I2C_CONTROLLER_ACK_DISABLE);

    while (DL_I2C_getControllerStatus(I2C_0_INST) & DL_I2C_CONTROLLER_STATUS_BUSY);
    
    /* 清除写地址阶段的错误 */
    if (DL_I2C_getControllerStatus(I2C_0_INST) & DL_I2C_CONTROLLER_STATUS_ERROR) {
        DL_I2C_clearInterruptStatus(I2C_0_INST, DL_I2C_INTERRUPT_CONTROLLER_NACK);
        DL_I2C_startControllerTransfer(I2C_0_INST, MPU6050_I2C_ADDR, DL_I2C_CONTROLLER_DIRECTION_TX, 0); 
        return;
    }

    DL_I2C_flushControllerRXFIFO(I2C_0_INST);
    DL_I2C_startControllerTransfer(I2C_0_INST, MPU6050_I2C_ADDR, DL_I2C_CONTROLLER_DIRECTION_RX, Length);

    for (uint16_t i = 0; i < Length; i++) {
        // 如果出错直接跳出，防止死循环
        while (DL_I2C_isControllerRXFIFOEmpty(I2C_0_INST)) {
             if (DL_I2C_getControllerStatus(I2C_0_INST) & DL_I2C_CONTROLLER_STATUS_ERROR) break;
        }
        if (!(DL_I2C_getControllerStatus(I2C_0_INST) & DL_I2C_CONTROLLER_STATUS_ERROR)) {
             Buffer[i] = DL_I2C_receiveControllerData(I2C_0_INST);
        }
    }

    while (DL_I2C_getControllerStatus(I2C_0_INST) & DL_I2C_CONTROLLER_STATUS_BUSY);
    
    if (DL_I2C_getControllerStatus(I2C_0_INST) & DL_I2C_CONTROLLER_STATUS_ERROR) {
        DL_I2C_clearInterruptStatus(I2C_0_INST, DL_I2C_INTERRUPT_CONTROLLER_NACK);
    }
}

/**
  * 函    数：MPU6050初始化
  * 参    数：无
  * 返 回 值：无
  */
void MPU6050_Init(void)
{	
    /* 确保I2C已初始化 (通常在 main 中的 SYSCFG_DL_init 调用) */
    
	/*MPU6050寄存器初始化*/
	MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x01);		//电源管理寄存器1，取消睡眠模式，时钟源为X轴陀螺仪
	MPU6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00);		//电源管理寄存器2，保持默认值0
	MPU6050_WriteReg(MPU6050_SMPLRT_DIV, 0x04);		//采样率分频 200Hz
	MPU6050_WriteReg(MPU6050_CONFIG, 0x06);			//配置DLPF
	MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x18);	//陀螺仪满量程±2000°/s
	MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x18);	//加速度计满量程±16g
}

/**
  * 函    数：MPU6050获取ID号
  * 参    数：无
  * 返 回 值：MPU6050的ID号
  */
uint8_t MPU6050_GetID(void)
{
	return MPU6050_ReadReg(MPU6050_WHO_AM_I);		//返回WHO_AM_I寄存器的值
}

/**
  * 函    数：MPU6050获取数据 (生数据)
  * 参    数：raw_data 用于存放7组生数据的结构体指针
  * 返 回 值：无
  */
void MPU6050_GetData(MPU6050_RawData *raw_data)
{
    /* 初始化为0，防止读取失败时出现随机乱码 */
    uint8_t buffer[14] = {0}; 
    
    /* 连续读取14个字节的寄存器数据 */
    MPU6050_ReadRegs_Burst(MPU6050_ACCEL_XOUT_H, buffer, 14);
    
    raw_data->AccX  = (int16_t)((buffer[0] << 8)  | buffer[1]);
    raw_data->AccY  = (int16_t)((buffer[2] << 8)  | buffer[3]);
    raw_data->AccZ  = (int16_t)((buffer[4] << 8)  | buffer[5]);
    
    raw_data->Temp  = (int16_t)((buffer[6] << 8)  | buffer[7]);
    
    raw_data->GyroX = (int16_t)((buffer[8] << 8)  | buffer[9]);
    raw_data->GyroY = (int16_t)((buffer[10] << 8) | buffer[11]);
    raw_data->GyroZ = (int16_t)((buffer[12] << 8) | buffer[13]);
}
 
/**
  * 函    数：MPU6050_Read (数据转换)
  * 参    数：raw_data 包含7个通道生数据的结构体 (单个参数)
  * 返 回 值：转换成物理标准单位后的结构体 (单个返回值)
  * 说    明：将I2C读取的整型生数据，基于配置的量程转换为 g, °/s, 和 °C
  */
MPU6050_RealData MPU6050_Read(MPU6050_RawData raw_data) 
{
    MPU6050_RealData standard_data;
    
    /* 1. 转换加速度 (量程±16g，灵敏度 2048 LSB/g) */
    standard_data.AccX = (float)raw_data.AccX / 2048.0f;
    standard_data.AccY = (float)raw_data.AccY / 2048.0f;
    standard_data.AccZ = (float)raw_data.AccZ / 2048.0f;
    
    /* 2. 转换角速度/陀螺仪 (量程±2000°/s，灵敏度 16.4 LSB/(°/s)) */
    standard_data.GyroX = (float)raw_data.GyroX / 16.4f;
    standard_data.GyroY = (float)raw_data.GyroY / 16.4f;
    standard_data.GyroZ = (float)raw_data.GyroZ / 16.4f;
    
    /* 3. 转换温度 (MPU6050官方推荐的温度换算公式) */
    standard_data.Temp = ((float)raw_data.Temp / 340.0f) + 36.53f;
    
    return standard_data;
}

#define Ki  0.005f         // 比例积分控制器的积分增益
#define DT  0.005f         // 算法运行周期的一半 (假设为 10ms 周期，则 DT = 0.005)

// 全局静态四元数 (初始状态表示没有旋转)
static float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;
// 误差积分变量
static float exInt = 0.0f, eyInt = 0.0f, ezInt = 0.0f;

/**
  * 函    数：ComputeEulerAngles (解算欧拉角)
  * 参    数：real_data 传入转换好的物理量结构体指针
  * 参    数：pitch, roll, yaw 用于输出计算结果的指针 (单位：弧度)
  */
void ComputeEulerAngles(MPU6050_RealData *real_data, float *pitch, float *roll, float *yaw) 
{
    float norm;
    float ax = real_data->AccX;
    float ay = real_data->AccY;
    float az = real_data->AccZ;
    
    // 将陀螺仪的度/秒 (°/s) 转换为 弧度/秒 (rad/s)
    float gx = real_data->GyroX * 0.0174533f;
    float gy = real_data->GyroY * 0.0174533f;
    float gz = real_data->GyroZ * 0.0174533f;

    // 1. 加速度计数据归一化
    norm = sqrtf(ax * ax + ay * ay + az * az);
    if (norm > 1e-6f) {  // 防止除零
        ax /= norm;
        ay /= norm;
        az /= norm;
    } else {
        return; // 加速度为0时无意义，直接退出
    }

    // 2. 提取四元数的等效重力分量
    float gravity_x = 2.0f * (q1 * q3 - q0 * q2);
    float gravity_y = 2.0f * (q0 * q1 + q2 * q3);
    float gravity_z = q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3;
    
    // 3. 计算测量重力与等效重力的叉乘误差
    float error_x = ay * gravity_z - az * gravity_y;
    float error_y = az * gravity_x - ax * gravity_z;
    float error_z = ax * gravity_y - ay * gravity_x;
    
    // 4. PI 控制器
    float Kp = 0.5f;
    exInt += Ki * error_x;
    eyInt += Ki * error_y;
    ezInt += Ki * error_z;
    
    // 修正后的角速度
    gx += Kp * error_x + exInt;
    gy += Kp * error_y + eyInt;
    gz += Kp * error_z + ezInt;
    
    // 5. 龙格-库塔法更新四元数微分方程
    float q0_dot = (-q1 * gx - q2 * gy - q3 * gz) * DT; 
    float q1_dot = ( q0 * gx - q3 * gy + q2 * gz) * DT;
    float q2_dot = ( q3 * gx + q0 * gy - q1 * gz) * DT;
    float q3_dot = (-q2 * gx + q1 * gy + q0 * gz) * DT;
    
    q0 += q0_dot;
    q1 += q1_dot;
    q2 += q2_dot;
    q3 += q3_dot;
    
    // 6. 四元数重新归一化
    norm = sqrtf(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    if (norm > 1e-6f) {
        q0 /= norm;
        q1 /= norm;
        q2 /= norm;
        q3 /= norm;
    }
    
    // 7. 将四元数转换为欧拉角 (返回值为弧度 rad)
    *roll  = atan2f(2.0f * (q2 * q3 + q0 * q1), q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3);
    *pitch = asinf(-2.0f * (q1 * q3 - q0 * q2));
    *yaw   = atan2f(2.0f * (q1 * q2 + q0 * q3), q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3);
}