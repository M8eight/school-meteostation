//Общие (SD, BMP)
#include <SPI.h>

//Влажность
#include "DHT.h"
DHT dht(9, DHT11);

//Давление и температура
#include <Wire.h>
#include <Adafruit_BMP280.h>
Adafruit_BMP280 bmp;

//RTC
#include <ThreeWire.h>
#include <RtcDS1302.h>
ThreeWire myWire(7, 6, 8);
RtcDS1302<ThreeWire> Rtc(myWire);

//LCD
#define _LCD_TYPE 1
#include <LCD_1602_RUS_ALL.h>
LCD_1602_RUS lcd(0x27, 20, 4);
bool lcdLight = true;

//SD
#include <SD.h>
const int chipSelect = 4;
int counter = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("DEBUG!");

  //Кнопка
  pinMode(3, INPUT);

  //DHT Проверка
  dht.begin();
  if (isnan(dht.readHumidity())) {
    Serial.println("DHT11 FAIL");
    return;
  }

  //BMP Проверка
  if (!bmp.begin()) {               // Если датчик BMP280 не найден
    Serial.println("BMP280 FAIL");  // Выводим сообщение об ошибке
    return;
  }
  //BMP Установка
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16,
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);

  Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    return;
  }
  Serial.println("card initialized.");

  RtcDateTime now = Rtc.GetDateTime();
  String time;
  time += now.Day();
  time += F(".");
  time += now.Month();
  time += F(".");
  time += now.Year();
  time += F(" ");
  time += now.Hour();
  time += F(":");
  time += now.Minute();

  Serial.println("write table deader");
  File dataFile = SD.open("LOG.txt", FILE_WRITE);
  if (dataFile) {
    String dataString;
    dataString += F("\WRITE AT: ");
    dataString += time;
    dataString += F(" \ntime,temperature,pressure,humidity");
    dataFile.println(dataString);
    dataFile.close();
    Serial.println(dataString);
  } else {
    Serial.println("Sd open error");
  }

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Запуск");
  delay(500);
  lcd.clear();

  //Названия на дисплей
  lcd.setCursor(0, 0);
  lcdAdapter("TIME: ");
  lcd.setCursor(0, 1);
  lcdAdapter("TEMP: ");
  lcd.setCursor(0, 2);
  lcdAdapter("PRES: ");
  lcd.setCursor(0, 3);
  lcdAdapter("HUMI: ");
}

void loop() {

  if (digitalRead(3) == HIGH) {
    lcdLight = !lcdLight;
    lcd.setBacklight(lcdLight); 
  }

  //Значения
  float temperature = bmp.readTemperature();
  int pressure = int(bmp.readPressure() / 133.3);
  int humidity = int(dht.readHumidity());
  RtcDateTime now = Rtc.GetDateTime();

  //Получение времени
  String time;
  time += now.Hour();
  time += F(":");
  time += now.Minute();

  //Время
  lcd.setCursor(7, 0);
  lcdAdapter(time);

  //Температура
  lcd.setCursor(7, 1);
  lcdAdapter(String(temperature));
  lcdAdapter("^C");

  //Давление
  lcd.setCursor(7, 2);
  lcdAdapter(String(pressure));
  lcdAdapter("mm");

  //Влажность
  lcd.setCursor(7, 3);
  lcdAdapter(String(humidity));
  lcdAdapter("%");

  counter++;
  if (counter >= 30) {
    counter = 0;
    File dataFile = SD.open("LOG.txt", FILE_WRITE);
    if (dataFile) {
      String dataString;
      dataString += time;
      dataString += F(",");
      dataString += temperature;
      dataString += F(",");
      dataString += pressure;
      dataString += F(",");
      dataString += humidity;
      dataFile.println(dataString);
      dataFile.close();
      Serial.println(dataString);
    }
  }

  delay(2000);
}

void lcdAdapter(String text) {
  //LCD Вывод
  lcd.print(text);
}