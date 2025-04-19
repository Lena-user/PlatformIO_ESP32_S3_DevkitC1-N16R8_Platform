#include "PN532Reader.h"

// Khởi tạo PN532 với SDA và SCL
// SDA: GPIO14, SCL: GPIO13
PN532Reader::PN532Reader(uint8_t sda, uint8_t scl) : nfc(sda, scl), PN532_SDA(sda), PN532_SCL(scl) {}

// Khởi tạo PN532
void PN532Reader::begin()
{
    Wire.begin(PN532_SDA, PN532_SCL);
    nfc.begin();
    nfc.SAMConfig();
    Serial.println("PN532 Initialized and Ready!");
}

// Quét thẻ NFC
// uid[]: Mảng chứa UID của thẻ
// uidLength: Kích thước UID
// Trả về true nếu quét thành công, false nếu không có thẻ nào được quét    
bool PN532Reader::scanCard(uint8_t *uid, uint8_t &uidLength)
{
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength))
    {
        return true;
    }
    return false;
}

// In UID của thẻ ra Serial Monitor
// uid[]: Mảng chứa UID của thẻ
// uidLength: Kích thước UID
// Ví dụ: UID = 0xDE, 0xAD, 0xBE, 0xEF
// In ra: Card UID: DE:AD:BE:EF
void PN532Reader::printUID(uint8_t *uid, uint8_t uidLength)
{
    Serial.print("Card UID: ");
    for (uint8_t i = 0; i < uidLength; i++)
    {
        Serial.printf("%02X", uid[i]);
        if (i < uidLength - 1)
            Serial.print(":");
    }
    Serial.println();
}

// Xác thực block với key A (key[0] = 0xFF, key[1] = 0xFF, key[2] = 0xFF, key[3] = 0xFF, key[4] = 0xFF, key[5] = 0xFF)
// uid[]: Mảng chứa UID của thẻ
// uidLength: Kích thước UID
// block: Block cần xác thực (0-15)
// key: Mảng chứa key A (6 byte)
// Trả về true nếu xác thực thành công, false nếu không xác thực được
bool PN532Reader::authenticateBlock(uint8_t block, uint8_t *key, uint8_t *uid, uint8_t uidLength)
{
    return nfc.mifareclassic_AuthenticateBlock(uid, uidLength, block, 0, key);
}

// Ghi dữ liệu từ mảng uint8_t[]
// Dữ liệu ghi vào block sẽ được lưu trong mảng data[] với kích thước 16 byte
// block: Block cần ghi (0-15)
// data[]: Mảng chứa dữ liệu cần ghi (16 byte)
// Trả về true nếu ghi thành công, false nếu không ghi được
bool PN532Reader::writeBlock(uint8_t block, uint8_t *data)
{
    return nfc.mifareclassic_WriteDataBlock(block, data);
}

// Ghi dữ liệu từ String
bool PN532Reader::writeBlock(uint8_t block, String data)
{
    uint8_t buffer[16];
    memset(buffer, 0, 16); // Xóa dữ liệu cũ
    data.getBytes(buffer, 16);
    return writeBlock(block, buffer);
}

// Ghi dữ liệu từ mảng ký tự const char*
bool PN532Reader::writeBlock(uint8_t block, const char *data)
{
    uint8_t buffer[16];
    memset(buffer, 0, 16);
    strncpy((char *)buffer, data, 16);
    return writeBlock(block, buffer);
}

// Ghi dữ liệu dài hơn 16 byte vào nhiều block liên tiếp
bool PN532Reader::writeBlock(uint8_t startBlock, uint8_t *data, uint8_t dataLength)
{
    uint8_t block = startBlock;
    uint8_t buffer[16];

    for (uint8_t i = 0; i < dataLength; i += 16)
    {
        memset(buffer, 0, 16);
        memcpy(buffer, &data[i], (dataLength - i >= 16) ? 16 : dataLength - i);

        if (!writeBlock(block, buffer))
        {
            Serial.print("Failed to write block ");
            Serial.println(block);
            return false;
        }
        block++;
    }
    return true;
}

// Đọc dữ liệu từ block
// Dữ liệu đọc được sẽ được lưu vào mảng data[] với kích thước 16 byte
// block: Block cần đọc (0-15)
// data[]: Mảng chứa dữ liệu đọc được (16 byte)
// Trả về true nếu đọc thành công, false nếu không đọc được
void PN532Reader::readBlock(uint8_t block, uint8_t *data)
{
    if (nfc.mifareclassic_ReadDataBlock(block, data))
    {
        Serial.print("Data from block ");
        Serial.print(block);
        Serial.print(": ");

        // Hiển thị dữ liệu dạng chuỗi
        Serial.print("[ ");
        for (int i = 0; i < 16; i++)
        {
            Serial.print((char)data[i]); // Chuyển đổi dữ liệu từ uint8_t về char
        }
        Serial.println(" ]");

    }
    else
    {
        Serial.println("Failed to read block.");
    }
}


std::string PN532Reader::readBlockAsString(uint8_t block)
{
    uint8_t data[16];
    std::string result = "";

    if (nfc.mifareclassic_ReadDataBlock(block, data))
    {
        for (int i = 0; i < 16; i++)
        {
            result += (char)data[i];
        }
    }
    return result;
}