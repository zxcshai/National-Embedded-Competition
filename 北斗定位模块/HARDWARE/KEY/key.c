#include "key.h"
#include "delay.h"

//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK NANO STM32������
//������������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2018/3/27
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2018-2028
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	 
 	    
//������ʼ������ 
void KEY_Init(void)
{
	
	RCC->APB2ENR|=1<<2;     //ʹ��PORTAʱ��
	RCC->APB2ENR|=1<<4;     //ʹ��PORTCʱ��
	RCC->APB2ENR|=1<<5;     //ʹ��PORTDʱ��
	GPIOA->CRL&=0XFFFFFFF0;	//PA0���ó�����	  
	GPIOA->CRL|=0X00000008;
	
	GPIOC->CRH&=0XFFFFFF00;	//PC8��9���ó�����	  
	GPIOC->CRH|=0X00000088; 			 
	GPIOC->ODR|=1<<8;	   	//PC8����,PC8Ĭ������
	GPIOC->ODR|=1<<9;	   	//PC9����,PC9Ĭ������
	
	
	GPIOD->CRL&=0XFFFFF0FF;	//PD2���ó�����	  
	GPIOD->CRL|=0X00000800;   
	GPIOD->ODR|=1<<2;	   	//PD2���� 
	
		
} 
//����������
//���ذ���ֵ
//mode:0,��֧��������;1,֧��������;
//����ֵ��
//0��û���κΰ�������
//KEY0_PRES��KEY0����
//KEY1_PRES��KEY1����
//KEY2_PRES��KEY2����
//WKUP_PRES��WK_UP���� 
//ע��˺�������Ӧ���ȼ�,KEY0>KEY1>KEY2>WK_UP!!
u8 KEY_Scan(u8 mode)
{	 
	static u8 key_up=1;//�������ɿ���־
	if(mode)key_up=1;  //֧������		  
	if(key_up&&(KEY0==0||KEY1==0||KEY2==0||WK_UP==1))
	{
		delay_ms(10);//ȥ���� 
		key_up=0;
		if(KEY0==0)return KEY0_PRES;
		else if(KEY1==0)return KEY1_PRES;
		else if(KEY2==0)return KEY2_PRES;
		else if(WK_UP==1)return WKUP_PRES; 
	}else if(KEY0==1&&KEY1==1&&KEY2==1&&WK_UP==0)key_up=1; 	     
	return 0;// �ް�������
}
