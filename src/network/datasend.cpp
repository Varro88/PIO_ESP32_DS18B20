#include <ArduinoJson.h>
#include <network/datasend.h>
#include <HTTPClient.h>
#include <network/wificlient.h>

#include "network/webclient.h"

// All sensitive data is here in format `#define WIFI_SSID "MyHomeWiFi"`
#include <secrets.h>

int sendMeteoData(DynamicJsonDocument jsonData) {
  if (!connectIfNotConnected()) {
    return -1;
  }

  if (WiFi.status() == WL_CONNECTED) {
    String jsonString;
    serializeJson(jsonData, jsonString);
    HTTPClient http;
    http.begin(DATA_URL);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(jsonString);
    Serial.print("JSON sending HTTP code: ");
    Serial.println(httpResponseCode);
    Serial.print("Send meteo response: ");
    Serial.println(http.getString());
    http.end();
    return httpResponseCode;
  } else {
    Serial.print("Failed to connect to WiFi. Status is: ");
    Serial.println(WiFi.status());
    return -1;
  }
}