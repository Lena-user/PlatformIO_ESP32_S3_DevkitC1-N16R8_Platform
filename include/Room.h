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
    // Constructors (khai báo)
    Room();
    Room(const String& id);
    Room(const String& id, const String& name);

    // Getters (khai báo)
    String getId() const;
    String getName() const;
    bool isOccupied() const;
    String getOccupantUid() const;

    // Room operations (khai báo)
    bool checkIn(const String& uid);
    bool checkOut(const String& uid);

    // Get occupancy duration in seconds (khai báo)
    unsigned long getOccupancyDuration() const;

    // Print room information (khai báo)
    void printInfo() const;
};

#endif // ROOM_H