#ifndef CONFIG_H
#define CONFIG_H

#include <unordered_map>
#include <vector>

// Cấu hình giao tiếp I2C
#define SDA_PIN 14
#define SCL_PIN 13

// Cấu hình Serial Monitor
#define SERIAL_BAUD 115200

// Cấu hình thời gian quét NFC
#define NFC_SCAN_DELAY 2000  // 2 giây giữa mỗi lần quét

// Credentials
const char *WIFI_SSID       = "MikaSoCute!!!";
const char *WIFI_PASSWORD   = "12345671";
#define DEVICE_ACCESS_TOKEN "es12alt4fa25ryy4yej4"   // Token truy cập thiết bị

// ThingsBoard configuration
constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;
constexpr uint16_t MAX_MESSAGE_SEND_SIZE = 256U;
constexpr uint16_t MAX_MESSAGE_RECEIVE_SIZE = 256U;
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;
constexpr uint8_t MAX_RPC_SUBSCRIPTIONS = 3U;
constexpr uint8_t MAX_RPC_RESPONSE = 5U;
const std::unordered_map<std::string, std::vector<std::string>> RPC_CALLBACK = {
    {"METHODS", {"CHECK_IN_OUT", "ANOTHER_METHOD"}},
    {"KEYS", {"REQUEST", "ROOM"}}
};

// Global variables
bool subscribed = false; // Biến toàn cục để kiểm tra xem đã đăng ký RPC chưa

#endif // CONFIG_H