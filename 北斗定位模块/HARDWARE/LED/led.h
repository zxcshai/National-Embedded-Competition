#ifndef __LED_H
#define __LED_H	 
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK NANO STM32������
//LED��������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2018/3/27
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2018-2028
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 

#define LED0 PCout(0)	// PC0
#define LED1 PCout(1)	// PC1
#define LED2 PCout(2)	// PC2
#define LED3 PCout(3)	// PC3
#define LED4 PCout(4)	// PC4
#define LED5 PCout(5)	// PC5
#define LED6 PCout(6)	// PC6
#define LED7 PCout(7)	// PC7


void LED_Init(void);//��ʼ��

		 				    
#endif
