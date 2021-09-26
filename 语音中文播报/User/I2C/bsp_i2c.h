#ifndef __BSP_I2C_H
#define	__BSP_I2C_H

#include "stm32f10x.h"
#include <stdio.h>
#include <string.h>

#define I2C_ADDR                    0x60   //����ʶ��ģ���ַ,ģ���ַΪ0x30���������ұ�һλҪ����дλ����������һλΪ0x60

#define DATAHEAD										0xFD   //֡ͷ


typedef struct
{
		u8 DataHead;
		u8 Length_HH;
		u8 Length_LL;
		u8 Commond;
		u8 EncodingFormat;
		u8* Text;
} XFS_Protocol_TypeDef;

typedef enum
{
		GB2312 = 0x00,
		GBK = 0x01,
		BIG5 = 0x02,
		UNICODE = 0x03
} EncodingFormat_Type;//�ı��ı����ʽ

void speech_text(u8 *str,u8 encoding_format);


typedef enum
{
		ChipStatus_InitSuccessful = 0x4A,//��ʼ���ɹ��ش�
		ChipStatus_CorrectCommand = 0x41,//�յ���ȷ������֡�ش�
		ChipStatus_ErrorCommand = 0x45,//�յ�����ʶ������֡�ش�
		ChipStatus_Busy = 0x4E,//оƬæµ״̬�ش�
		ChipStatus_Idle = 0x4F//оƬ����״̬�ش�
} ChipStatus_Type;//оƬ�ش�

u8 GetChipStatus(void);

typedef enum
{
		Style_Single,//��Ϊ 0��һ��һ�ٵķ��
		Style_Continue//��Ϊ 1�������ϳ�
} Style_Type; //�ϳɷ������ [f?]
void SetStyle(Style_Type style);

typedef enum
{
		Language_Auto,//? Ϊ 0���Զ��ж�����
		Language_Chinese,//? Ϊ 1�����������֡�������λ��������ŵȺϳ�Ϊ����
		Language_English//? Ϊ 2�����������֡�������λ��������ŵȺϳ�ΪӢ��
} Language_Type; //�ϳ��������� [g?]
void SetLanguage(Language_Type language);

typedef enum
{
		Articulation_Auto,//? Ϊ 0���Զ��жϵ��ʷ�����ʽ
		Articulation_Letter,//? Ϊ 1����ĸ������ʽ
		Articulation_Word//? Ϊ 2�����ʷ�����ʽ
} Articulation_Type; //���õ��ʵķ�����ʽ [h?]
void SetArticulation(Articulation_Type articulation);

typedef enum
{
		Spell_Disable,//? Ϊ 0����ʶ����ƴ��
		Spell_Enable//? Ϊ 1������ƴ����1 λ���֣���������ʶ��Ϊ����ƴ�������磺 hao3
} Spell_Type; //���öԺ���ƴ����ʶ�� [i?]
void SetSpell(Spell_Type spell);

typedef enum
{
		Reader_XiaoYan = 3,//? Ϊ 3�����÷�����ΪС��(Ů��, �Ƽ�������)
		Reader_XuJiu = 51,//? Ϊ 51�����÷�����Ϊ���(����, �Ƽ�������)
		Reader_XuDuo = 52,//? Ϊ 52�����÷�����Ϊ���(����)
		Reader_XiaoPing = 53,//? Ϊ 53�����÷�����ΪСƼ(Ů��)
		Reader_DonaldDuck = 54,//? Ϊ 54�����÷�����Ϊ����Ѽ(Ч����)
		Reader_XuXiaoBao = 55//? Ϊ 55�����÷�����Ϊ��С��(Ůͯ��)
} Reader_Type;//ѡ������ [m?]
void SetReader(Reader_Type reader);

typedef enum
{
		NumberHandle_Auto,//? Ϊ 0���Զ��ж�
		NumberHandle_Number,//? Ϊ 1�����������봦��
		NumberHandle_Value//? Ϊ 2����������ֵ����
} NumberHandle_Type; //�������ִ������ [n?]
void SetNumberHandle(NumberHandle_Type numberHandle);

typedef enum
{
		ZeroPronunciation_Zero,//? Ϊ 0�����ɡ�zero
		ZeroPronunciation_O//? Ϊ 1�����ɡ�ŷ����
} ZeroPronunciation_Type; //���֡�0���ڶ� ��Ӣ�ġ�����ʱ �Ķ��� [o?]
void SetZeroPronunciation(ZeroPronunciation_Type zeroPronunciation);



typedef enum
{
		NamePronunciation_Auto,//? Ϊ 0���Զ��ж����϶���
		NamePronunciation_Constraint//? Ϊ 1��ǿ��ʹ�����϶�������
} NamePronunciation_Type; //������������ ���� [r?]
void SetNamePronunciation(NamePronunciation_Type namePronunciation);

void SetSpeed(int speed);//�������� [s?] ? Ϊ����ֵ��ȡֵ��0��10
void SetIntonation(int intonation);//������� [t?] ? Ϊ���ֵ��ȡֵ��0��10
void SetVolume(int volume);//�������� [v?] ? Ϊ����ֵ��ȡֵ��0��10

typedef enum
{
		OnePronunciation_Yao,//? Ϊ 0���ϳɺ��롰1��ʱ���ɡ���
		OnePronunciation_Yi//? Ϊ 1���ϳɺ��롰1��ʱ���ɡ�һ��
} OnePronunciation_Type; //���ú����С�1���Ķ��� [y?]
void SetOnePronunciation(OnePronunciation_Type onePronunciation);

typedef enum
{
		Rhythm_Diasble,//? Ϊ 0���� *���͡�#����������
		Rhythm_Enable//? Ϊ 1����������ɣ���*�����ڶϴʣ���#������ͣ��
} Rhythm_Type; //�Ƿ�ʹ������ ��ǡ�*���͡�#�� [z?]
void SetRhythm(Rhythm_Type rhythm);

void SetRestoreDefault();//�ָ�Ĭ�ϵĺϳɲ��� [d] �������ã������������á����������⣩�ָ�ΪĬ��ֵ



/*�ȴ���ʱʱ��*/
#define I2CT_FLAG_TIMEOUT         ((uint32_t)0x1000)
#define I2CT_LONG_TIMEOUT         ((uint32_t)(10 * I2CT_FLAG_TIMEOUT))

#define MPU_ERROR(fmt,arg...)          printf("<<-MPU-ERROR->> "fmt"\n",##arg)


void I2C_Bus_Init(void);
uint8_t I2C_ByteWrite(u8 pBuffer);
void I2C_Writes_Bytes(u8 *buff,int number);


void SetBase(u8 *str);
void TextCtrl(char c, int d);

void delay(unsigned long uldata);



#endif /* __BSP_I2C_H */
