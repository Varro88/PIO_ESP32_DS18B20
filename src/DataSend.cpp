#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* WIFI_SSID = "Kyivstar-89E7";
const char* WIFI_PASS = "41323427";
const int WIFI_CONNECT_TIMEOUT_MS = 30000;
const String dataUrl = "http://192.168.1.1/collect";
const String statusUrl = "http://192.168.1.1/status";

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(500);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("Connecting...");
  unsigned int startTime = millis();
  while (WiFi.status() != WL_CONNECTED && startTime + WIFI_CONNECT_TIMEOUT_MS > millis())  {
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("Failed to connect to WiFi. Please verify credentials.");
      Serial.println();
    }
    Serial.println("Waiting for connect...");
    delay(5000);
  }
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.print("Failed to connect to WiFi: ");
    Serial.println(WiFi.status());
  }
}

int sendDataToServer(DynamicJsonDocument jsonData)
{
  if(WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  if(WiFi.status() == WL_CONNECTED){
    String jsonString;
    serializeJson(jsonData, jsonString);
    HTTPClient http;
    http.begin(dataUrl);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(jsonString);
    Serial.print("JSON sending HTTP code: ");
    Serial.println(httpResponseCode);
    Serial.println("Response:");
    Serial.println(http.getString());
    return httpResponseCode;
  }
  else
  {
    Serial.print("Failed to connect to WiFi. Status is: ");
    Serial.println(WiFi.status());
    return -1;
  }
}
