#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

int sendDataToServer(DynamicJsonDocument jsonData);
void connectToWiFi();
