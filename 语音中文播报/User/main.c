#include "stm32f10x.h"
#include "bsp_i2c.h"
#include "bsp_usart1.h"

int main(void)
{
	u8 result = 0xff;
	//I2C��ʼ��
	I2C_Bus_Init();
	NVIC_Configuration();
	USARTx_Config();

	SetVolume(160);
	
	
	
  while(1){
		
	SetReader(Reader_XiaoYan);
	speech_text("�����������ͷ�³�ʾ�����벢��������",GB2312);
	while(GetChipStatus() != ChipStatus_Idle)
	{
	  delay(10000);
	}
	
	SetReader(Reader_XiaoYan);
	speech_text("�����������ͷ�³�ʾ�����벢��������",GB2312);
	while(GetChipStatus() != ChipStatus_Idle)
	{
	  delay(10000);
	}
	

	SetArticulation(Articulation_Word);//����Ӣ�ĵ��ʷ���
	SetReader(Reader_XiaoPing);
	speech_text("hello, please show your health code under the camera and take your temperature",GB2312);
	while(GetChipStatus() != ChipStatus_Idle)
	{
	  delay(100000);
	}
	}
		
}

/*********************************************END OF FILE**********************/
