#ifndef ROOM_MANAGER_H
#define ROOM_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <map>
#include <ThingsBoard.h>
#include "Room.h"
#include "DoorControl.h"
#include <Preferences.h> // Thêm dòng này

// Khai báo các mảng key (định nghĩa trong globals.cpp)
extern const char *DOOR_KEYS[];
extern const char *STATUS_KEYS[];

class RoomManager
{
private:
    ThingsBoard &_tb;
    std::vector<Room> _rooms;
    std::map<String, String> _cardRoomMap;  // UID -> RoomID mapping
    std::vector<DoorControl> _doorControls;
    Preferences preferences; // Thêm đối tượng preferences

    // --- HÀM HELPER GỬI DỮ LIỆU (khai báo) ---
    void sendDoorStatusUpdate(uint8_t roomNumber, bool isOpen);
    void sendRoomStatusUpdate(const String &roomId, bool isOccupied);

    // --- HÀM HELPER TÌM KIẾM (khai báo) ---
    DoorControl *findDoorControlByNumber(uint8_t roomNumber);
    DoorControl *findDoorControl(const String &roomId);
    Room *findRoom(const String &roomId); // Đã có sẵn

    // --- HÀM HELPER KHỞI TẠO (khai báo) ---
    void addRoom(const String &id, const String &name);

public:
    // Constructor (khai báo)
    RoomManager(ThingsBoard &tb_ref);

    // Gửi trạng thái ban đầu (khai báo)
    void sendInitialStates();

    // Đăng ký và tìm kiếm (khai báo)
    bool registerCard(const String &uid, const String &blockData);
    Room *getRoomForCard(const String &uid);
    // findRoom đã là private helper

    // CheckIn/CheckOut (khai báo)
    bool checkIn(const String &uid);
    bool checkOut(const String &uid);

    // Điều khiển cửa (khai báo)
    bool openRoomDoor(const String &roomId);
    bool lockRoomDoor(const String &roomId);

    // Update auto-lock (khai báo)
    void update();

    // Print status (khai báo)
    void printAllRoomsStatus();

    void saveStateToFlash();
    void loadStateFromFlash();
};

#endif // ROOM_MANAGER_H