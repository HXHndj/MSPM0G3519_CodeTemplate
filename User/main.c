#include "ti_msp_dl_config.h"
#include "bsp.h"
#include "OLED.h"
#include "delay.h"
#include "mpu6050.h"
#include <math.h>

// --- 3D 坐标数据结构 ---
typedef struct { float x, y, z; } Point3D;
typedef struct { int16_t x, y; } Point2D;

// 正方体的 8 个初始顶点
const Point3D cube_vertices[8] = 
{
    {-20, -20, -20}, { 20, -20, -20}, { 20,  20, -20}, {-20,  20, -20},
    {-20, -20,  20}, { 20, -20,  20}, { 20,  20,  20}, {-20,  20,  20}
};

// 正方体的 12 条边
const uint8_t cube_edges[12][2] = 
{
    {0,1}, {1,2}, {2,3}, {3,0},
    {4,5}, {5,6}, {6,7}, {7,4},
    {0,4}, {1,5}, {2,6}, {3,7}
};

// --- 3D 渲染函数 ---
void Draw_3D_Cube(float pitch, float roll, float yaw) 
{
    Point2D screen_pts[8];
    float sin_p = sinf(pitch), cos_p = cosf(pitch);
    float sin_r = sinf(roll),  cos_r = cosf(roll);
    float sin_y = sinf(yaw),   cos_y = cosf(yaw);

    for (int i = 0; i < 8; i++) {
        float x = cube_vertices[i].x;
        float y = cube_vertices[i].y;
        float z = cube_vertices[i].z;

        // 旋转
        float xy = cos_p * y - sin_p * z;
        float xz = sin_p * y + cos_p * z;
        y = xy; z = xz;

        float yx =  cos_r * x + sin_r * z;
        float yz = -sin_r * x + cos_r * z;
        x = yx; z = yz;

        float zx = cos_y * x - sin_y * y;
        float zy = sin_y * x + cos_y * y;
        x = zx; y = zy;

        // 投影
        float z_depth = z + 80.0f; 
        screen_pts[i].x = (int16_t)((x * 64.0f) / z_depth) + 64; 
        screen_pts[i].y = (int16_t)((y * 64.0f) / z_depth) + 32; 
    }

    // 绘制连线
    for (int i = 0; i < 12; i++) {
        OLED_DrawLine(screen_pts[cube_edges[i][0]].x, screen_pts[cube_edges[i][0]].y, 
                      screen_pts[cube_edges[i][1]].x, screen_pts[cube_edges[i][1]].y);
    }
}

int main(void)
{
    // 系统初始化
    SYSCFG_DL_init();
    MPU6050_Init();
    OLED_Init();

    // 局部变量
    float pitch = 0, roll = 0, yaw = 0;
    MPU6050_RawData raw_data;
    MPU6050_RealData real_data;

    while (1)
    {
        // 获取数据解算欧拉角
        MPU6050_GetData(&raw_data);
        real_data = MPU6050_Read(raw_data);
        ComputeEulerAngles(&real_data, &pitch, &roll, &yaw);

        OLED_Clear();
        // 绘制带有姿态的 3D 正方体 
        // 加上负号可以修正转动方向，例如：Draw_3D_Cube(-pitch, -roll, yaw);
        Draw_3D_Cube(pitch, roll, yaw);
        OLED_Update();

        // 补充延时，配合 Mahony 算法的 5ms (0.005f) 积分时间
        // 因为 Oled 刷屏和 I2C 大约耗时 2ms，这里延时 3ms 刚好补满 5ms。
        delay_ms(3); 
    }
}

//int main(void)
//{
//	MPU6050_RawData raw;
//	MPU6050_RealData real;
//	
//    SYSCFG_DL_init();
//    MPU6050_Init();
//    OLED_Init();

//    /* --- 1. 开机动画与 Logo 显示 --- */
//    OLED_Clear();
//    OLED_ShowImage(0, 0, 128, 64, Ark);
//    OLED_Update();
//    delay_ms(500);

//    /* --- 2. 绘制静态 UI 框架 (放在循环外提高性能) --- */
//    OLED_Clear();
//    
//    // 显示设备ID和温度
//    OLED_ShowString(0, 0, "ID:", OLED_6X8);
//    OLED_ShowNum(18, 0, MPU6050_GetID(), 3, OLED_6X8); // 正常应该显示 104 (0x68)
//	OLED_ShowString(42, 0, "Temp:", OLED_6X8);
//    
//    // 绘制加速度标签 (左半屏)
//    OLED_ShowString(0, 16, "AX:", OLED_6X8);
//    OLED_ShowString(0, 32, "AY:", OLED_6X8);
//    OLED_ShowString(0, 48, "AZ:", OLED_6X8);
//    
//    // 绘制角速度标签 (右半屏)
//    OLED_ShowString(64, 16, "GX:", OLED_6X8);
//    OLED_ShowString(64, 32, "GY:", OLED_6X8);
//    OLED_ShowString(64, 48, "GZ:", OLED_6X8);
//    
//    OLED_Update();
//    delay_ms(500); // 延时让人看清 ID

//    /* --- 3. 实时数据获取与刷新 --- */
//    while (1)
//    {
//        MPU6050_GetData(&raw);
//        real = MPU6050_Read(raw);

//        // 显示加速度数据 (起始X坐标20，预留5位数字宽度，带正负号)
//        OLED_ShowSignedNum(20, 16, raw.AccX, 5, OLED_6X8);
//        OLED_ShowSignedNum(20, 32, raw.AccY, 5, OLED_6X8);
//        OLED_ShowSignedNum(20, 48, raw.AccZ, 5, OLED_6X8);

//		OLED_ShowFloatNum(72, 0, real.Temp, 2, 1, OLED_6X8);
//        // 显示陀螺仪(角速度)数据 (起始X坐标84，预留5位数字宽度，带正负号)
//        OLED_ShowSignedNum(84, 16, raw.GyroX, 5, OLED_6X8);
//        OLED_ShowSignedNum(84, 32, raw.GyroY, 5, OLED_6X8);
//        OLED_ShowSignedNum(84, 48, raw.GyroZ, 5, OLED_6X8);

//        // 统一推送到 OLED 屏幕
//        OLED_Update();

//        // 加入适当的延时限制刷新率
//        delay_ms(50);
//    }