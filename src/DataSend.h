#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

enum Status {
    INIT,
    NO_ALERT,
    ALERT_ON,
    PARTIAL_ALERT,
    TOO_MANY_REQUEST,
    WIFI_FAILED,
    CONNECTION_FAILED
};

void connectToWiFi();
int sendMeteoData(DynamicJsonDocument jsonData);
Status getAlerts();
