//Общие (SD, BMP)
#include <SPI.h>

//DHT11
#include "DHT.h"
DHT dht(9, DHT11);

//BMP280
#include <Wire.h>
#include <Adafruit_BMP280.h>
Adafruit_BMP280 bmp;

//RTC1302
#include <ThreeWire.h>
#include <RtcDS1302.h>
ThreeWire myWire(7, 6, 8);
RtcDS1302<ThreeWire> Rtc(myWire);

//LCD2040
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
  Serial.println(F("Start!"));

  //Кнопка
  pinMode(3, INPUT);
  Serial.println(F("Button S"));

  //DHT Проверка
  dht.begin();
  if (isnan(dht.readHumidity())) {
    Serial.println("DHT11 FAIL");
    return;
  }

  //BMP Проверка
  if (!bmp.begin()) {
    Serial.println("BMP280 F");
    return;
  }
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16,
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);
  Serial.println("BMP280 S");

  //SD Проверка
  Serial.print("SD Init");
  if (!SD.begin(chipSelect)) {
    Serial.println("SD F");
    return;
  }
  Serial.println("SD S");

  //Получаем timestamps
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

  Serial.println("SD Write start");
  File dataFile = SD.open("LOG.txt", FILE_WRITE);
  if (dataFile) {
    String dataString;
    dataString += F("\nWRITE AT: ");
    dataString += time;
    dataString += F(" \ntime,temperature,pressure,humidity");
    dataFile.println(dataString);
    dataFile.close();
    Serial.println(dataString);
  } else {
    Serial.println("Sd Write F");
  }

  //Вывод заголовка и задержка перед стартом
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Запуск");
  delay(500);
  lcd.clear();

  //Вывод заголовков
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
  //Проверка нажати ли кнопка
  if (digitalRead(3) == HIGH) {
    lcdLight = !lcdLight;
    lcd.setBacklight(lcdLight);
  }

  //Получение значений
  float temperature = bmp.readTemperature();
  int pressure = int(bmp.readPressure() / 133.3);
  int humidity = int(dht.readHumidity());
  RtcDateTime now = Rtc.GetDateTime();

  //Форматирование времени
  String time;
  time += now.Hour();
  time += F(":");
  time += now.Minute();

  //Вывод времени
  lcd.setCursor(7, 0);
  lcdAdapter(time);

  //Вывод температуры
  lcd.setCursor(7, 1);
  lcdAdapter(String(temperature));
  lcdAdapter("^C");

  //Вывод давления
  lcd.setCursor(7, 2);
  lcdAdapter(String(pressure));
  lcdAdapter("mm");

  //Вывод влажности
  lcd.setCursor(7, 3);
  lcdAdapter(String(humidity));
  lcdAdapter("%");

  //Запись значений каждую минуту
  counter++;
  if (counter >= 30) { //Проверка прошла ли минута
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

//Функция для вывода текста на lcd
void lcdAdapter(String text) {
  //LCD Вывод
  lcd.print(text);
}