#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <string>  // Thư viện string
#include <sstream> // Thư viện stringstream

// Private Include
#include "PN532Reader.h" // Thư viện PN532
#include "config.h"      // Thư viện cấu hình

// Thư viện liên quan đến kết nối mạng và ThingsBoard
#include <WiFi.h>
#include <ThingsBoard.h>         // Thư viện ThingsBoard cho ESP32
#include <Arduino_MQTT_Client.h> // Thư viện MQTT cho ESP32
#include <Server_Side_RPC.h>     // Thư viện cho server-side RPC

// Thư viện FreeRTOS
#include <array>               // Thư viện array
#include "freertos/FreeRTOS.h" // Thư viện FreeRTOS
#include "freertos/task.h"     // Thư viện FreeRTOS task
#include "freertos/queue.h"    // Thư viện FreeRTOS queue
#include "freertos/semphr.h"   // Thư viện FreeRTOS semaphore

// Initialize the WiFi and MQTT client
WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
Server_Side_RPC<MAX_RPC_SUBSCRIPTIONS, MAX_RPC_RESPONSE> rpc;
const std::array<IAPI_Implementation *, 1U> apis = {
    &rpc};

// Initialize ThingsBoard instance with the maximum needed buffer size
ThingsBoard tb(
    mqttClient,
    MAX_MESSAGE_RECEIVE_SIZE,
    MAX_MESSAGE_SEND_SIZE,
    Default_Max_Stack_Size,
    apis);

// Task handles
TaskHandle_t SeverTaskHandle = NULL; // Handle cho task server
TaskHandle_t WiFiTaskHandle = NULL;  // Handle cho task Wi-Fi
TaskHandle_t ScanTaskHandle = NULL;  // Handle cho task đọc NFC
TaskHandle_t ReadTaskHandle = NULL;  // Handle cho task đọc NFC

void ScanTask(void *pvParameters);
void ReadTask(void *pvParameters);

void CheckInnProcess(const JsonVariantConst &data, JsonDocument &response)
{
    // Xử lý dữ liệu từ RPC
    Serial.println("Processing CheckInn RPC...");
    std::string request = data[RPC_CALLBACK.at("KEYS")[0].c_str()];
    std::string room = data[RPC_CALLBACK.at("KEYS")[1].c_str()];

    Serial.print(request.c_str());
    Serial.print(" - ");        
    Serial.println(room.c_str()); // In dữ liệu ra Serial Monitor 
}

void wifiTask(void *pvParameters)
{
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi");
    xTaskCreate(ScanTask, "ScanTask", 4096, NULL, 1, &ScanTaskHandle); // Tạo task quét thẻ NFC
    vTaskDelete(WiFiTaskHandle); // Xóa task Wi-Fi sau khi hoàn thành
}
void SeverTask(void *pvParameters)
{
    if (!tb.connected())
    {
        Serial.println("Reconnecting to ThingsBoard...");
        while (!tb.connect(THINGSBOARD_SERVER, DEVICE_ACCESS_TOKEN, THINGSBOARD_PORT))
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            Serial.println("Failed to reconnect");
        }
    }
    if (!subscribed)
    {
        Serial.println("Subscribing for RPC...");
        const std::array<RPC_Callback, MAX_RPC_SUBSCRIPTIONS> callbacks = {
            RPC_Callback{RPC_CALLBACK.at("METHODS")[0].c_str(), CheckInnProcess}};
        if (!rpc.RPC_Subscribe(callbacks.cbegin(), callbacks.cend()))
        {
            Serial.println("Failed to subscribe for RPC");
            return;
        }

        Serial.println("Subscribe done");
        subscribed = true;
    }
    vTaskResume(ScanTaskHandle); // Resume the scan task after server task is ready
}

extern PN532Reader nfcReader; // Khai báo đối tượng PN532Reader

#endif // MAIN_H