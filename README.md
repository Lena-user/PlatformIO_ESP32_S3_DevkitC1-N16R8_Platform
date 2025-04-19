# PlatformIO ESP32-S3 DevkitC1-N16R8 Platform

## 📌 Giới thiệu
Dự án này cung cấp cấu hình và mã nguồn cho **ESP32-S3 DevkitC1-N16R8**, giúp phát triển ứng dụng nhúng với **PlatformIO** một cách dễ dàng.

## 🚀 Tính năng chính
- Hỗ trợ **ESP32-S3 DevkitC1-N16R8** với **PlatformIO**.
- Cấu hình **I2C, SPI, UART** và các giao thức phổ biến.
- Tích hợp **RFID PN532** để đọc thẻ NFC.
- Quản lý bộ nhớ Flash và PSRAM tối ưu.

## 🛠 Cách cài đặt
1. **Clone repository**:
   ```bash
   git clone https://github.com/Lena-user/PlatformIO_ESP32_S3_DevkitC1-N16R8_Platform.git
   cd PlatformIO_ESP32_S3_DevkitC1-N16R8_Platform
2. **Cài đặt PlatformIO** (nếu chưa có):
   - [Hướng dẫn cài đặt PlatformIO](https://platformio.org/install)
   - Hoặc dùng lệnh để cài đặt trên VS Code:
     ```bash
     code --install-extension platformio.platformio-ide
     ```

3. **Mở dự án với VS Code**:
   - Mở VS Code và chọn **File > Open Folder**.
   - Chọn thư mục `PlatformIO_ESP32_S3_DevkitC1-N16R8_Platform`.

4. **Biên dịch và tải chương trình lên ESP32-S3**:
   - Kết nối ESP32-S3 với máy tính qua cổng USB.
   - Chạy lệnh sau để biên dịch và tải lên thiết bị:
     ```bash
     pio run --target upload
     ```

5. **Giám sát Serial**:
   - Để kiểm tra log của thiết bị, dùng lệnh:
     ```bash
     pio device monitor
     ```

---

