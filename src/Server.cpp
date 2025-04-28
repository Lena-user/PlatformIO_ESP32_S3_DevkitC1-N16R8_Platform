#include "main.h"

// RPC callback function
void CheckInnProcess(const JsonVariantConst &data, JsonDocument &response)
{
    Serial.println("==================================");
    Serial.println("Processing CheckInn RPC...");

    // Extract request type and room from JSON
    std::string request = data[RPC_CALLBACK.at("KEYS")[0].c_str()];
    std::string room = data[RPC_CALLBACK.at("KEYS")[1].c_str()];

    Serial.print("Request: ");
    Serial.print(request.c_str());
    Serial.print(", Room: ");
    Serial.println(room.c_str());

    // Create write request
    WriteRequest writeReq;
    writeReq.blockNumber = 4; // Use block 4
    writeReq.isString = true; // We're writing a string

    bool processSuccess = false;

    if (request == "CHECK_IN")
    {
        Serial.println("[INFO] Processing CHECK_IN request");
        writeReq.stringData = String(room.c_str()); // Convert room number to String

        // Send write request to queue
        if (xQueueSend(writeQueue, &writeReq, pdMS_TO_TICKS(NFC_WRITE_DELAY)) == pdTRUE)
        {
            Serial.print("[SUCCESS] Queued CHECK_IN for room ");
            Serial.println(room.c_str());
            response["status"] = "OK";
        }
        else
        {
            response["status"] = "ERROR";
            Serial.println("[ERROR] Failed to queue CHECK_IN request (queue full)");
        }
    }
    else if (request == "CHECK_OUT")
    {
        Serial.println("[INFO] Processing CHECK_OUT request");
        writeReq.stringData = ""; // Empty string for checkout

        // Send write request to queue
        if (xQueueSend(writeQueue, &writeReq, pdMS_TO_TICKS(NFC_WRITE_DELAY)) == pdTRUE)
        {
            Serial.println("[SUCCESS] Queued CHECK_OUT request");
            processSuccess = true;
            response["status"] = "OK";
        }
        else
        {
            Serial.println("[ERROR] Failed to queue CHECK_OUT request (queue full)");
            response["status"] = "ERROR";
        }
    }
    Serial.println("==================================");
}

// WiFi task function
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
    vTaskResume(serverTaskHandle); // Resume server task after WiFi is connected
    vTaskDelete(wifiTaskHandle);   // Delete WiFi task after completion
}

// Server task function
void serverTask(void *pvParameters)
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

    Serial.println("Connected to ThingsBoard");
    roomManager.sendInitialStates(); // Send initial states to ThingsBoard
    
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
    
    // Create read and write tasks
    xTaskCreatePinnedToCore(readTask, "ReadTask", NFC_READ_TASK_STACK_SIZE, NULL, NFC_TASK_PRIORITY, &readTaskHandle, 1);
    xTaskCreatePinnedToCore(writeTask, "WriteTask", NFC_WRITE_TASK_STACK_SIZE, NULL, NFC_TASK_PRIORITY, &writeTaskHandle, 0);
    
    // Delete server task after completion
    vTaskDelete(serverTaskHandle);
    Serial.println("Server task finished");
}