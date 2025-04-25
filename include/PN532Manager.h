#ifndef PN532_MANAGER_H
#define PN532_MANAGER_H

#include <Wire.h>
#include <Adafruit_PN532.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class PN532Manager {
private:
    // Hardware configuration
    uint8_t _sda_pin;
    uint8_t _scl_pin;
    Adafruit_PN532* _nfc;
    
    // Thread safety
    SemaphoreHandle_t _mutex;
    
    // Card state tracking
    bool _cardPresent;
    uint8_t _uid[7];
    uint8_t _uidLength;
    
    // Authentication keys
    uint8_t _keyA[6];
    uint8_t _keyB[6];

public:
    /**
     * Constructor
     * @param sda_pin SDA pin for I2C communication
     * @param scl_pin SCL pin for I2C communication
     */
    PN532Manager(uint8_t sda_pin, uint8_t scl_pin);
    
    /**
     * Destructor
     */
    ~PN532Manager();
    
    /**
     * Initialize the PN532 module
     * @return true if initialization succeeded, false otherwise
     */
    bool begin();
    
    /**
     * Try to detect a card
     * @param timeout Maximum time to wait for a card in milliseconds
     * @return true if a card was detected, false otherwise
     */
    bool detectCard(uint16_t timeout = 100);
    
    /**
     * Get the UID of the last detected card
     * @param uid Pointer to array where UID will be stored
     * @param uidLength Pointer to variable where UID length will be stored
     */
    void getUID(uint8_t* uid, uint8_t* uidLength);
    
    /**
     * Authenticate a block using key A
     * @param blockNumber Block number to authenticate
     * @param keyA Key A to use for authentication (NULL to use default)
     * @return true if authentication succeeded, false otherwise
     */
    bool authenticateBlockA(uint8_t blockNumber, uint8_t* keyA = NULL);
    
    /**
     * Authenticate a block using key B
     * @param blockNumber Block number to authenticate
     * @param keyB Key B to use for authentication (NULL to use default)
     * @return true if authentication succeeded, false otherwise
     */
    bool authenticateBlockB(uint8_t blockNumber, uint8_t* keyB = NULL);
    
    /**
     * Read data from a block
     * @param blockNumber Block number to read
     * @param data Pointer to array where data will be stored (must be at least 16 bytes)
     * @param authenticate Whether to authenticate the block before reading
     * @return true if read succeeded, false otherwise
     */
    bool readBlock(uint8_t blockNumber, uint8_t* data, bool authenticate = true);
    
    /**
     * Write data to a block
     * @param blockNumber Block number to write
     * @param data Pointer to array containing data to write (must be exactly 16 bytes)
     * @param authenticate Whether to authenticate the block before writing
     * @return true if write succeeded, false otherwise
     */
    bool writeBlock(uint8_t blockNumber, uint8_t* data, bool authenticate = true);
    
    /**
     * Set the default key A
     * @param keyA Pointer to array containing key A (must be exactly 6 bytes)
     */
    void setDefaultKeyA(uint8_t* keyA);
    
    /**
     * Set the default key B
     * @param keyB Pointer to array containing key B (must be exactly 6 bytes)
     */
    void setDefaultKeyB(uint8_t* keyB);
    
    /**
     * Check if a card is currently present
     * @return true if a card is present, false otherwise
     */
    bool isCardPresent();
    
    /**
     * Force the state to "no card present"
     * Useful when a card has been removed but the class doesn't know it yet
     */
    void resetCardState();
    
    /**
     * Lock access to the PN532
     * @param timeout Maximum time to wait for the lock in milliseconds
     * @return true if lock was acquired, false otherwise
     */
    bool lock(uint32_t timeout = portMAX_DELAY);
    
    /**
     * Unlock access to the PN532
     */
    void unlock();
};

#endif // PN532_MANAGER_H