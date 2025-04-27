#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <string>
#include <sstream>
#include <array>
#include <ArduinoJson.h>

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

// Network and ThingsBoard includes
#include <WiFi.h>
#include <ThingsBoard.h>
#include <Arduino_MQTT_Client.h>
#include <Server_Side_RPC.h>

// Project includes
#include "config.h"
#include "PN532_NFC.h"
#include "RoomManager.h"

// Task stack sizes
#define SERVER_TASK_STACK_SIZE 8192
#define WIFI_TASK_STACK_SIZE   4096
#define NFC_READ_TASK_STACK_SIZE 4096
#define NFC_WRITE_TASK_STACK_SIZE 4096

// Task priorities
#define SERVER_TASK_PRIORITY 2
#define WIFI_TASK_PRIORITY 3
#define NFC_TASK_PRIORITY 1

// Forward declarations of structs
struct WriteRequest {
    uint8_t blockNumber;
    uint8_t data[16];
    bool isString;
    String stringData;
};

// Global variables declaration (use extern)
extern WiFiClient wifiClient;
extern Arduino_MQTT_Client mqttClient;
extern Server_Side_RPC<MAX_RPC_SUBSCRIPTIONS, MAX_RPC_RESPONSE> rpc;
extern ThingsBoard tb;
extern PN532_NFC nfcReader;

// Queue and semaphore handles
extern QueueHandle_t writeQueue;
extern SemaphoreHandle_t nfcMutex;

// Task handles
extern TaskHandle_t serverTaskHandle;
extern TaskHandle_t wifiTaskHandle;
extern TaskHandle_t readTaskHandle;
extern TaskHandle_t writeTaskHandle;

// State
extern bool subscribed;

// Class instances
extern RoomManager roomManager;

// Function declarations
void log(const char* level, const char* component, const char* message);
bool setupWiFi();
void readTask(void *pvParameters);
void writeTask(void *pvParameters);
void serverTask(void *pvParameters);
void wifiTask(void *pvParameters);
void CheckInnProcess(const JsonVariantConst &data, JsonDocument &response);

#endif // MAIN_H