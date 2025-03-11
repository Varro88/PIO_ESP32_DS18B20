#include <WiFi.h>
#include <secrets.h>
#include <wificlient.h>

const int WIFI_CONNECT_TIMEOUT_MS = 30 * 1000;

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(500);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("Connecting...");
  unsigned int startTime = millis();
  while (WiFi.status() != WL_CONNECTED &&
         startTime + WIFI_CONNECT_TIMEOUT_MS > millis()) {
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println(
          "Failed to connect to WiFi. Please verify credentials and signal.");
      Serial.println();
    }
    Serial.println("Waiting for connect...");
    delay(5000);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected. IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.print("Failed to connect to WiFi: ");
    Serial.println(WiFi.status());
  }
}

bool connectIfNotConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Failed to connect to WiFi. Status is: ");
    Serial.println(WiFi.status());
    return false;
  }
  return true;
}