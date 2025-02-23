#include <stdint.h>

void initMHZ19B();
void calibrate();
void setAutocalibration(bool enabled);
float getCO2Concentration();
bool checkResponse(uint8_t * response);