#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

enum Status {
    INIT,
    GET_ALERTS_FAILED,
    NO_ALERT,
    ALERT_ON,
    TOO_MANY_REQUEST
};

void connectToWiFi();
int sendMeteoData(DynamicJsonDocument jsonData);
Status getAlerts();
