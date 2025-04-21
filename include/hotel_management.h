#ifndef HOTEL_MANAGEMENT_H
#define HOTEL_MANAGEMENT_H

#include <iostream>
#include <unordered_map>
#include "Hotel_Management/room.h"
#include "Hotel_Management/guest.h"
#include "Hotel_Management/access_control.h"

// Lớp HotelManagement – Quản lý tổng thể hệ thống khách sạn
class HotelManagement {
private:
    std::unordered_map<std::string, Room> roomData; // Danh sách phòng
    std::unordered_map<std::string, Guest> guestData; // Danh sách khách hàng
    AccessControl accessControl; // Hệ thống kiểm soát truy cập

public:
    HotelManagement();

    // Quản lý khách hàng
    void registerGuest(const std::string& guestID, const std::string& cardUID, const std::string& roomNumber);
    void checkOutGuest(const std::string& roomID);

    // Quản lý phòng
    void printAllRooms() const;
    void printAllGuests() const;

    // Xử lý mở khóa phòng
    bool guestUnlockRoom(const std::string& guestID);
};

#endif // HOTEL_MANAGEMENT_H