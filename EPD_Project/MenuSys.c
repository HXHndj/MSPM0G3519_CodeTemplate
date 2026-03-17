#include "MenuSys.h"
#include "EEPROM_WR.h"

uint8_t g_CursorPos = 0; // 全局变量：光标位置（0表示第一行）
uint8_t g_PageIndex = 0; // 全局变量：当前页面ID
bool OLED_Updateflag = 1; // 只有当该标志位为1时，屏幕才更新，防止一直刷新

uint8_t BlackImage[2888]; // 存放黑色内容的缓冲区
uint8_t RedImage[2888];   // 存放红色内容的缓冲区

ShopList Product;

uint8_t Get_Max_Days(uint16_t year, uint8_t month)
{
    if (month == 2) 
    {
        // 闰年判断：能被4整除且不能被100整除，或者能被400整除
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            return 29;
        } else {
            return 28;
        }
    } 
    else if (month == 4 || month == 6 || month == 9 || month == 11) 
    {
        return 30; // 小月
    } 
    else 
    {
        return 31; // 大月
    }
}

void Cursor_ResShow(void)
{
	uint8_t target_id = g_CursorPos;
	if (!Product.if_plus && g_CursorPos >= 4) 
	{
		target_id += 1;
	}

	switch (target_id)
	{
		// --- 第一页属性 ---
		case 0: // 品名
			if (Product.name <= 2) { OLED_ReverseArea(48, 0, 64, 16); }
			else { OLED_ReverseArea(48, 0, 79, 16); } // 顺手帮你把之前提到的 128 越界隐患改成了 79
			break;
		case 1: // 加量开关
			OLED_ReverseArea(48, 16, 8, 16); break;
		case 2: // 单位
			OLED_ReverseArea(112, 16, 16, 16); break;
		case 3: // 规格
			OLED_ReverseArea(48, 32, 24, 16); break;	
		case 4: // 加量规格 (仅当 Product.if_plus == 1 时才会被映射到)
			OLED_ReverseArea(80, 32, 24, 16); break;
		case 5: // 原价整数
			OLED_ReverseArea(48, 48, 24, 16); break;
		case 6: // 原价小数
			OLED_ReverseArea(80, 48, 16, 16); break;	
		
		// --- 第二页属性 ---
		case 7: // 会员价整数
			OLED_ReverseArea(64, 0, 24, 16); break;
		case 8: // 会员价小数
			OLED_ReverseArea(96, 0, 16, 16); break;
		case 9: // 生产年
			OLED_ReverseArea(0, 32, 32, 16); break;
		case 10: // 生产月
			OLED_ReverseArea(48, 32, 16, 16); break;	
		case 11: // 生产日
			OLED_ReverseArea(80, 32, 16, 16); break;
		case 12: // 保质期
			OLED_ReverseArea(64, 48, 16, 16); break;
		case 13: // 保质期单位
			OLED_ReverseArea(80, 48, 16, 16); break;	
	}
}

void List_DataCTLdown(void)
{
	uint8_t target_id = g_CursorPos;
	if (!Product.if_plus && g_CursorPos >= 4) { target_id += 1; }

	switch (target_id)
	{
		case 0:  Product.name = (Product.name == 0 || Product.name > 3) ? 3 : Product.name - 1; break;
		case 1:  Product.if_plus = !Product.if_plus; break;
		case 2:  Product.size_scale = (Product.size_scale == 0) ? 3 : Product.size_scale - 1; break;
		case 3:  Product.size1 = (Product.size1 == 0) ? 999 : Product.size1 - 1; break;
		case 4:  Product.size2 = (Product.size2 == 0) ? 999 : Product.size2 - 1; break;
		case 5:  Product.price1_front = (Product.price1_front == 0) ? 999 : Product.price1_front - 1; break;
		case 6:  Product.price1_back = (Product.price1_back == 0) ? 99 : Product.price1_back - 1; break;
		case 7:  Product.price2_front = (Product.price2_front == 0) ? 999 : Product.price2_front - 1; break;
		case 8:  Product.price2_back = (Product.price2_back == 0) ? 99 : Product.price2_back - 1; break;
		case 9:  
			Product.pd_year = (Product.pd_year <= 2000) ? 2050 : Product.pd_year - 1; // 顺手加个2000年的底限
			// 如果当年当月的最大天数变小了，把溢出的天数拉回来
			if (Product.pd_day > Get_Max_Days(Product.pd_year, Product.pd_month)) {
				Product.pd_day = Get_Max_Days(Product.pd_year, Product.pd_month);
			}
			break;
		case 10: 
			Product.pd_month = (Product.pd_month <= 1) ? 12 : Product.pd_month - 1; 
			// 同理，月份改变也可能导致天数溢出
			if (Product.pd_day > Get_Max_Days(Product.pd_year, Product.pd_month)) {
				Product.pd_day = Get_Max_Days(Product.pd_year, Product.pd_month);
			}
			break;
		case 11: 
			{
				uint8_t max_days = Get_Max_Days(Product.pd_year, Product.pd_month);
				// 减到1或者意外大于当月最大值时，循环回最大天数
				Product.pd_day = (Product.pd_day <= 1 || Product.pd_day > max_days) ? max_days : Product.pd_day - 1; 
			}
			break;
		case 12: Product.duration = (Product.duration == 0) ? 99 : Product.duration - 1; break;
		case 13: Product.duration_scale = (Product.duration_scale == 0) ? 3 : Product.duration_scale - 1; break;
	}
}

void List_DataCTLup(void)
{
	uint8_t target_id = g_CursorPos;
	if (!Product.if_plus && g_CursorPos >= 4) { target_id += 1; }

	switch (target_id)
	{
		case 0:  Product.name = (Product.name >= 3) ? 0 : Product.name + 1; break;
		case 1:  Product.if_plus = !Product.if_plus; break;
		case 2:  Product.size_scale = (Product.size_scale == 3) ? 0 : Product.size_scale + 1; break;
		case 3:  Product.size1 = (Product.size1 == 999) ? 0 : Product.size1 + 1; break;
		case 4:  Product.size2 = (Product.size2 == 999) ? 0 : Product.size2 + 1; break;
		case 5:  Product.price1_front = (Product.price1_front == 999) ? 0 : Product.price1_front + 1; break;
		case 6:  Product.price1_back = (Product.price1_back == 99) ? 0 : Product.price1_back + 1; break;
		case 7:  Product.price2_front = (Product.price2_front == 999) ? 0 : Product.price2_front + 1; break;
		case 8:  Product.price2_back = (Product.price2_back == 99) ? 0 : Product.price2_back + 1; break;
		case 9:  
			Product.pd_year = (Product.pd_year >= 2050) ? 2000 : Product.pd_year + 1; 
			if (Product.pd_day > Get_Max_Days(Product.pd_year, Product.pd_month)) {
				Product.pd_day = Get_Max_Days(Product.pd_year, Product.pd_month);
			}
			break;
		case 10: 
			Product.pd_month = (Product.pd_month >= 12) ? 1 : Product.pd_month + 1; 
			if (Product.pd_day > Get_Max_Days(Product.pd_year, Product.pd_month)) {
				Product.pd_day = Get_Max_Days(Product.pd_year, Product.pd_month);
			}
			break;
		case 11: 
			{
				uint8_t max_days = Get_Max_Days(Product.pd_year, Product.pd_month);
				// 加到最大值时，循环回1
				Product.pd_day = (Product.pd_day >= max_days) ? 1 : Product.pd_day + 1; 
			}
			break;
		case 12: Product.duration = (Product.duration == 99) ? 0 : Product.duration + 1; break;
		case 13: Product.duration_scale = (Product.duration_scale == 3) ? 0 : Product.duration_scale + 1; break;
	}
}

void Menu_Main_Show(uint8_t cursor)
{
	OLED_Clear();
	OLED_ShowString(0, 0,"墨水屏标签系统",OLED_8X16);
	OLED_ShowString(0,16,"按任意键继续",OLED_8X16);
}

uint8_t Menu_MainContrl(uint8_t key, uint8_t cursor)
{
	g_PageIndex = 1;
	OLED_Updateflag = 1;
	return 0;
}

void Menu_List_Show(uint8_t cursor)
{
	OLED_Clear();
	OLED_ShowString(0,0,"Press 1: Setting",OLED_8X16);
	OLED_ShowString(0,16,"Press 2: Updata",OLED_8X16);
}

uint8_t Menu_ListContrl(uint8_t key, uint8_t cursor)
{
	switch (key)
	{
		case 1:g_PageIndex = 2;OLED_Updateflag = 1;break;
		case 2:g_PageIndex = 3;OLED_Updateflag = 1;break;
	}
	return 0;
}

void Menu_List_Setting_Show(uint8_t cursor)
{
	OLED_Clear();
	if (g_CursorPos <= 6 - (!Product.if_plus)) // 第一页
	{
		OLED_ShowString(0,0,"品名:",OLED_8X16);
		switch (Product.name)
		{
			case 0:OLED_ShowString(48,0,"乐事薯片",OLED_8X16);break;
			case 1:OLED_ShowString(48,0,"可口可乐",OLED_8X16);break;
			case 2:OLED_ShowString(48,0,"百事可乐",OLED_8X16);break;
			case 3:OLED_ShowString(48,0,"超级大辣皮",OLED_8X16);break;
		}
		
		OLED_ShowString(0,16,"加量:",OLED_8X16);
		if (Product.if_plus){OLED_ShowString(48,16,"T",OLED_8X16);}
		else if (!Product.if_plus){OLED_ShowString(48,16,"F",OLED_8X16);}

		OLED_ShowString(64,16,"单位:",OLED_8X16);
		switch (Product.size_scale)
		{
			case 0:OLED_ShowString(112,16,"kg",OLED_8X16);break;
			case 1:OLED_ShowString(112,16," g",OLED_8X16);break;
			case 2:OLED_ShowString(112,16," L",OLED_8X16);break;
			case 3:OLED_ShowString(112,16,"mL",OLED_8X16);break;
		}
		
		
		OLED_ShowString(0,32,"规格:",OLED_8X16);
		OLED_ShowNum(48,32,Product.size1,3,OLED_8X16);
		if (Product.if_plus)
		{
			OLED_ShowString(72,32,"+",OLED_8X16);
			OLED_ShowNum(80,32,Product.size2,3,OLED_8X16);
		}

		OLED_ShowString(0,48,"原价:",OLED_8X16);
		OLED_ShowNum(48,48,Product.price1_front,3,OLED_8X16);
		OLED_ShowString(72,48,".",OLED_8X16);
		OLED_ShowNum(80,48,Product.price1_back,2,OLED_8X16);
		OLED_ShowString(96,48,"元",OLED_8X16);
	}
	
	else if (g_CursorPos >(6 - (!Product.if_plus)) && g_CursorPos <= (13 - (!Product.if_plus))) // 第二页
	{
		OLED_ShowString(0,0,"会员价:",OLED_8X16);
		OLED_ShowNum(64,0,Product.price2_front,3,OLED_8X16);
		OLED_ShowString(88,0,".",OLED_8X16);
		OLED_ShowNum(96,0,Product.price2_back,2,OLED_8X16);
		OLED_ShowString(112,0,"元",OLED_8X16);
		
		OLED_ShowString(0,16,"生产日期:",OLED_8X16);
		OLED_ShowNum(0,32,Product.pd_year,4,OLED_8X16);
		OLED_ShowString(32,32,"年",OLED_8X16);
		
		OLED_ShowNum(48,32,Product.pd_month,2,OLED_8X16);
		OLED_ShowString(64,32,"月",OLED_8X16);
		
		OLED_ShowNum(80,32,Product.pd_day,2,OLED_8X16);
		OLED_ShowString(96,32,"日",OLED_8X16);
		
		OLED_ShowString(0,48,"保质期:",OLED_8X16);
		OLED_ShowNum(64,48,Product.duration,2,OLED_8X16);
		switch (Product.duration_scale)
		{
			case 0:OLED_ShowString(80,48,"年",OLED_8X16);break;
			case 1:OLED_ShowString(80,48,"月",OLED_8X16);break;
			case 2:OLED_ShowString(80,48,"日",OLED_8X16);break;
			case 3:OLED_ShowString(80,48,"时",OLED_8X16);break;
		}
	}

	Cursor_ResShow();
}

uint8_t Menu_List_Setting_Contrl(uint8_t key, uint8_t cursor)
{
	// 动态计算当前状态下最大光标索引 (有加量是 13，无加量是 12)
	uint8_t max_cursor_index = Product.if_plus ? 13 : 12; 

	switch (key)
	{
		case 2: // 上移
			if(g_CursorPos == 0) { g_CursorPos = max_cursor_index; }
			else { g_CursorPos -= 1; }
			OLED_Updateflag = 1; break;
			
		case 10: // 下移
			if (g_CursorPos >= max_cursor_index) { g_CursorPos = 0; }
			else { g_CursorPos += 1; }
			OLED_Updateflag = 1; break;
			
		case 5: List_DataCTLdown(); OLED_Updateflag = 1; break; // 左
		case 7: List_DataCTLup();   OLED_Updateflag = 1; break; // 右
		case 4: Flash_SaveProduct(&Product, &EEPROMEmulationState);g_PageIndex = 1;g_CursorPos=0;OLED_Updateflag = 1;break;
	}
	return 0;
}

void Menu_List_Updata_Show(uint8_t cursor)
{
	OLED_Clear();
	OLED_ShowString(0,0,"Press 1: Updata",OLED_8X16);
	OLED_ShowString(0,16,"Press 2: BLE",OLED_8X16);
	OLED_ShowString(0,32,"Press 4: Exit",OLED_8X16);
}

uint8_t Menu_List_Upadata_Contrl(uint8_t key, uint8_t cursor)
{
	switch (key)
	{
		case 1: // 更新到墨水屏
			EPD_Init();
			EPD_Display_Clear(); // 清除屏幕电荷（物理清屏）

			// 在绘制之前，要先将两个画布全部清空
			Paint_NewImage(BlackImage, 152, 152, 0, WHITE);
			Paint_SelectImage(BlackImage);// 初始化黑白层画布
			Paint_Clear(WHITE); // 背景涂白

			Paint_NewImage(RedImage, 152, 152, 0, WHITE);
			Paint_SelectImage(RedImage);// 初始化红色层画布
			Paint_Clear(WHITE); // 背景涂白

			// --- 绘制黑色部分 ---
			Paint_SelectImage(BlackImage);
		
			EPD_ShowChinese(0, 16, "品名", 12, BLACK);
			EPD_ShowString(24, 20, ":", 8, BLACK);
			switch (Product.name)
			{
				case 0:EPD_ShowChinese(32, 4, "乐事薯片", 24, BLACK);break;
				case 1:EPD_ShowChinese(32, 4, "可口可乐", 24, BLACK);break;
				case 2:EPD_ShowChinese(32, 4, "百事可乐", 24, BLACK);break;
				case 3:EPD_ShowChinese(32, 4, "超级大辣皮", 24, BLACK);break;
			}
			
			EPD_ShowChinese(0, 32, "规格", 12, BLACK);
			EPD_ShowString(24, 36, ":", 8, BLACK);
			switch (Product.size_scale)
			{
				case 0:		
					if (Product.if_plus){EPD_ShowString(32, 32, "   kg+    kg", 12, BLACK);}
					else {EPD_ShowString(32,32, "   kg", 12, BLACK);}break;
				case 1:
					if (Product.if_plus){EPD_ShowString(32, 32, "   g +    g", 12, BLACK);}
					else {EPD_ShowString(32,32, "   g", 12, BLACK);}break;
				case 2:
					if (Product.if_plus){EPD_ShowString(32, 32, "   L +    L", 12, BLACK);}
					else {EPD_ShowString(32,32, "   L", 12, BLACK);}break;
				case 3:
					if (Product.if_plus){EPD_ShowString(32, 32, "   mL+    mL", 12, BLACK);}
					else {EPD_ShowString(32,32, "   mL", 12, BLACK);}break;
			}

			EPD_ShowNum(32,32,Product.size1,3,12,BLACK);
			if (Product.if_plus){EPD_ShowNum(74,32,Product.size2,3,12,BLACK);}
			
			EPD_ShowChinese(0, 48, "原价", 12, BLACK);
			EPD_ShowString(24, 52, ":", 8, BLACK);
			EPD_ShowNum(32,48,Product.price1_front,3,12,BLACK);
			EPD_ShowString(50, 48, ".", 12, BLACK);
			EPD_ShowNum(56,48,Product.price1_back,2,12,BLACK);
			EPD_ShowChinese(68, 48, "元", 12, BLACK);
			
			EPD_ShowChinese(0, 108, "生产日期", 12, BLACK);
			
			EPD_ShowString(48, 112, ":", 8, BLACK);
			EPD_ShowString(0, 124, "    /  /  ", 12, BLACK);
			EPD_ShowNum(0,124,Product.pd_year,4,12,BLACK);
			EPD_ShowNum(30,124,Product.pd_month,2,12,BLACK);
			EPD_ShowNum(48,124,Product.pd_day,2,12,BLACK);
			
			EPD_ShowChinese(0, 140, "保质期", 12, BLACK);
			EPD_ShowString(36, 144, ":", 8, BLACK);
			EPD_ShowNum(44,140,Product.duration,2,12,BLACK);
			switch (Product.duration_scale)
			{
				case 0:EPD_ShowChinese(56, 140, "年", 12, BLACK);break;
				case 1:EPD_ShowChinese(56, 140, "月", 12, BLACK);break;
				case 2:EPD_ShowChinese(56, 140, "日", 12, BLACK);break;
				case 3:EPD_ShowChinese(56, 140, "时", 12, BLACK);break;
			}
			
			EPD_ShowQRCode(112,112,"https://baidu.com",BLACK);


			// --- 绘制红色部分 ---
			Paint_SelectImage(RedImage); // 注意：在 RedImage 上画 BLACK，最终在屏幕上会显示为红色

			EPD_DrawLine(28, 52, 67, 52, BLACK);
			EPD_ShowNum(12,60,Product.price2_front,3,48,BLACK);
			EPD_ShowString(84, 60, ".", 48, BLACK);
			EPD_ShowNum(100,60,Product.price2_back,2,48,BLACK);
			if (Product.if_plus){EPD_DrawRectangle(112, 20, 144, 36, BLACK, 1);EPD_ShowChinese(112, 20, "促销", 16, WHITE);}
			EPD_ShowString(0, 72, ">", 24, BLACK);

			EPD_Display(BlackImage, RedImage);
			EPD_Update();// 触发屏幕物理刷新
			EPD_DeepSleep();// 刷新完成后休眠
			
			OLED_Updateflag = 1; break;
			
		case 2: // 蓝牙
			g_PageIndex = 1;OLED_Updateflag = 1; break;  
		case 4: // 退出
			g_PageIndex = 1;OLED_Updateflag = 1; break;  
	}

	return 0;
}

Menu_Page table[] = 
{
    // 显示函数 项数 逻辑函数
    {Menu_Main_Show,        		0,    Menu_MainContrl}, 			// 索引0: 主界面模式
    {Menu_List_Show,        		2,    Menu_ListContrl}, 			// 索引1: 列表模式
	{Menu_List_Setting_Show,        3,    Menu_List_Setting_Contrl},	// 索引2: 列表模式 设置
	{Menu_List_Updata_Show,        	3,    Menu_List_Upadata_Contrl}		// 索引3: 列表模式 更新
};
