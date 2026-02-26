#include "zw101.h"

// 默认设备地址
const uint32_t ZW101_ADDR = 0xFFFFFFFF; 

/**
 * @brief  毫秒级延时函数 (内部调用)
 * @param  ms: 延时时间(毫秒)
 * @return 无
 */
static void ZW101_DelayMs(uint32_t ms) 
{
    delay_ms(ms);
}

/**
 * @brief  串口发送单字节数据 (内部调用)
 * @param  data: 要发送的字节数据
 * @return 无
 */
static void ZW101_SendByte(uint8_t data) 
{
    DL_UART_transmitDataBlocking(ZW101_UART_INST, data);// DL库底层串口发送封装
}

/**
 * @brief  封装并发送指令包到指纹模块 (内部调用)
 * @param  cmd: 指令码 (例如 0x01 表示获取图像)
 * @param  params: 指令参数数组指针
 * @param  param_len: 参数数组的长度
 * @return 无
 */
static void ZW101_SendCmd(uint8_t cmd, uint8_t *params, uint16_t param_len) 
{
    uint16_t len = param_len + 3; // 参数长度 + 指令码(1) + 校验和(2)
    uint16_t sum = 0x01 + len + cmd; // 包标识 0x01 + 长度 + 指令码
    
    ZW101_SendByte(0xEF); // 包头
    ZW101_SendByte(0x01); // 包头
    ZW101_SendByte((ZW101_ADDR >> 24) & 0xFF); // 地址
    ZW101_SendByte((ZW101_ADDR >> 16) & 0xFF);
    ZW101_SendByte((ZW101_ADDR >> 8) & 0xFF);
    ZW101_SendByte(ZW101_ADDR & 0xFF);
    
    ZW101_SendByte(0x01); // 包标识: 命令包
    ZW101_SendByte((len >> 8) & 0xFF);
    ZW101_SendByte(len & 0xFF);
    ZW101_SendByte(cmd);
    
    for(uint16_t i = 0; i < param_len; i++) 
    {
        ZW101_SendByte(params[i]);
        sum += params[i];
    }
    
    ZW101_SendByte((sum >> 8) & 0xFF);
    ZW101_SendByte(sum & 0xFF);
}

/**
 * @brief  接收模块的应答包并解析提取数据 (内部调用)
 * @param  ack_code: 指向用于存储确认码的变量指针
 * @param  ret_data: 指向用于存储返回数据的缓冲区的指针 (可为NULL)
 * @param  ret_len: 指向用于存储返回数据长度的变量指针 (可为NULL)
 * @param  timeout_ms: 通信超时时间(毫秒)
 * @return bool: true 接收并解析成功, false 接收超时或数据异常
 */
static bool ZW101_ReceiveAck(uint8_t *ack_code, uint8_t *ret_data, uint16_t *ret_len, uint32_t timeout_ms) 
{
    uint8_t rx_buf[64];
    uint16_t rx_idx = 0;
    uint32_t count = 0;
    uint32_t timeout_cycles = timeout_ms * 3200; // 粗略估算超时循环
    
    while(count < timeout_cycles) 
    {
        if (!DL_UART_isRXFIFOEmpty(ZW101_UART_INST)) 
        {
            rx_buf[rx_idx++] = DL_UART_receiveData(ZW101_UART_INST);
            count = 0; // 收到数据重置超时
            
            // 检查是否接收完包头、地址、标识符、长度
            if(rx_idx >= 9) 
            {
                uint16_t pkg_len = (rx_buf[7] << 8) | rx_buf[8];
                if(rx_idx >= (9 + pkg_len)) 
                {
                    // 接收完毕，提取确认码 (确认码在索引9的位置)
                    *ack_code = rx_buf[9]; 
                    if(ret_data != NULL && ret_len != NULL) 
                    {
                        *ret_len = pkg_len - 3; // 减去确认码和校验和
                        for(uint16_t i = 0; i < *ret_len; i++) 
                        {
                            ret_data[i] = rx_buf[10 + i];
                        }
                    }
                    return true;
                }
            }
        } 
        else 
        {
            count++;
        }
    }
    return false; // 超时
}

/**
 * @brief  控制模块采集指纹图像 (内部调用)
 * @return bool: true 采图成功, false 采图失败(如无手指或超时)
 */
static bool ZW101_GetImage(void) 
{
    uint8_t ack;
    ZW101_SendCmd(ZW101_CMD_GET_IMAGE, NULL, 0);
    if(ZW101_ReceiveAck(&ack, NULL, NULL, 500) && ack == ZW101_ACK_SUCCESS) return true;
    return false;
}

/**
 * @brief  将采集到的图像生成特征并存入指定 Buffer (内部调用)
 * @param  buffer_id: 特征缓冲区ID (通常为 1 或 2)
 * @return bool: true 生成成功, false 生成失败
 */
static bool ZW101_GenChar(uint8_t buffer_id) 
{
    uint8_t ack;
    ZW101_SendCmd(ZW101_CMD_GEN_CHAR, &buffer_id, 1);
    if(ZW101_ReceiveAck(&ack, NULL, NULL, 500) && ack == ZW101_ACK_SUCCESS) return true;
    return false;
}

// ========================================================================= //
// ---------------------------- 外部调用 API ------------------------------- //
// ========================================================================= //

/**
 * @brief  指纹模块初始化，清空串口接收 FIFO
 * @return 无
 */
void ZW101_Init(void) 
{
    // DL库的串口初始化已经在 ti_msp_dl_config.c 中完成了
    // 这里只需清空一下 RX FIFO
    while(!DL_UART_isRXFIFOEmpty(ZW101_UART_INST)) 
    {
        DL_UART_receiveData(ZW101_UART_INST);
    }
}

/**
 * @brief  自定义次数的指纹注册函数 (修复闪存覆写版)
 * @param  id: 存储的指纹ID (0-49) 
 * @param  times: 录入次数 (建议 5-10 次) 
 * @return bool: true 为成功，false 为失败
 */
bool ZW101_Enroll(uint16_t id, uint8_t times) 
{
    uint8_t ack;
    if (times < 2 || times > 12) return false; 

    for(uint8_t step = 1; step <= times; step++) 
    {
        // 1. 等待手指按下
        while(DL_GPIO_readPins(GPIOB, ZW_TouchOut_PIN) == 0) 
        {
            ZW101_DelayMs(50);
        }

        // 2. 采图
        if (!ZW101_GetImage()) 
        {
            step--; 
            continue;
        }

        // 3. 生成特征【核心修复：杜绝使用 3 以上的 BufferID】
        // 第一次按压生成到 Buffer 1，后续所有的按压都生成到临时 Buffer 2
        uint8_t buffer_id = (step == 1) ? 1 : 2;
        if(!ZW101_GenChar(buffer_id)) return false;
        
        // 4. 合并特征【核心修复：每次采图后进行累加合并】
        if (step > 1) 
        {
            // ZW101_CMD_REG_MODEL 会自动把 Buffer1 和 Buffer2 融合成更优的模板，并存回 Buffer1
            ZW101_SendCmd(ZW101_CMD_REG_MODEL, NULL, 0);
            if(!ZW101_ReceiveAck(&ack, NULL, NULL, 1500) || ack != ZW101_ACK_SUCCESS) 
            {
                return false; 
            }
        }
        
        // 提示单次成功
        ZW101_LedCtrl(ZW101_LED_FLASH, ZW101_LED_GREEN, 1); 
        
        // 5. 等待手指抬起
        if (step < times) 
        {
            while(DL_GPIO_readPins(GPIOB, ZW_TouchOut_PIN) != 0) 
            {
                ZW101_DelayMs(50); 
            }
        }
    }
    
    // 6. 循环结束后，10 次按压融合成的“最强指纹特征”已经安全地存放在 Buffer 1 中了
    // 现在将其真正写入到 Flash 闪存中的指定 id 位置
    uint8_t store_params[3] = {0x01, (uint8_t)(id >> 8), (uint8_t)(id & 0xFF)};
    ZW101_SendCmd(ZW101_CMD_STORE_CHAR, store_params, 3);
    if(!ZW101_ReceiveAck(&ack, NULL, NULL, 1000) || ack != ZW101_ACK_SUCCESS) 
    {
        return false; 
    }
    
    return true;
}

/**
 * @brief  采集当前指纹并在库中搜索匹配 (1:N 识别)
 * @param  matched_id: 指向用于存储匹配成功ID的指针
 * @return bool: true 匹配成功, false 匹配失败或未检测到手指
 */
bool ZW101_Match(uint16_t *matched_id) 
{
    uint8_t ack;
    uint8_t ret_data[4];
    uint16_t ret_len;
    
    if(!ZW101_GetImage()) return false;
    if(!ZW101_GenChar(1)) return false;
    
    // 在全库中搜索: BufferID=1, StartPage=0, PageNum=0xFFFF
    uint8_t search_params[5] = {0x01, 0x00, 0x00, 0xFF, 0xFF};
    ZW101_SendCmd(ZW101_CMD_SEARCH, search_params, 5);
    
    if(ZW101_ReceiveAck(&ack, ret_data, &ret_len, 1000)) 
    {
        if(ack == ZW101_ACK_SUCCESS && ret_len >= 4) 
        {
            *matched_id = (ret_data[0] << 8) | ret_data[1];
            return true;
        }
    }
    return false;
}

/**
 * @brief  删除指定 ID 的指纹模板
 * @param  id: 要删除的指纹 ID (0-49)
 * @return bool: true 删除成功, false 删除失败
 */
bool ZW101_Delete(uint16_t id) 
{
    uint8_t ack;
    uint8_t params[4] = {(uint8_t)(id >> 8), (uint8_t)(id & 0xFF), 0x00, 0x01}; // 删除1个
    ZW101_SendCmd(ZW101_CMD_DELETE, params, 4);
    if(ZW101_ReceiveAck(&ack, NULL, NULL, 1000) && ack == ZW101_ACK_SUCCESS) return true;
    return false;
}

/**
 * @brief  清空整个指纹库中的所有模板
 * @return bool: true 清空成功, false 清空失败
 */
bool ZW101_Clear(void) 
{
    uint8_t ack;
    ZW101_SendCmd(ZW101_CMD_CLEAR_LIB, NULL, 0);
    if(ZW101_ReceiveAck(&ack, NULL, NULL, 2000) && ack == ZW101_ACK_SUCCESS) return true;
    return false;
}

/**
 * @brief  控制指纹模块的 LED 灯效
 * @param  mode: 灯效模式控制码
 * @param  color: 起始及结束颜色代码
 * @param  count: 闪烁/呼吸的循环次数 (0 通常代表常量或无限循环)
 * @return 无
 */
void ZW101_LedCtrl(uint8_t mode, uint8_t color, uint8_t count) 
{
    uint8_t ack;
    
    uint8_t params[4] = {mode, color, color, count};
    ZW101_SendCmd(ZW101_CMD_LED_CTRL, params, 4);
    ZW101_ReceiveAck(&ack, NULL, NULL, 200); 
}

/**
 * @brief  带有手指抬起保护的识别函数 (防闪烁、非阻塞推荐版)
 * @param  matched_id: 输出匹配成功的 ID
 * @return 0: 无手指; 1: 匹配成功; 2: 匹配失败/超时
 */
uint8_t ZW101_Identify(uint16_t *matched_id) 
{
    uint8_t ack;
    uint8_t ret_data[4];
    uint16_t ret_len;

    // 1. 硬件预检测：无手指则立即退出，不产生闪烁 
    if (DL_GPIO_readPins(GPIOB, ZW_TouchOut_PIN) == 0) 
    {
        return 0; 
    }

    // 2. 识别流程：获取图像 -> 生成特征 -> 搜索
    ZW101_LedCtrl(ZW101_LED_ON, ZW101_LED_BLUE, 0); // 识别中亮蓝灯

    // 获取图像指令 (0x01) 
    ZW101_SendCmd(ZW101_CMD_GET_IMAGE, NULL, 0);
    if (!ZW101_ReceiveAck(&ack, NULL, NULL, 300) || ack != ZW101_ACK_SUCCESS) 
    {
        goto WAIT_FOR_LIFT; // 识别中断也要等手指抬起，防止反复报错
    }

    // 生成特征 (0x02)
    if (!ZW101_GenChar(1)) 
    {
        ZW101_LedCtrl(ZW101_LED_FLASH, ZW101_LED_RED, 2);
        goto WAIT_FOR_LIFT;
    }

    // 搜索指纹库 (0x04)
    uint8_t search_params[5] = {0x01, 0x00, 0x00, 0xFF, 0xFF};
    ZW101_SendCmd(ZW101_CMD_SEARCH, search_params, 5);
    
    if (ZW101_ReceiveAck(&ack, ret_data, &ret_len, 1000)) 
    {
        if (ack == ZW101_ACK_SUCCESS && ret_len >= 4) // 0x00 为执行成功
        {
            *matched_id = (ret_data[0] << 8) | ret_data[1];
            ZW101_LedCtrl(ZW101_LED_FLASH, ZW101_LED_GREEN, 2); // 成功亮绿灯
            
            // --- 关键保护逻辑：等待手指抬起 ---
            // 只有检测到 TOUCH_OUT 为 0 时才退出循环 
            while(DL_GPIO_readPins(GPIOB, ZW_TouchOut_PIN) != 0) 
            {
                ZW101_DelayMs(10); 
            }
            return 1; 
        }
    }

    // 识别失败提示
    ZW101_LedCtrl(ZW101_LED_FLASH, ZW101_LED_RED, 2);

// 标签：无论成功失败，只要触发了识别，就强制要求抬起手指
WAIT_FOR_LIFT:
    while(DL_GPIO_readPins(GPIOB, ZW_TouchOut_PIN) != 0) 
    {
        ZW101_DelayMs(10); 
    }
    return 2; 
}

/**
 * @brief  获取当前指纹库中已录入的指纹数量
 * @param  count: 指向用于存储获取到数量的变量指针
 * @return bool: true 获取成功，false 获取/通信失败
 */
bool ZW101_GetEnrollCount(uint16_t *count) 
{
    uint8_t ack;
    uint8_t ret_data[4];
    uint16_t ret_len;
    
    // 发送读有效模板个数指令 (指令码 0x1D)
    ZW101_SendCmd(ZW101_CMD_VALID_TEMP_NUM, NULL, 0);
    
    // 接收应答包 (该指令处理极快，500ms 超时足够)
    if(ZW101_ReceiveAck(&ack, ret_data, &ret_len, 500)) 
    {
        // 确认码 0x00 表示读取成功 
        // 且返回数据长度至少应为 2 字节（数量的高低字节）
        if(ack == ZW101_ACK_SUCCESS && ret_len >= 2) 
        {
            // 组合高低字节得出真实数量 
            *count = (ret_data[0] << 8) | ret_data[1];
            return true;
        }
    }
    return false;
}

/**
 * @brief  自动扫描指纹库，获取第一个未被使用的空闲 ID
 * @param  free_id: 指向用于存储空闲 ID 的指针
 * @return bool: true 找到空闲ID, false 库已满或通信失败
 */
bool ZW101_GetFreeID(uint16_t *free_id) 
{
    uint8_t ack;
    uint8_t param[1] = {0x00}; // 查询第0页（包含0-255号ID状态）
    uint8_t ret_data[34];
    uint16_t ret_len;

    ZW101_SendCmd(ZW101_CMD_READ_INDEX, param, 1);
    
    if(ZW101_ReceiveAck(&ack, ret_data, &ret_len, 500)) 
    {
        if(ack == ZW101_ACK_SUCCESS && ret_len >= 32) 
        {
            // 遍历 32 字节的位图 (32 * 8 = 256 个 ID)
            for(uint16_t i = 0; i < 32; i++) 
            {
                for(uint8_t bit = 0; bit < 8; bit++) 
                {
                    if(!(ret_data[i] & (1 << bit))) // 如果该位是 0，说明空闲
                    {
                        *free_id = i * 8 + bit;
                        // ZW101该型号最大支持 50 枚，超过则认为已满
                        if (*free_id >= 50) return false; 
                        return true;
                    }
                }
            }
        }
    }
    return false; // 通信失败或库已满
}

/**
 * @brief  强行打断并终止模块当前正在进行的操作
 * @return bool: true 终止成功, false 终止失败
 */
bool ZW101_Cancel(void) 
{
    uint8_t ack;
    ZW101_SendCmd(ZW101_CMD_CANCEL, NULL, 0);
    
    // 中断指令响应极快
    if(ZW101_ReceiveAck(&ack, NULL, NULL, 200)) 
    {
        return (ack == ZW101_ACK_SUCCESS);
    }
    return false;
}

/**
 * @brief  检查某个指定的 ID 是否已经录入了指纹
 * @param  id: 需要查询的指纹 ID (0 - 49)
 * @return bool: true 已录入(被占用), false 未录入(空闲) 或 查询失败
 */
bool ZW101_CheckEnrolled(uint16_t id) 
{
    uint8_t ack;
    uint8_t param[1] = {0x00}; 
    uint8_t ret_data[34];
    uint16_t ret_len;

    if (id >= 50) return false; // 参数保护

    ZW101_SendCmd(ZW101_CMD_READ_INDEX, param, 1);
    
    if(ZW101_ReceiveAck(&ack, ret_data, &ret_len, 500)) 
    {
        if(ack == ZW101_ACK_SUCCESS && ret_len >= 32) 
        {
            // 计算该 ID 在位图中的字节位置和具体的位偏移
            uint8_t byte_index = id / 8;
            uint8_t bit_offset = id % 8;
            
            // 提取该位的值，如果为 1 返回 true，为 0 返回 false
            if(ret_data[byte_index] & (1 << bit_offset)) 
            {
                return true; 
            }
            return false;
        }
    }
    return false;
}