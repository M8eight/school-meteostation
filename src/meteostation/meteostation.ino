//Глобальные переменные 
#define GRAPH_UPDATE_TIME 10000 //В миллисекундах
#define MQ135_PIN A1
#define DHT11_PIN 9
#define RTC_RST 8
#define RTC_DAT 7
#define RTC_CLK 6
#define SD_CS 4
#define SENSOR_PIN 3
#define WRITE_SD 60 //В секундах
#define CO2_NORMAL_LIMIT 1000 //В ppm

//mq-135
  #include <TroykaMQ.h>
  MQ135 mq135(MQ135_PIN);
//lcd
  #include <Wire.h>
  #include <LiquidCrystal_I2C.h>
//BMP280
  #include <GyverBME280.h>
  GyverBME280 bmp;
//DHT11
  #include "DHT.h"
  DHT dht(DHT11_PIN, DHT11);
//RTC1302
  #include <ThreeWire.h>
  #include <RtcDS1302.h>
  ThreeWire myWire(RTC_DAT, RTC_CLK, RTC_RST);
  RtcDS1302<ThreeWire> Rtc(myWire);
//SD
  #include <SD.h>
  byte counter = 0;
//lcd init
  LiquidCrystal_I2C lcd(0x27, 20, 4);

//Прототипы
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

void writeRow(String* time, float* temperature, float* pressure, int* humidity, int* co2);
void writeHeader();
void drawPlotUp(byte pos, byte row, byte width, byte height, int min_val, int max_val, int fill_val);

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.clear();

  pinMode(SENSOR_PIN, INPUT);
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
  if (!SD.begin(SD_CS)) {
    Serial.println("SD F");
    return;
  }
  Serial.println("SD S");

  mq135.calibrate(60);
  Serial.print("Mq calibrate: ");
  Serial.println(mq135.getRo());
  Serial.println("Mq-135 S");

  writeHeader();
}

uint32_t myTimer1;
static byte state = 0;
void loop() {
  if (digitalRead(3) == HIGH) {
    state++;
    lcd.clear();
    checkStateFirst();
    delay(500);
  }
  if (state >= 3) {
    lcd.clear();
    state = 0;
  }

  switch (state) {
    case 0:
      oneScreen();
      break;
    case 1:
      if (millis() - myTimer1 >= GRAPH_UPDATE_TIME) {
        myTimer1 = millis();
        twoScreen();
      }
      break;
    default:
      if (millis() - myTimer1 >= GRAPH_UPDATE_TIME) {
        myTimer1 = millis();
        threeScreen();
      }
  }

  delay(1000);
}

void checkStateFirst() {
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
}

//ЭКРАН 1
void oneScreen() {
  printHeader();
  
  byte celsia[] = {
    0x1C,
    0x14,
    0x1C,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00
  };
  byte danger[] = {
    0x00,
    0x1B,
    0x0E,
    0x04,
    0x0E,
    0x1B,
    0x00,
    0x00
  };  
  byte accept[] = {
    B00000,
    B00001,
    B00011,
    B10110,
    B11100,
    B01000,
    B00000,
    B00000
  };
  lcd.createChar(0, celsia);
  lcd.createChar(2, danger);
  lcd.createChar(1, accept);

  float temperature = 0;
  getTemperature(&temperature);
  lcd.setCursor(5, 0);
  lcd.print(String(temperature));
  lcd.print('c');
  lcd.write(0);

  int humidity = 0;
  getHumidity(&humidity);
  lcd.setCursor(5, 1);
  lcd.print(String(humidity));
  lcd.print('%');

  int co2 = 0;
  getCo2(&co2);
  lcd.setCursor(5, 2);
  lcd.print(String(co2));
  lcd.print("ppm");
  lcd.setCursor(13, 2);
  
  if(co2 > CO2_NORMAL_LIMIT) {
    lcd.write(2);
  } else {
    lcd.write(1);
  }
  
  float pressure = 0;
  getPressure(&pressure);
  lcd.setCursor(5, 3);
  lcd.print(String(pressure));
  lcd.print("mm");

  String time = "";
  getTime(&time);
  lcd.setCursor(15, 0);
  lcd.print(time);

  counter++;
  if (counter >= WRITE_SD) {
    counter = 0;
    String timestamps = "";
    getTimestamps(&timestamps);
    writeRow(&timestamps, &temperature, &pressure, &humidity, &co2);
  }
}

//ЭКРАН 2
void twoScreen() {
  initPlot();

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
}

//ЭКРАН 3
void threeScreen() {
  initPlot();

  lcd.setCursor(17, 0);
  lcd.print("Co2");
  lcd.setCursor(17, 2);
  lcd.print("Pre");

  int co2 = 0;
  getCo2(&co2);
  drawPlotUp(0, 1, 17, 2, 300, 1000, co2);
  lcd.setCursor(17, 1);
  lcd.print(co2);

  float pressure = 0;
  getPressure(&pressure);
  drawPlotDown(0, 3, 17, 2, 730, 780, pressure);
  lcd.setCursor(17, 3);
  lcd.print(int(pressure));
}

void getTemperature(float* temperature) {
  *temperature = bmp.readTemperature();
}

void getHumidity(int* humidity) {
  *humidity = int(dht.readHumidity());
}

void getCo2(int* co2) {
    *co2 = (int) mq135.readCO2();
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

void getTimestamps(String* time) {
  RtcDateTime now = Rtc.GetDateTime();
  *time += now.Day();
  *time += F("/");
  *time += now.Month();
  *time += F("/");
  *time += now.Year();
  *time += F(" ");
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
  File dataFile = SD.open("LOG.csv", FILE_WRITE);
  if (dataFile) {
    String dataString;
    dataString += F("\nWRITE: ");
    dataString += F("\ntime,temperature,pressure,humidity,co2");
    dataFile.println(dataString);
    dataFile.close();
    Serial.println(dataString);
  } else {
    Serial.println(F("Sd Write F"));
  }
}

void writeRow(String* time, float* temperature, float* pressure, int* humidity, int* co2) {
  File dataFile = SD.open("LOG.csv", FILE_WRITE);

  String tempStr = String(*temperature);
  tempStr.replace(".", ",");

  String presStr = String(int(floor(*pressure)));
  tempStr.replace(".", ",");

  if (dataFile) {
    String dataString;
    dataString += F("\"");
    dataString += *time;
    dataString += F("\"");
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
    dataString += F(",");
    dataString += *co2;

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

  for (byte i = 0; i < width; i++) {  
    byte infill, fract;
    infill = floor((float)(plot_arrayUp[i] - min_val) / (max_val - min_val) * height * 10);
    fract = (infill % 10) * 8 / 10; 
    infill = infill / 10;

    for (byte n = 0; n < height; n++) {  
      if (n < infill && infill > 0) {    
        lcd.setCursor(i, (row - n));     
        lcd.write(0);
      }
      if (n >= infill) {  
        lcd.setCursor(i, (row - n));
        if (fract > 0) lcd.write(fract);         
        else lcd.write(0x20);                    
        for (byte k = n + 1; k < height; k++) {  
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

  for (byte i = 0; i < width; i++) { 
    byte infill, fract;
    infill = floor((float)(plot_arrayDown[i] - min_val) / (max_val - min_val) * height * 10);
    fract = (infill % 10) * 8 / 10;
    infill = infill / 10;

    for (byte n = 0; n < height; n++) {  
      if (n < infill && infill > 0) {    
        lcd.setCursor(i, (row - n));     
        lcd.write(0);
      }
      if (n >= infill) {  
        lcd.setCursor(i, (row - n));
        if (fract > 0) lcd.write(fract);        
        else lcd.write(0x20);                   
        for (byte k = n + 1; k < height; k++) { 
          lcd.setCursor(i, (row - k));
          lcd.write(0x20);
        }
        break;
      }
    }
  }
}