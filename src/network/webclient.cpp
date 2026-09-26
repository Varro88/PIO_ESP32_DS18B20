#include "network/webclient.h"
#include <ArduinoJson.h>
#include "../logger.h"

/*HttpResponse sendGetRequest(const String& url, const std::map<String, String>& headers) {
  HTTPClient http;
  http.begin(url);
  http.setTimeout(7000);

  for (const auto& header : headers) {
    http.addHeader(header.first, header.second);
  }

  int httpResponseCode = http.GET();
  String responseBody = "";

  if (httpResponseCode > 0) {
    responseBody = http.getString();
  }

  http.end();
  return {httpResponseCode, responseBody};
}*/
HTTPClient http;


HttpResponse sendGetRequest(const String& url, const std::map<String, String>& headers) {
  static unsigned long requestId = 0;
  requestId++;

  const unsigned long requestStart = millis();

  logDebug((String)"\n========== HTTP GET #" + requestId + " START ==========\n");

  logDebug((String)"[HTTP] URL: " + url.c_str() );
  logDebug((String)"[HTTP] WiFi status: " +  WiFi.status());
  logDebug((String)"[HTTP] RSSI: " + WiFi.RSSI() + " dBm");
  logDebug((String)"[HTTP] Free heap: " + ESP.getFreeHeap());

  if (!http.begin(url)) {
    logDebug((String)"[HTTP] " + requestId + " begin() FAILED\n");
    return {-1, ""};
  }

  http.setTimeout(7000);

  for (const auto& header : headers) {
    http.addHeader(header.first, header.second);
  }

  const unsigned long getStart = millis();

  int httpResponseCode = http.GET();
  const unsigned long getElapsed = millis() - getStart;

  logDebug((String) "[HTTP] #" + requestId + " GET -> " + httpResponseCode + " in " + getElapsed + " ms" );
  String responseBody = "";

  if (httpResponseCode > 0) {
    responseBody = http.getString();

    logDebug((String) "[HTTP] #" + requestId +" body length: " + responseBody.length());
  } else {
    logDebug((String)"[HTTP] # " + requestId + "ERROR: " + http.errorToString(httpResponseCode).c_str());
    logDebug((String)"[HTTP] # " + requestId + "RSSI at failure: " + WiFi.RSSI() + " dBm");
    logDebug((String)"[HTTP] # " + requestId + "status at failure: " + WiFi.status() + " dBm");
    logDebug((String)"[HTTP] # " + requestId + "Free heap at failure: " + ESP.getFreeHeap());
  }
  http.end();
  logDebug((String)"========== HTTP GET #" + requestId + " END ==========\n");
  return {httpResponseCode, responseBody};
}

HttpResponse sendPostRequest(const String& url, const std::map<String, String>& headers, const String& body) {
    http.begin(DATA_URL);
    http.addHeader("Content-Type", "application/json");
    for (const auto& header : headers) {
      http.addHeader(header.first, header.second);
    }
    int httpResponseCode = http.POST(jsonString);
    logInfo((String)"JSON sending HTTP code: " + httpResponseCode);
    logInfo((String)"Send meteo response: " + http.getString());
    http.end();
}



bool stringToJson(DynamicJsonDocument& targetDoc, const String& sourceStr) {
  DeserializationError error = deserializeJson(targetDoc, sourceStr);
    
  if (error) {
      logWarn((String)"Failed to parse JSON: " + error.c_str());
      return false;
  }
  
  return true;
}