#include "RoomManager.h"

// --- Constructor ---
RoomManager::RoomManager(ThingsBoard &tb_ref) : _tb(tb_ref)
{
    // Khởi tạo phòng và DoorControl tương ứng
    addRoom("1", "Room 101");
    addRoom("2", "Room 102");
    addRoom("3", "Room 103");
    addRoom("4", "Room 104");
    // Việc gửi trạng thái ban đầu sẽ gọi hàm sendInitialStates() từ bên ngoài

}

// --- HÀM HELPER GỬI DỮ LIỆU ---
void RoomManager::sendDoorStatusUpdate(uint8_t roomNumber, bool isOpen)
{
    if (!_tb.connected())
    {
        // Serial.println("ThingsBoard not connected, cannot send door status.");
        return;
    }

    const char *doorKey = nullptr;
    if (roomNumber >= 1 && roomNumber <= 4)
    {
        doorKey = DOOR_KEYS[roomNumber - 1];
    }

    if (doorKey != nullptr)
    {
        Serial.printf("RoomManager: Sending door status for Room %d (%s) -> %s\n",
                      roomNumber, doorKey, isOpen ? "open" : "closed");
        if (!_tb.sendAttributeData(doorKey, isOpen)) // Gửi dạng Attribute
        {
            Serial.println("RoomManager: Failed to send door status.");
        }
    }
    else
    {
        Serial.printf("RoomManager: ERROR - Could not find door key for room %d\n", roomNumber);
    }
}

void RoomManager::sendRoomStatusUpdate(const String &roomId, bool isOccupied)
{
    if (!_tb.connected())
    {
        Serial.println("ThingsBoard not connected, cannot send room status.");
        return;
    }

    int roomIndex = roomId.toInt() - 1;
    const char *statusKey = nullptr;

    if (roomIndex >= 0 && roomIndex < 4)
    {
        statusKey = STATUS_KEYS[roomIndex];
    }

    if (statusKey != nullptr)
    {
        Serial.printf("RoomManager: Sending room status for Room %s (%s) -> %s\n",
                      roomId.c_str(), statusKey, isOccupied ? "true" : "false");
        if (!_tb.sendAttributeData(statusKey, isOccupied)) // Gửi dạng Attribute
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
DoorControl *RoomManager::findDoorControlByNumber(uint8_t roomNumber)
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

DoorControl *RoomManager::findDoorControl(const String &roomId)
{
    int roomNumber = roomId.toInt();
    if (roomNumber >= 1 && roomNumber <= 4)
    {
        return findDoorControlByNumber(static_cast<uint8_t>(roomNumber));
    }
    Serial.println("RoomManager: ERROR - Invalid room ID format for finding door control: " + roomId);
    return nullptr;
}

Room *RoomManager::findRoom(const String &roomId)
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

// --- HÀM HELPER KHỞI TẠO ---
void RoomManager::addRoom(const String &id, const String &name)
{
    uint8_t roomIdInt = id.toInt();
    if (roomIdInt < 1 || roomIdInt > 4)
    {
        Serial.println("RoomManager: ERROR - Room number out of range: " + id);
        return;
    }

    for (const auto &room : _rooms)
    {
        if (room.getId() == id) return; // Tránh thêm trùng
    }

    _rooms.emplace_back(id, name); // Sử dụng constructor Room(id, name)
    _doorControls.emplace_back(roomIdInt); // Sử dụng constructor DoorControl(roomNumber)
    Serial.printf("RoomManager: Added Room %s and DoorControl %d\n", id.c_str(), roomIdInt);
}

// --- Gửi trạng thái ban đầu ---
void RoomManager::sendInitialStates()
{
    if (!_tb.connected())
    {
        Serial.println("RoomManager: Cannot send initial states, ThingsBoard not connected.");
        return;
    }
    loadStateFromFlash(); // Gọi hàm load ở đây
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

// --- Đăng ký và tìm kiếm ---
bool RoomManager::registerCard(const String &uid, const String &blockData)
{
    String roomId;
    int roomStart = blockData.indexOf("ROOM:");
    if (roomStart >= 0)
    {
        roomStart += 5;
        int roomEnd = blockData.indexOf(";", roomStart);
        if (roomEnd < 0) roomEnd = blockData.length();
        roomId = blockData.substring(roomStart, roomEnd);
    }

    if (roomId.length() == 0) {
        Serial.println("RoomManager::registerCard ERROR - Could not parse RoomID from block data: " + blockData);
        return false;
    }
    Serial.println("RoomManager::registerCard - Parsed room ID: " + roomId);

    if (_cardRoomMap.count(uid)) // Dùng count() hiệu quả hơn find() != end()
    {
        if (_cardRoomMap[uid] == roomId)
        {
            Serial.println("RoomManager::registerCard INFO - Card " + uid + " already registered to this Room " + roomId);
            return true; // Đã đăng ký đúng phòng này rồi
        }
        else
        {
             Serial.println("RoomManager::registerCard ERROR - Card " + uid + " already registered to another Room (" + _cardRoomMap[uid] + ")");
            return false; // Đã đăng ký phòng khác
        }
    }

    Room *room = findRoom(roomId);
    if (room == nullptr)
    {
        Serial.println("RoomManager::registerCard ERROR - Room " + roomId + " not found.");
        return false;
    }

    _cardRoomMap[uid] = roomId;
    Serial.println("RoomManager::registerCard SUCCESS - Card " + uid + " registered to room " + roomId);
    
    Serial.println("RoomManager::registerCard - Attempting to save state to flash...");
    saveStateToFlash();
    Serial.println("RoomManager::registerCard - State after saving:");
    printAllRoomsStatus(); // In trạng thái sau khi lưu
    return true;
}

Room *RoomManager::getRoomForCard(const String &uid)
{
    auto it = _cardRoomMap.find(uid);
    if (it == _cardRoomMap.end())
    {
        return nullptr; // Card not registered
    }
    return findRoom(it->second); // it->second là roomId
}

// --- CheckIn/CheckOut ---
bool RoomManager::checkIn(const String &uid)
{
    Room *room = getRoomForCard(uid);
    if (room == nullptr)
    {
        Serial.println("RoomManager::checkIn ERROR - Card " + uid + " not registered to any room.");
        return false;
    }

    // *** THÊM LOGIC KIỂM TRA NGƯỜI ĐANG Ở QUÉT LẠI ***
    if (room->isOccupied()) {
        if (room->getOccupantUid() == uid) {
            // Người đang ở quét lại thẻ -> Chỉ mở cửa
            Serial.println("RoomManager::checkIn INFO - Occupant " + uid + " re-scanned card for room " + room->getId() + ". Opening door.");
            openRoomDoor(room->getId()); // Mở cửa
            return true; // Coi như check-in thành công (vì cửa đã mở)
        } else {
            // Phòng đã có người khác ở -> Từ chối
            Serial.println("RoomManager::checkIn ERROR - Room " + room->getId() + " already occupied by another user (" + room->getOccupantUid() + ").");
            return false;
        }
    }
    // *** KẾT THÚC LOGIC KIỂM TRA ***

    // Nếu phòng chưa có người ở (isOccupied() là false), thực hiện check-in bình thường
    bool success = room->checkIn(uid);
    if (success)
    {
        Serial.println("RoomManager::checkIn - Checked in to room " + room->getId());
        sendRoomStatusUpdate(room->getId(), true);
        openRoomDoor(room->getId());
        
        Serial.println("RoomManager::checkIn - Attempting to save state to flash...");
        saveStateToFlash();
        Serial.println("RoomManager::checkIn - State after saving:");
        printAllRoomsStatus(); // In trạng thái sau khi lưu
    }
    else
    {
        // Lỗi này không nên xảy ra nếu logic ở trên đúng, nhưng để đề phòng
        Serial.println("RoomManager::checkIn - Failed for room " + room->getId() + " (Unexpected error in Room::checkIn).");
    }
    return success;
}

bool RoomManager::checkOut(const String &uid)
{
    Room *room = getRoomForCard(uid);
    if (room == nullptr)
    {
        Serial.println("RoomManager::checkOut ERROR - Card " + uid + " not found in map.");
        return false;
    }

    String roomId = room->getId(); // Lấy ID trước khi checkOut có thể thay đổi trạng thái

    // Gọi checkOut của đối tượng Room
    bool success = room->checkOut(uid);
    if (success)
    {
        Serial.println("RoomManager::checkOut - Checked out from room " + roomId);
        sendRoomStatusUpdate(roomId, false);
        lockRoomDoor(roomId);

        // Xóa khỏi map
        auto it = _cardRoomMap.find(uid);
        if (it != _cardRoomMap.end()) {
            _cardRoomMap.erase(it);
            Serial.println("RoomManager::checkOut - Removed card " + uid + " from registration map.");
        }
        
        Serial.println("RoomManager::checkOut - Attempting to save state to flash...");
        saveStateToFlash();
        Serial.println("RoomManager::checkOut - State after saving:");
        printAllRoomsStatus(); // In trạng thái sau khi lưu
    }
    else
    {
        // Lỗi đã được log bên trong Room::checkOut
         Serial.println("RoomManager::checkOut - Failed for room " + roomId);
    }
    return success;
}

// --- Điều khiển cửa ---
bool RoomManager::openRoomDoor(const String &roomId)
{
    DoorControl *door = findDoorControl(roomId);
    if (door != nullptr)
    {
        if (door->openDoor()) // Chỉ gửi nếu trạng thái thực sự thay đổi
        {
            sendDoorStatusUpdate(door->getRoomNumber(), true);
            return true;
        }
        // Nếu cửa đã mở sẵn thì không cần gửi lại
        return true; // Coi như thành công nếu cửa đã mở
    }
    Serial.println("RoomManager::openRoomDoor ERROR - DoorControl not found for room " + roomId);
    return false;
}

bool RoomManager::lockRoomDoor(const String &roomId)
{
    DoorControl *door = findDoorControl(roomId);
    if (door != nullptr)
    {
        if (door->lockDoor()) // Chỉ gửi nếu trạng thái thực sự thay đổi
        {
            sendDoorStatusUpdate(door->getRoomNumber(), false);
            return true;
        }
         // Nếu cửa đã khóa sẵn thì không cần gửi lại
        return true; // Coi như thành công nếu cửa đã khóa
    }
     Serial.println("RoomManager::lockRoomDoor ERROR - DoorControl not found for room " + roomId);
    return false;
}

// --- Update auto-lock ---
void RoomManager::update()
{
    for (auto &door : _doorControls)
    {
        if (door.update()) // Nếu cửa vừa tự khóa
        {
            sendDoorStatusUpdate(door.getRoomNumber(), false); // Gửi trạng thái mới (đã khóa)
        }
    }
}

// --- Print status ---
void RoomManager::printAllRoomsStatus()
{
    Serial.println("==== ALL ROOMS STATUS ====");
    for (const auto &room : _rooms)
    {
        Serial.print(room.getId() + ": " + room.getName() + " - ");
        Serial.print(room.isOccupied() ? "Occupied" : "Available");
        if(room.isOccupied()) {
            Serial.print(" by " + room.getOccupantUid());
        }
        Serial.println();
    }
     Serial.println("==== REGISTERED CARDS ====");
     if (_cardRoomMap.empty()) {
         Serial.println("No cards registered.");
     } else {
        for(const auto& pair : _cardRoomMap) {
            Serial.println("Card: " + pair.first + " -> Room: " + pair.second);
        }
     }
    Serial.println("==========================");
}

void RoomManager::saveStateToFlash() {
    if (!preferences.begin("room_manager", false)) { // false: read/write mode
        Serial.println("RoomManager: ERROR - Could not begin preferences for writing.");
        return;
    }
    preferences.clear(); // Xóa hết key cũ trong namespace này để đảm bảo sạch sẽ

    // --- Save _cardRoomMap ---
    DynamicJsonDocument jsonMap(1024); // Tăng kích thước nếu map lớn
    JsonObject mapObject = jsonMap.to<JsonObject>();
    for (const auto& pair : _cardRoomMap) {
        mapObject[pair.first] = pair.second;
    }
    String serializedMap;
    serializeJson(jsonMap, serializedMap);
    preferences.putString("cardRoomMap", serializedMap);
    Serial.println("RoomManager: Saved cardRoomMap to flash: " + serializedMap);

    // --- Save individual room states ---
    for (const auto& room : _rooms) { // room ở đây là một Room object (hoặc const Room&)
        String prefix = "room_" + room.getId() + "_"; // Sửa room-> thành room.
        preferences.putBool((prefix + "occupied").c_str(), room.isOccupied()); // Sửa room-> thành room.
        preferences.putString((prefix + "uid").c_str(), room.getOccupantUid()); // Sửa room-> thành room.
        // preferences.putULong((prefix + "chkTime").c_str(), room.getCheckInTime()); // Sửa room-> thành room. (Nếu cần)
        Serial.printf("RoomManager: Saved state for Room %s: Occupied=%d, UID=%s\n",
                      room.getId().c_str(), room.isOccupied(), room.getOccupantUid().c_str()); // Sửa room-> thành room.
    }

    preferences.end();
    Serial.println("RoomManager: All states saved to flash.");
}

void RoomManager::loadStateFromFlash() {
    if (!preferences.begin("room_manager", true)) { // true: read-only mode
        Serial.println("RoomManager: Preferences not found or error, using default state.");
        // Nếu không mở được ở read-only (có thể là lần đầu chạy), thử mở ở read/write để tạo namespace
        if (!preferences.begin("room_manager", false)) {
             Serial.println("RoomManager: ERROR - Could not begin preferences even for writing.");
             return;
        }
        preferences.end(); // Đóng lại ngay nếu chỉ để tạo
        return; // Không có gì để load
    }

    Serial.println("RoomManager: Attempting to load state from flash...");

    // --- Load _cardRoomMap ---
    String serializedMap = preferences.getString("cardRoomMap", "");
    if (!serializedMap.isEmpty()) {
        DynamicJsonDocument jsonMap(1024);
        DeserializationError error = deserializeJson(jsonMap, serializedMap);
        if (!error) {
            _cardRoomMap.clear(); // Xóa map hiện tại
            JsonObject mapObject = jsonMap.as<JsonObject>();
            for (JsonPair kv : mapObject) {
                _cardRoomMap[String(kv.key().c_str())] = kv.value().as<String>();
            }
            Serial.println("RoomManager: Loaded cardRoomMap from flash: " + serializedMap);
        } else {
            Serial.printf("RoomManager: Failed to deserialize cardRoomMap: %s\n", error.c_str());
        }
    } else {
        Serial.println("RoomManager: No cardRoomMap found in flash.");
    }

    // --- Load individual room states ---
    for (auto& room_ptr : _rooms) { // room_ptr ở đây là một tham chiếu đến Room object
        String prefix = "room_" + room_ptr.getId() + "_"; // Sửa room_ptr-> thành room_ptr.
        bool occupied = preferences.getBool((prefix + "occupied").c_str(), false);
        String uid = preferences.getString((prefix + "uid").c_str(), "");
        // unsigned long checkInTime = preferences.getULong((prefix + "chkTime").c_str(), 0); // Nếu cần

        room_ptr.restoreState(occupied, uid, 0 /*checkInTime*/); // Sửa room_ptr-> thành room_ptr.
        if(occupied) {
            Serial.printf("RoomManager: Loaded state for Room %s: Occupied, UID=%s\n",
                          room_ptr.getId().c_str(), uid.c_str()); // Sửa room_ptr-> thành room_ptr.
        } else {
            Serial.printf("RoomManager: Loaded state for Room %s: Available\n", room_ptr.getId().c_str()); // Sửa room_ptr-> thành room_ptr.
        }
    }
    preferences.end();
    Serial.println("RoomManager: State loading finished.");
    printAllRoomsStatus(); // In ra để kiểm tra
}