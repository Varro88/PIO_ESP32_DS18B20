#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "network/alerts.h"
#include "network/wificlient.h"
#include "../logger.h"

#include "network/webclient.h"

// All sensitive data is here in format `#define WIFI_SSID "MyHomeWiFi"`
#include <secrets.h>

const String TARGET_REGION_INDEX_STR = "22";
const String TARGET_CITY_INDEX_STR = "6293";
const String REGION_STR = "Харківська область";

void testHttpRequests(unsigned long intervalMs = 10000) {
  const String url = "http://85.209.51.185:42042/alertStatus";
  const int requestCount = 100;

  int successCount = 0;
  int failureCount = 0;

  unsigned long lastRequestEnd = 0;

  HTTPClient http;

  Serial.println();
  Serial.println("========================================");
  Serial.printf("Starting HTTP test: %d requests\n", requestCount);
  Serial.printf("Interval: %lu ms\n", intervalMs);
  Serial.println("HTTPClient: SINGLE INSTANCE, REUSED");
  Serial.println("========================================");

  // Initialize HTTPClient once.
  if (!http.begin(url)) {
    logDebug("[TEST] http.begin() FAILED");
    return;
  }

  http.setTimeout(7000);

  const unsigned long testStart = millis();

  for (int i = 1; i <= requestCount; i++) {
    Serial.printf("\n----- Request %d/%d -----\n", i, requestCount);

    const unsigned long now = millis();

    if (lastRequestEnd != 0) {
      Serial.printf(
        "[TEST] Time since previous request: %lu ms\n",
        now - lastRequestEnd
      );
    }

    const unsigned long start = millis();

    int httpResponseCode = http.GET();

    const unsigned long elapsed = millis() - start;

    // Record the exact moment this request finished.
    lastRequestEnd = millis();

    if (httpResponseCode > 0) {
      successCount++;

      String responseBody = http.getString();

      logCustom(
        "[TEST] ",
        (String)"#" + i +
        " SUCCESS: HTTP " + httpResponseCode +
        ", " + elapsed + " ms" +
        ", body length " + responseBody.length()
      );
    } else {
      failureCount++;

      String error = http.errorToString(httpResponseCode);

      logCustom(
        "[TEST] ",
        (String)"#" + i +
        " FAILURE: code " + httpResponseCode +
        ", error: " + error +
        ", " + elapsed + " ms"
      );

      // Additional diagnostics only when a request fails.
      Serial.printf(
        "[TEST] WiFi status: %d, RSSI: %d dBm, free heap: %u\n",
        WiFi.status(),
        WiFi.RSSI(),
        ESP.getFreeHeap()
      );
    }

    // Don't wait after the last request.
    if (i < requestCount) {
      delay(intervalMs);
    }
  }

  // Clean up only once, after all requests are complete.
  http.end();

  const unsigned long totalTime = millis() - testStart;

  logDebug("========================================");
  logDebug("HTTP test complete");
  logDebug((String)"Successful: " + successCount);
  logDebug((String)"Failed:     " + failureCount);
  logDebug((String)"Total:      " + requestCount);
  logDebug((String)"Total time: " + totalTime + " ms");
  logDebug("========================================");
}


void testHttpRequestsPersistent() {
  const String url = "http://85.209.51.185:42042/alertStatus";

  int successCount = 0;
  int failureCount = 0;

  logDebug("========================================");
  logDebug("Starting HTTP stress test: 500 requests");
  logDebug("HTTPClient: REUSED");
  logDebug("");

  HTTPClient http;

  if (!http.begin(url)) {
    logDebug("[TEST] http.begin() FAILED");
    return;
  }

  http.setTimeout(7000);

  const unsigned long testStart = millis();

  for (int i = 1; i <= 500; i++) {
    const unsigned long start = millis();

    int statusCode = http.GET();

    const unsigned long elapsed = millis() - start;

    if (statusCode > 0) {
      successCount++;

      // Consume the response so the connection can potentially be reused.
      String responseBody = http.getString();

      logCustom(
        "[TEST] ",
        (String)"#" + i +
        " SUCCESS: HTTP " + statusCode +
        ", " + elapsed + " ms" +
        ", body=" + responseBody.length() + " bytes"
      );
    } else {
      failureCount++;

      logCustom(
        "[TEST] ",
        (String)"#" + i +
        " FAILURE: code " + statusCode +
        ", " + elapsed + " ms" +
        ", error=" + http.errorToString(statusCode)
      );
    }

    delay(100);
  }

  http.end();

  logDebug("");
  logDebug("HTTP persistent stress test complete");
  logDebug((String)"Successful: " + successCount);
  logDebug((String)"Failed:     " + failureCount);

  unsigned int time = millis() - testStart;
  logDebug((String)"Total time: " + time + " ms");

  logDebug("========================================");
}

AlertData getSimpleAlerts() {
  if (!connectIfNotConnected()) {
    return {-1, Status::WIFI_FAILED};
  }

  HttpResponse response = sendGetRequest(String(ALERTS_CUSTOM), {});
  logInfo((String)"Simple alerts HTTP code: " + response.statusCode);

  if (response.statusCode == -1) {
    logWarn("Network failed. No internet or alerts host is not accessible.");
    return {-1, CONNECTION_FAILED};
  }

  if (response.statusCode != 200) {
    logWarn("Status above was not valid. Body: " + response.responseBody);
    Status status;
    switch(response.statusCode) {
      case 400: status = ERR_400;
      case 401: status = ERR_401;
      case 402: status = ERR_402;
      case 403: status = ERR_403;
      case 404: status = ERR_404;
      case 409: status = ERR_409;
      case 429: status = ERR_429;
      case 502: status = ERR_502;
      case 503: status = ERR_503;
      case 504: status = ERR_504;
      default: status = RESPONSE_CODE_FAILED;
    }
    return {response.statusCode, status};
  }

  if(response.responseBody == "A") {
    return {200, ALERT_ON};
  } else if (response.responseBody == "P") {
    return {200, DISTRICT_ALERT};
  } else if (response.responseBody == "N") {
    return {200, NO_ALERT};
  } else {
    return {200, RESPONSE_BODY_FAILED};
  }
}
