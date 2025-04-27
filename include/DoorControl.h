#ifndef DOOR_CONTROL_H
#define DOOR_CONTROL_H

#include <Arduino.h> // Cần cho Serial, millis(), uint8_t, unsigned long

extern const char *DOOR_KEYS[]; // Định nghĩa trong globals.cpp

class DoorControl
{
private:
    bool _isOpen;                // Door state (simulated)
    unsigned long _openTime;     // Time when door was opened
    unsigned long _openDuration; // How long to keep door open (ms)
    uint8_t _roomNumber;         // Room number (1-4)

    const char *getDoorKeyInternal() const // Đổi tên để tránh nhầm lẫn nếu làm public
    {
        if (_roomNumber >= 1 && _roomNumber <= 4) {
            return DOOR_KEYS[_roomNumber - 1];
        }
        return nullptr;
    }

public:
    DoorControl(uint8_t roomNumber = 1, unsigned long openDuration = 5000) :
        _isOpen(false),
        _openTime(0),
        _openDuration(openDuration),
        _roomNumber(1) // Khởi tạo mặc định
    {
        // Chỉ gán roomNumber hợp lệ
        if (roomNumber >= 1 && roomNumber <= 4) {
             _roomNumber = roomNumber;
        } else {
             Serial.printf("ERROR: Invalid initial room number %d for DoorControl. Using %d.\n", roomNumber, _roomNumber);
        }

        Serial.printf("==== DOOR CONTROL %d INITIALIZED (SIMULATION) ====\n", _roomNumber);
        Serial.println("Door is locked");
    }

    bool openDoor()
    {
        if (!_isOpen) {
            _isOpen = true;
            _openTime = millis();
            Serial.printf("Door %d state changed to OPEN (will auto-lock in %.1f s)\n", _roomNumber, _openDuration / 1000.0);
            return true; // Trạng thái đã thay đổi
        }
        return false; // Trạng thái không đổi
    }

    bool lockDoor()
    {
        if (_isOpen) {
            _isOpen = false;
            Serial.printf("Door %d state changed to LOCKED\n", _roomNumber);
            return true; // Trạng thái đã thay đổi
        }
        return false; // Trạng thái không đổi
    }

    bool update()
    {
        if (_isOpen) {
            unsigned long currentTime = millis();
            unsigned long elapsedTime;
            // Handle millis() overflow
            if (currentTime < _openTime) {
                elapsedTime = (0xFFFFFFFF - _openTime) + currentTime + 1;
            } else {
                elapsedTime = currentTime - _openTime;
            }

            if (elapsedTime >= _openDuration) {
                Serial.printf("[DEBUG] Door %d auto-locking due to timeout.\n", _roomNumber);
                return lockDoor(); // Gọi lockDoor và trả về kết quả của nó
            }
        }
        return false; // Không có thay đổi trạng thái do update
    }

    bool isOpen() const { return _isOpen; }
    uint8_t getRoomNumber() const { return _roomNumber; }
    unsigned long getRemainingOpenTime() const
    {
        if (!_isOpen) return 0;

        unsigned long currentTime = millis();
        unsigned long elapsedTime;

        if (currentTime < _openTime) // Check for overflow
        {
             elapsedTime = (0xFFFFFFFF - _openTime) + currentTime + 1;
        } else {
             elapsedTime = currentTime - _openTime;
        }

        return (elapsedTime >= _openDuration) ? 0 : (_openDuration - elapsedTime);
    }

    const char* getDoorKey() const {
        return getDoorKeyInternal();
    }
};

#endif // DOOR_CONTROL_H