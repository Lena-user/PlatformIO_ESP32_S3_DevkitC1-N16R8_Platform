#include "main.h"

PN532Reader nfcReader(SDA_PIN, SCL_PIN); // Khởi tạo PN532 với SDA = GPIO14, SCL = GPIO13

void setup()
{
    Serial.begin(115200);
    nfcReader.begin(); // Khởi tạo PN532
    Serial.println("PN532 Initialized and Ready!");
}

void loop()
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
            if (nfcReader.writeBlock(block, "Love Mika!!!"))
            { // Ghi dữ liệu vào block
                Serial.println("Data written successfully!");
                std::string data = nfcReader.readBlockAsString(block); // Đọc dữ liệu từ block
                Serial.print("Data from block: ");
                Serial.println(data.c_str()); // In dữ liệu ra Serial Monitor
            }
            else
            {
                Serial.println("Failed to write data!");
            }
        }
        else
        {
            Serial.println("Authentication failed!");
        }
    }
    else
    {
        Serial.println("No NFC card detected!");
    }
    delay(NFC_SCAN_DELAY);
}