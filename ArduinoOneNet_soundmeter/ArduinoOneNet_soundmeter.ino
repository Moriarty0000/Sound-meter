// include the library code:
#include <LiquidCrystal.h>
// with the arduino pin number it is connected to
const int rs = 7, en = 8, d4 = 9, d5 = 10, d6 = 11, d7 = 12;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//配置ESP8266WIFI设置
#include "ESP8266.h"
#include "SoftwareSerial.h"
#define SSID "Mori's Mate"    //填写2.4GHz的WIFI名称，不要使用校园网
#define PASSWORD "00000000"//填写自己的WIFI密码
#define HOST_NAME "api.heclouds.com"  //API主机名称，连接到OneNET平台，无需修改
#define DEVICE_ID "644785956"       //填写自己的OneNet设备ID
#define HOST_PORT (80)                //API端口，连接到OneNET平台，无需修改
String APIKey = "iUc36fianzeyJXCMhjJUGX2bOnc="; //与设备绑定的APIKey

#define INTERVAL_SENSOR 250 //定义传感器采样及发送时间间隔

#define FILTER_N 10
const int soundpin=A0;
int value;
double filter_buf[FILTER_N];

//定义ESP8266所连接的软串口
/*********************
 * 该实验需要使用软串口
 * Arduino上的软串口RX定义为D3,
 * 接ESP8266上的TX口,
 * Arduino上的软串口TX定义为D2,
 * 接ESP8266上的RX口.
 * D3和D2可以自定义,
 * 但接ESP8266时必须恰好相反
 *********************/
SoftwareSerial mySerial(3, 2);
ESP8266 wifi(mySerial);

void setup(){
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd.print("connecting wifi");

  //LED
  pinMode(5,OUTPUT);
  
  filter_buf[FILTER_N-2]=60;
  mySerial.begin(115200); //初始化软串口
  Serial.begin(9600);     //初始化串口
  Serial.print("setup begin\r\n");

  //以下为ESP8266初始化的代码
  Serial.print("FW Version: ");
  Serial.println(wifi.getVersion().c_str());

  if (wifi.setOprToStation()) {
    Serial.print("to station ok\r\n");
  } else {
    Serial.print("to station err\r\n");
  }

  //ESP8266接入WIFI
  if (wifi.joinAP(SSID, PASSWORD)) {
    Serial.print("Join AP success\r\n");
    Serial.print("IP: ");
    Serial.println(wifi.getLocalIP().c_str());
  } else {
    Serial.print("Join AP failure\r\n");
  }

//  Serial.println("");
//  Serial.print("DHT11 LIBRARY VERSION: ");
//  Serial.println(DHT11LIB_VERSION);

  mySerial.println("AT+UART_CUR=9600,8,1,0,0");
  mySerial.begin(9600);
  Serial.println("setup end\r\n");
}

unsigned long net_time1 = millis(); //数据上传服务器时间

void loop(){
  double v = analogRead(soundpin);
  double volume=40+abs(v - 550)/0.7;
  value = Filter(volume);

  //lcd
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);
  lcd.print(value);
  lcd.setCursor(0, 1);
  lcd.print("dB");

  //LED
  if(value>43){
    digitalWrite(5,HIGH);
  }else{
    digitalWrite(5,LOW);
  }
  
  if (net_time1 > millis())
    net_time1 = millis();

  if (millis() - net_time1 > INTERVAL_SENSOR) //发送数据时间间隔
  {

  int chk=value;
   Serial.print("Read sensor: ");
   switch (chk) {
//      case DHTLIB_OK:
//        Serial.println("OK");
//        break;
//      case DHTLIB_ERROR_CHECKSUM:
//        Serial.println("Checksum error");
//        break;
//      case DHTLIB_ERROR_TIMEOUT:
//        Serial.println("Time out error");
//        break;
//      default:
//        Serial.println("Unknown error");
//        break;
      case true:
          Serial.println("OK");
          break;
      case false:
          Serial.println("err");
          break;
    }

      Serial.print("dB: ");
      Serial.println(value);
      Serial.println("");

    if (wifi.createTCP(HOST_NAME, HOST_PORT)) { //建立TCP连接，如果失败，不能发送该数据
      Serial.print("create tcp ok\r\n");
      char buf[10];
      //拼接发送data字段字符串
        String jsonToSend = "{\"dB\":";
        dtostrf(value, 3, 1, buf);
        jsonToSend += "\"" + String(buf) + "\"";
        jsonToSend += "}";

      //拼接POST请求字符串
      String postString = "POST /devices/";
      postString += DEVICE_ID;
      postString += "/datapoints?type=3 HTTP/1.1";
      postString += "\r\n";
      postString += "api-key:";
      postString += APIKey;
      postString += "\r\n";
      postString += "Host:api.heclouds.com\r\n";
      postString += "Connection:close\r\n";
      postString += "Content-Length:";
      postString += jsonToSend.length();
      postString += "\r\n";
      postString += "\r\n";
      postString += jsonToSend;
      postString += "\r\n";
      postString += "\r\n";
      postString += "\r\n";

      const char *postArray = postString.c_str(); //将str转化为char数组

      Serial.println(postArray);
      wifi.send((const uint8_t *)postArray, strlen(postArray)); //send发送命令，参数必须是这两种格式，尤其是(const uint8_t*)
      Serial.println("send success");
      if (wifi.releaseTCP()) { //释放TCP连接
        Serial.print("release tcp ok\r\n");
      } else {
        Serial.print("release tcp err\r\n");
      }
      postArray = NULL; //清空数组，等待下次传输数据
    } else {
      Serial.print("create tcp err\r\n");
    }

    Serial.println("");

    net_time1 = millis();
  }
}

// 限幅平均滤波法
#define FILTER_A 30
double Filter(double a) {
  int i;
  double filter_sum = 0;
  filter_buf[FILTER_N - 1] = a;
  if(abs(filter_buf[FILTER_N - 1] - filter_buf[FILTER_N - 2]) > FILTER_A)
    filter_buf[FILTER_N - 1] = filter_buf[FILTER_N - 2];
  for(i = 0; i < FILTER_N - 1; i++) {
    filter_buf[i] = filter_buf[i + 1];
    filter_sum += filter_buf[i];
  }
  return (double)filter_sum / (FILTER_N - 1);
}
