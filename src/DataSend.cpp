#include <ArduinoJson.h>
#include <DataSend.h>
#include <HTTPClient.h>
#include <wifi.h>
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

Status getAlerts() {
  if (!connectIfNotConnected()) {
    return WIFI_FAILED;
  }

  HTTPClient http;
  String targetUrl = String(ALERTS_REGION_IOT_URL);
  targetUrl.replace("<id>", TARGET_REGION_INDEX_STR);
  http.begin(targetUrl);
  String bearerToken = "Bearer " ALERTS_TOKEN;
  http.addHeader("Authorization", bearerToken);
  http.addHeader("Host", "api.alerts.in.ua");
  int httpResponseCode = http.GET();
  Serial.print("Alerts HTTP code: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode == -1) {
    Serial.println(
        "[WARNING] Network request failed. No internet or alerts host is not "
        "accessible.");
    http.end();
    return CONNECTION_FAILED;
  }

  if (httpResponseCode == 429) {
    Serial.println("[WARNING] TOO MANY REQUESTS");
    Serial.println(http.getString());
    http.end();
    return TOO_MANY_REQUEST;
  }

  String response = http.getString();
  http.end();
  Serial.print("Alerts response: ");
  Serial.println(response);

  response.replace("\"", "");

  // char status = response[TARGET_REGION_INDEX];
  char status = response[0];
  if (status == 'A') {
    return ALERT_ON;
  } else if (status == 'P') {
    return DISTRICT_ALERT;
  } else {
    return NO_ALERT;
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

  //Serial.print("Alerts v2 response: ");
  //Serial.println(response.responseBody);

  String resp = "{\"alerts\":[{\"id\":8757,\"location_title\":\"Луганська область\",\"location_type\":\"oblast\",\"started_at\":\"2022-04-04T16:45:39.000Z\",\"finished_at\":null,\"updated_at\":\"2023-10-29T18:22:37.357Z\",\"alert_type\":\"air_raid\",\"location_oblast\":\"Луганська область\",\"location_uid\":\"16\",\"notes\":null,\"country\":null,\"deleted_at\":null,\"calculated\":null,\"location_oblast_uid\":16},{\"id\":28288,\"location_title\":\"Автономна Республіка Крим\",\"location_type\":\"oblast\",\"started_at\":\"2022-12-10T22:22:00.000Z\",\"finished_at\":null,\"updated_at\":\"2023-10-29T16:56:12.340Z\",\"alert_type\":\"air_raid\",\"location_oblast\":\"Автономна Республіка Крим\",\"location_uid\":\"29\",\"notes\":\"Згідно інформації з Офіційних карт тривог\",\"country\":null,\"deleted_at\":null,\"calculated\":null,\"location_oblast_uid\":29}],\"meta\":{\"last_updated_at\":\"2025/03/11 19:23:10 +0000\",\"type\":\"full\"},\"disclaimer\":\"If you use python try our official alerts_in_ua PiP package.\"}";

  DynamicJsonDocument doc(2048);
    
    // Parse JSON string into DynamicJsonDocument
    DeserializationError error = deserializeJson(doc, resp);
    
    // Check for errors
    if (error) {
        Serial.print("Failed to parse JSON: ");
        Serial.println(error.c_str());
        return RESPONSE_FAILED;
    }

    // Extract JSON object
    JsonObject obj = doc.as<JsonObject>();
    
    // Iterate through top-level keys and print them
    Serial.println("Top-level keys in JSON:");
    for (JsonPair kv : obj) {
        Serial.println(kv.key().c_str());
    }

    // Check if "alerts" array exists
    if (obj.containsKey("alerts")) {
        JsonArray alerts = obj["alerts"].as<JsonArray>();
        
        Serial.println("\nLocation UIDs in alerts:");
        for (JsonObject alert : alerts) {
            if (alert.containsKey("location_uid")) {
                Serial.println(alert["location_uid"].as<String>());
            }
        }
    }


  //DynamicJsonDocument doc(response.responseBody.length() * 2);
  DynamicJsonDocument jsonHolder(32768);
  if(!stringToJson(jsonHolder, resp)) {
    Serial.println("Failed to convert response to JSON");
    return RESPONSE_FAILED;
  }

  JsonObject jsonObj = jsonHolder.as<JsonObject>();

  if (!jsonObj.containsKey("alerts")) {
    Serial.println("No 'alerts' found in the JSON response");
    return RESPONSE_FAILED;
  }

  JsonArray alerts = jsonObj["alerts"].as<JsonArray>();

  bool regionAlert;
  bool districtAlert;
  for (JsonObject alert : alerts) {
    const char* location_uid = alert["location_uid"];
    if (!location_uid) {
      continue;
    }
    Serial.print("Current alert location original: ");
    Serial.println(location_uid);
    Serial.print("Current alert location String: ");
    Serial.println(String(location_uid));

    if (String(location_uid) == TARGET_CITY_INDEX_STR) {
      Serial.println("CITY ALERT");
      return ALERT_ON;
    } else if (String(location_uid) == TARGET_REGION_INDEX_STR) {
      Serial.println("REGION ALERT");
      regionAlert = true;
      continue;
    } else {
      const char* location_oblast = alert["location_oblast"];
      if (location_oblast && String(location_oblast) == REGION_STR) {
        Serial.println("District in region alert");
        regionAlert = true;
      }
    }
  }

  if (regionAlert) {
    return REGION_ALERT;
  } 
  if(districtAlert) {
    return DISTRICT_ALERT;
  }
  else {
    return NO_ALERT;
  }
}
