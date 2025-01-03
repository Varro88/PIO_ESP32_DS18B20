#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <array>

#define BME280_I2C_ADDRESS 0x76
Adafruit_BME280 bme;

void initBME280()
{
    bool status;    
    status = bme.begin(BME280_I2C_ADDRESS);
    if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
    }
}

std::array<float, 3> getBME280Measurings()
{
  std::array<float, 3> measurings;
  try {
    measurings[0] = bme.readTemperature();
  }
  catch (...) {
    measurings[0] = NAN;
  }

  try {
    measurings[1] = bme.readHumidity();
  }
  catch (...) {
    measurings[1] = NAN;
  }

  try {
    measurings[2] = bme.readPressure() / 100.0F;
  }
  catch (...) {
    measurings[2] = NAN;
  }
  return measurings;
}