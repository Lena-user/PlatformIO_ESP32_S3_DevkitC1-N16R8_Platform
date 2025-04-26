#ifndef ROOM_MANAGER_H
#define ROOM_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <map>
#include "Room.h"
#include "User.h"

class RoomManager {
private:
    std::vector<Room> _rooms;
    std::map<String, String> _cardRoomMap; // UID -> RoomID mapping
    
public:
    // Constructor
    RoomManager() {
        // Pre-populate with some test rooms
        addRoom("101", "Executive Suite");
        addRoom("102", "Double Room");
        addRoom("103", "Single Room");
    }
    
    // Add a room
    void addRoom(const String& id, const String& name) {
        Room room(id, name);
        _rooms.push_back(room);
    }
    
    // Register a card for a room
    bool registerCard(const String& uid, const String& roomId) {
        // Check if card is already registered
        if (_cardRoomMap.find(uid) != _cardRoomMap.end()) {
            // Card already registered
            if (_cardRoomMap[uid] == roomId) {
                // Already registered to this room
                return true;
            } else {
                // Registered to different room
                Serial.println("Card already registered to room: " + _cardRoomMap[uid]);
                return false;
            }
        }
        
        // Check if room exists
        Room* room = findRoom(roomId);
        if (room == nullptr) {
            Serial.println("Room not found: " + roomId);
            return false;
        }
        
        // Register card
        _cardRoomMap[uid] = roomId;
        Serial.println("Card " + uid + " registered to room " + roomId);
        return true;
    }
    
    // Get room for a card
    Room* getRoomForCard(const String& uid) {
        if (_cardRoomMap.find(uid) == _cardRoomMap.end()) {
            return nullptr; // Card not registered
        }
        
        return findRoom(_cardRoomMap[uid]);
    }
    
    // Find room by ID
    Room* findRoom(const String& roomId) {
        for (auto& room : _rooms) {
            if (room.getId() == roomId) {
                return &room;
            }
        }
        return nullptr;
    }
    
    // Check in a user
    bool checkIn(const String& uid) {
        Room* room = getRoomForCard(uid);
        if (room == nullptr) {
            Serial.println("No room assigned for this card");
            return false;
        }
        
        room->printInfo(); // Print room info before check-in
        bool success = room->checkIn(uid);
        
        if (success) {
            Serial.println("Checked in to room " + room->getId());
        } else {
            Serial.println("Failed to check in (room may be occupied)");
        }
        
        return success;
    }
    
    // Check out a user
    bool checkOut(const String& uid) {
        Room* room = getRoomForCard(uid);
        if (room == nullptr) {
            Serial.println("No room assigned for this card");
            return false;
        }
        
        room->printInfo(); // Print room info before check-out
        bool success = room->checkOut(uid);
        
        if (success) {
            Serial.println("Checked out from room " + room->getId());
        } else {
            Serial.println("Failed to check out");
        }
        
        return success;
    }
    
    // Print all rooms status
    void printAllRoomsStatus() {
        Serial.println("==== ALL ROOMS STATUS ====");
        for (const auto& room : _rooms) {
            Serial.print(room.getId() + ": " + room.getName() + " - ");
            Serial.println(room.isOccupied() ? "Occupied" : "Available");
        }
        Serial.println("==========================");
    }
};

#endif // ROOM_MANAGER_H