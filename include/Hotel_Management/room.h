#ifndef ROOM_H
#define ROOM_H

#include <string>

// Trạng thái phòng
enum RoomStatus { VACANT, OCCUPIED };

// Lớp Room – Quản lý thông tin từng phòng
class Room {
private:
    std::string roomNumber;
    std::string guestUID;
    RoomStatus status;

public:
    Room(const std::string& number);
    
    // Getter
    std::string getRoomNumber() const;
    std::string getGuestUID() const;
    RoomStatus getStatus() const;

    // Setter
    void assignGuest(const std::string& uid);
    // Thay đổi trạng thái phòng
    void vacateRoom();

    // Kiểm tra trạng thái phòng
    bool isOccupied() const;
};

#endif // ROOM_H