#include "stm32f10x.h"
#include "bsp_i2c.h"
#include "bsp_usart1.h"

int main(void)
{
	u8 result = 0xff;
	//I2C初始化
	I2C_Bus_Init();
	NVIC_Configuration();
	USARTx_Config();

	SetVolume(160);
	
	
	
  while(1){
		
	SetReader(Reader_XiaoYan);
	speech_text("你好请在摄像头下出示健康码并测量体温",GB2312);
	while(GetChipStatus() != ChipStatus_Idle)
	{
	  delay(10000);
	}
	
	SetReader(Reader_XiaoYan);
	speech_text("你好请在摄像头下出示健康码并测量体温",GB2312);
	while(GetChipStatus() != ChipStatus_Idle)
	{
	  delay(10000);
	}
	

	SetArticulation(Articulation_Word);//设置英文单词发音
	SetReader(Reader_XiaoPing);
	speech_text("hello, please show your health code under the camera and take your temperature",GB2312);
	while(GetChipStatus() != ChipStatus_Idle)
	{
	  delay(100000);
	}
	}
		
}

/*********************************************END OF FILE**********************/
