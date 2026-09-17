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
    RESPONSE_CODE_FAILED,
    ERR_400,
    ERR_401,
    ERR_402,
    ERR_403,
    ERR_404,
    ERR_409,
    ERR_502,
    ERR_503,
    RESPONSE_BODY_FAILED
};

Status getAlertsV2();
Status getSimpleAlerts();