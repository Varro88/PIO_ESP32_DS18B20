#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

enum Status {
    INIT,
    NO_ALERT,
    ALERT_ON,
    REGION_ALERT,
    DISTRICT_ALERT,
    TOO_MANY_REQUEST,
    WIFI_FAILED,
    CONNECTION_FAILED,
    RESPONSE_FAILED
};

void connectToWiFi();
int sendMeteoData(DynamicJsonDocument jsonData);
Status getAlerts();
Status getAlertsV2();
