#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <Arduino.h>
//#include <LiquidCrystal_I2C.h>
#include <LCDI2C_Multilingual.h>
#include <rom/rtc.h>
#include <sensors/BME280.h>
#include <sensors/DS18B20.h>
#include <sensors/MHZ19B.h>
#include <Preferences.h>

#include <array>

#include "network/datasend.h"
#include "network/alerts.h"
#include "network/wificlient.h"
#include "logger.h"
#include "time.h"

String getResetReason(RESET_REASON reason);
void processSimpleAlert();
void processAlert();
void processCalibration();
void processDaylight();
void printMeteoData(String t1, String t2, String h, String p, String c);
void printStatusWithTime(int column, int row, String format);
void printStatus(AlertStatus status);
void printStatusTime(String time);
void printAndShow(int row, String text);
void switchNetworkIndicator(boolean isShown);
void printAndShowAsLog(String text);
String getTimeString();

#define SEALEVELPRESSURE_HPA (1013.25)
const float kPaToMmHg = 1.33322;
//const int MAIN_LOOP_DELAY_MS = 15 * 1000;
const int GET_ALERTS_DELAY_MS = 10 * 1000;
const int SEND_TO_SERVER_PERIOD_MS = 5 * 60 * 1000;
const int TOO_MANY_REQUESTS_PAUSE_MS = 2 * 60 * 1000;
const int LCD_ROW_LENGTH = 20;
unsigned long lastSendMs = 0;
unsigned long lastGetAlertsMs = 0;
const String LCD_DEGREES = "\xDF";
const String DATETIME_FORMAT = "%A, %B %d %Y %H:%M:%S";
const char *TIME_FORMAT = "%H:%M";

Preferences preferences;
const char *PREFS_NAMESPACE = "generalPrefs";

String lcdLog[4] = {"", "", "", ""};

const char *NTP_SERVER = "ua.pool.ntp.org";
const long GMT_OFFSET_SEC = 7200;
const int DAYLIGHT_OFFSET_SEC = 3600;
int PIN_DAYLIGHT_INVERT = 5;
const char *PREF_DAYLIGHT = "IS_DAYLIGHT";

const char *PREF_CALIB = "ALLOW_CALIB";
int PIN_CALIBRATION_CO2 = 4;

const int MIN_HOURS = 7;
const int MAX_HOURS = 22;
int hours = 0;

int countSensors;
LCDI2C_Russian lcd(0x27, 20, 4);
int counter = 0;
String SHORT_DIAGNOSTIC = "";
Status LAST_STATUS = INIT;
Status LAST_VALID_STATUS = INIT;
String LAST_VALID_STATUS_TIME = "";

bool HAD_ALERT = false;
String LAST_ALERT_CHANGE_TIME = "--:--";

const uint8_t number[] = {
    0xFF, 0x00, 0xFF, 0xFF, 0x01, 0xFF,  // 0
    0x00, 0xFF, 0x20, 0x01, 0xFF, 0x01,  // 1
    0x00, 0x00, 0xFF, 0xFF, 0x02, 0x01,  // 2
    0x00, 0x02, 0xFF, 0x01, 0x01, 0xFF,  // 3
    0xFF, 0x20, 0xFF, 0x00, 0x00, 0xFF,  // 4
    0xFF, 0x02, 0x02, 0x01, 0x01, 0xFF,  // 5
    0xFF, 0x02, 0x02, 0xFF, 0x01, 0xFF,  // 6
    0x00, 0x00, 0xFF, 0x20, 0x20, 0xFF,  // 7
    0xFF, 0x02, 0xFF, 0xFF, 0x01, 0xFF,  // 8
    0xFF, 0x02, 0xFF, 0x01, 0x01, 0xFF,  // 9
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20   //' '
};

byte transferIndicator[] = {B11111, B11111, B11111, B11111,
                            B11111, B11111, B11111, B11111};

#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();

void setup() {
  Serial.begin(115200);

  //LCD
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, transferIndicator);
  printAndShowAsLog("Please wait sys init");

  // Sensors
  initDS18B20();
  initBME280();
  initMHZ19B();

  // Diagnostic
  SHORT_DIAGNOSTIC = "Last reset: CPU0=" + getResetReason(rtc_get_reset_reason(0)) + " / CPU1=" + rtc_get_reset_reason(1);

  connectToWiFi();

  preferences.begin(PREFS_NAMESPACE);
  processCalibration();
  processDaylight();
  preferences.end();

  lcd.noBacklight();

  lcd.clear();

  //testHttpRequests(10000);
}

void loop() {
  lcd.setCursor(0, 1);
  lcd.print(getTimeString());

  // DS18B20
  float outerTemperature = getOuterDS18B20Value();
  float innerTemperature = getInnerDS18B20Value();
  logInfo("DS18B20:");
  logInfo((String)outerTemperature);
  logInfo((String)innerTemperature);
  logInfo("---");

  // BME280
  std::array<float, 3> bme280 = getBME280Measurings();
  logInfo("BME280:");
  logInfo((String)bme280[0]);
  logInfo((String)bme280[1]);
  logInfo((String)bme280[2]);
  logInfo("---");

  // MHZ19B
  float co2Concentration = getCO2Concentration();

  printMeteoData("T0=" + String(outerTemperature, 1) + LCD_DEGREES + "C",
                 "T1=" + String(innerTemperature, 1) + LCD_DEGREES + "C",
                 "H=" + String(bme280[1], 1) + "%",
                 "P=" + String(bme280[2] / 1.33322, 0) + "mm",
                 "C=" + String(co2Concentration, 0) +  "ppm");

  logInfo((String)SHORT_DIAGNOSTIC + "; esp32 T=" + (temprature_sens_read() - 32) / 1.8);

  if(lastSendMs == 0 ||millis() >= lastSendMs + SEND_TO_SERVER_PERIOD_MS) {
    DynamicJsonDocument jsonData(128);
    jsonData["tempIn"] = innerTemperature;
    jsonData["tempOut"] = outerTemperature;
    jsonData["humidity"] = bme280[1];
    jsonData["pressure"] = bme280[2];
    jsonData["co2"] = co2Concentration;
    sendMeteoData(jsonData);
    lastSendMs = millis();
  }

  //processSimpleAlert();
  processAlert();

  logInfo("=====");
  delay(GET_ALERTS_DELAY_MS);
}

void processCalibration() {
  preferences.putBool(PREF_CALIB, false);

  bool calibrationAllowed = preferences.getBool(PREF_CALIB, false);
  printAndShowAsLog("Is calib allowed:");
  printAndShowAsLog(String(calibrationAllowed));
  int calibrationPinState = digitalRead(PIN_CALIBRATION_CO2);
  printAndShowAsLog("Calib pin state:");
  printAndShowAsLog(String(calibrationPinState));
  if(calibrationAllowed && calibrationPinState == HIGH) {
    printAndShowAsLog("Entering CO2 calib");
    preferences.putBool(PREF_CALIB, false);
    printAndShowAsLog("CO2 calib in 1 min");
    delay(30*1000);
    printAndShowAsLog("CO2 calib in 30 seconds");
    delay(30*1000);
    printAndShowAsLog("CO2 CALIB STARTED");
    calibrate();
    preferences.putBool(PREF_CALIB, false);
  }
  //setAutocalibration(false);
}

void processDaylight() {
  try {
    pinMode(PIN_DAYLIGHT_INVERT, INPUT);
    int readPinState = digitalRead(PIN_DAYLIGHT_INVERT);
    printAndShowAsLog("Daylight pin state:");
    printAndShowAsLog(String(readPinState));
    bool currentDaylightState = preferences.getBool(PREF_DAYLIGHT, 0);
    printAndShowAsLog("Is daylight time:");
    printAndShowAsLog(String(currentDaylightState));
    if(readPinState == LOW) {
      printAndShowAsLog("Inverting daylight time");
      currentDaylightState = !currentDaylightState;
      preferences.putBool(PREF_DAYLIGHT, currentDaylightState);
    }
    printAndShowAsLog("ConfigTime params:");
    printAndShowAsLog(String(GMT_OFFSET_SEC));
    printAndShowAsLog(String(currentDaylightState * DAYLIGHT_OFFSET_SEC));
    printAndShowAsLog(String(NTP_SERVER));
    configTime(GMT_OFFSET_SEC, currentDaylightState * DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  } catch (...) {
    printAndShowAsLog("Can't set time");
  }
}

void processSimpleAlert() {
  switchNetworkIndicator(true);
  AlertStatus alertStatus = getSimpleAlerts();
  Status newStatus = alertStatus.status;
  lastGetAlertsMs = millis();

  logInfo("New status: ");
  logInfo(String(newStatus));

  // special conditions
  if (newStatus == ALERT_ON) {
    lcd.setBacklight(hours >= MIN_HOURS && hours < MAX_HOURS);
  } else if (newStatus == TOO_MANY_REQUEST) {
    logInfo("[WARNING] TOO MANY REQUESTS");
    lastGetAlertsMs += TOO_MANY_REQUESTS_PAUSE_MS;
  } else {
    lcd.noBacklight();
  }

  lcd.setCursor(8, 0);
  // Only in case of changes
  if (newStatus != LAST_STATUS) {
    switch (newStatus) {
      case ALERT_ON:
      case REGION_ALERT:
      case DISTRICT_ALERT:
      case NO_ALERT:
        if (newStatus != LAST_VALID_STATUS && LAST_VALID_STATUS != INIT) {
          printStatusTime(getTimeString());
        }
        else {
          printStatusTime("--:--");
        }
        LAST_VALID_STATUS = newStatus;
        printStatus(alertStatus);
        break;
      default: String respCode = String(alertStatus.responseCode); 
        printStatus(alertStatus);
        break;
    }
    
    LAST_STATUS = newStatus;
  }
  switchNetworkIndicator(false);
}

void processAlert() {
  switchNetworkIndicator(true);
  AlertReason alertReason = getAlertReason();
  String text = "";
  
  if(!alertReason.isValid) {
    logInfo("Alert is not valid");
    if(hours < MIN_HOURS || hours >= MAX_HOURS) {
      lcd.noBacklight();
    }
    if(alertReason.responseCode == 429) {
      logInfo("[WARNING] TOO MANY REQUESTS");
      lastGetAlertsMs += TOO_MANY_REQUESTS_PAUSE_MS;
    }
    text = (String)"E." + alertReason.reason;
  }
  else if(alertReason.isAlert) {
    logInfo("Alert is active");
    logInfo((String)"Hours: " + (String)hours);
    if(hours >= MIN_HOURS && hours < MAX_HOURS) {
      lcd.backlight();
    } else {
      lcd.noBacklight();
    }
    if(!HAD_ALERT) {
      printStatusTime(getTimeString());
    }
    HAD_ALERT = true;
    text = alertReason.reason;
  } else {
    logInfo("No alert");
    lcd.noBacklight();
    if(HAD_ALERT) {
      printStatusTime(getTimeString());
    }
    HAD_ALERT = false;
    text = alertReason.reason;
  }
  int statusPosition = 8;
  for(int i = statusPosition + text.length(); i < LCD_ROW_LENGTH - 1 - 1; i++) {
    text = text + " ";
  }
  //logInfo((String)"Reason length: " + (String)text.length());
  //logInfo((String)"Reason: '" + text + "'");

  Serial.println("Reason length: " + text.length());
  Serial.println("Reason: '" + text + "'");
  //logInfo((String)"Reason: '" + text + "'");

  lcd.setCursor(statusPosition, 0);
  lcd.print(text);

  switchNetworkIndicator(false);
}

void switchNetworkIndicator(boolean isShown) {
  lcd.setCursor(19, 0);
  if (isShown) {
    lcd.write(0);
  } else {
    lcd.print(" ");
  }
}

String getTimeString() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    hours = timeinfo.tm_hour;
    char buffer[6];
    strftime(buffer, sizeof(buffer), TIME_FORMAT, &timeinfo);
    String s = String(buffer);
    return s;
  } else {
    return String("--:--");
  }
}

void printStatus(AlertStatus alertStatus) {
  lcd.setCursor(5, 0);
  String strStatus = "";
  switch (alertStatus.status) {
    case ALERT_ON:
      strStatus = " - ALERT";
      break;
    case REGION_ALERT:
        strStatus = " - REG ALERT";
        break;
    case DISTRICT_ALERT:
      strStatus = " - dist alert";
      break;
    case WIFI_FAILED:
      strStatus = " - ERR_WIFI";
      break;
    case CONNECTION_FAILED:
      strStatus = " - ERR_CONN";
      break;
    case TOO_MANY_REQUEST:
      strStatus = " - 429";
      break;
    case NO_ALERT:
      strStatus = " - relax";
      break;
    case ERR_400: strStatus = " - ERR_400"; break;
    case ERR_401: strStatus = " - ERR_401"; break;
    case ERR_402: strStatus = " - ERR_402"; break;
    case ERR_403: strStatus = " - ERR_403"; break;
    case ERR_404: strStatus = " - ERR_404"; break;
    case ERR_409: strStatus = " - ERR_409"; break;
    case ERR_502: strStatus = " - ERR_502"; break;
    case ERR_503: strStatus = " - ERR_503"; break;
    case RESPONSE_CODE_FAILED:
      strStatus = String(alertStatus.responseCode);
      break;
    case RESPONSE_BODY_FAILED:
      strStatus = " - ERR_RBODY";
      break;
    default:
      strStatus = " - ???";
      break;
  }
  // chars for status = lcd length - time length - status length - transmitting
  // char at the end
  int spacesNum = LCD_ROW_LENGTH - 5 - strStatus.length() - 1;
  for (int i = 0; i < spacesNum; i++) {
    strStatus += " ";
  }
  lcd.print(strStatus);
}

void printStatusTime(String time) {
  lcd.setCursor(0, 0);
  lcd.print((String)time + " - ");
}

String getResetReason(RESET_REASON reason) {
  switch (reason) {
    case 1: return "POWERON_RESET"; /**<1,  Vbat power on reset*/
    case 3: return "SW_RESET"; /**<3,  Software reset digital core*/
    case 4: return "OWDT_RESET"; /**<4,  Legacy watch dog reset digital core*/
    case 5: return "DEEPSLEEP_RESET"; /**<5,  Deep Sleep reset digital core*/
    case 6: return "SDIO_RESET"; /**<6,  Reset by SLC module, reset digital core*/
    case 7: return "TG0WDT_SYS_RESET"; /**<7,  Timer Group0 Watch dog reset digital core*/
    case 8:  return "TG1WDT_SYS_RESET"; /**<8,  Timer Group1 Watch dog reset digital core*/
    case 9: return "RTCWDT_SYS_RESET"; /**<9,  RTC Watch dog Reset digital core*/
    case 10: return "INTRUSION_RESET"; /**<10, Instrusion tested to reset CPU*/
    case 11: return "TGWDT_CPU_RESET"; /**<11, Time Group reset CPU*/
    case 12: return "SW_CPU_RESET"; /**<12, Software reset CPU*/
    case 13: return "RTCWDT_CPU_RESET"; /**<13, RTC Watch dog Reset CPU*/
    case 14: return "EXT_CPU_RESET"; /**<14, for APP CPU, reseted by PRO CPU*/
    case 15: return "RTCWDT_BROWN_OUT_RESET"; /**<15, Reset when the vdd voltage is not stable*/
    case 16: return "RTCWDT_RTC_RESET"; /**<16, RTC Watch dog reset digital core and rtc module*/
    default: return "N/A";
  }
}

void printMeteoData(String t0, String t1, String h, String p, String c) {
  int halfLineSize = 10;

  logInfo(t0);
  lcd.setCursor(0, 2);
  while (t0.length() < halfLineSize) {
    t0 += " ";
  }
  lcd.print(t0);

  logInfo(t1);
  lcd.setCursor(0, 3);
  while (t1.length() != halfLineSize) {
    t1 += " ";
  }
  lcd.print(t1);

  logInfo(p);
  lcd.setCursor(10, 2);
  while (p.length() != halfLineSize) {
    p += " ";
  }
  lcd.print(p);

  logInfo(h);
  lcd.setCursor(10, 3);
  while (h.length() != halfLineSize) {
    h += " ";
  }
  lcd.print(h);

  lcd.setCursor(10, 1);
  while (c.length() != halfLineSize) {
    c += " ";
  }
  lcd.print(c);
}

void printAndShow(int row, String text) {
  logInfo(text);
  lcd.setCursor(0, row);
  lcd.print(text);
}

void printAndShowAsLog(String text) {
  logInfo(text);

  lcdLog[0] = lcdLog[1];
  lcdLog[1] = lcdLog[2];
  lcdLog[2] = lcdLog[3];
  lcdLog[3] = text;

  for (int i = 0; i < 4; i++) {
    lcd.setCursor(0, i);
    lcd.print(lcdLog[i]);
    while (lcdLog[i].length() <= LCD_ROW_LENGTH) {
      lcdLog[i] += " ";
    }
    delay(750);
  }
}
