//lcd
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//BMP280
#include <GyverBME280.h>
GyverBME280 bmp;

//DHT11
#include "DHT.h"
DHT dht(9, DHT11);

//RTC1302
#include <ThreeWire.h>
#include <RtcDS1302.h>
ThreeWire myWire(7, 6, 8);
RtcDS1302<ThreeWire> Rtc(myWire);

//SD
#include <SD.h>
byte counter = 0;

//lcd init
LiquidCrystal_I2C lcd(0x27, 20, 4);

//Прототипы
//Считавание с датчиков
void getTemperature(float* temperature);
void getHumidity(byte* humidity);
void getCo2(int* co2);
void getPressure(int* pressure);
void getTime(String* time);

void oneScreen();
void twoScreen();
void threeScreen();

void initPlot();
void printHeader();

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.clear();

  //graph init
  initPlot();

  pinMode(3, INPUT);
  Serial.println("Button S");

  if (!bmp.begin()) {
    Serial.println("BMP280 F");
    return;
  }
  Serial.println(F("BMP S"));

  dht.begin();
  if (isnan(dht.readHumidity())) {
    Serial.println("DHT11 F");
    return;
  }
  Serial.println("DHT11 S");

  Serial.print("SD Init");
  if (!SD.begin(4)) {
    Serial.println("SD F");
    return;
  }
  Serial.println("SD S");

  writeHeader();
}

void loop() {
  static byte state = 0;

  if (digitalRead(3) == HIGH) {
    state++;
    lcd.clear();
    delay(500);
  }
  if (state >= 3) {
    state = 0;
  }

  switch (state) {
    case 0:
      oneScreen();
      break;
    case 1:
      twoScreen();
      break;
    default:
      threeScreen();
  }

  delay(1000);
}


//ЭКРАН 1
void oneScreen() {
  printHeader();

  float temperature = 0;
  getTemperature(&temperature);
  lcd.setCursor(5, 0);
  lcd.print(String(temperature));

  int humidity = 0;
  getHumidity(&humidity);
  lcd.setCursor(5, 1);
  lcd.print(String(humidity));

  int co2 = 0;
  getCo2(&co2);
  lcd.setCursor(5, 2);
  lcd.print(String(co2));

  float pressure = 0;
  getPressure(&pressure);
  lcd.setCursor(5, 3);
  lcd.print(String(pressure));

  String time = "";
  getTime(&time);
  lcd.setCursor(15, 0);
  lcd.print(time);

  counter++;
  if (counter >= 60) {  //Проверка прошла ли минута
    counter = 0;
    writeRow(&time, &temperature, &pressure, &humidity);
  }
}

//ЭКРАН 2
void twoScreen() {
  lcd.setCursor(17, 0);
  lcd.print("Tmp");
  lcd.setCursor(17, 2);
  lcd.print("Hum");

  float temperature = 0.0;
  getTemperature(&temperature);
  drawPlotUp(0, 1, 17, 2, 20, 40, temperature);
  lcd.setCursor(18, 1);
  lcd.print(int(temperature));

  int humidity = 0.0;
  getHumidity(&humidity);
  drawPlotDown(0, 3, 17, 2, 0, 100, humidity);
  lcd.setCursor(18, 3);
  lcd.print(byte(humidity));

  //Сделать работу на таймере
  delay(1000);
}

//ЭКРАН 3
void threeScreen() {
  lcd.setCursor(17, 0);
  lcd.print("Co2");
  lcd.setCursor(17, 2);
  lcd.print("Pre");

  int co2 = 0;
  getCo2(&co2);
  drawPlotUp(0, 1, 17, 2, 200, 300, co2);
  lcd.setCursor(18, 1);
  lcd.print(co2);

  float pressure = 0;
  getPressure(&pressure);
  drawPlotDown(0, 3, 17, 2, 730, 780, pressure);
  lcd.setCursor(17, 3);
  lcd.print(int(pressure));

  delay(1000);
}


void getTemperature(float* temperature) {
  *temperature = bmp.readTemperature();
}

void getHumidity(int* humidity) {
  *humidity = int(dht.readHumidity());
}

void getCo2(int* co2) {
  *co2 = random(200, 300);
}

void getPressure(float* pressure) {
  *pressure = bmp.readPressure() / 133.3;
}

void getTime(String* time) {
  RtcDateTime now = Rtc.GetDateTime();
  if (now.Hour() < 10) {
    *time += F("0");
  }
  *time += now.Hour();
  *time += F(":");
  if (now.Minute() < 10) {
    *time += F("0");
  }
  *time += now.Minute();
}


void writeHeader() {
  Serial.println(F("SD Write start"));
  File dataFile = SD.open("LOG.txt", FILE_WRITE);
  if (dataFile) {
    String dataString;
    dataString += F("\nWRITE: ");
    dataString += F("\ntime,temperature,pressure,humidity");
    dataFile.println(dataString);
    dataFile.close();
    Serial.println(dataString);
  } else {
    Serial.println(F("Sd Write F"));
  }
}

void writeRow(String* time, float* temperature, float* pressure, int* humidity) {
  File dataFile = SD.open("LOG.txt", FILE_WRITE);

  String tempStr = String(*temperature);
  tempStr.replace(".", ",");

  String presStr = String(int(floor(*pressure)));
  tempStr.replace(".", ",");

  if (dataFile) {
    String dataString;
    dataString += *time;
    dataString += F(",");
    dataString += F("\"");
    dataString += tempStr;
    dataString += F("\"");
    dataString += F(",");
    dataString += F("\"");
    dataString += presStr;
    dataString += F("\"");
    dataString += F(",");
    dataString += *humidity;

    dataFile.println(dataString);
    dataFile.close();
    Serial.println(dataString);
  }
}


void printHeader() {
  lcd.home();
  lcd.print(F("TEMP"));

  lcd.setCursor(0, 1);
  lcd.print(F("HUMI"));

  lcd.setCursor(0, 2);
  lcd.print(F("CO2"));

  lcd.setCursor(0, 3);
  lcd.print(F("PRES"));
}


void initPlot() {
  byte row8[8] = { 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111 };
  byte row7[8] = { 0b00000, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111 };
  byte row6[8] = { 0b00000, 0b00000, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111 };
  byte row5[8] = { 0b00000, 0b00000, 0b00000, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111 };
  byte row4[8] = { 0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b11111, 0b11111, 0b11111 };
  byte row3[8] = { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b11111, 0b11111 };
  byte row2[8] = { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b11111 };
  byte row1[8] = { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111 };
  lcd.createChar(0, row8);
  lcd.createChar(1, row1);
  lcd.createChar(2, row2);
  lcd.createChar(3, row3);
  lcd.createChar(4, row4);
  lcd.createChar(5, row5);
  lcd.createChar(6, row6);
  lcd.createChar(7, row7);
}

int plot_arrayUp[20];
void drawPlotUp(byte pos, byte row, byte width, byte height, int min_val, int max_val, int fill_val) {
  for (byte i = 0; i < width; i++) {
    plot_arrayUp[i] = plot_arrayUp[i + 1];
  }

  fill_val = constrain(fill_val, min_val, max_val);
  plot_arrayUp[width] = fill_val;

  for (byte i = 0; i < width; i++) {  // каждый столбец параметров
    byte infill, fract;
    infill = floor((float)(plot_arrayUp[i] - min_val) / (max_val - min_val) * height * 10);
    fract = (infill % 10) * 8 / 10;  // найти количество оставшихся полосок
    infill = infill / 10;

    for (byte n = 0; n < height; n++) {  // для всех строк графика
      if (n < infill && infill > 0) {    // пока мы ниже уровня
        lcd.setCursor(i, (row - n));     // заполняем полными ячейками
        lcd.write(0);
      }
      if (n >= infill) {  // если достигли уровня
        lcd.setCursor(i, (row - n));
        if (fract > 0) lcd.write(fract);         // заполняем дробные ячейки
        else lcd.write(0x20);                    // если дробные == 0, заливаем пустой
        for (byte k = n + 1; k < height; k++) {  // всё что сверху заливаем пустыми
          lcd.setCursor(i, (row - k));
          lcd.write(0x20);
        }
        break;
      }
    }
  }
}

int plot_arrayDown[20];
void drawPlotDown(byte pos, byte row, byte width, byte height, int min_val, int max_val, int fill_val) {
  for (byte i = 0; i < width; i++) {
    plot_arrayDown[i] = plot_arrayDown[i + 1];
  }

  fill_val = constrain(fill_val, min_val, max_val);
  plot_arrayDown[width] = fill_val;

  for (byte i = 0; i < width; i++) {  // каждый столбец параметров
    byte infill, fract;
    infill = floor((float)(plot_arrayDown[i] - min_val) / (max_val - min_val) * height * 10);
    fract = (infill % 10) * 8 / 10;  // найти количество оставшихся полосок
    infill = infill / 10;

    for (byte n = 0; n < height; n++) {  // для всех строк графика
      if (n < infill && infill > 0) {    // пока мы ниже уровня
        lcd.setCursor(i, (row - n));     // заполняем полными ячейками
        lcd.write(0);
      }
      if (n >= infill) {  // если достигли уровня
        lcd.setCursor(i, (row - n));
        if (fract > 0) lcd.write(fract);         // заполняем дробные ячейки
        else lcd.write(0x20);                    // если дробные == 0, заливаем пустой
        for (byte k = n + 1; k < height; k++) {  // всё что сверху заливаем пустыми
          lcd.setCursor(i, (row - k));
          lcd.write(0x20);
        }
        break;
      }
    }
  }
}
