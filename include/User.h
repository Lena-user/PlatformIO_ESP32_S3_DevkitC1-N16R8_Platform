#ifndef USER_H
#define USER_H

#include <Arduino.h>

class User {
private:
    String _uid;           // Card UID
    String _name;          // User name
    String _roomId;        // Associated room ID (only one room per card)
    bool _isAuthorized;    // Whether the user is authorized
    
public:
    // Constructors
    User() : _uid(""), _name(""), _roomId(""), _isAuthorized(false) {}
    User(const String& uid) : _uid(uid), _name("User_" + uid), _roomId(""), _isAuthorized(false) {}
    
    // Getters
    String getUid() const { return _uid; }
    String getName() const { return _name; }
    String getRoomId() const { return _roomId; }
    bool isAuthorized() const { return _isAuthorized; }
    
    // Setters
    void setUid(const String& uid) { _uid = uid; }
    void setName(const String& name) { _name = name; }
    void setRoomId(const String& roomId) { _roomId = roomId; }
    void setAuthorized(bool auth) { _isAuthorized = auth; }
    
    // Parse data from NFC card
    // Parse data from NFC card with compact format support
void updateFromCardData(const String& cardData) {
    Serial.println("Parsing card data: \"" + cardData + "\"");
    
    // Try standard format first (NAME:John;ROOM:101)
    int nameStart = cardData.indexOf("NAME:");
    if (nameStart >= 0) {
        nameStart += 5; // Skip "NAME:"
        int nameEnd = cardData.indexOf(";", nameStart);
        if (nameEnd < 0) nameEnd = cardData.length();
        _name = cardData.substring(nameStart, nameEnd);
        Serial.println("Found name (standard format): \"" + _name + "\"");
    } else {
        // Try compact format (N:John;R:101)
        nameStart = cardData.indexOf("N:");
        if (nameStart >= 0) {
            nameStart += 2; // Skip "N:"
            int nameEnd = cardData.indexOf(";", nameStart);
            if (nameEnd < 0) nameEnd = cardData.length();
            _name = cardData.substring(nameStart, nameEnd);
            Serial.println("Found name (compact format): \"" + _name + "\"");
        }
    }
    
    // Parse room ID - standard format
    int roomStart = cardData.indexOf("ROOM:");
    if (roomStart >= 0) {
        roomStart += 5; // Skip "ROOM:"
        int roomEnd = cardData.indexOf(";", roomStart);
        if (roomEnd < 0) roomEnd = cardData.length();
        _roomId = cardData.substring(roomStart, roomEnd);
        Serial.println("Found room ID (standard format): \"" + _roomId + "\"");
    } else {
        // Try compact format (R:101)
        roomStart = cardData.indexOf("R:");
        if (roomStart >= 0) {
            roomStart += 2; // Skip "R:"
            int roomEnd = cardData.indexOf(";", roomStart);
            if (roomEnd < 0) roomEnd = cardData.length();
            _roomId = cardData.substring(roomStart, roomEnd);
            Serial.println("Found room ID (compact format): \"" + _roomId + "\"");
        }
    }
    
    // If room ID is set, user is authorized
    _isAuthorized = (_roomId.length() > 0);
}
    
    // Print user information
    void printInfo() const {
        Serial.println("==== USER INFORMATION ====");
        Serial.println("UID: " + _uid);
        Serial.println("Name: " + _name);
        Serial.println("Room: " + (_roomId.length() > 0 ? _roomId : "Not assigned"));
        Serial.println("Status: " + String(_isAuthorized ? "Authorized" : "Not authorized"));
        Serial.println("=========================");
    }
};

#endif // USER_H