#ifndef GUEST_H
#define GUEST_H

#include <iostream>
#include <string>
#include "access_control.h" // Để kiểm tra quyền mở cửa

class Guest {
private:
    std::string guestID;
    std::string registeredCard; // Thẻ NFC đã đăng ký
    std::string roomNumber; // Phòng mà khách đang ở

public:
    Guest(const std::string& id, const std::string& name, const std::string& card, const std::string& room);
    
    void printGuestInfo() const;
    bool unlockRoom(AccessControl& accessControl) const; // Mở khóa phòng

    std::string getRoomNumber() const { return roomNumber; }
};

#endif // GUEST_H