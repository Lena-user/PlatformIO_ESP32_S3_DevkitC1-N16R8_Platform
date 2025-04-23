#include "main.h"

PN532Reader nfcReader(SDA_PIN, SCL_PIN); // Khởi tạo PN532 với SDA = GPIO14, SCL = GPIO13

void ScanTask(void *pvParameters)
{
    Serial.println("Waiting for NFC card... ");
    uint8_t uid[7];        // Mảng chứa UID của thẻ
    uint8_t uidLength = 0; // Kích thước UID
    if (nfcReader.scanCard(uid, uidLength))
    {                                                          // Quét thẻ NFC
        nfcReader.printUID(uid, uidLength);                    // In UID của thẻ ra Serial Monitor
        uint8_t key[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Key A (6 byte)
        uint8_t block = 4;                                     // Block cần ghi (0-15)
        if (nfcReader.authenticateBlock(block, key, uid, uidLength))
        { // Xác thực block với key A
            
            Serial.println("Authentication successful!");
            vTaskDelay(portTICK_PERIOD_MS);
            xTaskCreate(ReadTask, "ReadTask", 4096, NULL, 1, &ReadTaskHandle); // Tạo task đọc thẻ NFC
        }
        else
        {
            Serial.println("Authentication failed!");
        }
    }
    vTaskDelete(ScanTaskHandle); // Xóa task sau khi hoàn thành
}

void ReadTask(void *pvParameters)
{
    uint8_t block = 4; // Block cần đọc (0-15)
    std::string data = nfcReader.readBlockAsString(block); // Đọc dữ liệu từ block
    Serial.print("Data from block: ");
    Serial.println(data.c_str()); // In dữ liệu ra Serial Monitor
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    xTaskCreate(ScanTask, "ScanTask", 4096, NULL, 1, &ScanTaskHandle); // Tạo lại task quét thẻ NFC
    vTaskDelete(ReadTaskHandle); // Xóa task sau khi hoàn thành
}

void setup()
{
    Serial.begin(115200);
    nfcReader.begin(); // Khởi động PN532
    xTaskCreate(wifiTask, "WiFiTask", 4096, NULL, 1, &WiFiTaskHandle);
    // xTaskCreate(SeverTask, "SeverTask", 4096, NULL, 1, &SeverTaskHandle);
    // xTaskCreate(scanTask, "ScanTask", 4096, NULL, 1, &ScanTaskHandle);
    // xTaskCreate(readTask, "ReadTask", 4096, NULL, 1, &ReadTaskHandle);
    // vTaskSuspend(SeverTaskHandle); // Tạm dừng task SeverTask
    // vTaskSuspend(ScanTaskHandle);  // Tạm dừng task ScanTask
}

void loop()
{

}