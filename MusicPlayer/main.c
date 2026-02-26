/*------------------------------------------------------------------------------
 * 工程名： evm_test
 * 作者： 孔
 * 创建时间： 2025.12.26.18.25
 * 版本： V1.0
 * 描述： 该工程基于TI-M0SDK-2_08_00_03生成，推荐sysconfig-1.25.0配套使用；目前配置
 时钟树，使用外部40M晶振作为时钟源，时钟分支皆以最高频率运行，其主要为EVM测试代码。
 *----------------------------------------------------------------------------*/
#include "ti_msp_dl_config.h"
#include "bsp.h"
#include "MusicPlayer.h"
#include "MusicScore.h"

int main(void)
{
    SYSCFG_DL_init();
	MusicPlayer_init();
	//playMusic(BirthdayScore, sizeof(BirthdayScore)/sizeof(BirthdayScore[0]));
	playMusic(Sakura, sizeof(Sakura)/sizeof(Sakura[0]));
	
    while (1) 
	{
		
    }
}
