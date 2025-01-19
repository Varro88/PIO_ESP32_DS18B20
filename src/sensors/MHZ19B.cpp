#include <HardwareSerial.h>
#include <Wire.h>
#include <sensors/MHZ19B.h>

#define TX_PIN 17 // ESP32 TX pin
#define RX_PIN 16 // ESP32 RX pin

HardwareSerial mySerial(2); // Use UART2

uint8_t cmdGetPpm[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
uint8_t cmdCalibrate[] = {0XFF, 0x01, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78};

void initMHZ19B() {
    mySerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
}

void calibrate() {
  mySerial.write(cmdCalibrate, sizeof(cmdCalibrate));
  delay(30*60*1000);
}

float getCO2Concentration() {
  mySerial.write(cmdGetPpm, sizeof(cmdGetPpm));
  
  delay(100);

  int i = 0;
  uint8_t response[127];
  String strResp = "";
  while (mySerial.available() > 0) {
    response[i] = mySerial.read();
    i++;

    char hexByte[5]; // "0x" + 2 hex chars + null terminator = 5
    snprintf(hexByte, sizeof(hexByte), "0x%02X", response[i]);
      if (!strResp.isEmpty()) {
      strResp += " ";  // Add a space between bytes
    }
    strResp += hexByte;

    //if (i == 9) break;
  }

  Serial.print("Response from MH-Z19H: ");
  Serial.println(strResp);

  Serial.print("Response size: ");
  Serial.println(sizeof(response));

  if (i < 9) {
    Serial.println("Error: Incomplete response from sensor");
    return NAN;
  }

  if (checkResponse(response)) {
    int high = response[2];
    int low = response[3];
    int ppm = (high << 8) + low;
    Serial.print("CO2 Concentration: ");
    Serial.print(ppm);
    Serial.println(" ppm");
    return (float)ppm;
  } else {
    Serial.println("Error: Checksum validation failed");
  }
  return NAN;
}

bool checkResponse(uint8_t * response) {
  uint8_t checksum = 0;
  for (int i = 1; i < 8; i++) {
    checksum += response[i];
  }
  checksum = 0xFF - checksum + 1;
  Serial.println("Checksum response: ");
  Serial.println(response[8]);

  Serial.println("Checksum calculated: ");
  Serial.println(checksum);

  return checksum == response[8];
}
