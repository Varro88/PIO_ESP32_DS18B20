#include <ArduinoJson.h>
#include <network/datasend.h>
#include <HTTPClient.h>
#include <network/wificlient.h>

#include "network/webclient.h"
#include "../logger.h"

// All sensitive data is here in format `#define WIFI_SSID "MyHomeWiFi"`
#include <secrets.h>

void sendMeteoData(DynamicJsonDocument jsonData) {
  if (!connectIfNotConnected()) {
    logError((String)"Wi-Fi connection failed. Status is: " + WiFi.status());
  }

  if (WiFi.status() == WL_CONNECTED) {
    String jsonString;
    serializeJson(jsonData, jsonString);
    logInfo((String)"Send meteo request: " + jsonString);
    std::map<String, String> headers;
    headers["Content-Type"] = "application/json";
    sendPostRequest(DATA_URL, headers, jsonString);
  } else {
    logWarn((String)"Not connected to WiFi. Status is: " + WiFi.status());
  }
}