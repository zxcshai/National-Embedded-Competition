#include "led.h"
#include "delay.h"
#include "key.h"
#include "sys.h"
#include "usart.h"	 
#include "mlx90614.h" 
#include "oled.h"
#include "buzzer.h"

float setNum = 37.3;
unsigned char q = 0;
//�¶ȵĵ�λ ��
unsigned char TempCompany[][16]=
{
	0x06,0x09,0x09,0xE6,0xF8,0x0C,0x04,0x02,0x02,0x02,0x02,0x02,0x04,0x1E,0x00,0x00,
	0x00,0x00,0x00,0x07,0x1F,0x30,0x20,0x40,0x40,0x40,0x40,0x40,0x20,0x10,0x00,0x00,/*"��",0*/	

};

int main(void)
{	
	uint8_t i,j;	
	float Temperature = 0;    //�¶����ݱ����������ͣ�
	char TempValue[80] = {0}; //�¶�ֵ���ַ�����
	delay_init();	  //��ʱ������ʼ��	  
	uart_init(115200);//���ڳ�ʼ��Ϊ115200
	OLED_Init()	;
	Buzzer_GPIO_Init();
	GPIO_SetBits(GPIOB,  GPIO_Pin_5);
	OLED_Clear() ;
 	LED_Init();		//LED�˿ڳ�ʼ��
  SMBus_Init();
  display();
	
	

	while(1)
	{		
		Temperature = SMBus_ReadTemp();
		printf("Temperature = %f\r\n",Temperature);	//�ڴ��ڵ��������ϴ�ӡ���
		delay_ms(100);//������ʱ�Ļ��ڵ��Դ��������Ͽ�����Ч��
		sprintf(TempValue,"%.1f", Temperature); //������ת�����ַ���		
		//�¶ȵ�λ��ʾ ���棩
		for(i = 6;i < 7;i++)
		{
			j = i - 6;
			OLED_ShowCHinese16x16(i * 16,2,j,TempCompany);			
		}
		OLED_ShowString(46,2,(uint8_t *)TempValue,16);//����OLED����ʾʵ�ʲ������¶�
		if(Temperature >= setNum)
		{
			for(q = 0;q < 2;q++){
				Buzzer0();
				delay_ms(1000);
				Buzzer1();
				delay_ms(1000);
				Buzzer0();
				delay_ms(1000);
			}
		}else{
				Buzzer1();
		}
			
	}
}


