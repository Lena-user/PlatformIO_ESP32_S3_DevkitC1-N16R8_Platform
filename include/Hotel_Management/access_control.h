#ifndef ACCESS_CONTROL_H
#define ACCESS_CONTROL_H

#include <string>
#include <unordered_map>

// Lớp quản lý quyền truy cập dựa trên thẻ đăng ký
class AccessControl {
private:
    std::unordered_map<std::string, std::string> authorizedCards; // Danh sách thẻ hợp lệ (UID -> RoomNumber)

public:
    AccessControl();

    // Đăng ký thẻ cho phòng cụ thể
    void registerCard(const std::string& cardUID, const std::string& roomNumber);

    // Kiểm tra quyền mở khóa
    bool verifyAccess(const std::string& cardUID, const std::string& roomNumber) const;
};

#endif // ACCESS_CONTROL_H