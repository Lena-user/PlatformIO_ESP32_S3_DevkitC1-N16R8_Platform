#include "main.h"

// Logging function
void log(const char *level, const char *component, const char *message)
{
    Serial.printf("[%s][%s] %s\n", level, component, message);
}

// Setup WiFi connection
bool setupWiFi()
{
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi.");

    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < 20)
    {
        delay(500);
        Serial.print(".");
        attempt++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println();
        Serial.println("Connected to WiFi");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        return true;
    }
    else
    {
        Serial.println();
        Serial.println("Failed to connect to WiFi");
        return false;
    }
}

// NFC read task
void readTask(void *pvParameters)
{
    const TickType_t xDelay = pdMS_TO_TICKS(NFC_SCAN_DELAY);

    // Create a user object for the current card
    String lastUid = "";
    unsigned long lastReadTime = 0;

    while (true)
    {
        Serial.println("------------------ NFC Read Task -----------------");
        // Try to take the NFC mutex with timeout
        if (xSemaphoreTake(nfcMutex, pdMS_TO_TICKS(500)) == pdTRUE)
        {
            // Scan for NFC card
            if (nfcReader.scanCard())
            {
                String uid = nfcReader.getUidString();
                unsigned long currentTime = millis();

                // Only process if it's a new card or the same card after a delay (3 seconds)
                if (uid != lastUid || (currentTime - lastReadTime > 3000))
                {
                    lastUid = uid;
                    lastReadTime = currentTime;
                    log("INFO", "NFC", ("Card found! UID: " + uid).c_str());

                    Room *room = roomManager.getRoomForCard(uid);

                    if (room != nullptr)
                    {
                        log("INFO", "NFC", ("Room found: " + room->getName()).c_str());
                        if (roomManager.openRoomDoor(room->getId()))
                            log("INFO", "System", ("Door for Room " + room->getId() + " opened.").c_str());
                        else
                            log("ERROR", "System", ("Failed to open door for Room " + room->getId()).c_str());
                    }
                    else
                        log("ERROR", "NFC", "No room found for this card");

                    // Try to authenticate and read data
                    if (nfcReader.authenticate(NFCConfig::DATA_BLOCK))
                    {
                        String blockData = nfcReader.readBlockAsString(NFCConfig::DATA_BLOCK);
                        log("INFO", "NFC", ("Block data: \"" + blockData + "\"").c_str());
                    }
                    else
                        log("ERROR", "NFC", "Authentication failed");
                }
            }
            // Release mutex
            xSemaphoreGive(nfcMutex);
        }
        Serial.println("------------------ NFC Read Task -----------------");
        vTaskDelay(xDelay);
    }
}

// NFC write task
void writeTask(void *pvParameters)
{
    WriteRequest request;
    while (true)
    {
        Serial.println("------------------ NFC Write Task -----------------");
        if (xQueueReceive(writeQueue, &request, portMAX_DELAY) == pdTRUE)
        {
            log("INFO", "NFC", "Processing write request");

            if (xSemaphoreTake(nfcMutex, pdMS_TO_TICKS(NFC_WRITE_DELAY)) == pdTRUE)
            {
                if (!nfcReader.isCardPresent())
                {
                    log("INFO", "NFC", "Waiting for card...");
                    for (int i = 0; i < 10; i++)
                    {
                        if (nfcReader.scanCard(500))
                            break;
                        Serial.print(".");
                    }
                    Serial.println();
                }
                if (nfcReader.isCardPresent())
                {
                    String uid = nfcReader.getUidString();
                    log("INFO", "NFC", ("Card found: " + uid).c_str());

                    Room *room = roomManager.getRoomForCard(uid);

                    if (room == nullptr)
                    {
                        log("ERROR", "NFC", "No room found for this card");
                        String dataToWrite = "ROOM:" + request.stringData;
                        log("INFO", "NFC", ("Data to write: \"" + dataToWrite + "\"").c_str());
                        if (nfcReader.authenticate(request.blockNumber))
                        {
                            bool success = false;

                            if (request.isString)
                                success = nfcReader.writeBlockString(request.blockNumber, dataToWrite);
                            else
                                success = nfcReader.writeBlock(request.blockNumber, request.data);

                            if (success)
                            {
                                log("INFO", "NFC", "Write successful");
                                if (roomManager.registerCard(uid, dataToWrite))
                                    log("INFO", "System", ("Card " + uid + " registered successfully to Room " + request.stringData).c_str());
                                else
                                    log("ERROR", "System", ("Failed to register card " + uid + " with RoomManager after writing.").c_str());
                            }
                            else
                                log("ERROR", "NFC", "Write failed");
                        }
                        else
                            log("ERROR", "NFC", "Authentication failed");
                    }
                    else
                    {
                        log("ERROR", "NFC", ("Card already registered to Room " + room->getName()).c_str());
                    }
                }
                else
                    log("ERROR", "NFC", "No card present for writing");
                xSemaphoreGive(nfcMutex);
            }
        }
        Serial.println("------------------ NFC Write Task -----------------");
    }
}

// Main setup function
void setup()
{
    // Initialize serial
    Serial.begin(SERIAL_BAUD);

    // Initialize I2C
    Wire.begin(SDA_PIN, SCL_PIN);

    // Initialize NFC reader
    if (!nfcReader.begin())
    {
        log("ERROR", "System", "Failed to initialize NFC module");
        while (1)
            ; // Critical error
    }
    Serial.println("NFC module initialized");

    // Create queue for write operations
    writeQueue = xQueueCreate(5, sizeof(WriteRequest));
    nfcMutex = xSemaphoreCreateMutex();

    if (writeQueue == NULL || nfcMutex == NULL)
    {
        log("ERROR", "System", "Failed to create queue or mutex");
        while (1)
            ; // Critical error
    }

    // Create server task in suspended state
    xTaskCreatePinnedToCore(
        serverTask,
        "ServerTask",
        SERVER_TASK_STACK_SIZE,
        NULL,
        SERVER_TASK_PRIORITY,
        &serverTaskHandle,
        0 // Core 0
    );
    vTaskSuspend(serverTaskHandle);

    // Create WiFi task
    xTaskCreatePinnedToCore(
        wifiTask,
        "WiFiTask",
        WIFI_TASK_STACK_SIZE,
        NULL,
        WIFI_TASK_PRIORITY,
        &wifiTaskHandle,
        0 // Core 0
    );
}

// Main loop
void loop()
{
    if (tb.connected())
    {
        tb.loop(); // Call ThingsBoard loop to process incoming messages
    }
    roomManager.update(); // Update room manager state
    // Just a small delay
    delay(100); // This should be small enough to detect the 5-second timeout
}