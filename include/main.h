#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <string>  // Thư viện string
#include <sstream> // Thư viện stringstream

// Private Include
#include "PN532Manager.h" // Thư viện PN532
#include "config.h"      // Thư viện cấu hình

// Thư viện liên quan đến kết nối mạng và ThingsBoard
#include <WiFi.h>
#include <Adafruit_MQTT.h> // Thư viện Adafruit MQTT
#include <Adafruit_MQTT_Client.h> // Thư viện Adafruit MQTT Client

// Thư viện FreeRTOS
#include <array>               // Thư viện array
#include "freertos/FreeRTOS.h" // Thư viện FreeRTOS
#include "freertos/task.h"     // Thư viện FreeRTOS task
#include "freertos/queue.h"    // Thư viện FreeRTOS queue
#include "freertos/semphr.h"   // Thư viện FreeRTOS semaphore

// Initialize the WiFi and MQTT client
WiFiClient wifiClient;
Adafruit_MQTT_Client mqtt(&wifiClient, IO_SERVER, IO_PORT, IO_USERNAME, IO_KEY);
Adafruit_MQTT_Subscribe subscribe(&mqtt, IO_FEEDS); // Subscribe to ThingsBoard
Adafruit_MQTT_Publish publish(&mqtt, IO_FEEDS); // Publish to ThingsBoard

// Initialize ThingsBoard instance with the maximum needed buffer size

// Task handles
TaskHandle_t ServerTaskHandle = NULL; // Handle cho task server
TaskHandle_t WiFiTaskHandle = NULL;  // Handle cho task Wi-Fi
TaskHandle_t readTaskHandle;
TaskHandle_t writeTaskHandle;


void writeTask(void *pvParameters);
void readTask(void *pvParameters);

void WiFiTask(void *pvParameters)
{
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi");
    vTaskResume(ServerTaskHandle); // Resume task server sau khi kết nối Wi-Fi thành công
    vTaskDelete(WiFiTaskHandle); // Xóa task Wi-Fi sau khi hoàn thành
}
void SeverTask(void *pvParameters) {
    while (true) { // Duy trì Task
        if (!mqtt.connected()) {
            Serial.println("⚠️ MQTT bị ngắt, đang thử kết nối lại...");
            uint8_t retries = 3;
            while (retries > 0 && !mqtt.connect()) {
                Serial.println("🔄 Đang thử lại...");
                vTaskDelay(pdMS_TO_TICKS(5000)); // Đợi 5 giây trước khi thử lại
                retries--;
            }
            if (mqtt.connected()) {
                Serial.println("✅ Đã kết nối lại MQTT!");
            } else {
                Serial.println("🚫 Không thể kết nối lại MQTT.");
                vTaskDelay(pdMS_TO_TICKS(10000)); // Chờ 10 giây rồi thử lại
                continue; // Quay lại vòng lặp để thử lại
            }
        }

        Serial.println("🌐 Đã kết nối MQTT - Khởi động Tasks");
        xTaskCreatePinnedToCore(readTask, "ReadTask", 4096, NULL, 1, &readTaskHandle, 0);
        xTaskCreatePinnedToCore(writeTask, "WriteTask", 4096, NULL, 1, &writeTaskHandle, 0);

        vTaskDelete(NULL); // Xóa chính task này khi hoàn thành
    }
}

#endif // MAIN_H