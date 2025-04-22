#include "main.h"

PN532Reader nfcReader(SDA_PIN, SCL_PIN); // Khởi tạo PN532 với SDA = GPIO14, SCL = GPIO13

void setup()
{
    Serial.begin(115200);
    xTaskCreate(wifiTask, "WiFiTask", 4096, NULL, 1, &WiFiTaskHandle);
    xTaskCreate(SeverTask, "SeverTask", 4096, NULL, 1, &SeverTaskHandle);
    vTaskSuspend(SeverTaskHandle); // Tạm dừng task SeverTask
}

void loop()
{

}