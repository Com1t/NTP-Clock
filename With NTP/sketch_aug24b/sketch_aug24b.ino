#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "SparkFunHTU21D.h"
#include <LiquidCrystal_I2C.h>

#define I2C_ADDRESS 0x68
#define NubberOfFields 7

#define timesyncButton 9 // time sync button (SD2)
#define mdisplayButton 10 // measurement display button (SD3)

#define tempLED 12 // temperature LED (D6)
#define syncLED 13 // time sync LED (D7)
#define powerLED 14 // power LED (D5)

const char *ssid     = "ASUS-G";
const char *password = "0922235662";

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// HTU21D sensor config.
HTU21D HTU;

LiquidCrystal_I2C lcd(0x27, 16, 2);       // PCF8574 0x27, PCF8574A 0x3F

const char week[7][4] = { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" };
const char month[12][4] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
uint16_t y;                                     // 年
uint8_t m, d, w, h, mi, s;                      // 月/日/週/時/分/秒
uint8_t curDay = 0;

uint8_t bcdTodec(uint8_t val);
uint8_t decToBcd(uint8_t val);
void setTime();
void getTime();
void digitalClockDisplay();
void measurementDisplay();

void setup() {
  pinMode(mdisplayButton, INPUT_PULLUP);
  pinMode(timesyncButton, INPUT_PULLUP);
  // tempLED initialization
  pinMode(tempLED, OUTPUT);
  digitalWrite(tempLED, HIGH);
  // syncLED initialization
  pinMode(syncLED, OUTPUT);
  digitalWrite(syncLED, HIGH);
  // powerLED initialization
  pinMode(powerLED, OUTPUT);
  digitalWrite(powerLED, HIGH);
  
  Wire.begin();
  HTU.begin();
  
  lcd.begin();
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Now loading");
  WiFi.begin(ssid, password);
  delay(500);
  
  uint8_t i = 13;
  while (WiFi.status() != WL_CONNECTED) {
    if(i >= 13 && i <= 15) {
      lcd.setCursor(i, 0);
      lcd.print(".");
      i++;
    }
    delay(500);
  }
    
  lcd.clear();
  lcd.print("Connected");
  uint8_t ssidCursor = 16 - String(ssid).length();
  if(ssidCursor < 0)
    ssidCursor = 0;
  lcd.setCursor(ssidCursor, 1);
  lcd.print(ssid);
  
  // Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(28800);
  
  setTime();
  delay(1000);
  
  lcd.clear();
  lcd.print("READ SENSOR");
  float temp = HTU.readTemperature();
  float humi = HTU.readHumidity();
  // Errors 998 if not sensor is detected. Error 999 if CRC is bad.
  if ( temp >= 998 || humi >= 998 ) {
    lcd.setCursor(11, 1);
    lcd.print("ERROR");
    delay(1000);
  }
  else {
    lcd.setCursor(14, 1);
    lcd.print("OK");
    delay(1000);
  }
  
  lcd.clear();
}

void loop() {
  getTime();               // 取得時間
  digitalClockDisplay();   // 顯示時間
  // auto x = millis();, From here
  if (!digitalRead(mdisplayButton)) {
    for (int positionCounter = 0; positionCounter <= 16; positionCounter++) {
      lcd.scrollDisplayLeft();  // scroll one position left
      delay(200); // wait a bit
    }
  
    measurementDisplay();
    
    for (int positionCounter = 0; positionCounter <= 16; positionCounter++) {
      lcd.scrollDisplayLeft();  // scroll one position left
      delay(200); // wait a bit
    }
    lcd.clear();
    curDay = 0;
  }
  
  if (!digitalRead(timesyncButton)) {
    digitalWrite(syncLED, LOW);   // turn the LED on (LOW is the voltage level)
    delay(100);
    digitalWrite(syncLED, HIGH);    // turn the LED off by making the voltage HIGH
    delay(100);
    setTime();
    digitalWrite(syncLED, LOW);   // turn the LED on (LOW is the voltage level)
    delay(100);                       // wait for a second
    digitalWrite(syncLED, HIGH);    // turn the LED off by making the voltage HIGH
    delay(100);
  }
  
  if(HTU.readTemperature() > 30)
    digitalWrite(tempLED, LOW);   // turn the LED on (LOW is the voltage level)
  else
    digitalWrite(tempLED, HIGH);    // turn the LED off by making the voltage HIGH
  // Serial.println(millis() - x);, to here takes average 51 milliseconds
  delay(950);
}
  
// BCD 轉 DEC
uint8_t bcdTodec(uint8_t val){
 return ((val / 16 * 10) + (val % 16));
}

// DEC 轉 BCD
uint8_t decToBcd(uint8_t val){
 return ((val / 10 * 16) + (val % 10));
} 

// 設定時間
void setTime(){
  timeClient.update();
  unsigned long rawTime = timeClient.getEpochTime() / 86400L;  // in days
  unsigned long days = 0, year = 1970;
  uint8_t month;
  static const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};
  
  while((days += (LEAP_YEAR(year) ? 366 : 365)) <= rawTime)
    year++;
  rawTime -= days - (LEAP_YEAR(year) ? 366 : 365); // now it is days in this year, starting at 0
  days=0;
  for (month=0; month<12; month++) {
    uint8_t monthLength;
    if (month==1) { // february
      monthLength = LEAP_YEAR(year) ? 29 : 28;
    } else {
      monthLength = monthDays[month];
    }
    if (rawTime < monthLength) break;
    rawTime -= monthLength;
  }
  
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(0);
  Wire.write(decToBcd(timeClient.getSeconds()));
  Wire.write(decToBcd(timeClient.getMinutes()));
  Wire.write(decToBcd(timeClient.getHours()));
  uint8_t week = timeClient.getDay();
  if( week == 0 )
    week = 7;
  Wire.write(decToBcd(week));
  Wire.write(decToBcd(++rawTime));
  Wire.write(decToBcd(++month));
  Wire.write(decToBcd(year-2000));
  Wire.endTransmission();
}

/*自DS1307 or DS3231 取得時間*/
void getTime() {
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();
  Wire.requestFrom(I2C_ADDRESS, NubberOfFields);

  s = bcdTodec(Wire.read() & 0x7f);
  mi = bcdTodec(Wire.read());
  h = bcdTodec(Wire.read() & 0x7f);
  w = bcdTodec(Wire.read());
  d = bcdTodec(Wire.read());
  m = bcdTodec(Wire.read());
  y = bcdTodec(Wire.read()) + 2000;
}

/*顯示時間*/
void digitalClockDisplay() {
  if( curDay != d ){
    lcd.clear();
    m -= 1;
    if (m >= 0 && m <= 11) {
      for (uint8_t f = 0; f < 3; f++) {
        lcd.print(month[m][f]);
      }
    }
    lcd.print(" ");
    lcd.print(d);
    lcd.print(" ");
    lcd.print(y);
    lcd.setCursor(12, 0);
    w -= 1;
    if (w >= 0 && w <= 6) {
      for (uint8_t e = 0; e < 3; e++) {
        lcd.print(week[w][e]);
      }
    }
    curDay = d;
  }
  
  lcd.setCursor(4, 1);

  if (h < 10) {
    lcd.print("0");
  }
  lcd.print(h);
  
  lcd.print(":");
  
  if (mi < 10) {
    lcd.print("0");
  }
  lcd.print(mi);
  
  lcd.print(":");
  
  if (s < 10) {
    lcd.print("0");
  }
  lcd.print(s);
  
}

/*顯示溫濕度*/
void measurementDisplay(){
  int temp = HTU.readTemperature();
  int humi = HTU.readHumidity();
  
  lcd.clear();
  // Errors 998 if not sensor is detected. Error 999 if CRC is bad.
  if ( temp >= 998 || humi >= 998 ) {
     lcd.setCursor(6, 0);
     lcd.print("ERROR");
     lcd.setCursor(1, 1);
     lcd.print("SENSOR OFFLINE");
     delay(1000);
   }
   else {
     lcd.print("Temperature");
     lcd.setCursor(12, 0);
     lcd.print(temp);
     lcd.print((char)0xDF);
     lcd.print("C");
     lcd.setCursor(0, 1);
     lcd.print("Humidity");
     lcd.setCursor(12, 1);
     lcd.print(humi);
     lcd.setCursor(15, 1);
     lcd.print("%");
     delay(2000);
  }
}
