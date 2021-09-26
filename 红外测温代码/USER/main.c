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
//温度的单位 ℃
unsigned char TempCompany[][16]=
{
	0x06,0x09,0x09,0xE6,0xF8,0x0C,0x04,0x02,0x02,0x02,0x02,0x02,0x04,0x1E,0x00,0x00,
	0x00,0x00,0x00,0x07,0x1F,0x30,0x20,0x40,0x40,0x40,0x40,0x40,0x20,0x10,0x00,0x00,/*"℃",0*/	

};

int main(void)
{	
	uint8_t i,j;	
	float Temperature = 0;    //温度数据变量（浮点型）
	char TempValue[80] = {0}; //温度值（字符串）
	delay_init();	  //延时函数初始化	  
	uart_init(115200);//串口初始化为115200
	OLED_Init()	;
	Buzzer_GPIO_Init();
	GPIO_SetBits(GPIOB,  GPIO_Pin_5);
	OLED_Clear() ;
 	LED_Init();		//LED端口初始化
  SMBus_Init();
  display();
	
	

	while(1)
	{		
		Temperature = SMBus_ReadTemp();
		printf("Temperature = %f\r\n",Temperature);	//在串口调试助手上打印结果
		delay_ms(100);//不加延时的话在电脑串口助手上看不到效果
		sprintf(TempValue,"%.1f", Temperature); //浮点型转换成字符串		
		//温度单位显示 （℃）
		for(i = 6;i < 7;i++)
		{
			j = i - 6;
			OLED_ShowCHinese16x16(i * 16,2,j,TempCompany);			
		}
		OLED_ShowString(46,2,(uint8_t *)TempValue,16);//在在OLED上显示实际测量的温度
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


