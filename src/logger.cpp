#include "logger.h"
#include "Arduino.h"

void logDebug(String message) {
    log("[DEBUG] " + message);
}

void logInfo(String message) {
    log("[INFO ] " + message);
}

void logWarn(String message) {
    log("[ WARN] " + message);
}
void logError(String message) {
    log("[ERROR] " + message);
}

void log(String message) {
    Serial.println(message);
}

void logCustom(String prefix, String message) {
    Serial.println(prefix + " " + message);
}