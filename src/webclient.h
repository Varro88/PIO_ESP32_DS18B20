#ifndef HTTP_CLIENT_WRAPPER_H
#define HTTP_CLIENT_WRAPPER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <map>
#include <ArduinoJson.h>

struct HttpResponse {
    int statusCode;
    String responseBody;
};

// Function prototypes
HttpResponse sendGetRequest(const String& url, const std::map<String, String>& headers);
bool stringToJson(DynamicJsonDocument doc, String response);

#endif // HTTP_CLIENT_WRAPPER_H
