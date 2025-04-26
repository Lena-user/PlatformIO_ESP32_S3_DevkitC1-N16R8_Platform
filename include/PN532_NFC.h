#ifndef PN532_NFC_H
#define PN532_NFC_H

#include <Arduino.h>
#include <Adafruit_PN532.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <Wire.h>

/**
 * PN532_NFC Class
 * A wrapper class for the Adafruit PN532 library with additional functionality
 * for MIFARE Classic card operations
 */
class PN532_NFC {
    private:
        Adafruit_PN532* nfc;
        SemaphoreHandle_t mutex;
        
        // Card state information
        volatile bool _cardPresent;
        uint8_t _uid[7];
        uint8_t _uidLength;
        uint8_t _keyA[6]; // Default key is initialized in constructor
        
    public:
        /**
         * Constructor
         * @param sda SDA pin for I2C communication
         * @param scl SCL pin for I2C communication
         */
        PN532_NFC(uint8_t sda, uint8_t scl);
        
        /**
         * Destructor
         */
        ~PN532_NFC();
        
        /**
         * Initialize the NFC module
         * @return True if initialization successful, false otherwise
         */
        bool begin();
        
        /**
         * Check if a card is present
         * @return True if card is present, false otherwise
         */
        bool isCardPresent();
        
        /**
         * Get the UID of the card as a string
         * @return UID in XX:XX:XX:XX format
         */
        String getUidString();
        
        /**
         * Get the raw UID byte array
         * @param buffer Buffer to copy the UID to
         * @return Length of the UID
         */
        uint8_t getUidBytes(uint8_t* buffer);
        
        /**
         * Set custom authentication key
         * @param keyA Pointer to 6-byte key A
         */
        void setKey(const uint8_t* keyA);
        
        /**
         * Scan for a card
         * @param timeout Timeout in milliseconds (0 for default)
         * @return True if card found, false otherwise
         */
        bool scanCard(uint16_t timeout = 500);
        
        /**
         * Authenticate with the card using stored key
         * @param block Block number to authenticate
         * @return True if successful, false otherwise
         */
        bool authenticate(uint8_t block);
        
        /**
         * Read block data
         * @param block Block number to read
         * @param data Buffer to store the data (must be at least 16 bytes)
         * @return True if successful, false otherwise
         */
        bool readBlock(uint8_t block, uint8_t* data);
        
        /**
         * Write block data
         * @param block Block number to write
         * @param data Data to write (must be 16 bytes)
         * @return True if successful, false otherwise
         */
        bool writeBlock(uint8_t block, const uint8_t* data);
        
        /**
         * Read block as string
         * @param block Block number to read
         * @return String representation of block data
         */
        String readBlockAsString(uint8_t block);
        
        /**
         * Write string to block
         * @param block Block number to write
         * @param data String to write (max 16 characters)
         * @return True if successful, false otherwise
         */
        bool writeBlockString(uint8_t block, const String& data);
        
        /**
         * Clear block (write all zeros)
         * @param block Block number to clear
         * @return True if successful, false otherwise
         */
        bool clearBlock(uint8_t block);
};

#endif // PN532_NFC_H