/**
 * PN532_NFC Class
 * A wrapper class for the Adafruit PN532 library with additional functionality
 * for MIFARE Classic card operations
 */

 #include "main.h" // Include main header for configuration and dependencies

class PN532_NFC {
    private:
        Adafruit_PN532* nfc;
        SemaphoreHandle_t mutex;
        
        // Card state information
        volatile bool _cardPresent;
        uint8_t _uid[7];
        uint8_t _uidLength;
        uint8_t _keyA[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Default key
        
    public:
        /**
         * Constructor
         * @param sda SDA pin for I2C communication
         * @param scl SCL pin for I2C communication
         */
        PN532_NFC(uint8_t sda, uint8_t scl) {
            nfc = new Adafruit_PN532(sda, scl);
            mutex = xSemaphoreCreateMutex();
            _cardPresent = false;
        }
        
        /**
         * Destructor
         */
        ~PN532_NFC() {
            delete nfc;
            vSemaphoreDelete(mutex);
        }
        
        /**
         * Initialize the NFC module
         * @return True if initialization successful, false otherwise
         */
        bool begin() {
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
        
        /**
         * Check if a card is present
         * @return True if card is present, false otherwise
         */
        bool isCardPresent() {
            return _cardPresent;
        }
        
        /**
         * Get the UID of the card as a string
         * @return UID in XX:XX:XX:XX format
         */
        String getUidString() {
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
        
        /**
         * Get the raw UID byte array
         * @param buffer Buffer to copy the UID to
         * @return Length of the UID
         */
        uint8_t getUidBytes(uint8_t* buffer) {
            if (!_cardPresent) return 0;
            memcpy(buffer, _uid, _uidLength);
            return _uidLength;
        }
        
        /**
         * Set custom authentication key
         * @param keyA Pointer to 6-byte key A
         */
        void setKey(const uint8_t* keyA) {
            memcpy(_keyA, keyA, 6);
        }
        
        /**
         * Scan for a card
         * @param timeout Timeout in milliseconds (0 for default)
         * @return True if card found, false otherwise
         */
        bool scanCard(uint16_t timeout = 500) {
            if (xSemaphoreTake(mutex, pdMS_TO_TICKS(timeout)) != pdTRUE) {
                Serial.println("[ERROR] Failed to acquire NFC mutex");
                return false;
            }
            
            _cardPresent = nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, _uid, &_uidLength);
            
            xSemaphoreGive(mutex);
            return _cardPresent;
        }
        
        /**
         * Authenticate with the card using stored key
         * @param block Block number to authenticate
         * @return True if successful, false otherwise
         */
        bool authenticate(uint8_t block) {
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
        
        /**
         * Read block data
         * @param block Block number to read
         * @param data Buffer to store the data (must be at least 16 bytes)
         * @return True if successful, false otherwise
         */
        bool readBlock(uint8_t block, uint8_t* data) {
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
        
        /**
         * Write block data
         * @param block Block number to write
         * @param data Data to write (must be 16 bytes)
         * @return True if successful, false otherwise
         */
        bool writeBlock(uint8_t block, const uint8_t* data) {
            if (!_cardPresent) return false;
            
            if (xSemaphoreTake(mutex, pdMS_TO_TICKS(500)) != pdTRUE) {
                return false;
            }
            
            bool result = nfc->mifareclassic_WriteDataBlock(block, const_cast<uint8_t*>(data));
            
            if (result) {
                Serial.printf("[SUCCESS] Data written to block %d\n", block);
            } else {
                Serial.printf("[ERROR] Failed to write to block %d\n", block);
            }
            
            xSemaphoreGive(mutex);
            return result;
        }
        
        /**
         * Read block as string
         * @param block Block number to read
         * @return String representation of block data
         */
        String readBlockAsString(uint8_t block) {
            uint8_t data[16];
            String result = "";
            
            if (readBlock(block, data)) {
                // Find length (stop at first null byte)
                int actualLength = 0;
                for (actualLength = 0; actualLength < 16; actualLength++) {
                    if (data[actualLength] == 0) break;
                }
                
                if (actualLength > 0) {
                    char tempBuffer[17]; // 16 bytes + null terminator
                    memset(tempBuffer, 0, sizeof(tempBuffer));
                    memcpy(tempBuffer, data, actualLength);
                    result = String(tempBuffer);
                }
            }
            
            return result;
        }
        
        /**
         * Write string to block
         * @param block Block number to write
         * @param data String to write (max 16 characters)
         * @return True if successful, false otherwise
         */
        bool writeBlockString(uint8_t block, const String& data) {
            uint8_t blockData[16] = {0}; // Initialize with zeros
            
            // Copy string to block data
            size_t dataLen = data.length() > 16 ? 16 : data.length();
            memcpy(blockData, data.c_str(), dataLen);
            
            // Write block
            return writeBlock(block, blockData);
        }
        
        /**
         * Clear block (write all zeros)
         * @param block Block number to clear
         * @return True if successful, false otherwise
         */
        bool clearBlock(uint8_t block) {
            uint8_t emptyBlock[16] = {0};
            return writeBlock(block, emptyBlock);
        }
    };