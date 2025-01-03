#include <HardwareSerial.h>
#include <Wire.h>
#include <sensors/MHZ19B.h>

#define RX_PIN 16  // ESP32 RX pin
#define TX_PIN 17  // ESP32 TX pin

HardwareSerial mySerial(2); // Use UART2

uint8_t getPPM[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
uint8_t response[9];

void initMHZ19B() {
    mySerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
}

float getCO2Concentration() {
  mySerial.write(getPPM, sizeof(getPPM));
  
  delay(100);

  int i = 0;
  while (mySerial.available() > 0) {
    response[i] = mySerial.read();
    i++;
    if (i == 9) break;
  }

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
  return checksum == response[8];
}
