#ifndef CONFIG_H
#define CONFIG_H

#include <unordered_map>
#include <vector>
#include <string>

// Cấu hình giao tiếp I2C
constexpr int SDA_PIN = 14;
constexpr int SCL_PIN = 13;

// Cấu hình Serial Monitor
constexpr unsigned long SERIAL_BAUD = 115200UL;

// Cấu hình thời gian quét NFC (tính bằng mili-giây)
constexpr unsigned int NFC_SCAN_DELAY = 2000;  // 2 giây giữa mỗi lần quét

// Credentials WiFi
constexpr char WIFI_SSID[]     = "MikaSoCute!!!";
constexpr char WIFI_PASSWORD[] = "12345671";

// Token truy cập thiết bị
constexpr char DEVICE_ACCESS_TOKEN[] = "es12alt4fa25ryy4yej4";

// ThingsBoard configuration
constexpr char THINGSBOARD_SERVER[]       = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT         = 1883U;
constexpr uint16_t MAX_MESSAGE_SEND_SIZE    = 256U;
constexpr uint16_t MAX_MESSAGE_RECEIVE_SIZE = 256U;
constexpr uint32_t SERIAL_DEBUG_BAUD        = 115200U;
constexpr uint8_t MAX_RPC_SUBSCRIPTIONS     = 3U;
constexpr uint8_t MAX_RPC_RESPONSE          = 5U;

// RPC Callback configuration
const std::unordered_map<std::string, std::vector<std::string>> RPC_CALLBACK = {
    {"METHODS", {"CHECK_IN_OUT", "ANOTHER_METHOD"}},
    {"KEYS", {"REQUEST", "ROOM"}}
};

// Biến toàn cục 
bool subscribed = false;

#endif // CONFIG_H