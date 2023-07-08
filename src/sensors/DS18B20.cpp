#include <sensors/DS18B20.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>

#define ONE_WIRE_BUS 19

OneWire dsWire(ONE_WIRE_BUS);
DallasTemperature ds18B20(&dsWire);
DeviceAddress *sensorsUnique;

void initDS18B20() {
    ds18B20.begin();
}

float getDS18B20Value() {
    ds18B20.requestTemperatures();
    float ds18b20Temperature = NAN;
    ds18b20Temperature = ds18B20.getTempCByIndex(0);
    if(ds18b20Temperature < -50) {
        Serial.print("DS18B20 out sensor error");
        ds18b20Temperature = NAN;
  }
  return ds18b20Temperature;
}