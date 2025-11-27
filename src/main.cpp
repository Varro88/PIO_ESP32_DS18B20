#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <rom/rtc.h>
#include <Preferences.h>

#include <array>

#include "network/alerts.h"
#include "network/wificlient.h"
#include "time.h"

String getResetReason(RESET_REASON reason);
void processAlert();
void processCalibration();
void processDaylight();
void printStatusWithTime(int column, int row, String format);
void printStatus(Status status);
void printStatusTime();
void printDiagnostic(int rowIndex);
void printAndShow(int row, String text);
void switchNetworkIndicator(boolean isShown);
String getTimeString();

const int MAIN_LOOP_DELAY_MS = 20 * 1000;
const int GET_ALERTS_DELAY_MS = 10 * 1000;
const int TOO_MANY_REQUESTS_PAUSE_MS = 2 * 60 * 1000;
const int LCD_ROW_LENGTH = 20;
unsigned long lastGetAlertsMillis = millis() - GET_ALERTS_DELAY_MS;
const String DATETIME_FORMAT = "%A, %B %d %Y %H:%M:%S";
const char *TIME_FORMAT = "%H:%M";

Preferences preferences;
const char *PREFS_NAMESPACE = "generalPrefs";

const char *NTP_SERVER = "ua.pool.ntp.org";
const long GMT_OFFSET_SEC = 7200;
const int DAYLIGHT_OFFSET_SEC = 3600;
int PIN_DAYLIGHT_INVERT = 5;
const char *PREF_DAYLIGHT = "IS_DAYLIGHT";

const int MIN_HOURS = 7;
const int MAX_HOURS = 22;
int hours = 0;

int countSensors;
LiquidCrystal_I2C lcd(0x27, 20, 4);
int counter = 0;
String SHORT_DIAGNOSTIC = "";
Status LAST_STATUS = INIT;
String LAST_VALID_STATUS_TIME = "";
Status LAST_VALID_STATUS = INIT;

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
  printAndShow(2, "Please wait sys init");

  // Diagnostic
  SHORT_DIAGNOSTIC = "Last reset: CPU0=" + getResetReason(rtc_get_reset_reason(0)) + " / CPU1=" + rtc_get_reset_reason(1);

  connectToWiFi();

  preferences.begin(PREFS_NAMESPACE);
  processDaylight();
  preferences.end();

  lcd.noBacklight();
  lcd.clear();
}

void loop() {
  lcd.setCursor(15, 2);
  lcd.print(getTimeString());

  Serial.println(SHORT_DIAGNOSTIC + "; esp32 T=" + (temprature_sens_read() - 32) / 1.8);

  if (millis() >= lastGetAlertsMillis + GET_ALERTS_DELAY_MS) {
    processAlert();
  }

  Serial.println("=====");
  delay(MAIN_LOOP_DELAY_MS);
}

void processDaylight() {
  try {
    pinMode(PIN_DAYLIGHT_INVERT, INPUT);
    int readPinState = digitalRead(PIN_DAYLIGHT_INVERT);
    Serial.print("Daylight pin state: ");
    Serial.println(readPinState);
    bool currentDaylightState = preferences.getBool(PREF_DAYLIGHT, 0);
    Serial.print("Is daylight time: ");
    Serial.println(currentDaylightState);
    if(readPinState == LOW) {
      Serial.println("Inverting daylight time");
      currentDaylightState = !currentDaylightState;
      preferences.putBool(PREF_DAYLIGHT, currentDaylightState);
    }
    Serial.println("ConfigTime params:");
    Serial.println(GMT_OFFSET_SEC);
    Serial.println(currentDaylightState * DAYLIGHT_OFFSET_SEC);
    Serial.println(NTP_SERVER);
    configTime(GMT_OFFSET_SEC, currentDaylightState * DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  } catch (...) {
    Serial.print("Can't set time");
  }
}

void processAlert() {
  switchNetworkIndicator(true);
  Status newStatus = getSimpleAlerts();
  lastGetAlertsMillis = millis();

  Serial.print("Current status: ");
  Serial.println(newStatus);

  // special conditions
  if (newStatus == ALERT_ON) {
    lcd.setBacklight(hours >= MIN_HOURS && hours < MAX_HOURS);
  } else if (newStatus == TOO_MANY_REQUEST) {
    Serial.println("[WARNING] TOO MANY REQUESTS");
    lastGetAlertsMillis += TOO_MANY_REQUESTS_PAUSE_MS;
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
        if (newStatus != LAST_VALID_STATUS) {
          printStatusTime();
        }
        LAST_VALID_STATUS = newStatus;
        break;
      default: break;
    }
    printStatus(newStatus);
    LAST_STATUS = newStatus;
  }
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

void printStatus(Status status) {
  lcd.setCursor(5, 1);
  String strStatus = "";
  switch (status) {
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

void printStatusTime() {
  lcd.setCursor(0, 1);
  String time = getTimeString();
  lcd.print(time);
}

String getResetReason(RESET_REASON reason) {
  switch (reason) {
    case 1:
      return "POWERON_RESET"; /**<1,  Vbat power on reset*/
    case 3:
      return "SW_RESET"; /**<3,  Software reset digital core*/
    case 4:
      return "OWDT_RESET"; /**<4,  Legacy watch dog reset digital core*/
    case 5:
      return "DEEPSLEEP_RESET"; /**<5,  Deep Sleep reset digital core*/
    case 6:
      return "SDIO_RESET"; /**<6,  Reset by SLC module, reset digital core*/
    case 7:
      return "TG0WDT_SYS_RESET"; /**<7,  Timer Group0 Watch dog reset digital core*/
    case 8:
      return "TG1WDT_SYS_RESET"; /**<8,  Timer Group1 Watch dog reset digital core*/
    case 9:
      return "RTCWDT_SYS_RESET"; /**<9,  RTC Watch dog Reset digital core*/
    case 10:
      return "INTRUSION_RESET"; /**<10, Instrusion tested to reset CPU*/
    case 11:
      return "TGWDT_CPU_RESET"; /**<11, Time Group reset CPU*/
    case 12:
      return "SW_CPU_RESET"; /**<12, Software reset CPU*/
    case 13:
      return "RTCWDT_CPU_RESET"; /**<13, RTC Watch dog Reset CPU*/
    case 14:
      return "EXT_CPU_RESET"; /**<14, for APP CPU, reseted by PRO CPU*/
    case 15:
      return "RTCWDT_BROWN_OUT_RESET"; /**<15, Reset when the vdd voltage is not stable*/
    case 16:
      return "RTCWDT_RTC_RESET"; /**<16, RTC Watch dog reset digital core and rtc module*/
    default:
      return "N/A";
  }
}

void printAndShow(int row, String text) {
  Serial.println(text);
  lcd.setCursor(0, row);
  lcd.print(text);
}
