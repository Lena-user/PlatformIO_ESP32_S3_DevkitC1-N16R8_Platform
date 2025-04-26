#ifndef ROOM_H
#define ROOM_H

#include <Arduino.h>

class Room {
private:
    String _id;               // Room identifier
    String _name;             // Room name
    bool _isOccupied;         // Whether room is occupied
    String _occupantUid;      // UID of occupying card
    unsigned long _checkInTime; // Time of check-in
    
public:
    // Constructor
    Room() : _id(""), _name(""), _isOccupied(false), _occupantUid(""), _checkInTime(0) {}
    Room(const String& id) : _id(id), _name("Room_" + id), _isOccupied(false), _occupantUid(""), _checkInTime(0) {}
    Room(const String& id, const String& name) : _id(id), _name(name), _isOccupied(false), _occupantUid(""), _checkInTime(0) {}
    
    // Getters
    String getId() const { return _id; }
    String getName() const { return _name; }
    bool isOccupied() const { return _isOccupied; }
    String getOccupantUid() const { return _occupantUid; }
    
    // Room operations
    bool checkIn(const String& uid) {
        if (_isOccupied) {
            return false; // Already occupied
        }
        
        _isOccupied = true;
        _occupantUid = uid;
        _checkInTime = millis();
        return true;
    }
    
    bool checkOut(const String& uid) {
        if (!_isOccupied) {
            return false; // Not occupied
        }
        
        if (_occupantUid != uid) {
            return false; // Not the same card
        }
        
        _isOccupied = false;
        _occupantUid = "";
        _checkInTime = 0;
        return true;
    }
    
    // Get occupancy duration in seconds
    unsigned long getOccupancyDuration() const {
        if (!_isOccupied || _checkInTime == 0) {
            return 0;
        }
        return (millis() - _checkInTime) / 1000;
    }
    
    // Print room information
    void printInfo() const {
        Serial.println("==== ROOM INFORMATION ====");
        Serial.println("ID: " + _id);
        Serial.println("Name: " + _name);
        Serial.println("Status: " + String(_isOccupied ? "Occupied" : "Available"));
        
        if (_isOccupied) {
            Serial.println("Occupant: " + _occupantUid);
            Serial.println("Duration: " + String(getOccupancyDuration()) + " seconds");
        }
        Serial.println("=========================");
    }
};

#endif // ROOM_H