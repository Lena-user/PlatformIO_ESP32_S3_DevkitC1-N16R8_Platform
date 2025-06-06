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

// Server task function - MODIFIED TO RUN CONTINUOUSLY
void serverTask(void *pvParameters)
{
    log("INFO", "Server", "Task started, waiting for WiFi to connect...");
    // ServerTask thường được resume bởi WiFiTask, nên không cần đợi WiFi ở đây nữa.

    bool tasks_created = false; // Cờ để đảm bảo task con chỉ tạo 1 lần

    while (true) // Vòng lặp chính của ServerTask
    {
        // 1. Kiểm tra và kết nối ThingsBoard nếu cần
        if (!tb.connected())
        {
            log("INFO", "Server", "ThingsBoard disconnected. Attempting to reconnect...");
            // Vòng lặp thử kết nối lại
            while (!tb.connect(THINGSBOARD_SERVER, DEVICE_ACCESS_TOKEN, THINGSBOARD_PORT))
            {
                log("ERROR", "Server", "Failed to reconnect to ThingsBoard. Retrying in 5s...");
                vTaskDelay(pdMS_TO_TICKS(5000)); // Đợi 5 giây trước khi thử lại
            }
            // Sau khi kết nối lại thành công
            log("INFO", "Server", "Reconnected to ThingsBoard successfully!");
            roomManager.sendInitialStates(); // Gửi lại trạng thái ban đầu
            subscribed = false; // Đặt lại cờ để subscribe lại RPC
        }

        // 2. Nếu đã kết nối, thực hiện các hoạt động cần thiết
        if (tb.connected())
        {
            // Đăng ký RPC nếu chưa làm hoặc sau khi kết nối lại
            if (!subscribed)
            {
                log("INFO", "Server", "Subscribing for RPC...");
                const std::array<RPC_Callback, MAX_RPC_SUBSCRIPTIONS> callbacks = {
                    RPC_Callback{RPC_CALLBACK.at("METHODS")[0].c_str(), CheckInnProcess}};

                if (!rpc.RPC_Subscribe(callbacks.cbegin(), callbacks.cend())) // Giả sử rpc là đối tượng toàn cục hoặc có thể truy cập
                {
                    log("ERROR", "Server", "Failed to subscribe for RPC");
                    // Có thể thêm logic thử lại subscribe ở đây nếu cần
                }
                else
                {
                    log("INFO", "Server", "RPC Subscribe done.");
                    subscribed = true;
                }
            }

            // Tạo các task đọc/ghi NFC chỉ một lần và khi đã kết nối + subscribe
            if (!tasks_created && subscribed)
            {
                log("INFO", "Server", "Creating NFC Read/Write tasks...");
                if (readTaskHandle == NULL) { // Kiểm tra để tránh tạo lại nếu đã có
                    xTaskCreatePinnedToCore(readTask, "ReadTask", NFC_READ_TASK_STACK_SIZE, NULL, NFC_TASK_PRIORITY, &readTaskHandle, 1);
                }
                if (writeTaskHandle == NULL) { // Kiểm tra để tránh tạo lại nếu đã có
                    xTaskCreatePinnedToCore(writeTask, "WriteTask", NFC_WRITE_TASK_STACK_SIZE, NULL, NFC_TASK_PRIORITY, &writeTaskHandle, 0);
                }
                if(readTaskHandle != NULL && writeTaskHandle != NULL) {
                    tasks_created = true;
                    log("INFO", "Server", "NFC Read/Write tasks created.");
                } else {
                    log("ERROR", "Server", "Failed to create one or more NFC tasks.");
                }
            }

            // --- TÍCH HỢP LOGIC TỪ LOOP() VÀO ĐÂY ---
            tb.loop();            // Gọi ThingsBoard loop để xử lý message đến/đi
            roomManager.update(); // Cập nhật trạng thái room manager
            // --- KẾT THÚC PHẦN TÍCH HỢP ---

        } // Kết thúc khối if (tb.connected())

        // Delay để nhường CPU và định tần suất hoạt động
        // Tần suất này sẽ ảnh hưởng đến việc tb.loop() và roomManager.update() được gọi
        vTaskDelay(pdMS_TO_TICKS(100)); // Ví dụ: chạy mỗi 100ms
    }
    // Dòng vTaskDelete(serverTaskHandle); sẽ không bao giờ được đạt tới trong cấu trúc này
}