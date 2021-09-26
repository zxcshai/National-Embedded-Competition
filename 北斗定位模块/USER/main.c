#include "led.h"
#include "delay.h"
#include "sys.h"
#include "usart2.h"
#include "string.h"	 
#include "key.h"
#include "usart.h"	
#include "usmart.h"
#include "gps.h"

/************************************************
 ALIENTEK NANO STM32开发板 扩展实验X
 ATK-S1216F8 GPS模块测试实验-寄存器版本
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com 
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/

u8 USART1_TX_BUF[USART2_MAX_RECV_LEN]; 					//串口1,发送缓存区
nmea_msg gpsx; 											//GPS信息
__align(4) u8 dtbuf[50];   								//打印缓存器
const u8*fixmode_tbl[4]={"Fail","Fail"," 2D "," 3D "};	//fix mode字符串 

//显示GPS定位信息 
void Gps_Msg_Show(void)
{
	float tp;		   
	tp=gpsx.longitude;	   
	sprintf((char *)dtbuf,"Longitude:%.5f %1c   ",tp/=100000,gpsx.ewhemi);	//得到经度字符串	 	
    printf("%s\r\n",dtbuf);
	tp=gpsx.latitude;	   
	sprintf((char *)dtbuf,"Latitude:%.5f %1c   ",tp/=100000,gpsx.nshemi);	//得到纬度字符串	
    printf("%s\r\n",dtbuf);
	tp=gpsx.altitude;	   
	sprintf((char *)dtbuf,"Altitude:%.1fm     ",tp/=10);	    			//得到高度字符串	 		
    printf("%s\r\n",dtbuf);
	tp=gpsx.speed;	   
	sprintf((char *)dtbuf,"Speed:%.3fkm/h     ",tp/=1000);		    		//得到速度字符串	  		
    printf("%s\r\n",dtbuf);
	if(gpsx.fixmode<=3)														//定位状态
	{  
		sprintf((char *)dtbuf,"Fix Mode:%s",fixmode_tbl[gpsx.fixmode]);	
        printf("%s\r\n",dtbuf);		
	}	 	   
	sprintf((char *)dtbuf,"GPS+BD Valid satellite:%02d",gpsx.posslnum);	 		//用于定位的GPS卫星数	  
    printf("%s\r\n",dtbuf);
	sprintf((char *)dtbuf,"GPS Visible satellite:%02d",gpsx.svnum%100);	 		//可见GPS卫星数
	printf("%s\r\n",dtbuf);
	sprintf((char *)dtbuf,"BD Visible satellite:%02d",gpsx.beidou_svnum%100);	 		//可见北斗卫星数
	printf("%s\r\n",dtbuf);
	sprintf((char *)dtbuf,"UTC Date:%04d/%02d/%02d   ",gpsx.utc.year,gpsx.utc.month,gpsx.utc.date);	//显示UTC日期	
    printf("%s\r\n",dtbuf);	
	sprintf((char *)dtbuf,"UTC Time:%02d:%02d:%02d   ",gpsx.utc.hour,gpsx.utc.min,gpsx.utc.sec);	//显示UTC时间
    printf("%s\r\n",dtbuf);	
}


int main(void)
{	

	u16 i,rxlen;
	u16 lenx;
	u8 key=0XFF;
	u8 upload=0; 
	u8 stop=0;
	 
	Stm32_Clock_Init(9); //系统时钟设置
	delay_init(72);	     //延时初始化
	uart_init(72,115200);//串口1初始化 
	USART2_Init(36,38400);//串口2初始化
	LED_Init();		  	 //初始化与LED连接的硬件接口
	KEY_Init();			 //初始化按键
	usmart_dev.init(72); //初始化USMART 
   	printf("ALIENTEK NANO STM32\r\n");
    printf("SkyTraF8-BD TEST\r\n");
	if(SkyTra_Cfg_Rate(5)!=0)	//设置定位信息更新速度为5Hz,顺便判断GPS模块是否在位. 
	{
		
		printf("SkyTraF8-BD 配置中...、\r\n");
	    do
		{
			USART2_Init(36,9600);			  //初始化串口3波特率为9600	
	  	    SkyTra_Cfg_Prt(3);			    //重新设置模块的波特率为38400
			USART2_Init(36,38400);			//初始化串口3波特率为38400
            key=SkyTra_Cfg_Tp(100000);	//脉冲宽度为100ms
		}while(SkyTra_Cfg_Rate(5)!=0&&key!=0);//配置SkyTraF8-BD的更新速率为5Hz
		printf("SkyTraF8-BD 设置完成\r\n");
		
	}
   	while(1)
	{
		 delay_ms(1);
		 if(USART2_RX_STA&0X8000)		//接收到一次数据了
		 {
			 
		    rxlen=USART2_RX_STA&0X7FFF;	//得到数据长度
			for(i=0;i<rxlen;i++)USART1_TX_BUF[i]=USART2_RX_BUF[i];	   
 			USART2_RX_STA=0;		   	//启动下一次接收
			USART1_TX_BUF[i]=0;			//自动添加结束符
	
			 if(!stop)
			 {
				 if(upload) printf("\r\n%s\r\n",USART1_TX_BUF);//发送接收到的数据到串口1 
				 else
				 {
					GPS_Analysis(&gpsx,(u8*)USART1_TX_BUF);//分析字符串
					Gps_Msg_Show();			//显示信息	
					printf("\r\n");	
				 }
			 }
		 }
		 key=KEY_Scan(0);
		 if(key==KEY1_PRES)//控制GPS数据是否上传上位机
		 {
			 upload=!upload;
		 }else if(key==WKUP_PRES)//控制USMART调试
		 {
			 stop=!stop;
			 printf("USMART ON!\r\n");
		 }
		 if((lenx%500)==0)
			LED0=!LED0;
		lenx++;	
		
	}
}
