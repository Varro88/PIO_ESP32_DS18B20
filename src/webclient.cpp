#include "webclient.h"

#include <ArduinoJson.h>

HttpResponse sendGetRequest(const String& url,
                            const std::map<String, String>& headers) {
  HTTPClient http;
  http.begin(url);

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
}

bool stringToJson(DynamicJsonDocument targetDoc, String sourceStr) {
  DeserializationError parseError = deserializeJson(targetDoc, sourceStr);

  // Check for errors
  if (parseError) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(parseError.c_str());
    return false;
  }

  return true;
}