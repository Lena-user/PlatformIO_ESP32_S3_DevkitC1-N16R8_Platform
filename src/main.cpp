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
                if (uid != lastUid || (currentTime - lastReadTime > 1000))
                {
                    lastUid = uid;
                    lastReadTime = currentTime;
                    log("INFO", "NFC", ("Card found! UID: " + uid).c_str());

                    if (roomManager.checkIn(uid))
                        log("INFO", "NFC", ("Room found: " + roomManager.getRoomForCard(uid)->getName()).c_str());
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
    const unsigned long CARD_WAIT_TIMEOUT_MS = 20000; // Chờ thẻ tối đa 20 giây

    while (true)
    {
        // Chờ yêu cầu ghi từ hàng đợi
        if (xQueueReceive(writeQueue, &request, portMAX_DELAY) == pdTRUE)
        {
            Serial.println("------------------ NFC Write Task Processing -----------------");
            log("INFO", "NFC", "Processing write request");

            // Cố gắng lấy quyền truy cập NFC
            if (xSemaphoreTake(nfcMutex, pdMS_TO_TICKS(NFC_WRITE_DELAY)) == pdTRUE)
            {
                bool cardFound = false;
                unsigned long waitStartTime = millis();
                String uid = ""; // Lưu UID khi tìm thấy thẻ

                // --- Logic chờ thẻ (sử dụng millis() để chính xác hơn) ---
                log("INFO", "NFC", "Waiting for card to write...");
                while (millis() - waitStartTime < CARD_WAIT_TIMEOUT_MS)
                {
                    if (nfcReader.scanCard(100)) // Quét nhanh trong 100ms
                    {
                        uid = nfcReader.getUidString(); // Lấy UID khi tìm thấy
                        cardFound = true;
                        log("INFO", "NFC", ("Card detected: " + uid).c_str());
                        break; // Thoát vòng lặp chờ
                    }
                    Serial.print(".");
                    vTaskDelay(pdMS_TO_TICKS(100)); // Chờ một chút trước khi quét lại
                }
                Serial.println(); // Xuống dòng sau khi chờ

                // Nếu tìm thấy thẻ trong thời gian chờ
                if (cardFound)
                {
                    Room *room = roomManager.getRoomForCard(uid); // Kiểm tra thẻ đã đăng ký chưa

                    // --- Xử lý dựa trên trạng thái thẻ và loại request ---

                    // Trường hợp 1: Thẻ CHƯA đăng ký (room == nullptr)
                    if (room == nullptr)
                    {
                        // 1a: Nếu request là CHECK_IN (có số phòng) -> Đăng ký
                        if (!request.stringData.isEmpty())
                        {
                            log("INFO", "NFC", "Card not registered. Processing registration...");
                            String dataToWrite = "ROOM:" + request.stringData;
                            log("INFO", "NFC", ("Data to write: \"" + dataToWrite + "\"").c_str());

                            if (nfcReader.authenticate(request.blockNumber))
                            {
                                bool writeSuccess = false;
                                if (request.isString)
                                    writeSuccess = nfcReader.writeBlockString(request.blockNumber, dataToWrite);
                                else
                                    writeSuccess = nfcReader.writeBlock(request.blockNumber, request.data); // Giữ lại nếu cần ghi byte thô

                                if (writeSuccess)
                                {
                                    log("INFO", "NFC", "Write successful");
                                    // Đăng ký với RoomManager
                                    if (roomManager.registerCard(uid, dataToWrite))
                                        log("INFO", "System", ("Card " + uid + " registered successfully to Room " + request.stringData).c_str());
                                    else
                                        log("ERROR", "System", ("Failed to register card " + uid + " with RoomManager after writing.").c_str());
                                }
                                else
                                    log("ERROR", "NFC", "Write failed during registration");
                            }
                            else
                                log("ERROR", "NFC", "Authentication failed for writing");
                        }
                        // 1b: Nếu request là CHECK_OUT (stringData rỗng) cho thẻ chưa đăng ký -> Bỏ qua
                        else
                        {
                            log("WARN", "NFC", "Cannot perform CHECK_OUT. Card not registered.");
                        }
                    }
                    // Trường hợp 2: Thẻ ĐÃ đăng ký (room != nullptr)
                    else
                    {
                        // 2a: Nếu request là CHECK_OUT (stringData rỗng) -> Thực hiện check-out
                        if (request.stringData == "")
                        {
                            log("INFO", "NFC", ("Card registered to Room " + room->getId() + ". Processing CHECK_OUT...").c_str());
                            // Xác thực trước khi xóa dữ liệu thẻ
                            if (nfcReader.authenticate(request.blockNumber))
                            {
                                // Ghi chuỗi rỗng để xóa dữ liệu
                                bool writeSuccess = nfcReader.writeBlockString(request.blockNumber, "");
                                if (writeSuccess)
                                {
                                    log("INFO", "NFC", "Card data cleared successfully.");
                                    // Gọi RoomManager để hoàn tất check-out
                                    if (roomManager.checkOut(uid))
                                    {
                                        log("INFO", "System", ("Card " + uid + " checked out successfully from Room " + room->getId()).c_str());
                                        // Giả định roomManager.checkOut() đã gửi cập nhật lên server
                                    }
                                    else
                                    {
                                        log("ERROR", "System", ("Failed to update RoomManager state for check out of card " + uid).c_str());
                                    }
                                }
                                else
                                {
                                    log("ERROR", "NFC", "Failed to clear card data during check out.");
                                }
                            }
                            else
                            {
                                log("ERROR", "NFC", "Authentication failed for clearing card data.");
                            }
                        }
                        // 2b: Nếu request là CHECK_IN (stringData không rỗng) cho thẻ đã đăng ký -> Bỏ qua
                        else
                        {
                            log("WARN", "NFC", ("Card already registered to Room " + room->getId() + ". CHECK_IN request ignored for writing.").c_str());
                            // Không cần ghi lại hay đăng ký lại
                        }
                    }
                }
                else // Hết thời gian chờ mà không thấy thẻ
                {
                    log("ERROR", "NFC", "Timeout waiting for card to write");
                }

                // Luôn nhả mutex sau khi xử lý xong
                xSemaphoreGive(nfcMutex);
            }
            else // Không lấy được mutex
            {
                log("ERROR", "NFC", "Could not obtain NFC mutex for writing");
                // Cân nhắc: Gửi lại request vào queue hoặc báo lỗi về server?
            }
            Serial.println("------------------ NFC Write Task Finished -----------------");
        }
        // Không cần delay ở đây vì task bị block bởi xQueueReceive
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