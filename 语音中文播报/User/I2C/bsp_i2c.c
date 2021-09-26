#include "bsp_i2c.h"
#include "bsp_usart1.h"
/* 这个地址只要与STM32外挂的I2C器件地址不一样即可 */
#define I2Cx_OWN_ADDRESS7      0X0A 

/* STM32 I2C 快速模式 */
#define I2C_Speed              100000
static __IO uint32_t  I2CTimeout = I2CT_LONG_TIMEOUT; 


static void Delay_( u8 i)
 {     
    while( i--)
       {	

        }
 }	

 void delay(unsigned long uldata)
{
	u8 j  =  0;
	unsigned int g  =  0;
	for (j=0;j<5;j++)
	{
		for (g=0;g<uldata;g++)
		{
      Delay_(120);
		}
	}
}



static void I2C_GPIO_Config(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure; 

	/* 使能与 I2C1 有关的时钟 */
	RCC_APB1PeriphClockCmd ( RCC_APB1Periph_I2C1, ENABLE );
	RCC_APB2PeriphClockCmd ( RCC_APB2Periph_GPIOB, ENABLE );
	
    
  /* PB6-I2C1_SCL、PB7-I2C1_SDA*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;	       // 开漏输出
  GPIO_Init(GPIOB, &GPIO_InitStructure);
	
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;	       // 开漏输出
  GPIO_Init(GPIOB, &GPIO_InitStructure);		
	
	
	
	
}

static void I2C_Mode_Configu(void)
{
	
  I2C_InitTypeDef  I2C_InitStructure;
	I2C_DeInit(I2C1);//使用I2C1	

  /* I2C 配置 */
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
	
	/* 高电平数据稳定，低电平数据变化 SCL 时钟线的占空比 */
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	
  I2C_InitStructure.I2C_OwnAddress1 =I2Cx_OWN_ADDRESS7; 
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable ;
	
	/* I2C的寻址模式 */
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	
	/* 通信速率 */
  I2C_InitStructure.I2C_ClockSpeed = I2C_Speed;
  
	/* I2C1 初始化 */
  I2C_Init(I2C1, &I2C_InitStructure);
  
	/* 使能 I2C1 */
  I2C_Cmd(I2C1, ENABLE);   

}

void I2C_Bus_Init(void)
{
  I2C_GPIO_Config(); 
 
  I2C_Mode_Configu();

}

/**
  * @brief  Basic management of the timeout situation.
  * @param  None.
  * @retval None.
  */
static  uint8_t I2C_TIMEOUT_UserCallback(void)
{
  /* Block communication and all processes */
  //MPU_ERROR("I2C Timeout error!"); 
  
  return 0;
}



/**
  * @brief   写一个字节到I2C设备中
  * @param   
  *		@arg pBuffer:写的值
  * @retval  正常返回1，异常返回0
  */
uint8_t I2C_ByteWrite(u8 pBuffer)
{
	I2CTimeout = I2CT_FLAG_TIMEOUT;
	while(I2C_GetFlagStatus(I2C1,I2C_FLAG_BUSY))
	{
		if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback();
	}
	
  /* Send STRAT condition */
  I2C_GenerateSTART(I2C1, ENABLE);
	
	I2CTimeout = I2CT_FLAG_TIMEOUT;


  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback();
  } 

  /* Send slave address for write */
  I2C_Send7bitAddress(I2C1, I2C_ADDR, I2C_Direction_Transmitter);
  
	I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) 
	{
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback();
  }  
      

  /* Send the byte to be written */
  I2C_SendData(I2C1, pBuffer); 
	
	I2CTimeout = I2CT_FLAG_TIMEOUT;
   
  /* Test on EV8 and clear it */
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))	
	{
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback();
  } 	
	delay(200);
  /* Send STOP condition */
  I2C_GenerateSTOP(I2C1, ENABLE);
	delay(200);
	return 1; //正常返回1
}


void I2C_Writes_Bytes(u8 *buff,int number)
{
	int i;
	for(i = 0;i < number;i++)
	{
		I2C_ByteWrite(buff[i]);
	}
}



/**
  * @brief   获取芯片状态值 
  * @param   
  * @retval  返回状态值
  */
u8 GetChipStatus(void) 
{  
	u16 NumByteToRead = 1;
	u8 pBuffer = 0xff;
  u8 AskState[4] = {0xFD,0x00,0x01,0x21};
	
	I2C_Writes_Bytes(AskState,4);
			
		delay(350);
  /* Send STRAT condition a second time */  
  I2C_GenerateSTART(I2C1, ENABLE);
  
	I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
	{
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback();
  }
		
  /* Send slave address for read */
  I2C_Send7bitAddress(I2C1, I2C_ADDR, I2C_Direction_Receiver);
  
	I2CTimeout = I2CT_FLAG_TIMEOUT;
	 
  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
	{
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback();
  }


  /* While there is data to be read */
  while(NumByteToRead)  
  {
    if(NumByteToRead == 1)
    {
	      /* Disable Acknowledgement */
      I2C_AcknowledgeConfig(I2C1, DISABLE);
	
        /* Send STOP Condition */
			I2C_GenerateSTOP(I2C1, ENABLE);	
    }

    /* Test on EV7 and clear it */
    if(I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED))  
    {      
			
      /* Read a byte from the slave */
      pBuffer = I2C_ReceiveData(I2C1);
     
      /* Decrement the read bytes counter */
      NumByteToRead--;        
    }   
  }
  /* Enable Acknowledgement to be ready for another reception */
	
	return pBuffer;
}



void speech_text(u8 *str,u8 encoding_format)
{
	 u16 size = strlen(str) + 2;
	 
	 XFS_Protocol_TypeDef DataPack;
	
	 DataPack.DataHead = DATAHEAD;
	 DataPack.Length_HH = size >>8;
	 DataPack.Length_LL = size;
	 DataPack.Commond = 0x01;
	 DataPack.EncodingFormat = encoding_format;
	 DataPack.Text = str;
	 
	 I2C_ByteWrite(DataPack.DataHead );
	 I2C_ByteWrite(DataPack.Length_HH);	
	 I2C_ByteWrite(DataPack.Length_LL);
	 I2C_ByteWrite(DataPack.Commond);	
	 I2C_ByteWrite(DataPack.EncodingFormat);		
	
	 I2C_Writes_Bytes(DataPack.Text,strlen(DataPack.Text));	 
}

void SetBase(u8 *str)
{
	 u16 size = strlen(str) + 2;
	 
	 XFS_Protocol_TypeDef DataPack;
	
	 DataPack.DataHead = DATAHEAD;
	 DataPack.Length_HH = size >>8;
	 DataPack.Length_LL = size;
	 DataPack.Commond = 0x01;
	 DataPack.EncodingFormat = 0x00;
	 DataPack.Text = str;
	 
	 I2C_ByteWrite(DataPack.DataHead );
	 I2C_ByteWrite(DataPack.Length_HH);	
	 I2C_ByteWrite(DataPack.Length_LL);
	 I2C_ByteWrite(DataPack.Commond);	
	 I2C_ByteWrite(DataPack.EncodingFormat);
	
	 I2C_Writes_Bytes(DataPack.Text,strlen(DataPack.Text));
}

void TextCtrl(char c, int d)
{
  char str[10];
  if (d != -1)
    sprintf(str, "[%c%d]", c, d);
  else
    sprintf(str, "[%c]", c);
  SetBase(str);
	
}

void SetStyle(Style_Type style)
{
  TextCtrl('f', style);
	while(GetChipStatus() != ChipStatus_Idle)
	{
	  delay(10);
	}
}

void SetLanguage(Language_Type language)
{
	  TextCtrl('g', language);
		while(GetChipStatus() != ChipStatus_Idle)
		{
	  delay(10);
		}
}

void SetArticulation(Articulation_Type articulation)
{
  TextCtrl('h', articulation);
	while(GetChipStatus() != ChipStatus_Idle)
	{
	 delay(10);
	}
}

void SetSpell(Spell_Type spell)
{
  TextCtrl('i', spell);
	while(GetChipStatus() != ChipStatus_Idle)
	{
	 delay(10);
	}
}

void SetReader(Reader_Type reader)
{
  TextCtrl('m', reader);
	while(GetChipStatus() != ChipStatus_Idle)
	{
	 delay(10);
	}
}

void SetNumberHandle(NumberHandle_Type numberHandle)
{
  TextCtrl('n', numberHandle);
	while(GetChipStatus() != ChipStatus_Idle)
	{
	 delay(10);
	}
}

void SetZeroPronunciation(ZeroPronunciation_Type zeroPronunciation)
{
  TextCtrl('o', zeroPronunciation);
	while(GetChipStatus() != ChipStatus_Idle)
	{
	 delay(10);
	}
}


void SetNamePronunciation(NamePronunciation_Type namePronunciation)
{
  TextCtrl('r', namePronunciation);
	while(GetChipStatus() != ChipStatus_Idle)
	{
	 delay(10);
	}
}

void SetSpeed(int speed)
{
  TextCtrl('s', speed);
	while(GetChipStatus() != ChipStatus_Idle)
	{
	 delay(10);
	}
}

void SetIntonation(int intonation)
{
  TextCtrl('t', intonation);
	while(GetChipStatus() != ChipStatus_Idle)
	{
	 delay(10);
	}
}

void SetVolume(int volume)
{
  TextCtrl('v', volume);
	while(GetChipStatus() != ChipStatus_Idle)
	{
	 delay(10);
	}
}

void SetOnePronunciation(OnePronunciation_Type onePronunciation)
{
  TextCtrl('y', onePronunciation);
	while(GetChipStatus() != ChipStatus_Idle)
	{
	 delay(10);
	}
}

void SetRhythm(Rhythm_Type rhythm)
{
  TextCtrl('z', rhythm);
	while(GetChipStatus() != ChipStatus_Idle)
	{
	 delay(10);
	}
}

void SetRestoreDefault()
{
  TextCtrl('d', -1);
	while(GetChipStatus() != ChipStatus_Idle)
	{
	 delay(10);
	}
}




