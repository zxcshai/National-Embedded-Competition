#ifndef __BSP_I2C_H
#define	__BSP_I2C_H

#include "stm32f10x.h"
#include <stdio.h>
#include <string.h>

#define I2C_ADDR                    0x60   //语音识别模块地址,模块地址为0x30，由于最右边一位要做读写位，所以左移一位为0x60

#define DATAHEAD										0xFD   //帧头


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
} EncodingFormat_Type;//文本的编码格式

void speech_text(u8 *str,u8 encoding_format);


typedef enum
{
		ChipStatus_InitSuccessful = 0x4A,//初始化成功回传
		ChipStatus_CorrectCommand = 0x41,//收到正确的命令帧回传
		ChipStatus_ErrorCommand = 0x45,//收到不能识别命令帧回传
		ChipStatus_Busy = 0x4E,//芯片忙碌状态回传
		ChipStatus_Idle = 0x4F//芯片空闲状态回传
} ChipStatus_Type;//芯片回传

u8 GetChipStatus(void);

typedef enum
{
		Style_Single,//？为 0，一字一顿的风格
		Style_Continue//？为 1，正常合成
} Style_Type; //合成风格设置 [f?]
void SetStyle(Style_Type style);

typedef enum
{
		Language_Auto,//? 为 0，自动判断语种
		Language_Chinese,//? 为 1，阿拉伯数字、度量单位、特殊符号等合成为中文
		Language_English//? 为 2，阿拉伯数字、度量单位、特殊符号等合成为英文
} Language_Type; //合成语种设置 [g?]
void SetLanguage(Language_Type language);

typedef enum
{
		Articulation_Auto,//? 为 0，自动判断单词发音方式
		Articulation_Letter,//? 为 1，字母发音方式
		Articulation_Word//? 为 2，单词发音方式
} Articulation_Type; //设置单词的发音方式 [h?]
void SetArticulation(Articulation_Type articulation);

typedef enum
{
		Spell_Disable,//? 为 0，不识别汉语拼音
		Spell_Enable//? 为 1，将“拼音＋1 位数字（声调）”识别为汉语拼音，例如： hao3
} Spell_Type; //设置对汉语拼音的识别 [i?]
void SetSpell(Spell_Type spell);

typedef enum
{
		Reader_XiaoYan = 3,//? 为 3，设置发音人为小燕(女声, 推荐发音人)
		Reader_XuJiu = 51,//? 为 51，设置发音人为许久(男声, 推荐发音人)
		Reader_XuDuo = 52,//? 为 52，设置发音人为许多(男声)
		Reader_XiaoPing = 53,//? 为 53，设置发音人为小萍(女声)
		Reader_DonaldDuck = 54,//? 为 54，设置发音人为唐老鸭(效果器)
		Reader_XuXiaoBao = 55//? 为 55，设置发音人为许小宝(女童声)
} Reader_Type;//选择发音人 [m?]
void SetReader(Reader_Type reader);

typedef enum
{
		NumberHandle_Auto,//? 为 0，自动判断
		NumberHandle_Number,//? 为 1，数字作号码处理
		NumberHandle_Value//? 为 2，数字作数值处理
} NumberHandle_Type; //设置数字处理策略 [n?]
void SetNumberHandle(NumberHandle_Type numberHandle);

typedef enum
{
		ZeroPronunciation_Zero,//? 为 0，读成“zero
		ZeroPronunciation_O//? 为 1，读成“欧”音
} ZeroPronunciation_Type; //数字“0”在读 作英文、号码时 的读法 [o?]
void SetZeroPronunciation(ZeroPronunciation_Type zeroPronunciation);



typedef enum
{
		NamePronunciation_Auto,//? 为 0，自动判断姓氏读音
		NamePronunciation_Constraint//? 为 1，强制使用姓氏读音规则
} NamePronunciation_Type; //设置姓名读音 策略 [r?]
void SetNamePronunciation(NamePronunciation_Type namePronunciation);

void SetSpeed(int speed);//设置语速 [s?] ? 为语速值，取值：0～10
void SetIntonation(int intonation);//设置语调 [t?] ? 为语调值，取值：0～10
void SetVolume(int volume);//设置音量 [v?] ? 为音量值，取值：0～10

typedef enum
{
		OnePronunciation_Yao,//? 为 0，合成号码“1”时读成“幺
		OnePronunciation_Yi//? 为 1，合成号码“1”时读成“一”
} OnePronunciation_Type; //设置号码中“1”的读法 [y?]
void SetOnePronunciation(OnePronunciation_Type onePronunciation);

typedef enum
{
		Rhythm_Diasble,//? 为 0，“ *”和“#”读出符号
		Rhythm_Enable//? 为 1，处理成韵律，“*”用于断词，“#”用于停顿
} Rhythm_Type; //是否使用韵律 标记“*”和“#” [z?]
void SetRhythm(Rhythm_Type rhythm);

void SetRestoreDefault();//恢复默认的合成参数 [d] 所有设置（除发音人设置、语种设置外）恢复为默认值



/*等待超时时间*/
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
