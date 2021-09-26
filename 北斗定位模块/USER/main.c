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
 ALIENTEK NANO STM32������ ��չʵ��X
 ATK-S1216F8 GPSģ�����ʵ��-�Ĵ����汾
 ����֧�֣�www.openedv.com
 �Ա����̣�http://eboard.taobao.com 
 ��ע΢�Ź���ƽ̨΢�źţ�"����ԭ��"����ѻ�ȡSTM32���ϡ�
 ������������ӿƼ����޹�˾  
 ���ߣ�����ԭ�� @ALIENTEK
************************************************/

u8 USART1_TX_BUF[USART2_MAX_RECV_LEN]; 					//����1,���ͻ�����
nmea_msg gpsx; 											//GPS��Ϣ
__align(4) u8 dtbuf[50];   								//��ӡ������
const u8*fixmode_tbl[4]={"Fail","Fail"," 2D "," 3D "};	//fix mode�ַ��� 

//��ʾGPS��λ��Ϣ 
void Gps_Msg_Show(void)
{
	float tp;		   
	tp=gpsx.longitude;	   
	sprintf((char *)dtbuf,"Longitude:%.5f %1c   ",tp/=100000,gpsx.ewhemi);	//�õ������ַ���	 	
    printf("%s\r\n",dtbuf);
	tp=gpsx.latitude;	   
	sprintf((char *)dtbuf,"Latitude:%.5f %1c   ",tp/=100000,gpsx.nshemi);	//�õ�γ���ַ���	
    printf("%s\r\n",dtbuf);
	tp=gpsx.altitude;	   
	sprintf((char *)dtbuf,"Altitude:%.1fm     ",tp/=10);	    			//�õ��߶��ַ���	 		
    printf("%s\r\n",dtbuf);
	tp=gpsx.speed;	   
	sprintf((char *)dtbuf,"Speed:%.3fkm/h     ",tp/=1000);		    		//�õ��ٶ��ַ���	  		
    printf("%s\r\n",dtbuf);
	if(gpsx.fixmode<=3)														//��λ״̬
	{  
		sprintf((char *)dtbuf,"Fix Mode:%s",fixmode_tbl[gpsx.fixmode]);	
        printf("%s\r\n",dtbuf);		
	}	 	   
	sprintf((char *)dtbuf,"GPS+BD Valid satellite:%02d",gpsx.posslnum);	 		//���ڶ�λ��GPS������	  
    printf("%s\r\n",dtbuf);
	sprintf((char *)dtbuf,"GPS Visible satellite:%02d",gpsx.svnum%100);	 		//�ɼ�GPS������
	printf("%s\r\n",dtbuf);
	sprintf((char *)dtbuf,"BD Visible satellite:%02d",gpsx.beidou_svnum%100);	 		//�ɼ�����������
	printf("%s\r\n",dtbuf);
	sprintf((char *)dtbuf,"UTC Date:%04d/%02d/%02d   ",gpsx.utc.year,gpsx.utc.month,gpsx.utc.date);	//��ʾUTC����	
    printf("%s\r\n",dtbuf);	
	sprintf((char *)dtbuf,"UTC Time:%02d:%02d:%02d   ",gpsx.utc.hour,gpsx.utc.min,gpsx.utc.sec);	//��ʾUTCʱ��
    printf("%s\r\n",dtbuf);	
}


int main(void)
{	

	u16 i,rxlen;
	u16 lenx;
	u8 key=0XFF;
	u8 upload=0; 
	u8 stop=0;
	 
	Stm32_Clock_Init(9); //ϵͳʱ������
	delay_init(72);	     //��ʱ��ʼ��
	uart_init(72,115200);//����1��ʼ�� 
	USART2_Init(36,38400);//����2��ʼ��
	LED_Init();		  	 //��ʼ����LED���ӵ�Ӳ���ӿ�
	KEY_Init();			 //��ʼ������
	usmart_dev.init(72); //��ʼ��USMART 
   	printf("ALIENTEK NANO STM32\r\n");
    printf("SkyTraF8-BD TEST\r\n");
	if(SkyTra_Cfg_Rate(5)!=0)	//���ö�λ��Ϣ�����ٶ�Ϊ5Hz,˳���ж�GPSģ���Ƿ���λ. 
	{
		
		printf("SkyTraF8-BD ������...��\r\n");
	    do
		{
			USART2_Init(36,9600);			  //��ʼ������3������Ϊ9600	
	  	    SkyTra_Cfg_Prt(3);			    //��������ģ��Ĳ�����Ϊ38400
			USART2_Init(36,38400);			//��ʼ������3������Ϊ38400
            key=SkyTra_Cfg_Tp(100000);	//������Ϊ100ms
		}while(SkyTra_Cfg_Rate(5)!=0&&key!=0);//����SkyTraF8-BD�ĸ�������Ϊ5Hz
		printf("SkyTraF8-BD �������\r\n");
		
	}
   	while(1)
	{
		 delay_ms(1);
		 if(USART2_RX_STA&0X8000)		//���յ�һ��������
		 {
			 
		    rxlen=USART2_RX_STA&0X7FFF;	//�õ����ݳ���
			for(i=0;i<rxlen;i++)USART1_TX_BUF[i]=USART2_RX_BUF[i];	   
 			USART2_RX_STA=0;		   	//������һ�ν���
			USART1_TX_BUF[i]=0;			//�Զ���ӽ�����
	
			 if(!stop)
			 {
				 if(upload) printf("\r\n%s\r\n",USART1_TX_BUF);//���ͽ��յ������ݵ�����1 
				 else
				 {
					GPS_Analysis(&gpsx,(u8*)USART1_TX_BUF);//�����ַ���
					Gps_Msg_Show();			//��ʾ��Ϣ	
					printf("\r\n");	
				 }
			 }
		 }
		 key=KEY_Scan(0);
		 if(key==KEY1_PRES)//����GPS�����Ƿ��ϴ���λ��
		 {
			 upload=!upload;
		 }else if(key==WKUP_PRES)//����USMART����
		 {
			 stop=!stop;
			 printf("USMART ON!\r\n");
		 }
		 if((lenx%500)==0)
			LED0=!LED0;
		lenx++;	
		
	}
}
