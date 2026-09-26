#include <sensors/DS18B20.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include "../logger.h"

#define DS18B20_COUNT 2

// Pin assignments per sensor
static const uint8_t ds18b20Pins[DS18B20_COUNT] = { 18, 19 };

// One OneWire + DallasTemperature instance per bus/sensor
static OneWire dsWires[DS18B20_COUNT] = {
    OneWire(ds18b20Pins[0]),
    OneWire(ds18b20Pins[1])
};

static DallasTemperature ds18B20Sensors[DS18B20_COUNT] = {
    DallasTemperature(&dsWires[0]),
    DallasTemperature(&dsWires[1])
};

void initDS18B20() {
    Wire.begin();
    for (uint8_t i = 0; i < DS18B20_COUNT; i++) {
        ds18B20Sensors[i].begin();
    }
}

float getDS18B20Value(uint8_t index) {
    if (index >= DS18B20_COUNT) {
        logWarn("DS18B20 invalid sensor index");
        return NAN;
    }

    ds18B20Sensors[index].requestTemperatures();
    float temperature = ds18B20Sensors[index].getTempCByIndex(0);

    if (temperature < -50) {
        logWarn((String)"DS18B20 sensor " + index +" (pin " + ds18b20Pins[index] + ") error");
        return NAN;
    }

    return temperature;
}

float getOuterDS18B20Value() {
    return getDS18B20Value(0);
}

float getInnerDS18B20Value() {
    return getDS18B20Value(1);
}

void getAllDS18B20Values(float outValues[DS18B20_COUNT]) {
    for (uint8_t i = 0; i < DS18B20_COUNT; i++) {
        outValues[i] = getDS18B20Value(i);
    }
}