#include <Wire.h>
#include <Adafruit_PN532.h>

// Chân kết nối PN532 qua I2C
#define SDA_PIN 14
#define SCL_PIN 13

// Khởi tạo module NFC
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

// FreeRTOS
TaskHandle_t readTaskHandle;
TaskHandle_t writeTaskHandle;
QueueHandle_t writeQueue;

// Biến toàn cục để lưu dữ liệu quét
uint8_t uid[7] = {0};
uint8_t uidLength = 0;
uint8_t blockData[16] = {0};
uint8_t keyA[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Hàm đọc thẻ NFC (chạy nền)
void readTask(void *pvParameters) {
    while (true) {
        if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
            Serial.print("📌 Thẻ NFC được phát hiện! UID: ");
            for (uint8_t i = 0; i < uidLength; i++) {
                Serial.printf("%02X ", uid[i]);
            }
            Serial.println("");

            if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keyA)) {
                Serial.println("✅ Block 4 đã được xác thực.");

                if (nfc.mifareclassic_ReadDataBlock(4, blockData)) {
                    Serial.print("📖 Dữ liệu Block 4: ");
                    for (int i = 0; i < 16; i++) {
                        Serial.printf("%c", blockData[i]);
                    }
                    Serial.println("");
                } else {
                    Serial.println("❌ Không thể đọc dữ liệu từ block.");
                }
            } else {
                Serial.println("❌ Xác thực block thất bại.");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));  // Quét mỗi 500ms
    }
}

// Hàm ghi dữ liệu NFC (chạy khi có yêu cầu)
void writeTask(void *pvParameters) {
    uint8_t data[16];

    while (true) {
        if (xQueueReceive(writeQueue, &data, portMAX_DELAY)) {
            delay(500); // Đợi một chút trước khi ghi dữ liệu
            if (nfc.mifareclassic_WriteDataBlock(4, data)) {
                Serial.println("✅ Dữ liệu đã ghi vào thẻ NFC!");
            } else {
                Serial.println("❌ Ghi thất bại!");
            }
        }
    }
}

// Hàm yêu cầu ghi dữ liệu từ Serial Monitor
void requestWriteFromSerial() {
    if (Serial.available()) {
        char inputData[16]; // Dữ liệu ghi (16 byte)
        memset(inputData, 0, 16); // Xóa dữ liệu cũ

        Serial.readBytes(inputData, 16); // Đọc chuỗi từ Serial
        xQueueSend(writeQueue, inputData, portMAX_DELAY); // Gửi vào Queue

        Serial.print("📥 Đã nhận yêu cầu ghi: ");
        Serial.println(inputData);
    }
}

// Khởi động hệ thống
void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN);
    
    nfc.begin();
    nfc.SAMConfig();
    Serial.println("PN532 Ready!");

    writeQueue = xQueueCreate(10, sizeof(uint8_t[16]));

    xTaskCreatePinnedToCore(readTask, "ReadTask", 4096, NULL, 1, &readTaskHandle, 1);
    xTaskCreatePinnedToCore(writeTask, "WriteTask", 4096, NULL, 1, &writeTaskHandle, 0);
}

void loop() {
    requestWriteFromSerial(); // Kiểm tra nếu có yêu cầu ghi từ Serial
    delay(100);  // Tránh tiêu tốn CPU quá mức
}