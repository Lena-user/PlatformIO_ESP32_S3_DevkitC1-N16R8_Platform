#include "main.h"

void setup()
{
    Serial.begin(115200);
    Serial.print("Hello World! ");
}

void loop()
{
    Serial.println("Hello from ESP32!");
    delay(1000); // Delay 1 giây
}