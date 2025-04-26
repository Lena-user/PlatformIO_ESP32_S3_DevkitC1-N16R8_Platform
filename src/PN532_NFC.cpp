#include "PN532_NFC.h"
#include "config.h"

// Constructor
PN532_NFC::PN532_NFC(uint8_t sda, uint8_t scl) {
    nfc = new Adafruit_PN532(sda, scl);
    mutex = xSemaphoreCreateMutex();
    _cardPresent = false;
    
    // Set default key
    memcpy(_keyA, NFCConfig::DEFAULT_KEY, 6);
}

// Destructor
PN532_NFC::~PN532_NFC() {
    delete nfc;
    vSemaphoreDelete(mutex);
}

// Initialize the NFC module
bool PN532_NFC::begin() {
    nfc->begin();
    uint32_t versiondata = nfc->getFirmwareVersion();
    if (!versiondata) {
        Serial.println("[ERROR] Couldn't find PN532 board");
        return false;
    }
    
    // Got valid data, print it out
    Serial.print("[INFO] Found PN532 with firmware version: ");
    Serial.print((versiondata >> 24) & 0xFF, HEX); 
    Serial.print(".");
    Serial.println((versiondata >> 16) & 0xFF, HEX);
    
    // Configure board to read RFID tags
    nfc->SAMConfig();
    Serial.println("[INFO] PN532 Ready!");
    return true;
}

// Check if a card is present
bool PN532_NFC::isCardPresent() {
    return _cardPresent;
}

// Get the UID of the card as a string
String PN532_NFC::getUidString() {
    if (!_cardPresent) return "";
    
    String result = "";
    for (uint8_t i = 0; i < _uidLength; i++) {
        if (_uid[i] < 0x10) {
            result += "0";
        }
        result += String(_uid[i], HEX);
        
        if (i < _uidLength - 1) {
            result += ":";
        }
    }
    result.toUpperCase();
    return result;
}

// Get the raw UID byte array
uint8_t PN532_NFC::getUidBytes(uint8_t* buffer) {
    if (!_cardPresent) return 0;
    memcpy(buffer, _uid, _uidLength);
    return _uidLength;
}

// Set custom authentication key
void PN532_NFC::setKey(const uint8_t* keyA) {
    memcpy(_keyA, keyA, 6);
}

// Scan for a card
bool PN532_NFC::scanCard(uint16_t timeout) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(timeout)) != pdTRUE) {
        Serial.println("[ERROR] Failed to acquire NFC mutex");
        return false;
    }
    
    _cardPresent = nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, _uid, &_uidLength);
    
    xSemaphoreGive(mutex);
    return _cardPresent;
}

// Authenticate with the card using stored key
bool PN532_NFC::authenticate(uint8_t block) {
    if (!_cardPresent) return false;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(500)) != pdTRUE) {
        return false;
    }
    
    bool result = nfc->mifareclassic_AuthenticateBlock(_uid, _uidLength, block, 0, _keyA);
    
    if (result) {
        Serial.printf("[SUCCESS] Block %d authenticated\n", block);
    } else {
        Serial.printf("[ERROR] Block %d authentication failed\n", block);
    }
    
    xSemaphoreGive(mutex);
    return result;
}

// Read block data
bool PN532_NFC::readBlock(uint8_t block, uint8_t* data) {
    if (!_cardPresent) return false;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(500)) != pdTRUE) {
        return false;
    }
    
    bool result = nfc->mifareclassic_ReadDataBlock(block, data);
    
    if (result) {
        Serial.printf("[SUCCESS] Block %d read\n", block);
    } else {
        Serial.printf("[ERROR] Failed to read block %d\n", block);
    }
    
    xSemaphoreGive(mutex);
    return result;
}

// Write block data
bool PN532_NFC::writeBlock(uint8_t block, const uint8_t* data) {
    if (!_cardPresent) return false;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(500)) != pdTRUE) {
        return false;
    }
    
    uint8_t tempData[16];
    memcpy(tempData, data, 16); // Copy data to a non-const buffer
    bool result = nfc->mifareclassic_WriteDataBlock(block, tempData);
    
    if (result) {
        Serial.printf("[SUCCESS] Data written to block %d\n", block);
    } else {
        Serial.printf("[ERROR] Failed to write to block %d\n", block);
    }
    
    xSemaphoreGive(mutex);
    return result;
}

// Read block as string
String PN532_NFC::readBlockAsString(uint8_t blockNumber) {
    uint8_t data[16];
    if (!readBlock(blockNumber, data)) {
        return "";
    }
    
    // Debug: show hex values of what we're reading
    Serial.print("HEX dump of block ");
    Serial.print(blockNumber);
    Serial.print(": ");
    for (int i = 0; i < 16; i++) {
        if (data[i] < 0x10) Serial.print("0"); // Add leading zero for single-digit hex
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    
    // Convert to string
    String result = "";
    for (int i = 0; i < 16; i++) {
        // Stop at null terminator or non-printable chars
        if (data[i] == 0 || data[i] < 32) break;
        result += (char)data[i];
    }
    
    return result;
}

// Write string to block
bool PN532_NFC::writeBlockString(uint8_t block, const String& data) {
    uint8_t blockData[16] = {0}; // Initialize with zeros
    
    // Copy string to block data
    size_t dataLen = data.length() > 16 ? 16 : data.length();
    memcpy(blockData, data.c_str(), dataLen);
    
    // Write block
    return writeBlock(block, blockData);
}

// Clear block (write all zeros)
bool PN532_NFC::clearBlock(uint8_t block) {
    uint8_t emptyBlock[16] = {0};
    return writeBlock(block, emptyBlock);
}