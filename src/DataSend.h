#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

const int GET_ALERTS_FAILED = -1;
const int NO_ALERTS = 0;
const int ALERT_ON = 1;
const int TOO_MANY_REQUEST = 429;

void connectToWiFi();
int sendMeteoData(DynamicJsonDocument jsonData);
bool getAlerts();
