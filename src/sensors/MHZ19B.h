#include <stdint.h>

void initMHZ19B();
void calibrate();
float getCO2Concentration();
bool checkResponse(uint8_t * response);