#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
#include <ESP8266WiFi.h>
#include <AliyunIoTSDK.h>
static WiFiClient espClient;
#define BuzzerPin D5

// 设置产品和设备的信息，从阿里云设备信息里查看
#define PRODUCT_KEY "a1riwvdcy3f"
#define DEVICE_NAME "Arduino"
#define DEVICE_SECRET "296a23333af28d33a207342b4cfeaf0c"
#define REGION_ID "cn-shanghai"

// 设置 wifi 信息
#define WIFI_SSID "TP-605"
#define WIFI_PASSWD "605605605"

TinyGPSPlus gps;
TinyGPSAltitude Altitude;
SoftwareSerial GPS(4,3);

float latitude;
float longitude;
float altitude;
int year,month,day,hour,minute,second;
String CameraData,syear,smonth,sday,shour,sminute,ssecond;

void setup(){
  wifiInit(WIFI_SSID, WIFI_PASSWD);
  AliyunIoTSDK::begin(espClient, PRODUCT_KEY, DEVICE_NAME, DEVICE_SECRET, REGION_ID);
  GPS.begin(9600);
  GPS.listen();
  pinMode(BuzzerPin,OUTPUT);
  digitalWrite(BuzzerPin,LOW);
  Serial.begin(9600);
}

void SuccessBuzzer(){
  digitalWrite(BuzzerPin,HIGH);
  delay(300);
  digitalWrite(BuzzerPin,LOW);
  delay(1000);
}

void FailBuzzer(){
  for(unsigned int i=0;i<4;i++){
  digitalWrite(BuzzerPin,HIGH);
  delay(1000);
  digitalWrite(BuzzerPin,LOW);
  delay(500);
  }
  digitalWrite(BuzzerPin,LOW);
}

void ErrorBuzzer(){
  for(unsigned int i=0;i<2;i++){
      digitalWrite(BuzzerPin,HIGH);
      delay(1000);
      digitalWrite(BuzzerPin,LOW);
      delay(500);
  }
  digitalWrite(BuzzerPin,LOW);
}

void AliYunLocationTransfer(){
   while (GPS.available() > 0)
      {
        if(gps.encode(GPS.read()))
        {
          if (gps.location.isValid())
          {
                latitude = gps.location.lat(); //gps.location.lat() can export latitude
                char c[20];
                dtostrf(latitude,2,6,c);
                longitude = gps.location.lng();//gps.location.lng() can export latitude
                char d[20];
                uint8_t s=String(longitude/1).length();
                dtostrf(longitude,s,6,d);
                AliyunIoTSDK::send("latitude",c);
                delay(50);
                AliyunIoTSDK::send("longitude",d);
                AliyunIoTSDK::send("Flag",1);
                delay(50);
          }else{
            AliyunIoTSDK::send("Flag",0);
          }
          if(gps.time.isValid()&&gps.date.isValid())
          {
            year=gps.date.year();
            month=gps.date.month();
            day=gps.date.day();
            hour=(8+gps.time.hour())%24;
            minute=gps.time.minute();
            second=gps.time.second();
            syear=String(year);
            smonth=FormatInt(month);
            sday=FormatInt(day);
            shour=FormatInt(hour);
            sminute=FormatInt(minute);
            ssecond=FormatInt(second);
            String TimeForm=syear +"-"+ smonth  +"-"+ sday +","+ shour +":"+ sminute +":"+ ssecond;
            char TimeFormat[TimeForm.length() + 1];
            TimeForm.toCharArray(TimeFormat, TimeForm.length() + 1);
            AliyunIoTSDK::send("AlarmTime",TimeFormat);
            delay(50);
          }
            altitude=Altitude.meters();
            AliyunIoTSDK::send("Altitude",altitude);
            delay(50);
          }
      }
}

void Position(){
  while (GPS.available() > 0)
      {
        if(gps.encode(GPS.read()))
        {
          if (gps.location.isValid())
          {
                latitude = gps.location.lat(); //gps.location.lat() can export latitude
                char c[20];
                dtostrf(latitude,2,6,c);
                longitude = gps.location.lng();//gps.location.lng() can export latitude
                char d[20];
                uint8_t s=String(longitude/1).length();
                dtostrf(longitude,s,6,d);
                AliyunIoTSDK::send("WPosition",c);
                delay(50);
                AliyunIoTSDK::send("EPosition",d);
                AliyunIoTSDK::send("Flag",1);
                delay(50);
          }else{
            AliyunIoTSDK::send("Flag",0);
          }
        }
      }
}

String FormatInt(int a){
  if(a<10){
    return String("0"+String(a));
  }else{
    return String(a);
  }
}

void loop(){
       while(Serial.available()>0){
        CameraData=Serial.readStringUntil('\n'); 
        Serial.print(CameraData);
        AliyunIoTSDK::loop();
        Position();
        if(CameraData=="1"){
          SuccessBuzzer();
          delay(100);
        }else if(CameraData=="2"){
          ErrorBuzzer();
          delay(100);
        }else if(CameraData=="3"){
          FailBuzzer();
          delay(100);
          AliYunLocationTransfer();
          delay(100);
        }else {
          delay(100);
          }
      }
}

// 初始化 wifi 连接
void wifiInit(const char *ssid, const char *passphrase)
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, passphrase);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
    }
}

// 电源属性修改的回调函数
void powerCallback(JsonVariant p)
{
    int PowerSwitch = p["PowerSwitch"];
    if (PowerSwitch == 1)
    {
        // 启动设备
    } 
}
