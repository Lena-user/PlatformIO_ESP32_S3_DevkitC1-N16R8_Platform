#include "main.h"

// Khởi tạo module NFC
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

// FreeRTOS Handles
QueueHandle_t writeQueue;
SemaphoreHandle_t nfcSemaphore; // Mutex to protect NFC access

// Struct để gửi thông tin qua Queue
typedef struct
{
    uint8_t data[16];
    bool needAuth;
} WriteRequest;

// Biến toàn cục để lưu dữ liệu quét
volatile bool cardPresent = false;
uint8_t uid[7] = {0};
uint8_t uidLength = 0;
uint8_t blockData[16] = {0};
uint8_t keyA[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Hàm xác thực thẻ NFC
bool authenticateCard()
{
    if (!cardPresent)
        return false;

    if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keyA))
    {
        Serial.println("✅ Block 4 đã được xác thực.");
        return true;
    }
    else
    {
        Serial.println("❌ Xác thực block thất bại.");
        return false;
    }
}

// Hàm đọc thẻ NFC (chạy nền)
void readTask(void *pvParameters)
{
    while (true)
    {
        // Lấy quyền truy cập NFC
        if (xSemaphoreTake(nfcSemaphore, pdMS_TO_TICKS(500)) == pdTRUE)
        {
            Serial.println("Reading NFC...");
            cardPresent = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
            if (cardPresent)
            {
                Serial.print("📌 Thẻ NFC được phát hiện! UID: ");
                String uidString = "";
                for (uint8_t i = 0; i < uidLength; i++)
                {
                    char hexStr[3];                  // Mảng chứa ký tự HEX (2 ký tự + null)
                    sprintf(hexStr, "%02X", uid[i]); // Chuyển đổi từng byte thành HEX
                    uidString += hexStr;             // Nối vào chuỗi kết quả
                }
                Serial.println(uidString);
                if (publish.publish(uidString.c_str())) {
                    Serial.println("📤 Đã gửi UID lên Adafruit IO!");
                } else {
                    Serial.println("⚠️ Gửi dữ liệu thất bại!");
                }
                if (authenticateCard())
                {
                    if (nfc.mifareclassic_ReadDataBlock(4, blockData))
                    {
                        String blockDataString = String((char *)blockData, 16);
                        Serial.print("📖 Dữ liệu Block 4: ");
                        for (int i = 0; i < 16; i++)
                        {
                            Serial.printf("%c", blockData[i]);
                        }
                        Serial.println(blockDataString);
                    }
                    else
                    {
                        Serial.println("❌ Không thể đọc dữ liệu từ block.");
                    }
                }
            }
            else
            {
                // No card detected
                Serial.println("Không phát hiện thẻ NFC.");
            }

            // Trả lại quyền truy cập NFC
            xSemaphoreGive(nfcSemaphore);
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
            // Lấy quyền truy cập NFC
            if (xSemaphoreTake(nfcSemaphore, pdMS_TO_TICKS(2000)) == pdTRUE)
            {
                Serial.println("Write task has NFC access");

                // Kiểm tra thẻ và xác thực nếu cần
                bool writeReady = false;

                if (cardPresent)
                {
                    // Thẻ đã được phát hiện bởi readTask
                    if (request.needAuth)
                    {
                        // Cần xác thực lại
                        writeReady = authenticateCard();
                    }
                    else
                    {
                        // Đã xác thực trước đó
                        writeReady = true;
                    }
                }
                else
                {
                    // Thử phát hiện thẻ
                    cardPresent = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
                    if (cardPresent)
                    {
                        writeReady = authenticateCard();
                    }
                    else
                    {
                        Serial.println("❌ Không tìm thấy thẻ để ghi!");
                    }
                }

                // Ghi dữ liệu nếu xác thực thành công
                if (writeReady)
                {
                    if (nfc.mifareclassic_WriteDataBlock(4, request.data))
                    {
                        Serial.println("✅ Dữ liệu đã ghi vào thẻ NFC!");
                    }
                    else
                    {
                        Serial.println("❌ Ghi thất bại!");
                    }
                }

                // Trả lại quyền truy cập NFC
                xSemaphoreGive(nfcSemaphore);
            }
            else
            {
                Serial.println("❌ Không thể truy cập NFC để ghi (timeout)");
            }
        }
    }
}

// Hàm yêu cầu ghi dữ liệu từ Serial Monitor
void requestWriteFromSerial()
{
    if (Serial.available() > 0)
    {
        WriteRequest request;
        memset(request.data, 0, 16); // Xóa dữ liệu cũ

        int bytesRead = Serial.readBytes((char *)request.data, 16);

        // Đặt cờ cần xác thực
        request.needAuth = true;

        // Gửi vào Queue
        if (xQueueSend(writeQueue, &request, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            Serial.print("📥 Đã nhận yêu cầu ghi: ");
            for (int i = 0; i < bytesRead; i++)
            {
                Serial.print((char)request.data[i]);
            }
            Serial.println();
        }
        else
        {
            Serial.println("❌ Queue đầy, không thể gửi yêu cầu ghi!");
        }
    }
}

// Khởi động hệ thống
void setup()
{
    Serial.begin(115200);
    while (!Serial)
        delay(10); // Optional: Wait for Serial to be ready

    Wire.begin(SDA_PIN, SCL_PIN);

    nfc.begin();
    uint32_t versiondata = nfc.getFirmwareVersion();
    if (!versiondata)
    {
        Serial.println("Không tìm thấy PN532!");
        while (1)
            ; // Halt
    }

    nfc.SAMConfig();
    Serial.println("PN532 Ready!");

    // Khởi tạo mutex và queue
    nfcSemaphore = xSemaphoreCreateMutex();
    writeQueue = xQueueCreate(5, sizeof(WriteRequest));

    // Tạo task
    xTaskCreate(WiFiTask, "WiFiTask", 4096, NULL, 1, &WiFiTaskHandle);
    xTaskCreate(SeverTask, "ServerTask", 4096, NULL, 1, &ServerTaskHandle);
    vTaskSuspend(ServerTaskHandle); // Tạm dừng task server cho đến khi Wi-Fi kết nối thành công
}

void loop()
{
    requestWriteFromSerial();  // Kiểm tra yêu cầu ghi từ Serial Monitor
    mqtt.processPackets(1000); // Xử lý các gói tin MQTT
    delay(100);                // Đợi 100ms trước khi tiếp tục
}