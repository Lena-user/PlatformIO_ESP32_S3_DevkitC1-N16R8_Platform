#ifndef ROOM_MANAGER_H
#define ROOM_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <map>
#include <ThingsBoard.h> // *** THÊM: Include ThingsBoard ***
#include "Room.h"
#include "DoorControl.h" // DoorControl đã được đơn giản hóa (như đã làm trước đó)

// Khai báo các mảng key (định nghĩa trong globals.cpp)
extern const char *DOOR_KEYS[];
extern const char *STATUS_KEYS[];

class RoomManager
{
private:
    ThingsBoard &_tb; // *** THÊM: Tham chiếu ThingsBoard ***
    std::vector<Room> _rooms;
    std::map<String, String> _cardRoomMap;  // UID -> RoomID mapping
    std::vector<DoorControl> _doorControls; // Door controls for each room

    // --- HÀM HELPER GỬI DỮ LIỆU ---

    /**
     * @brief Gửi trạng thái cửa (mở/đóng) của một phòng lên ThingsBoard.
     * @param roomNumber Số phòng (1-4).
     * @param isOpen Trạng thái cửa (true = mở, false = đóng).
     */
    void sendDoorStatusUpdate(uint8_t roomNumber, bool isOpen)
    {
        if (!_tb.connected())
        {
            // Serial.println("ThingsBoard not connected, cannot send door status.");
            return; // Không gửi nếu chưa kết nối
        }

        const char *doorKey = nullptr;
        if (roomNumber >= 1 && roomNumber <= 4)
        {
            // Giả sử DOOR_KEYS được định nghĩa trong globals.cpp
            doorKey = DOOR_KEYS[roomNumber - 1];
        }

        if (doorKey != nullptr)
        {
            Serial.printf("RoomManager: Sending door status for Room %d (%s) -> %s\n",
                          roomNumber, doorKey, isOpen ? "open" : "closed");
            if (!_tb.sendAttributeData(doorKey, isOpen))
            {
                Serial.println("RoomManager: Failed to send door status.");
            }
        }
        else
        {
            Serial.printf("RoomManager: ERROR - Could not find door key for room %d\n", roomNumber);
        }
    }

    /**
     * @brief Gửi trạng thái phòng (có người/trống) lên ThingsBoard.
     * @param roomId ID phòng (dạng String, ví dụ "1", "2").
     * @param isOccupied Trạng thái phòng (true = có người, false = trống).
     */
    void sendRoomStatusUpdate(const String &roomId, bool isOccupied)
    {
        if (!_tb.connected())
        {
            // Serial.println("ThingsBoard not connected, cannot send room status.");
            return; // Không gửi nếu chưa kết nối
        }

        // Chuyển String ID sang index (0-3)
        int roomIndex = roomId.toInt() - 1;
        const char *statusKey = nullptr;

        if (roomIndex >= 0 && roomIndex < 4)
        { // Giả sử có 4 phòng
            // Giả sử STATUS_KEYS được định nghĩa trong globals.cpp
            statusKey = STATUS_KEYS[roomIndex];
        }

        if (statusKey != nullptr)
        {
            const char *statusValue = isOccupied ? "Occupied" : "Available"; // Gửi trạng thái dạng chữ
            Serial.printf("RoomManager: Sending room status for Room %s (%s) -> %s\n",
                          roomId.c_str(), statusKey, statusValue);
            if (!_tb.sendAttributeData(statusKey, statusValue))
            {
                Serial.println("RoomManager: Failed to send room status.");
            }
        }
        else
        {
            Serial.printf("RoomManager: ERROR - Could not find status key for room %s\n", roomId.c_str());
        }
    }

    // --- HÀM HELPER TÌM KIẾM ---

    DoorControl *findDoorControlByNumber(uint8_t roomNumber)
    {
        for (auto &door : _doorControls)
        {
            if (door.getRoomNumber() == roomNumber)
            {
                return &door;
            }
        }
        return nullptr;
    }

    DoorControl *findDoorControl(const String &roomId)
    {
        int roomNumber = roomId.toInt();
        if (roomNumber >= 1 && roomNumber <= 4)
        { // Giả sử 4 phòng
            return findDoorControlByNumber(static_cast<uint8_t>(roomNumber));
        }
        Serial.println("RoomManager: ERROR - Invalid room ID format for finding door control: " + roomId);
        return nullptr;
    }

public:
    // *** SỬA: Constructor nhận tham chiếu ThingsBoard ***
    RoomManager(ThingsBoard &tb_ref) : _tb(tb_ref)
    {
        // Khởi tạo phòng và DoorControl tương ứng
        // Đảm bảo thứ tự addRoom và tạo DoorControl khớp nhau
        addRoom("1", "Room 101");
        addRoom("2", "Room 102");
        addRoom("3", "Room 103");
        addRoom("4", "Room 104");

        // Gửi trạng thái ban đầu của tất cả các cửa và phòng khi kết nối
        // Việc gửi nên được thực hiện sau khi tb.connected() là true
        // Có thể gọi một hàm riêng như initializeStatesToServer() từ bên ngoài
        // sau khi kết nối thành công.
    }

    /**
     * @brief Gửi trạng thái ban đầu của tất cả phòng và cửa lên server.
     *        Nên gọi hàm này sau khi ThingsBoard đã kết nối thành công.
     */
    void sendInitialStates()
    {
        if (!_tb.connected())
            return;
        Serial.println("RoomManager: Sending initial states to ThingsBoard...");
        for (const auto &door : _doorControls)
        {
            sendDoorStatusUpdate(door.getRoomNumber(), door.isOpen());
        }
        for (const auto &room : _rooms)
        {
            sendRoomStatusUpdate(room.getId(), room.isOccupied());
        }
        Serial.println("RoomManager: Initial states sent.");
    }

    // *** SỬA: addRoom cũng tạo DoorControl ***
    void addRoom(const String &id, const String &name)
    {
        uint8_t roomIdInt = id.toInt();
        if (roomIdInt < 1 || roomIdInt > 4)
        { // Giả sử giới hạn 1-4
            Serial.println("RoomManager: ERROR - Room number out of range: " + id);
            return;
        }

        // Kiểm tra trùng lặp nếu cần
        for (const auto &room : _rooms)
        {
            if (room.getId() == id)
                return;
        } // Tránh thêm trùng

        Room room(id, name);
        _rooms.push_back(room);

        // Tạo DoorControl tương ứng
        _doorControls.emplace_back(roomIdInt); // Sử dụng constructor của DoorControl
        Serial.printf("RoomManager: Added Room %s and DoorControl %d\n", id.c_str(), roomIdInt);
    }

    // --- Các hàm đăng ký và tìm kiếm (giữ nguyên) ---
    bool registerCard(const String &uid, const String &blockData)
    {
        // Parse room information from the block data
        String roomId;

        // Standard format (ROOM:1)
        int roomStart = blockData.indexOf("ROOM:");
        if (roomStart >= 0)
        {
            roomStart += 5; // Skip "ROOM:"
            int roomEnd = blockData.indexOf(";", roomStart);
            if (roomEnd < 0)
                roomEnd = blockData.length();
            roomId = blockData.substring(roomStart, roomEnd);
        }

        // If no valid room ID found, return false
        if (roomId.length() == 0)
            return false;
        else
        {
            Serial.println("Parsed room ID: " + roomId);
        }

        // Check if card is already registered
        if (_cardRoomMap.find(uid) != _cardRoomMap.end())
        {
            // Card already registered
            if (_cardRoomMap[uid] == roomId)
            {
                // Already registered to this room
                return true;
            }
            else
            {
                return false;
            }
        }

        // Check if room exists
        Room *room = findRoom(roomId);
        if (room == nullptr)
        {
            Serial.println("Room " + roomId + " not found");
            return false;
        }
        else
        {
            // Register card
            _cardRoomMap[uid] = roomId;
            Serial.println("Card " + uid + " registered to room " + roomId);
            return true;
        }
    }

    Room *getRoomForCard(const String &uid)
    {
        if (_cardRoomMap.find(uid) == _cardRoomMap.end())
        {
            return nullptr; // Card not registered
        }

        return findRoom(_cardRoomMap[uid]);
    }

    Room *findRoom(const String &roomId)
    {
        for (auto &room : _rooms)
        {
            if (room.getId() == roomId)
            {
                return &room;
            }
        }
        return nullptr;
    }

    // --- Các hàm CheckIn/CheckOut cập nhật trạng thái server ---
    bool checkIn(const String &uid)
    {
        Room *room = getRoomForCard(uid);
        if (room == nullptr)
        { /* ... lỗi ... */
            return false;
        }

        bool success = room->checkIn(uid);
        if (success)
        {
            Serial.println("Checked in to room " + room->getId());
            sendRoomStatusUpdate(room->getId(), true); // Gửi trạng thái mới
            // Tự động mở cửa khi check-in thành công
            openRoomDoor(room->getId());
        }
        else
        { /* ... lỗi ... */
        }
        return success;
    }

    bool checkOut(const String &uid)
    {
        Room *room = getRoomForCard(uid);
        if (room == nullptr)
        { /* ... lỗi ... */
            return false;
        }

        bool success = room->checkOut(uid);
        if (success)
        {
            Serial.println("Checked out from room " + room->getId());
            sendRoomStatusUpdate(room->getId(), false); // Gửi trạng thái mới
            // Đảm bảo cửa đã khóa khi check-out
            lockRoomDoor(room->getId());
        }
        else
        { /* ... lỗi ... */
        }
        return success;
    }

    // --- Các hàm điều khiển cửa và cập nhật trạng thái server ---
    bool openRoomDoor(const String &roomId)
    {
        DoorControl *door = findDoorControl(roomId);
        if (door != nullptr)
        {
            if (door->openDoor())
            { // Nếu trạng thái thực sự thay đổi
                sendDoorStatusUpdate(door->getRoomNumber(), true);
                return true;
            }
        }
        else
        { /* ... lỗi ... */
        }
        return false;
    }

    bool lockRoomDoor(const String &roomId)
    {
        DoorControl *door = findDoorControl(roomId);
        if (door != nullptr)
        {
            if (door->lockDoor())
            { // Nếu trạng thái thực sự thay đổi
                sendDoorStatusUpdate(door->getRoomNumber(), false);
                return true;
            }
        }
        else
        { /* ... lỗi ... */
        }
        return false;
    }

    // --- Hàm Update xử lý auto-lock ---
    void update()
    {
        for (auto &door : _doorControls)
        {
            if (door.update())
            {                                                              // Nếu update trả về true (cửa vừa tự khóa)
                sendDoorStatusUpdate(door.getRoomNumber(), door.isOpen()); // Gửi trạng thái mới (đã khóa)
            }
        }
    }

    // Print all rooms status (không đổi)
    void printAllRoomsStatus()
    {
        Serial.println("==== ALL ROOMS STATUS ====");
        for (const auto &room : _rooms)
        {
            Serial.print(room.getId() + ": " + room.getName() + " - ");
            Serial.println(room.isOccupied() ? "Occupied" : "Available");
        }
        Serial.println("==========================");
    }
};

#endif // ROOM_MANAGER_H