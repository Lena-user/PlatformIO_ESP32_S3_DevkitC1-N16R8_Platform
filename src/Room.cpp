#include "Room.h"

// --- Constructors ---
Room::Room() : _id(""), _name(""), _isOccupied(false), _occupantUid(""), _checkInTime(0) {}

Room::Room(const String& id) : _id(id), _name("Room_" + id), _isOccupied(false), _occupantUid(""), _checkInTime(0) {}

Room::Room(const String& id, const String& name) : _id(id), _name(name), _isOccupied(false), _occupantUid(""), _checkInTime(0) {}

// --- Getters ---
String Room::getId() const { return _id; }
String Room::getName() const { return _name; }
bool Room::isOccupied() const { return _isOccupied; }
String Room::getOccupantUid() const { return _occupantUid; }

// --- Room operations ---
bool Room::checkIn(const String& uid) {
    if (_isOccupied) {
        Serial.println("Room::checkIn ERROR - Room " + _id + " already occupied.");
        return false; // Already occupied
    }

    _isOccupied = true;
    _occupantUid = uid;
    _checkInTime = millis();
    Serial.println("Room::checkIn SUCCESS - Room " + _id + " checked in by " + uid);
    return true;
}

bool Room::checkOut(const String& uid) {
    if (!_isOccupied) {
         Serial.println("Room::checkOut ERROR - Room " + _id + " is not occupied.");
        return false; // Not occupied
    }

    // Cho phép check-out bằng bất kỳ thẻ nào đã đăng ký hoặc UID cụ thể?
    // Hiện tại đang yêu cầu đúng UID đã check-in.
    if (_occupantUid != uid) {
        Serial.println("Room::checkOut ERROR - Card " + uid + " did not check in to Room " + _id + " (Occupant: " + _occupantUid + ")");
        // return false; // Bỏ qua kiểm tra này nếu muốn cho phép thẻ khác check-out
    }

    _isOccupied = false;
    _occupantUid = "";
    _checkInTime = 0;
    Serial.println("Room::checkOut SUCCESS - Room " + _id + " checked out.");
    return true;
}

// --- Get occupancy duration ---
unsigned long Room::getOccupancyDuration() const {
    if (!_isOccupied || _checkInTime == 0) {
        return 0;
    }
    return (millis() - _checkInTime) / 1000;
}

// --- Print room information ---
void Room::printInfo() const {
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