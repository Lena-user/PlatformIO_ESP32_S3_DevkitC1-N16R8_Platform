# Hệ Thống Quản Lý Phòng Khách Sạn Thông Minh (ESP32-S3 & ThingsBoard)

## 1. Giới thiệu

Dự án này là một hệ thống quản lý truy cập phòng khách sạn tự động, sử dụng vi điều khiển ESP32-S3, đầu đọc thẻ NFC (PN532) và nền tảng IoT ThingsBoard. Hệ thống cho phép quản lý việc đăng ký (check-in) và hủy đăng ký (check-out) thẻ NFC cho các phòng, theo dõi trạng thái phòng (có người/trống), điều khiển khóa cửa và giám sát/quản lý từ xa qua giao diện dashboard của ThingsBoard.

## 2. Tính năng chính

*   **Quản lý 4 phòng:** Hệ thống được thiết kế để quản lý 4 phòng riêng biệt (ID 1-4).
*   **Theo dõi trạng thái phòng:** Mỗi phòng có trạng thái `Occupied` (có người) hoặc `Available` (trống), được đồng bộ lên ThingsBoard.
*   **Quản lý thẻ NFC (MIFARE Classic):**
    *   **Check-in:** Đăng ký thẻ cho một phòng cụ thể thông qua lệnh RPC từ ThingsBoard. Dữ liệu phòng được ghi vào thẻ NFC (Block 4).
    *   **Check-out:** Hủy đăng ký thẻ khỏi phòng thông qua lệnh RPC từ ThingsBoard. Dữ liệu trên thẻ được xóa.
*   **Kiểm soát truy cập:**
    *   Quét thẻ đã đăng ký vào phòng trống sẽ thực hiện check-in, mở cửa và cập nhật trạng thái.
    *   Quét thẻ đã đăng ký vào phòng đang có người (bởi chính thẻ đó) sẽ chỉ mở cửa.
    *   Quét thẻ chưa đăng ký hoặc thẻ đăng ký cho phòng khác sẽ bị từ chối.
*   **Điều khiển khóa cửa:**
    *   Mở khóa cửa tự động khi check-in hoặc khi người đang ở quét lại thẻ.
    *   Tự động khóa cửa sau một khoảng thời gian chờ (ví dụ: 5 giây).
    *   Khóa cửa khi check-out.
*   **Tích hợp ThingsBoard:**
    *   Kết nối an toàn qua MQTT.
    *   Gửi trạng thái phòng và cửa lên ThingsBoard dưới dạng Attributes (`status_Room_X`, `door_Room_X`).
    *   Nhận và xử lý lệnh RPC (`CHECK_IN_OUT`) để quản lý thẻ.
*   **Logging:** Ghi lại các hoạt động và lỗi hệ thống qua Serial Monitor.
*   **Đa nhiệm:** Sử dụng FreeRTOS để xử lý đồng thời các tác vụ mạng, đọc thẻ NFC và ghi thẻ NFC.

## 3. Phần cứng

*   **Vi điều khiển:** ESP32-S3 DevKitC-1 (hoặc tương đương có PSRAM).
*   **Đầu đọc NFC:** Module PN532 (giao tiếp I2C).
*   **(Tùy chọn)** Cơ cấu khóa cửa điện tử cho mỗi phòng.
*   Nguồn cấp phù hợp.

## 4. Kết nối Phần cứng (I2C)

| Chân PN532 | Chân ESP32-S3 | Chức năng |
|-----------|-------------|------------|
| **VCC**   | **3.3V**    | Nguồn      |
| **GND**   | **GND**     | Mass       |
| **SDA**   | **GPIO14**  | Dữ liệu I2C |
| **SCL**   | **GPIO13**  | Clock I2C  |

*(Lưu ý: Các chân GPIO có thể thay đổi trong file `config.h`)*

## 5. Phần mềm & Kiến trúc

*   **Môi trường:** PlatformIO trên Visual Studio Code.
*   **Framework:** Arduino ESP32.
*   **RTOS:** FreeRTOS.
*   **Thư viện chính:**
    *   `Adafruit PN532`: Giao tiếp NFC.
    *   `ThingsBoard`: Kết nối MQTT và RPC.
    *   `ArduinoJson`: Xử lý JSON.
    *   `WiFi`, `Arduino_MQTT_Client`: Mạng và MQTT.
*   **Kiến trúc Task (FreeRTOS):**
    *   `wifiTask`: Xử lý kết nối WiFi ban đầu.
    *   `serverTask`: Xử lý kết nối ThingsBoard, nhận RPC, khởi tạo các task khác.
    *   `readTask`: Liên tục quét thẻ NFC để kiểm soát truy cập.
    *   `writeTask`: Chờ và xử lý yêu cầu ghi/xóa dữ liệu thẻ từ RPC.
*   **Đồng bộ hóa:**
    *   `nfcMutex`: Đảm bảo chỉ một task (đọc hoặc ghi) truy cập module NFC tại một thời điểm.
    *   `writeQueue`: Hàng đợi để `serverTask` gửi yêu cầu ghi thẻ đến `writeTask`.
*   **Cấu trúc lớp:**
    *   `Room`: Mô hình hóa trạng thái và hành vi của một phòng.
    *   `DoorControl`: Mô hình hóa trạng thái và logic tự động khóa của cửa.
    *   `RoomManager`: Lớp trung tâm quản lý phòng, cửa, thẻ và logic nghiệp vụ chính.
    *   `PN532_NFC`: Lớp bao bọc, đơn giản hóa việc sử dụng thư viện PN532.

## 6. Cấu trúc thư mục dự án

```
ESP32_S3_Project/
│
├── include/            # Chứa các file header (.h)
│   ├── config.h        # Cấu hình chung (PIN, delay, keys...)
│   ├── secrets.h       # Thông tin nhạy cảm (WiFi, Token) - **KHÔNG COMMIT**
│   ├── DoorControl.h
│   ├── globals.h       # Khai báo biến toàn cục (extern)
│   ├── main.h          # Header chính (có thể chứa khai báo task, queue...)
│   ├── PN532_NFC.h
│   ├── Room.h
│   └── RoomManager.h
│
├── lib/                # Chứa các thư viện tùy chỉnh (nếu có)
│
├── src/                # Chứa các file mã nguồn (.cpp)
│   ├── DoorControl.cpp
│   ├── globals.cpp     # Định nghĩa biến toàn cục
│   ├── main.cpp        # Hàm setup(), loop(), định nghĩa task chính
│   ├── nfc_tasks.cpp   # (Tùy chọn) Chứa định nghĩa readTask, writeTask
│   ├── PN532_NFC.cpp
│   ├── Room.cpp
│   ├── RoomManager.cpp
│   └── Server.cpp      # (Tùy chọn) Chứa định nghĩa serverTask, wifiTask, xử lý RPC
│
├── test/               # Chứa các file kiểm thử (nếu có)
│
├── platformio.ini      # File cấu hình chính của PlatformIO
└── README.md           # File này
```

## 7. Cấu hình

*   **`include/secrets.h`:** Tạo file này và điền thông tin SSID/mật khẩu WiFi và ThingsBoard Access Token. **Quan trọng:** Thêm file này vào `.gitignore` để tránh commit lên Git.
    ```cpp
    // include/secrets.h
    #ifndef SECRETS_H
    #define SECRETS_H

    #define WIFI_SSID "YOUR_WIFI_SSID"
    #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
    #define TB_TOKEN "YOUR_THINGSBOARD_DEVICE_TOKEN"

    #endif // SECRETS_H
    ```
*   **`include/config.h`:** Chỉnh sửa các thông số như chân PIN (nếu khác mặc định), địa chỉ server ThingsBoard, tên key RPC/Attribute, thời gian delay, v.v. nếu cần.

## 8. Cài đặt và Chạy

1.  **Clone repository** (nếu chưa có):
    ```bash
    git clone <URL_repository_cua_ban>
    cd <ten_thu_muc_du_an>
    ```
2.  **Mở dự án:** Mở thư mục dự án bằng PlatformIO trong VS Code.
3.  **Cài đặt thư viện:** PlatformIO sẽ tự động cài đặt các thư viện được liệt kê trong `platformio.ini` khi bạn build dự án lần đầu.
4.  **Tạo file `include/secrets.h`:** Điền thông tin WiFi và ThingsBoard Token của bạn.
5.  **(Quan trọng) Thêm `include/secrets.h` vào `.gitignore`:**
    ```bash
    echo "include/secrets.h" >> .gitignore
    git add .gitignore
    git commit -m "Add secrets.h to gitignore"
    ```
6.  **Kết nối phần cứng:** Đảm bảo ESP32-S3 và PN532 được kết nối đúng theo sơ đồ ở mục 4.
7.  **Biên dịch và Nạp:** Sử dụng các nút của PlatformIO trong VS Code (Build, Upload) hoặc lệnh:
    ```bash
    pio run --target upload
    ```
8.  **Giám sát:** Mở Serial Monitor để xem log hoạt động:
    ```bash
    pio device monitor
    ```
9.  **Sử dụng ThingsBoard:** Truy cập dashboard của bạn trên ThingsBoard để gửi lệnh RPC (`CHECK_IN_OUT`) và theo dõi trạng thái các phòng, cửa.

