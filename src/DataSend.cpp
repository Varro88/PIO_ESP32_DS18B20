#include <ArduinoJson.h>
#include <datasend.h>
#include <HTTPClient.h>
#include <wificlient.h>

#include "webclient.h"

// All sensitive data is here in format `#define WIFI_SSID "MyHomeWiFi"`
#include <secrets.h>

#define ALL_OK 0;

const String TARGET_REGION_INDEX_STR = "22";
const String TARGET_CITY_INDEX_STR = "6293";
const String REGION_STR = "Харківська область";

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

Status getAlertsV2() {
  if (!connectIfNotConnected()) {
    return WIFI_FAILED;
  }
  String bearerToken = "Bearer " ALERTS_TOKEN;

  std::map<String, String> headers = {{"Host", "api.alerts.in.ua"},
                                      {"Authorization", bearerToken}};

  HttpResponse response = sendGetRequest(String(ALERTS_ACTIVE_URL), headers);
  Serial.print("Alerts HTTP code: ");
  Serial.println(response.statusCode);

  if (response.statusCode == -1) {
    Serial.println("[WARNING] Network failed. No internet or alerts host is not accessible.");
    return CONNECTION_FAILED;
  }

  if (response.statusCode == 429) {
    Serial.println("[WARNING] TOO MANY REQUESTS");
    Serial.println(response.responseBody);
    return TOO_MANY_REQUEST;
  }

  Serial.print("Alerts response length: ");
  Serial.println(response.responseBody.length());
  DynamicJsonDocument jsonHolder(response.responseBody.length());

  if (!stringToJson(jsonHolder, response.responseBody)) {
    Serial.println("Failed to convert response to JSON");
    return RESPONSE_FAILED;
  }

  JsonObject obj = jsonHolder.as<JsonObject>();

  bool regionAlert = false;
  bool districtAlert = false;
  if (obj.containsKey("alerts")) {
    JsonArray alerts = obj["alerts"].as<JsonArray>();

    Serial.println("\nLocation UIDs in alerts:");
    for (JsonObject alert : alerts) {
      if (alert.containsKey("location_uid")) {
        String locId = alert["location_uid"].as<String>();
        Serial.print("Location id: ");
        Serial.println(locId);

        if (locId == TARGET_CITY_INDEX_STR) {
          Serial.println("CITY_ALERT");
          return ALERT_ON;
        } else if (locId == TARGET_REGION_INDEX_STR) {
          Serial.println("REGION_ALERT");
          regionAlert = true;
          continue;
        } else {
          const char* location_oblast = alert["location_oblast"];
          if (location_oblast && String(location_oblast) == REGION_STR) {
            Serial.println("District in region alert");
            districtAlert = true;
          }
        }
      }
    }
  }
  else {
    Serial.println("No 'alerts' item in response");
    return RESPONSE_FAILED;
  }

  if (regionAlert) {
    return REGION_ALERT;
  }
  if (districtAlert) {
    return DISTRICT_ALERT;
  } else {
    return NO_ALERT;
  }
}
