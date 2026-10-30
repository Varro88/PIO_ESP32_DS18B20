#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <array>
#include "../logger.h"

#define BME280_I2C_ADDRESS 0x76
Adafruit_BME280 bme;
void initBME280()
{
    if (!bme.begin(BME280_I2C_ADDRESS)) {
        logWarn("Could not find a valid BME280 sensor, check wiring!");
        return;
    }

    // Fix for elevated temperature
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1,   // temperature
                    Adafruit_BME280::SAMPLING_X1,   // pressure
                    Adafruit_BME280::SAMPLING_X1,   // humidity
                    Adafruit_BME280::FILTER_OFF);
}

std::array<float, 3> getBME280Measurings()
{
    std::array<float, 3> m;
    if (!bme.takeForcedMeasurement()) {   // forcibly trigger one measurement
        m.fill(NAN);
        return m;
    }
    m[0] = bme.readTemperature();
    m[1] = bme.readHumidity();
    m[2] = bme.readPressure() / 100.0F;
    return m;
}