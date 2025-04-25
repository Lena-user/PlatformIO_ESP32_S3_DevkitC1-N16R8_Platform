#include "PN532Manager.h"

PN532Manager::PN532Manager(uint8_t sda_pin, uint8_t scl_pin) {
    _sda_pin = sda_pin;
    _scl_pin = scl_pin;
    _cardPresent = false;
    _uidLength = 0;
    
    // Default keys (FF FF FF FF FF FF)
    for (int i = 0; i < 6; i++) {
        _keyA[i] = 0xFF;
        _keyB[i] = 0xFF;
    }
    
    // Create mutex
    _mutex = xSemaphoreCreateMutex();
    
    // Create PN532 instance
    _nfc = new Adafruit_PN532(sda_pin, scl_pin);
}

PN532Manager::~PN532Manager() {
    // Clean up
    if (_nfc != NULL) {
        delete _nfc;
    }
    
    if (_mutex != NULL) {
        vSemaphoreDelete(_mutex);
    }
}

bool PN532Manager::begin() {
    if (_nfc == NULL) return false;
    
    _nfc->begin();
    
    // Check if PN532 is responding
    uint32_t versiondata = _nfc->getFirmwareVersion();
    if (!versiondata) {
        return false;
    }
    
    // Configure board
    _nfc->SAMConfig();
    return true;
}

bool PN532Manager::detectCard(uint16_t timeout) {
    if (!lock(timeout)) return false;
    
    _cardPresent = _nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, _uid, &_uidLength);
    
    unlock();
    return _cardPresent;
}

void PN532Manager::getUID(uint8_t* uid, uint8_t* uidLength) {
    if (uid != NULL && uidLength != NULL) {
        *uidLength = _uidLength;
        for (uint8_t i = 0; i < _uidLength; i++) {
            uid[i] = _uid[i];
        }
    }
}

bool PN532Manager::authenticateBlockA(uint8_t blockNumber, uint8_t* keyA) {
    if (!_cardPresent) return false;
    if (!lock()) return false;
    
    bool success = _nfc->mifareclassic_AuthenticateBlock(_uid, _uidLength, blockNumber, 0, 
                                                      (keyA != NULL) ? keyA : _keyA);
    
    unlock();
    return success;
}

bool PN532Manager::readBlock(uint8_t blockNumber, uint8_t* data, bool authenticate) {
    if (!_cardPresent || data == NULL) return false;
    if (!lock()) return false;
    
    bool success = true;
    
    // Authenticate first if requested
    if (authenticate) {
        success = _nfc->mifareclassic_AuthenticateBlock(_uid, _uidLength, blockNumber, 0, _keyA);
    }
    
    // Then read the block if authentication succeeded (or was not requested)
    if (success) {
        success = _nfc->mifareclassic_ReadDataBlock(blockNumber, data);
    }
    
    unlock();
    return success;
}

bool PN532Manager::writeBlock(uint8_t blockNumber, uint8_t* data, bool authenticate) {
    if (!_cardPresent || data == NULL) return false;
    if (!lock()) return false;
    
    bool success = true;
    
    // Authenticate first if requested
    if (authenticate) {
        success = _nfc->mifareclassic_AuthenticateBlock(_uid, _uidLength, blockNumber, 0, _keyA);
    }
    
    // Then write the block if authentication succeeded (or was not requested)
    if (success) {
        success = _nfc->mifareclassic_WriteDataBlock(blockNumber, data);
    }
    
    unlock();
    return success;
}

bool PN532Manager::lock(uint32_t timeout) {
    return (xSemaphoreTake(_mutex, pdMS_TO_TICKS(timeout)) == pdTRUE);
}

void PN532Manager::unlock() {
    xSemaphoreGive(_mutex);
}

bool PN532Manager::isCardPresent() {
    return _cardPresent;
}

void PN532Manager::resetCardState() {
    _cardPresent = false;
}