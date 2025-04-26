#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <string>  // Thư viện string
#include <sstream> // Thư viện stringstream

// Private Include
#include "config.h"      // Thư viện cấu hình
#include "Adafruit_PN532.h" // Thư viện NFC PN532

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
TaskHandle_t ServerTaskHandle = NULL; // Handle cho task server
TaskHandle_t WiFiTaskHandle = NULL;  // Handle cho task Wi-Fi
TaskHandle_t readTaskHandle = NULL;  // Handle cho task đọc NFC
TaskHandle_t writeTaskHandle = NULL; // Handle cho task ghi NFC

void CheckInnProcess(const JsonVariantConst &data, JsonDocument &response);
void readTask(void *pvParameters);
void writeTask(void *pvParameters);


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
    vTaskResume(ServerTaskHandle); // Xóa task server sau khi hoàn thành
    vTaskDelete(WiFiTaskHandle); // Xóa task Wi-Fi sau khi hoàn thành
}
void ServerTask(void *pvParameters)
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
    xTaskCreatePinnedToCore(readTask, "ReadTask", 4096, NULL, 1, &readTaskHandle, 1);
    xTaskCreatePinnedToCore(writeTask, "WriteTask", 4096, NULL, 1, &writeTaskHandle, 0);
    vTaskDelete(ServerTaskHandle); // Xóa task server sau khi hoàn thành
    Serial.println("Server task finished");
}

#endif // MAIN_H