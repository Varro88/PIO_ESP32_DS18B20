#include <HardwareSerial.h>
#include <Wire.h>
#include <sensors/MHZ19B.h>
#include "../logger.h"

#define TX_PIN 17 // ESP32 TX pin
#define RX_PIN 16 // ESP32 RX pin

HardwareSerial mySerial(2); // Use UART2

uint8_t cmdGetPpm[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
uint8_t cmdCalibrate[] = {0XFF, 0x01, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78};
uint8_t cmdAutocalibOn[] = {0xFF, 0x01, 0x79, 0xA0, 0x00, 0x00, 0x00, 0x00, 0xE6}; //NOT(0x01+0x79+0xA0)+1=E5+1=E6
uint8_t cmdAutocalibOff[] = {0xFF, 0x01, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86}; //NOT(0x01+0x79)+1=85+1=86

void initMHZ19B() {
    mySerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
}

void calibrate() {
  mySerial.write(cmdCalibrate, sizeof(cmdCalibrate));
  delay(30*60*1000);
}

void setAutocalibration(bool enabled) {
  if(enabled) {
    mySerial.write(cmdAutocalibOn, sizeof(cmdAutocalibOn));
  }
  else {
    mySerial.write(cmdAutocalibOff, sizeof(cmdAutocalibOff));
  }
}

float getCO2Concentration() {
  mySerial.write(cmdGetPpm, sizeof(cmdGetPpm));
  
  delay(100);

  int i = 0;
  uint8_t response[32];
  String strResp = "";
  while (mySerial.available() > 0) {
    response[i] = mySerial.read();

    char hexByte[5]; // "0x" + 2 hex chars + null terminator = 5
    snprintf(hexByte, sizeof(hexByte), "0x%02X", response[i]);
      if (!strResp.isEmpty()) {
      strResp += " ";  // Add a space between bytes
    }
    strResp += hexByte;

    if (i+1 == sizeof(response)) {
      logWarn("More than max allowable bytes in response, skipping next bytes");
      break;
    }
    i++;
  }
  mySerial.flush();

  int temp = response[4] - 40;
  logInfo((String)"Temperature: " + temp);
  logInfo((String)"Response CO2 from MH-Z19H: " + strResp);
  logInfo((String)"Response size: " + i);

  if (i < 9) {
    logError("Incomplete response from sensor");
    return NAN;
  }

  if (response[0] == 0 && response[1] == 0) {
    logError("Error: zeroes in response from sensor");
    return NAN;
  }

  if (checkResponse(response)) {
    int high = response[2];
    int low = response[3];
    int ppm = (high << 8) + low;
    logInfo((String)"CO2 Concentration: " + ppm + " ppm");
    return (float)ppm;
  } else {
    logWarn("Error: Checksum validation failed");
  }
  return NAN;
}

bool checkResponse(uint8_t * response) {
  uint8_t checksum = 0;
  for (int i = 1; i < 8; i++) {
    checksum += response[i];
  }
  checksum = 0xFF - checksum + 1;
  return checksum == response[8];
}
