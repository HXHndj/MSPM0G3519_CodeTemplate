#include "ti_msp_dl_config.h"
#include "bsp.h"
#include "KeyBroad.h"
#include "OLED.h"
#include "delay.h"
#include <string.h>

/* ================= 游戏尺寸宏定义 ================= */
#define MAP_SIZE 12   // 地图的尺寸 (12x12)
#define VIEW_ROWS 8   // 屏幕纵向最多显示的行数 (64/8 = 8)

/* ================= 游戏图形数据 (8x8 像素) ================= */
const uint8_t IMG_Empty[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const uint8_t IMG_Wall[8]  = {0xFF,0xC3,0xA5,0x99,0x99,0xA5,0xC3,0xFF}; 
const uint8_t IMG_Target[8]= {0x00,0x00,0x18,0x3C,0x3C,0x18,0x00,0x00};
const uint8_t IMG_Box[8]   = {0xFF,0x81,0x99,0xA5,0xA5,0x99,0x81,0xFF};
const uint8_t IMG_Player[8]= {0x00,0x24,0x18,0xFF,0xFF,0x18,0x24,0x00};
const uint8_t IMG_BoxOnT[8]= {0xFF,0xFF,0xDB,0xC3,0xC3,0xDB,0xFF,0xFF};

/* ================= 12x12 游戏地图数据 (共7关) ================= */
// 状态代号: 0空地, 1墙, 2目标, 3箱子, 4玩家, 5箱子在目标, 6玩家在目标
const uint8_t Level_Maps[7][MAP_SIZE][MAP_SIZE] = {
    // 关卡 1：经典入门 (3箱子)
    {
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,1,1,1,0,0,0,0,0},
        {0,0,0,0,1,2,1,0,0,0,0,0},
        {0,0,0,0,1,0,1,1,1,1,0,0},
        {0,0,1,1,1,3,0,3,2,1,0,0},
        {0,0,1,2,0,3,4,1,1,1,0,0},
        {0,0,1,1,1,1,3,1,0,0,0,0},
        {0,0,0,0,0,1,2,1,0,0,0,0},
        {0,0,0,0,0,1,1,1,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0}
    },
    // 关卡 2：初级迷宫 (2箱子)
    {
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,1,1,1,1,1,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,1,1,0,1,0,0,0,0},
        {0,0,1,2,3,4,0,1,0,0,0,0},
        {0,0,1,2,1,3,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,1,1,1,1,1,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0}
    },
    // 关卡 3：著名关卡 "Sasquatch" (4箱子)
    {
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,1,1,1,1,1,1,1,0,0,0},
        {0,0,1,0,0,0,0,0,1,0,0,0},
        {0,0,1,0,0,0,0,0,1,0,0,0},
        {0,0,1,2,0,1,0,0,1,0,0,0},
        {0,0,1,2,0,3,3,4,1,0,0,0},
        {0,0,1,2,3,3,0,0,1,0,0,0},
        {0,0,1,2,1,0,0,0,1,0,0,0},
        {0,0,1,1,1,1,1,1,1,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0}
    },
    // 关卡 4：十字牢笼 (4箱子)
    {
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,1,1,1,1,0,0,0,0},
        {0,0,0,0,1,0,0,1,0,0,0,0},
        {0,0,1,1,1,0,0,1,1,1,0,0},
        {0,0,1,2,2,3,3,0,0,1,0,0},
        {0,0,1,2,2,3,4,3,0,1,0,0},
        {0,0,1,1,1,0,0,1,1,1,0,0},
        {0,0,0,0,1,0,0,1,0,0,0,0},
        {0,0,0,0,1,1,1,1,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0}
    },
    // 关卡 5：拐角走廊 (3箱子)
    {
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,1,1,1,1,1,0,0,0,0,0},
        {0,0,1,0,0,0,1,0,0,0,0,0},
        {0,0,1,0,3,0,1,0,1,1,1,0},
        {0,0,1,0,3,0,1,0,1,2,1,0},
        {0,0,1,1,1,0,1,1,1,2,1,0},
        {0,0,0,1,1,0,0,0,0,2,1,0},
        {0,0,0,1,0,4,3,1,0,0,1,0},
        {0,0,0,1,0,0,0,1,1,1,1,0},
        {0,0,0,1,1,1,1,1,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0}
    },
    // 关卡 6：左右双室 (4箱子)
    {
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,1,1,1,1,1,1,1,0,0,0},
        {0,0,1,0,0,0,0,0,1,1,1,0},
        {0,1,1,3,1,1,1,0,0,0,1,0},
        {0,1,0,0,4,3,0,0,3,0,1,0},
        {0,1,0,2,2,1,0,3,0,1,1,0},
        {0,1,1,2,2,1,0,0,0,1,0,0},
        {0,0,1,1,1,1,1,1,1,1,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0}
    },
    // 关卡 7：阶梯走廊
    {
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,1,1,1,1,0,0,0,0},
        {0,0,0,1,1,0,0,1,0,0,0,0},
        {0,0,0,1,0,4,3,1,0,0,0,0},
        {0,0,0,1,1,3,0,1,1,0,0,0},
        {0,0,0,1,1,0,3,0,1,0,0,0},
        {0,0,0,1,2,3,0,0,1,0,0,0},
        {0,0,0,1,2,2,5,2,1,0,0,0},
        {0,0,0,1,1,1,1,1,1,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0}
    }
};

/* ================= 游戏状态与全局变量 ================= */
typedef enum {
    STATE_MENU,
    STATE_PLAYING
} GameState;

GameState current_state = STATE_MENU;
uint8_t current_level = 0;   
uint8_t current_map[MAP_SIZE][MAP_SIZE]; 
int8_t player_x = 0;
int8_t player_y = 0;
uint8_t KeyNum = 0;

/* ================= 游戏函数声明 ================= */
void Draw_BootAnimation(void);
void Draw_MainMenu(void);
void Game_LoadLevel(uint8_t level);
void Game_DrawMap(void);
void Game_Move(int8_t dx, int8_t dy);
uint8_t Game_CheckWin(void);

/* ================= MAIN 函数 ================= */
int main(void)
{
    SYSCFG_DL_init();
    OLED_Init();
    
    Draw_BootAnimation();
    Draw_MainMenu();
    current_state = STATE_MENU;

    while (1) 
    {
        KeyNum = GetKeyNum();
        
        if (KeyNum != 0) 
        {
            /* ---------- 状态1：在主菜单时 ---------- */
            if (current_state == STATE_MENU) 
            {
                // 支持按键 1~7 选择关卡
                if (KeyNum >= 1 && KeyNum <= 7) {
                    current_level = KeyNum - 1;
                    Game_LoadLevel(current_level);
                    current_state = STATE_PLAYING;
                    Game_DrawMap();
                }
            }
            /* ---------- 状态2：在游戏关卡内时 ---------- */
            else if (current_state == STATE_PLAYING) 
            {
                switch (KeyNum) {
                    case 2:  Game_Move(0, -1); break; // 上
                    case 10: Game_Move(0, 1);  break; // 下
                    case 5:  Game_Move(-1, 0); break; // 左
                    case 7:  Game_Move(1, 0);  break; // 右
                    case 1:  
                        Game_LoadLevel(current_level); // 按1重置本关
                        break; 
                    case 4:  
                        current_state = STATE_MENU;    // 按4放弃本关，返回菜单
                        Draw_MainMenu();
                        break; 
                    default: break;
                }
                
                if (current_state == STATE_PLAYING) {
                    Game_DrawMap();
                    
                    if (Game_CheckWin()) {
                        OLED_ShowString(24, 20, " YOU WIN! ", OLED_8X16);
                        OLED_ShowString(16, 40, "Press 4 Back", OLED_8X16);
                        OLED_Update();
                        
                        while(GetKeyNum() != 4);
                        current_state = STATE_MENU;
                        Draw_MainMenu();
                    }
                }
            }
        }
    }
}

/* ================== 界面与逻辑实现 ================== */

void Draw_BootAnimation(void)
{
    OLED_Clear();
    OLED_ShowImage(0, 0, 128, 64, Ark); 
    OLED_Update();
    delay_ms(200);
    
    OLED_Clear();
    OLED_ShowString(36, 24, "SOKOBAN", OLED_8X16); 
    OLED_ShowString(44, 45, "GAME", OLED_8X16);
    OLED_Update();
    delay_ms(200);
}

void Draw_MainMenu(void)
{
    OLED_Clear();
    OLED_ShowString(0, 0, "R:1", OLED_6X8);
    OLED_ShowString(107, 0, "W", OLED_6X8);
    OLED_ShowString(95, 8, "A S D", OLED_6X8);
    OLED_ShowString(12, 24, "Select Level:", OLED_8X16);
    OLED_ShowString(28, 44, "Press 1~7", OLED_8X16); // 提示改为 1~7
    OLED_Update();
}

//配置好地图和玩家的位置
void Game_LoadLevel(uint8_t level)
{
    memcpy(current_map, Level_Maps[level], sizeof(current_map));
    
    // 寻找玩家的初始位置
    for (int y = 0; y < MAP_SIZE; y++) {
        for (int x = 0; x < MAP_SIZE; x++) {
            if (current_map[y][x] == 4 || current_map[y][x] == 6) {
                player_x = x;
                player_y = y;
                return;
            }
        }
    }
}

void Game_DrawMap(void)
{
    OLED_Clear();
    
    // UI区保留在左侧前32个像素
    OLED_ShowString(0, 0, "Lv:", OLED_6X8);
    OLED_ShowNum(18, 0, current_level + 1, 1, OLED_6X8);
    OLED_ShowString(0, 16, "1:R", OLED_6X8);
    OLED_ShowString(0, 24, "4:Q", OLED_6X8);
    
    /* 1. 计算Y轴摄像机位置 (让玩家尽量位于屏幕中央) */
    int8_t cam_y = player_y - (VIEW_ROWS / 2); 
    
    if (cam_y < 0) {
        cam_y = 0; 
    }
    if (cam_y > MAP_SIZE - VIEW_ROWS) {
        cam_y = MAP_SIZE - VIEW_ROWS; 
    }

    /* 2. 渲染可视区域 */
    for (int row = 0; row < VIEW_ROWS; row++) {
        int map_y = cam_y + row;  
        
        for (int x = 0; x < MAP_SIZE; x++) {
            const uint8_t *img_ptr;
            switch(current_map[map_y][x]) {
                case 1: img_ptr = IMG_Wall;   break;
                case 2: img_ptr = IMG_Target; break;
                case 3: img_ptr = IMG_Box;    break;
                case 4: img_ptr = IMG_Player; break;
                case 5: img_ptr = IMG_BoxOnT; break;
                case 6: img_ptr = IMG_Player; break;
                default: img_ptr = IMG_Empty; break;
            }
            OLED_ShowImage(32 + (x * 8), row * 8, 8, 8, img_ptr);
        }
    }
    OLED_Update();
}

void Game_Move(int8_t dx, int8_t dy)
{
    int8_t tx = player_x + dx;
    int8_t ty = player_y + dy;
    int8_t bx = tx + dx;
    int8_t by = ty + dy;

    if (tx < 0 || tx >= MAP_SIZE || ty < 0 || ty >= MAP_SIZE) return;

    uint8_t target_val = current_map[ty][tx];

    if (target_val == 0 || target_val == 2) 
    {
        current_map[player_y][player_x] -= 4;
        player_x = tx;
        player_y = ty;
        current_map[player_y][player_x] += 4;
    }
    else if (target_val == 3 || target_val == 5) 
    {
        if (bx < 0 || bx >= MAP_SIZE || by < 0 || by >= MAP_SIZE) return;
        
        uint8_t box_next_val = current_map[by][bx];
        
        if (box_next_val == 0 || box_next_val == 2) 
        {
            current_map[ty][tx] -= 3;
            current_map[by][bx] += 3;
            
            current_map[player_y][player_x] -= 4;
            player_x = tx;
            player_y = ty;
            current_map[player_y][player_x] += 4;
        }
    }
}

uint8_t Game_CheckWin(void)
{
    for (int y = 0; y < MAP_SIZE; y++) {
        for (int x = 0; x < MAP_SIZE; x++) {
            if (current_map[y][x] == 3) {
                return 0; // 只要还有独立的3，就没有赢
            }
        }
    }
    return 1;
}