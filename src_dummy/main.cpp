// This is the dummy firmware that willl be flashed at 0x10000
// ./src forlder must be installed at the test partition

#include <Arduino.h>
void main() { Serial.begin(115200); }
void loop() {
    Serial.println("No firmware installed");
    delay(500);
}
