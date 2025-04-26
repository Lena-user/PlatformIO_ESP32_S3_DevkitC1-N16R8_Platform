#include "main.h"

// Create global instances for testing
DoorControl doorControl; // No GPIO pin needed, just simulation
RoomManager roomManager;

// Logging function
void log(const char* level, const char* component, const char* message) {
    Serial.printf("[%s][%s] %s\n", level, component, message);
}

// Setup WiFi connection
bool setupWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi.");
    
    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < 20) {
        delay(500);
        Serial.print(".");
        attempt++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.println("Connected to WiFi");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        return true;
    } else {
        Serial.println();
        Serial.println("Failed to connect to WiFi");
        return false;
    }
}

// NFC read task
void readTask(void *pvParameters) {
    const TickType_t xDelay = pdMS_TO_TICKS(NFC_SCAN_DELAY);
    
    // Create a user object for the current card
    User currentUser;
    String lastUid = "";
    unsigned long lastReadTime = 0;
    
    while (true) {
        // Update door state
        doorControl.update();
        
        // Try to take the NFC mutex with timeout
        if (xSemaphoreTake(nfcMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
            // Scan for NFC card
            if (nfcReader.scanCard()) {
                String uid = nfcReader.getUidString();
                unsigned long currentTime = millis();
                
                // Only process if it's a new card or the same card after a delay (3 seconds)
                if (uid != lastUid || (currentTime - lastReadTime > 3000)) {
                    lastUid = uid;
                    lastReadTime = currentTime;
                    log("INFO", "NFC", ("Card found! UID: " + uid).c_str());
                    
                    // Update user with card UID
                    currentUser.setUid(uid);
                    
                    // Try to authenticate and read data
                    if (nfcReader.authenticate(NFCConfig::DATA_BLOCK)) {
                        String blockData = nfcReader.readBlockAsString(NFCConfig::DATA_BLOCK);
                        log("INFO", "NFC", ("Block data: \"" + blockData + "\"").c_str());
                        
                        // Update user from card data
                        currentUser.updateFromCardData(blockData);
                        
                        // Print user information
                        currentUser.printInfo();
                        
                        // Register card to room if needed (from card data)
                        if (currentUser.getRoomId().length() > 0) {
                            roomManager.registerCard(uid, currentUser.getRoomId());
                        }
                        
                        // Check room status and open door if authorized
                        Room* room = roomManager.getRoomForCard(uid);
                        if (room != nullptr) {
                            // Check if room is occupied by this card
                            if (room->isOccupied() && room->getOccupantUid() == uid) {
                                // Check out
                                if (roomManager.checkOut(uid)) {
                                    // Print updated room info
                                    room->printInfo();
                                    
                                    // Open door for check-out
                                    doorControl.openDoor();
                                    log("INFO", "Door", "Opening door for check-out");
                                }
                            } else if (!room->isOccupied()) {
                                // Check in
                                if (roomManager.checkIn(uid)) {
                                    // Print updated room info
                                    room->printInfo();
                                    
                                    // Open door for check-in
                                    doorControl.openDoor();
                                    log("INFO", "Door", "Opening door for check-in");
                                }
                            } else {
                                log("WARN", "Room", "Room occupied by another card");
                            }
                        } else {
                            log("WARN", "NFC", "Card not registered to any room");
                        }
                    } else {
                        log("ERROR", "NFC", "Failed to authenticate with card");
                    }
                }
            }
            
            // Release mutex
            xSemaphoreGive(nfcMutex);
        }
        
        vTaskDelay(xDelay);
    }
}

// NFC write task
void writeTask(void *pvParameters) {
    WriteRequest request;
    
    while (true) {
        if (xQueueReceive(writeQueue, &request, portMAX_DELAY) == pdTRUE) {
            log("INFO", "NFC", "Processing write request");
            
            if (xSemaphoreTake(nfcMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                if (!nfcReader.isCardPresent()) {
                    log("INFO", "NFC", "Waiting for card...");
                    for (int i = 0; i < 10; i++) {
                        if (nfcReader.scanCard(500)) {
                            break;
                        }
                        Serial.print(".");
                    }
                    Serial.println();
                }
                
                if (nfcReader.isCardPresent()) {
                    String uid = nfcReader.getUidString();
                    log("INFO", "NFC", ("Card found: " + uid).c_str());
                    
                    if (nfcReader.authenticate(request.blockNumber)) {
                        bool success = false;
                        
                        if (request.isString) {
                            success = nfcReader.writeBlockString(request.blockNumber, request.stringData);
                        } else {
                            success = nfcReader.writeBlock(request.blockNumber, request.data);
                        }
                        
                        if (success) {
                            log("INFO", "NFC", "Write successful");
                        } else {
                            log("ERROR", "NFC", "Write failed");
                        }
                    } else {
                        log("ERROR", "NFC", "Authentication failed");
                    }
                } else {
                    log("ERROR", "NFC", "No card found for writing");
                }
                
                xSemaphoreGive(nfcMutex);
            }
        }
    }
}

// Register new card with more compact data format
void registerCardCommand(String roomId) {
    log("INFO", "System", ("Registering new card for room " + roomId).c_str());
    
    WriteRequest request;
    request.blockNumber = NFCConfig::DATA_BLOCK;
    request.isString = true;
    
    // More compact format to fit in 16 bytes
    // Format: N:xxx;R:yyy
    String cardData = "N:Guest;R:" + roomId;
    
    // Ensure it fits in 16 bytes
    if (cardData.length() > 16) {
        cardData = cardData.substring(0, 16);
    }
    
    request.stringData = cardData;
    
    Serial.println("Writing data: \"" + cardData + "\"");
    Serial.println("Data length: " + String(cardData.length()) + " bytes");
    
    if (xQueueSend(writeQueue, &request, pdMS_TO_TICKS(1000)) != pdTRUE) {
        log("ERROR", "System", "Failed to queue write request");
    }
}
// Main setup function
void setup() {
    // Initialize serial
    Serial.begin(SERIAL_BAUD);
    delay(1000);
    Serial.println("\n\n==================================");
    Serial.println("NFC Door Control Test System Starting");
    Serial.println("==================================\n");
    
    // Initialize I2C
    Wire.begin(SDA_PIN, SCL_PIN);
    
    // Initialize NFC reader
    if (!nfcReader.begin()) {
        log("ERROR", "System", "Failed to initialize NFC module");
        while(1); // Critical error
    }
    
    // Create queue for write operations
    writeQueue = xQueueCreate(5, sizeof(WriteRequest));
    nfcMutex = xSemaphoreCreateMutex();
    
    if (writeQueue == NULL || nfcMutex == NULL) {
        log("ERROR", "System", "Failed to create queue or mutex");
        while(1); // Critical error
    }
    
    // Create server task in suspended state
    xTaskCreatePinnedToCore(
        serverTask,
        "ServerTask",
        SERVER_TASK_STACK_SIZE,
        NULL,
        SERVER_TASK_PRIORITY,
        &serverTaskHandle,
        0  // Core 0
    );
    vTaskSuspend(serverTaskHandle);
    
    // Create WiFi task
    xTaskCreatePinnedToCore(
        wifiTask,
        "WiFiTask",
        WIFI_TASK_STACK_SIZE,
        NULL,
        WIFI_TASK_PRIORITY,
        &wifiTaskHandle,
        0  // Core 0
    );
    
    // Print room status
    roomManager.printAllRoomsStatus();
    
    Serial.println("\nSystem ready! Commands:");
    Serial.println("reg:101 - Register card for room 101");
    Serial.println("reg:102 - Register card for room 102");
    Serial.println("reg:103 - Register card for room 103");
    Serial.println("status - Show all room status");
}

// Main loop
void loop() {
    // Update door state - add this line!
    doorControl.update();
    
    // Check for serial commands
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command.startsWith("reg:")) {
            String roomId = command.substring(4);
            registerCardCommand(roomId);
        }
        else if (command == "status") {
            roomManager.printAllRoomsStatus();
        }
    }
    
    // Just a small delay
    delay(100);  // This should be small enough to detect the 5-second timeout
}