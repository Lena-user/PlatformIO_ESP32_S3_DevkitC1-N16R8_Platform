# PlatformIO ESP32-S3 DevkitC1-N16R8 - RFID PN532  

## 📌 Giới thiệu  
Dự án này tập trung vào việc tích hợp **RFID PN532** với **ESP32-S3 DevkitC1-N16R8**, giúp đọc và xử lý thẻ NFC một cách hiệu quả bằng **PlatformIO**.  

## 🚀 Tính năng chính  
- Hỗ trợ **RFID PN532** với giao tiếp **I2C/SPI/UART**.  
- Đọc UID của thẻ NFC và xử lý dữ liệu.  
- Ghi và đọc dữ liệu từ thẻ MIFARE.  
- Quản lý bộ nhớ Flash và PSRAM tối ưu trên ESP32-S3.  
- Dễ dàng mở rộng và tùy chỉnh theo nhu cầu.  

## 🛠 Cách cài đặt  
1. **Clone repository**:  
   ```bash  
   git clone https://github.com/Lena-user/PlatformIO_ESP32_S3_DevkitC1-N16R8_Platform.git  
   cd PlatformIO_ESP32_S3_DevkitC1-N16R8_Platform  

- Cài đặt PlatformIO (nếu chưa có):- Hướng dẫn cài đặt PlatformIO
- Hoặc dùng lệnh để cài đặt trên VS Code:code --install-extension platformio.platformio-ide  


- Mở dự án với VS Code:- Mở VS Code và chọn File > Open Folder.
- Chọn thư mục PlatformIO_ESP32_S3_DevkitC1-N16R8_Platform.

## 🔗 Kết nối ESP32-S3 với RFID PN532  

### 🟢 I2C (Khuyến nghị)  
| Chân PN532 | Chân ESP32-S3 | Chức năng |  
|-----------|-------------|------------|  
| **VCC**  | **3.3V**   | Nguồn      |  
| **GND**  | **GND**    | Mass       |  
| **SDA**  | **GPIO14** | Dữ liệu I2C |  
| **SCL**  | **GPIO13** | Clock I2C  |  

### 🔵 SPI  
| Chân PN532 | Chân ESP32-S3 | Chức năng |  
|-----------|-------------|------------|  
| **VCC**  | **3.3V**   | Nguồn      |  
| **GND**  | **GND**    | Mass       |  
| **SCK**  | **GPIO18** | Clock SPI  |  
| **MOSI** | **GPIO23** | Dữ liệu gửi |  
| **MISO** | **GPIO19** | Dữ liệu nhận |  
| **SS**   | **GPIO5**  | Chip Select |  

## 📂 Cấu trúc thư mục  

PlatformIO_ESP32-S3 DevkitC1-N16R8_Platform/
│── .vscode/            # Cấu hình VS Code
│── include/            # Header files (PN532Reader.h, config.h)
│── lib/                # Thư viện tùy chỉnh
│── src/                # Mã nguồn chính (PN532Reader.cpp)
│── test/               # Kiểm thử
│── platformio.ini      # Cấu hình PlatformIO
│── project_config.h    # Cấu hình dự án

Có vẻ như bạn cần chỉnh sửa một chút để đảm bảo đúng định dạng Markdown. Tôi thấy có một số khoảng cách thừa và thiếu dấu kết thúc trong phần khối mã. Dưới đây là phiên bản chuẩn hơn:

## 🔧 Cách sử dụng  

**Biên dịch và tải chương trình lên ESP32-S3:**  
```bash
pio run --target upload


**Giám sát Serial để kiểm tra dữ liệu RFID:**
'''bash
pio device monitor


**📜 Ghi chú**
Dự án này tập trung vào việc tích hợp RFID PN532 với ESP32-S3 bằng PlatformIO.
Nếu bạn muốn mở rộng tính năng, hãy đảm bảo tuân theo cấu trúc thư mục hiện có.
 


