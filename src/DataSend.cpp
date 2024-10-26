#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <DataSend.h>

// All sensitive data is here in format `#define WIFI_SSID "MyHomeWiFi"`
#include <secrets.h>

#define ALL_OK 0;

const int WIFI_CONNECT_TIMEOUT_MS = 30 * 1000;

const String number[] = {"Автономна Республіка Крим", // 0
                         "Волинська область",         // 1
                         "Вінницька область",         // 2
                         "Дніпропетровська область",  // 3
                         "Донецька область",          // 4
                         "Житомирська область",       // 5
                         "Закарпатська область",      // 6
                         "Запорізька область",        // 7
                         "Івано-Франківська область", // 8
                         "м. Київ",                   // 9
                         "Київська область",          // 10
                         "Кіровоградська область",    // 11
                         "Луганська область",         // 12
                         "Львівська область",         // 13
                         "Миколаївська область",      // 14
                         "Одеська область",           // 15
                         "Полтавська область",        // 16
                         "Рівненська область",        // 17
                         "м. Севастополь",            // 18
                         "Сумська область",           // 19
                         "Тернопільська область",     // 20
                         "Харківська область",        // 21
                         "Херсонська область",        // 22
                         "Хмельницька область",       // 23
                         "Черкаська область",         // 24
                         "Чернівецька область",       // 25
                         "Чернігівська область"};     // 26

const int TARGET_REGION_INDEX = 22;
const String TARGET_REGION_INDEX_STR = "22";

void connectToWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(500);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("Connecting...");
  unsigned int startTime = millis();
  while (WiFi.status() != WL_CONNECTED && startTime + WIFI_CONNECT_TIMEOUT_MS > millis())
  {
    if (WiFi.status() == WL_CONNECT_FAILED)
    {
      Serial.println("Failed to connect to WiFi. Please verify credentials and signal.");
      Serial.println();
    }
    Serial.println("Waiting for connect...");
    delay(5000);
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("");
    Serial.println("WiFi connected. IP address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.print("Failed to connect to WiFi: ");
    Serial.println(WiFi.status());
  }
}

int sendMeteoData(DynamicJsonDocument jsonData)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    connectToWiFi();
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    String jsonString;
    serializeJson(jsonData, jsonString);
    HTTPClient http;
    http.begin(DATA_URL);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(jsonString);
    Serial.print("JSON sending HTTP code: ");
    Serial.println(httpResponseCode);
    Serial.print("Response: ");
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

Status getAlerts()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    connectToWiFi();
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Failed to connect to WiFi. Status is: ");
    Serial.println(WiFi.status());
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

  if (httpResponseCode == -1)
  {
    Serial.println("[WARNING] Network request failed. No internet or alerts host is not accessible.");
    return CONNECTION_FAILED;
  }

  if (httpResponseCode == 429)
  {
    Serial.println("[WARNING] TOO MANY REQUESTS");
    Serial.println(http.getString());
    return TOO_MANY_REQUEST;
  }

  String response = http.getString();
  Serial.print("Response: ");
  Serial.println(response);

  response.replace("\"", "");

  //char status = response[TARGET_REGION_INDEX];
  char status = response[0];
  if (status == 'A')
  {
    return ALERT_ON;
  }
  else if (status == 'P')
  {
    return PARTIAL_ALERT;
  }
  else
  {
    return NO_ALERT;
  }
}
