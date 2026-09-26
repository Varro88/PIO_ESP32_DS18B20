#include <WiFi.h>
#include <secrets.h>
#include <network/wificlient.h>
#include "../logger.h"

const int WIFI_CONNECT_TIMEOUT_MS = 30 * 1000;

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(500);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  logInfo("Connecting...");
  unsigned int startTime = millis();
  while (WiFi.status() != WL_CONNECTED &&
         startTime + WIFI_CONNECT_TIMEOUT_MS > millis()) {
    if (WiFi.status() == WL_CONNECT_FAILED) {
      logWarn("Failed to connect to WiFi. Please verify credentials and signal.");
    }
    logInfo("Waiting for connect...");
    delay(5000);
  }
  if (WiFi.status() == WL_CONNECTED) {
    logInfo((String)"WiFi connected. IP address: " + WiFi.localIP());
  } else {
    logWarn("Failed to connect to WiFi: " + WiFi.status());
  }
}

bool connectIfNotConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  if (WiFi.status() != WL_CONNECTED) {
    logWarn("Failed to connect to WiFi. Status is: " + WiFi.status());
    return false;
  }

  int rssi = WiFi.RSSI();
  logInfo((String)"Signal Strength (RSSI), dBm: " + rssi);
  logInfo((String)"Free Heap, bytes: %d bytes" + ESP.getFreeHeap());

  return true;
}