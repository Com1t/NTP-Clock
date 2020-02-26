#include <Wire.h>
#include "SparkFunHTU21D.h"
#include <LiquidCrystal_I2C.h>

#define I2C_ADDRESS 0x68
#define NubberOfFields 7

#define mdisplayButton 10 // measurement display button (SD3)

// HTU21D sensor config.
HTU21D HTU;

LiquidCrystal_I2C lcd(0x27, 16, 2);       // PCF8574 0x27, PCF8574A 0x3F

const char week[7][4] = { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" };
const char month[12][4] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
uint16_t y;                                     // 年
uint8_t m, d, w, h, mi, s;                      // 月/日/週/時/分/秒
uint8_t curDay = 0;

uint8_t bcdTodec(uint8_t val);
void getTime();
void digitalClockDisplay();
void measurementDisplay();

void setup() {
  pinMode(mdisplayButton, INPUT_PULLUP);
  
  Wire.begin();
  HTU.begin();
  
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Now loading");
  delay(500);
  
  for (uint8_t i = 13; i >= 13 && i <= 15; i++) {
    lcd.setCursor(i, 0);
    lcd.print(".");
    delay(500);
  }
  delay(400);

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
  delay(1000);
}
  
/*BCD 轉 DEC*/
uint8_t bcdTodec(uint8_t val) {
  return ((val / 16 * 10) + (val % 16));
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
