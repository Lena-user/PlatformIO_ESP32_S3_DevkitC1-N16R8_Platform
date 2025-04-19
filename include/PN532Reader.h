#ifndef PN532READER_H
#define PN532READER_H

#include <Wire.h>
#include <Adafruit_PN532.h>

class PN532Reader
{
private:
    Adafruit_PN532 nfc;
    uint8_t PN532_SDA;
    uint8_t PN532_SCL;

public:
    PN532Reader(uint8_t sda, uint8_t scl);
    void begin();
    bool scanCard(uint8_t *uid, uint8_t &uidLength);
    void printUID(uint8_t *uid, uint8_t uidLength);
    bool authenticateBlock(uint8_t block, uint8_t *key, uint8_t *uid, uint8_t uidLength);
    bool writeBlock(uint8_t block, uint8_t *data);                          // Ghi từ mảng uint8_t[]
    bool writeBlock(uint8_t block, String data);                            // Ghi từ String
    bool writeBlock(uint8_t block, const char *data);                       // Ghi từ mảng ký tự char*
    bool writeBlock(uint8_t startBlock, uint8_t *data, uint8_t dataLength); // Ghi nhiều block
    void readBlock(uint8_t block, uint8_t *data);
    std::string readBlockAsString(uint8_t block); // Đọc dữ liệu từ block
};

#endif // PN532READER_H