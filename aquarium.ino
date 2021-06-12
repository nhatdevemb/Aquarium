#include <Servo.h>
#include <SPI.h>

#include <DallasTemperature.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <OneWire.h>
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <WidgetRTC.h>
#include <NTPtimeESP.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2);


#define ONE_WIRE_BUS 0
//Thiết đặt thư viện onewire
OneWire oneWire(ONE_WIRE_BUS);
//Mình dùng thư viện DallasTemperature để đọc cho nhanh
DallasTemperature sensors(&oneWire);
NTPtime NTPch("ch.pool.ntp.org");
strDateTime dateTime;
WidgetLED PUMP(V0);  // Đèn trạng thái bơm
WidgetLED LAMP(V1);  // Đèn trạng thái đèn sưởi
WidgetRTC rtc;
Servo servo;

#define PUMP_PIN 12   //May bom
#define LAMP_PIN 13   //May suoi
#define LED_Pin 2   //Led RGB
#define Led_Count 72 //so LED

Adafruit_NeoPixel strip = Adafruit_NeoPixel(Led_Count, LED_Pin, NEO_GRB + NEO_KHZ800);

/* TIMER */
#define READ_DATA   5L //Gửi dữ liệu lên terminal
#define SEND_UP_DATA   5L //Gửi dữ liệu lên blynk
#define COLD 35
#define AUTO_CTRL      20L  //Chế độ tư động


boolean pumpStatus = 0;
boolean lampStatus = 0;
double temp = 0.0f;

int timePumpOn = 10; // Thời gian bật bơm nước 
// Biến cho timer
long sampleTimingSeconds = 1000; // ==> Thời gian đọc cảm biến (s)
long startTiming = 0;
long elapsedTime = 0;
// Khởi tạo timer
SimpleTimer timer;

bool timeOnOff      = false;
bool oldtimeOnOff;
bool isFirstConnect;
unsigned int TimeStart, TimeStop ;
byte dayStartSelect = 0;
byte dayStopSelect = 0 ;

int oldSecond, nowSecond;
#define timeShow V7
#define timeInput V8
#define PinOut   12

byte thermometro[8] = //icon for termometer
{
  B00100,
  B01010,
  B01010,
  B01110,
  B01110,
  B11111,
  B11111,
  B01110
};


byte igrasia[8] = //icon for water droplet
{
  B00100,
  B00100,
  B01010,
  B01010,
  B10001,
  B10001,
  B10001,
  B01110,
};

//Token Blynk và wifi
char auth[] = "yYN1n_ecZYCUMD1ItMMbOM7P4glYGaDa";
char ssid[] = "Matwifiroi";
char pass[] = "kiduynhat";
    
void setup() {
      strip.begin();
      strip.setBrightness(50);
      Serial.begin(9600);
      sensors.begin();
      lcd.begin();
      lcd.backlight();
      lcd.createChar(1, thermometro);
      lcd.createChar(2, igrasia);
      lcd.setCursor(3,0);
      lcd.print("AQUARIUM");
      lcd.setCursor(1,1);
      lcd.print("DESIGN BY TDT");
      delay(2000);
      lcd.clear();
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(LAMP_PIN, OUTPUT);
    pinMode(PinOut,OUTPUT);
    digitalWrite(PinOut, timeOnOff);
    servo.attach(15);
    Blynk.begin(auth, ssid, pass);   
    Serial.println(WiFi.localIP());
       lcd.setCursor(0,0);
       lcd.print(WiFi.localIP());
       lcd.clear();
    aplyCmd();
    Blynk.syncAll();
    rtc.begin();
    PUMP.off();
    LAMP.off();
    startTimers();
}

BLYNK_CONNECTED()
{
  Blynk.syncAll();
  rtc.begin();
}

BLYNK_WRITE(timeInput) { 
  Serial.println("Time Input"); 
  TimeInputParam t(param); 
  // Process start time 
  if (t.hasStartTime())
  { 
    TimeStart = t.getStartHour()*60 + t.getStartMinute();
  }
  else TimeStart = 0;

  if (t.hasStopTime()) { 
    TimeStop = t.getStopHour()*60 + t.getStopMinute(); 
  }
  else TimeStop = 0;

  dayStartSelect = 0;
  dayStopSelect  = 0;
  for (int i = 1; i <= 7; i++) 
    if (t.isWeekdaySelected(i)) 
      if (i == 7) { 
        bitWrite(dayStartSelect, 0, 1); 
        bitWrite(dayStopSelect, 1, 1); 
      }
      else { 
        bitWrite(dayStartSelect, i, 1);
      bitWrite(dayStopSelect, i+1, 1); 
      } 
  }
  String twoDigits(int digits) {
  if(digits < 10) return "0" + String(digits); 
else return String(digits); }


 void TimeAuto() { //so sánh để tính toán ngày giờ hiện thời và thời gian hẹn
  
  unsigned int times = hour()*60 + minute();
  byte today = weekday(); //the weekday now (Sunday is day 1, Monday is day 2) 
  if (TimeStart == TimeStop) { timeOnOff = false; } 
  else if (TimeStart < TimeStop)
  if (bitRead(dayStartSelect, today - 1))
  if ((TimeStart <= times) && (times < TimeStop)) timeOnOff = true; 
      else timeOnOff = false; 
    else timeOnOff = false; 
  else if (TimeStart > TimeStop) 
  { 
    if ((TimeStop <= times) && (times < TimeStart)) timeOnOff = false;
    else if ((TimeStart <= times) && bitRead(dayStartSelect, today - 1)) timeOnOff = true; 
    else if ((TimeStop > times) && bitRead(dayStopSelect, today)) timeOnOff = true; } }


/* Hàm hiển thị thời gian thực */
 void showTime() { 
 nowSecond = second(); 
 if (oldSecond != nowSecond) 
 { 
  oldSecond = nowSecond; 
  String currentTime;
 if (isPM()) currentTime = twoDigits(hourFormat12()) + ":" + twoDigits(minute()) + ":" + twoDigits(second()) + " PM"; //hiển thị thời gian chiều tối
 else  currentTime = twoDigits(hourFormat12()) + ":" + twoDigits(minute()) + ":" + twoDigits(second()) + " AM"; //hiển thị thời gian sáng
 String currentDate = String(day()) + "/" + month() + "/" + year(); 
 Blynk.virtualWrite(timeShow, currentTime);
 Blynk.virtualWrite(V6, currentDate);

  if (oldtimeOnOff != timeOnOff) 
{ 
  if (timeOnOff) { //hẹn giờ
    LAMP.on();
    digitalWrite(PinOut, timeOnOff); 
  Serial.println("Time schedule is ON"); } 
  else { 
    LAMP.off(); 
  digitalWrite(PinOut, timeOnOff); 
  Serial.println("Time schedule is OFF"); 
}  
    oldtimeOnOff = timeOnOff;
    }
  }
}
void loop() {
 timer.run(); // Bắt đầu SimpleTimer
    Blynk.run();
    showTime();
    TimeAuto();
//    printLCD();
}

BLYNK_WRITE(5) // Điều khiển cửa 
{
     servo.write(param.asInt());
     delay (10);
}

BLYNK_WRITE(4) // Điều khiển may suoi
{
    int i = param.asInt();
    if (i == 1)
    {
      lampStatus = !lampStatus;
      aplyCmd();
    }
}

BLYNK_WRITE(3) // Điều khiển bơm
{
    int i = param.asInt();
    if (i == 1)
    {
      pumpStatus = !pumpStatus;
      if (pumpStatus == 1)
  {
    
    digitalWrite(PUMP_PIN, LOW);
    PUMP.off();
  }
  else
  {
    digitalWrite(PUMP_PIN, HIGH);
    PUMP.on();
  }
    }
}
 BLYNK_WRITE(6)
    {
    int R = param[0].asInt();
    int G = param[1].asInt();
    int B = param[2].asInt();
    for(int i=0;i<Led_Count;i++)
      {
      strip.setPixelColor(i, strip.Color(R,G,B));
      strip.show();
      }
    }

void aplyCmd()
{     
    if (lampStatus == 0)
    {
      digitalWrite(LAMP_PIN, LOW);
      LAMP.off();
    }  
    else {
     
      digitalWrite(LAMP_PIN, HIGH);
      LAMP.on();
    }     
}
void dataSensor(void)
{
  sensors.requestTemperatures();  
  Serial.print("Nhiet do");
  Serial.println(sensors.getTempCByIndex(0)); // vì 1 ic nên dùng 0
  temp = sensors.getTempCByIndex(0);
  lcd.setCursor(0,0);
  lcd.print("NHIET DO BE: ");
  lcd.setCursor(0,1);
  lcd.write(2);
  lcd.print(temp);
  lcd.print(" C");
 
}


void Auto(void)
{
    if (temp < COLD) //nếu giá trị cảm biến nhor honw 20 do thi bat may suoi
    {
      turnLampOn(); //bật máy bơm
    }
    if(temp > COLD){ //nếu giá trị cảm biến nhỏ hơn giá trị ban đầu =>hết cháy
      lampStatus = 0; //tắt máy bơm
      aplyCmd(); 
    }
}

void turnLampOn()
{
   lampStatus = 1;
    aplyCmd();
   
}

void startTimers(void)
{
    timer.setInterval(SEND_UP_DATA * 1000, sendBlynk); 
     timer.setInterval(READ_DATA * 1000, dataSensor);
     timer.setInterval(AUTO_CTRL * 1000, Auto);  
} 
/***************************************************
 * Gửi dữ liệu lên Blynk
 **************************************************/
 
void sendBlynk()
{
    
    Blynk.virtualWrite(12, sensors.getTempCByIndex(0)); // Độ ẩm đất với V12
}
