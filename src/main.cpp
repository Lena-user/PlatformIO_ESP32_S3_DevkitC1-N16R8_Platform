#include "main.h"
#include "PN532_NFC.h"

// Chân kết nối PN532 qua I2C
#define SDA_PIN 14
#define SCL_PIN 13

// Create NFC instance
PN532_NFC nfcReader(SDA_PIN, SCL_PIN);

// FreeRTOS Handles
QueueHandle_t writeQueue;

// Struct để gửi thông tin qua Queue
typedef struct
{
    uint8_t blockNumber;
    uint8_t data[16];
    bool isString;
    String stringData; // For string writes
} WriteRequest;

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
        if (xQueueSend(writeQueue, &writeReq, pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            Serial.print("[SUCCESS] Queued CHECK_IN for room ");
            Serial.println(room.c_str());
            processSuccess = true;
        }
        else
        {
            Serial.println("[ERROR] Failed to queue CHECK_IN request (queue full)");
        }
    }
    else if (request == "CHECK_OUT")
    {
        Serial.println("[INFO] Processing CHECK_OUT request");
        writeReq.stringData = ""; // Empty string for checkout

        // Send write request to queue
        if (xQueueSend(writeQueue, &writeReq, pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            Serial.println("[SUCCESS] Queued CHECK_OUT request");
            processSuccess = true;
        }
        else
        {
            Serial.println("[ERROR] Failed to queue CHECK_OUT request (queue full)");
        }
    }

    // Prepare response
    response["success"] = processSuccess;
    response["request_type"] = request.c_str();
    response["room"] = room.c_str();

    if (processSuccess)
    {
        response["message"] = request == "CHECK_IN" ? ("Room " + room + " checked in successfully") : "Card checked out successfully";
    }
    else
    {
        response["message"] = "Failed to process request";
    }

    // Add card UID to response if available
    if (nfcReader.isCardPresent())
    {
        response["cardUID"] = nfcReader.getUidString();
    }
    else
    {
        response["cardUID"] = "No card detected";
    }

    Serial.println("==================================");
}

// Hàm đọc thẻ NFC (chạy nền)
void readTask(void *pvParameters)
{
    while (true)
    {
        // Scan for card
        if (nfcReader.scanCard())
        {
            Serial.print("[DETECTED] NFC Card found! UID: ");
            Serial.println(nfcReader.getUidString());

            if (nfcReader.authenticate(4))
            {
                String blockData = nfcReader.readBlockAsString(4);
                Serial.print("[DATA] Block 4 content: \"");
                Serial.print(blockData);
                Serial.println("\"");
            }
        }
        else
        {
            Serial.println("No NFC card detected.");
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Đợi 1 giây trước khi quét lại
    }
}

void writeTask(void *pvParameters)
{
    WriteRequest request;

    while (true)
    {
        // Nhận dữ liệu từ hàng đợi
        if (xQueueReceive(writeQueue, &request, portMAX_DELAY))
        {
            Serial.println("Processing write request...");

            // Scan for card if needed
            if (!nfcReader.isCardPresent())
            {
                nfcReader.scanCard(1000); // Longer timeout for write operations
            }

            if (nfcReader.isCardPresent())
            {
                if (nfcReader.authenticate(request.blockNumber))
                {
                    bool success = false;

                    if (request.isString)
                    {
                        // Write string data
                        success = nfcReader.writeBlockString(request.blockNumber, request.stringData);
                    }
                    else
                    {
                        // Write raw data
                        success = nfcReader.writeBlock(request.blockNumber, request.data);
                    }

                    if (success)
                    {
                        Serial.printf("✅ Data written to block %d!\n", request.blockNumber);
                    }
                    else
                    {
                        Serial.printf("❌ Failed to write to block %d!\n", request.blockNumber);
                    }
                }
                else
                {
                    Serial.println("❌ Authentication failed!");
                }
            }
            else
            {
                Serial.println("❌ No card found for writing!");
            }
        }
    }
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        delay(10);

    Wire.begin(SDA_PIN, SCL_PIN);

    // Initialize NFC reader
    if (!nfcReader.begin())
    {
        Serial.println("Failed to initialize NFC module!");
        while (1)
            ; // Halt
    }

    // Create queue for write operations
    writeQueue = xQueueCreate(5, sizeof(WriteRequest));

    // Create tasks
    xTaskCreate(WiFiTask, "WiFiTask", 4096, NULL, 1, &WiFiTaskHandle);
    xTaskCreate(ServerTask, "ServerTask", 4096, NULL, 1, &ServerTaskHandle);
    vTaskSuspend(ServerTaskHandle); // Suspend server task until WiFi is connected
}

void loop()
{
    // Check for serial input for manual write commands
    if (tb.connected())
    {
        tb.loop();
    }
}