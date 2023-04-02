#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

void connectToWiFi();
int sendMeteoData(DynamicJsonDocument jsonData);
bool getAlerts();
