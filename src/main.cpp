#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <array>
#include "time.h"
#include <rom/rtc.h>
#include "DataSend.h"

String getResetReason(RESET_REASON reason);
std::array<float, 3> getBME280Measurings();
void printMeteoData(String t1, String t2, String h, String p);
void printLocalTime();
void printDiagnostic(int rowIndex);
void printAndShow(int row, String text);

#define ONE_WIRE_BUS 19
#define SEALEVELPRESSURE_HPA (1013.25)
const float kPaToMmHg = 1.33322;
const int MAIN_LOOP_DELAY_MS = 10*1000;
const int SEND_TO_SERVER_DELAY_MS = 3*60*1000;
const int GET_ALERTS_DELAY_MS = 45*1000;
unsigned long lastSendMillis = millis() - SEND_TO_SERVER_DELAY_MS;
unsigned long lastGetAlertsMillis = millis() - GET_ALERTS_DELAY_MS;
const String LCD_DEGREES = "\xDF";
const String DATETIME_FORMAT = "%A, %B %d %Y %H:%M:%S";

const char* ntpServer = "ua.pool.ntp.org";
const long gmtOffset_sec = 7200;
const int daylight_offset_sec = 3600;
const int daylight_enabled = 1;

const int MIN_HOURS = 7;
const int MAX_HOURS = 22;

OneWire dsWire(ONE_WIRE_BUS);
DallasTemperature ds18B20(&dsWire);
DeviceAddress *sensorsUnique;
int countSensors;
LiquidCrystal_I2C lcd(0x27,20,4);
int counter = 0;
int bme280Address = 0x76;
Adafruit_BME280 bme;
String SHORT_DIAGNOSTIC = "";

const uint8_t number[] = {
  0xFF, 0x00, 0xFF, 0xFF, 0x01, 0xFF, //0
  0x00, 0xFF, 0x20, 0x01, 0xFF, 0x01, //1
  0x00, 0x00, 0xFF, 0xFF, 0x02, 0x01, //2
  0x00, 0x02, 0xFF, 0x01, 0x01, 0xFF, //3
  0xFF, 0x20, 0xFF, 0x00, 0x00, 0xFF, //4
  0xFF, 0x02, 0x02, 0x01, 0x01, 0xFF, //5
  0xFF, 0x02, 0x02, 0xFF, 0x01, 0xFF, //6
  0x00, 0x00, 0xFF, 0x20, 0x20, 0xFF, //7
  0xFF, 0x02, 0xFF, 0xFF, 0x01, 0xFF, //8
  0xFF, 0x02, 0xFF, 0x01, 0x01, 0xFF, //9
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20 //' '
};

#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();

void setup() {
  Wire.begin();
  ds18B20.begin();
  Serial.begin(115200);
  
  //Diagnostic
  SHORT_DIAGNOSTIC = "Last reset: CPU0=" + getResetReason(rtc_get_reset_reason(0)) + " / CPU1=" + rtc_get_reset_reason(1);
  
  //BME280
  bool status;    
  status = bme.begin(0x76);  
  if (!status) {
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      while (1);
  }
  lcd.init();
  lcd.backlight();
  printAndShow(2, "Please wait sys init");
  connectToWiFi();

  try
  {
    configTime(gmtOffset_sec, daylight_enabled * daylight_offset_sec, ntpServer);
  }
  catch (...)
  {
    Serial.print("Can't set time");
  }

  lcd.noBacklight();
}
 
void loop() {
  int hours = 0;
  //Time
  struct tm timeinfo;
  lcd.setCursor(7, 1);
  if(getLocalTime(&timeinfo))
  {
    hours = timeinfo.tm_hour;
    lcd.print(&timeinfo, "%H:%M");
    Serial.println(&timeinfo, DATETIME_FORMAT.c_str());
  }
  else {
    Serial.println("Failed to get time");
    lcd.print("--:--");
  }

  //DS18B20
  ds18B20.requestTemperatures();
  float ds18b20Temperature = NAN;
  ds18b20Temperature = ds18B20.getTempCByIndex(0);
  if(ds18b20Temperature < -50) {
    Serial.print("DS18B20 out sensor error");
    ds18b20Temperature = NAN;
  }

  //BME280
  std::array<float, 3> bme280 = getBME280Measurings();
  printMeteoData( "T0=" + String(ds18b20Temperature, 1) + LCD_DEGREES + "C",
                  "T1=" + String(bme280[0], 1) + LCD_DEGREES + "C",
                  "H=" + String(bme280[1], 1) + "%",
                  "P=" + String(bme280[2] / 1.33322, 1) + "mm");
  DynamicJsonDocument jsonData(1024);
  jsonData["temp_out"] = ds18b20Temperature;
  jsonData["temp_in"] = bme280[0];
  jsonData["humidity"] = bme280[1];
  jsonData["pressure"] = bme280[2];

  Serial.println(SHORT_DIAGNOSTIC + "; esp32 T = " + (temprature_sens_read() - 32) / 1.8);

  if(millis() >= lastSendMillis + SEND_TO_SERVER_DELAY_MS) {
    //sendMeteoData(jsonData);
    lastSendMillis = millis();
  }

  if(millis() >= lastGetAlertsMillis + GET_ALERTS_DELAY_MS) {
    lastGetAlertsMillis = millis();
    lcd.setCursor(15, 0);
    if(getAlerts()) {
      lcd.setBacklight(hours >= MIN_HOURS && hours < MAX_HOURS);
      lcd.print("ALERT");
    }
    else {
      lcd.noBacklight();
      lcd.print("relax");
    }
    lastGetAlertsMillis = millis();
  }

  printAndShow(0, String(xPortGetFreeHeapSize()));
  
  Serial.println("=====");
  delay(MAIN_LOOP_DELAY_MS);
}

String getResetReason(RESET_REASON reason)
{
  switch (reason)
  {
    case 1 : return "POWERON_RESET";          /**<1,  Vbat power on reset*/
    case 3 : return "SW_RESET";               /**<3,  Software reset digital core*/
    case 4 : return "OWDT_RESET";             /**<4,  Legacy watch dog reset digital core*/
    case 5 : return "DEEPSLEEP_RESET";        /**<5,  Deep Sleep reset digital core*/
    case 6 : return "SDIO_RESET";            /**<6,  Reset by SLC module, reset digital core*/
    case 7 : return "TG0WDT_SYS_RESET";       /**<7,  Timer Group0 Watch dog reset digital core*/
    case 8 : return "TG1WDT_SYS_RESET";       /**<8,  Timer Group1 Watch dog reset digital core*/
    case 9 : return "RTCWDT_SYS_RESET";       /**<9,  RTC Watch dog Reset digital core*/
    case 10 : return "INTRUSION_RESET";       /**<10, Instrusion tested to reset CPU*/
    case 11 : return "TGWDT_CPU_RESET";       /**<11, Time Group reset CPU*/
    case 12 : return "SW_CPU_RESET";          /**<12, Software reset CPU*/
    case 13 : return "RTCWDT_CPU_RESET";      /**<13, RTC Watch dog Reset CPU*/
    case 14 : return "EXT_CPU_RESET";         /**<14, for APP CPU, reseted by PRO CPU*/
    case 15 : return "RTCWDT_BROWN_OUT_RESET";/**<15, Reset when the vdd voltage is not stable*/
    case 16 : return "RTCWDT_RTC_RESET";      /**<16, RTC Watch dog reset digital core and rtc module*/
    default : return "N/A";
  }
}

std::array<float, 3> getBME280Measurings()
{
  std::array<float, 3> measurings;
  try {
    measurings[0] = bme.readTemperature();
  }
  catch (...) {
    measurings[0] = NAN;
  }

  try {
    measurings[1] = bme.readHumidity();
  }
  catch (...) {
    measurings[1] = NAN;
  }

  try {
    measurings[2] = bme.readPressure() / 100.0F;
  }
  catch (...) {
    measurings[2] = NAN;
  }
  return measurings;
}

void printMeteoData(String t0, String t1, String h, String p)
{
  int halfLineSize = 10;

  Serial.println(t0);
  lcd.setCursor(0,2);
  while(t0.length() < halfLineSize) {
    t0 += " ";
  }
  lcd.print(t0);

  Serial.println(t1);
  lcd.setCursor(0,3);
  while(t1.length() != halfLineSize) {
    t1 += " ";
  }
  lcd.print(t1);

  Serial.println(p);
  lcd.setCursor(10, 2);
  while(p.length() != halfLineSize) {
    p += " ";
  }
  lcd.print(p);

  Serial.println(h);
  lcd.setCursor(10, 3);
  while(h.length() != halfLineSize) {
    h += " ";
  }
  lcd.print(h);
}

void printAndShow(int row, String text) 
{
  Serial.println(text);
  lcd.setCursor(0,row);
  lcd.print(text);
}
