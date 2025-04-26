#ifndef DOOR_CONTROL_H
#define DOOR_CONTROL_H

#include <Arduino.h>

class DoorControl {
private:
    bool _isOpen;           // Door state (simulated)
    unsigned long _openTime; // Time when door was opened
    unsigned long _openDuration; // How long to keep door open (ms)
    
public:
    // Constructor - no hardware pin needed
    DoorControl(unsigned long openDuration = 5000) : 
        _isOpen(false), 
        _openTime(0), 
        _openDuration(openDuration) {
        
        // Print initial state
        Serial.println("==== DOOR CONTROL INITIALIZED (SIMULATION) ====");
        Serial.println("Door is locked");
    }
    
    // Open the door (simulation)
    void openDoor() {
        _isOpen = true;
        _openTime = millis();
        Serial.println("\n╔════════════════════════════╗");
        Serial.println("║       DOOR OPENED         ║");
        Serial.println("╚════════════════════════════╝");
        Serial.println("Door will automatically lock in 5 seconds");
    }
    
    // Lock the door (simulation)
    void lockDoor() {
        _isOpen = false;
        Serial.println("\n╔════════════════════════════╗");
        Serial.println("║       DOOR LOCKED         ║");
        Serial.println("╚════════════════════════════╝");
    }
    
    // Check if door should be locked (based on timer)
// Check if door should be locked (based on timer)
void update() {
    if (_isOpen) {
        // Handle millis() overflow safely
        unsigned long currentTime = millis();
        unsigned long elapsedTime;
        
        if (currentTime < _openTime) {
            // Overflow occurred
            elapsedTime = (0xFFFFFFFF - _openTime) + currentTime;
        } else {
            elapsedTime = currentTime - _openTime;
        }
        
        if (elapsedTime >= _openDuration) {
            Serial.println("[DEBUG] Auto-locking door due to timeout");
            Serial.println("[DEBUG] Door was open for: " + String(elapsedTime/1000.0, 1) + " seconds");
            lockDoor();
        }
    }
}
    
    // Getters
    bool isOpen() const { return _isOpen; }
    unsigned long getRemainingOpenTime() const {
        if (!_isOpen) return 0;
        unsigned long elapsed = millis() - _openTime;
        return (elapsed >= _openDuration) ? 0 : (_openDuration - elapsed);
    }
};

#endif // DOOR_CONTROL_H