#include "network/webclient.h"
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

bool stringToJson(DynamicJsonDocument& targetDoc, const String& sourceStr) {
  DeserializationError error = deserializeJson(targetDoc, sourceStr);
    
  if (error) {
      Serial.print("Failed to parse JSON: ");
      Serial.println(error.c_str());
      return false;
  }

  /*JsonObject obj = targetDoc.as<JsonObject>();
  Serial.println("Top-level keys in JSON:");
  for (JsonPair kv : obj) {
      Serial.println(kv.key().c_str());
  } */
  return true;
}