#ifndef CONFIG_H
#define CONFIG_H

#include <unordered_map>
#include <vector>
#include <string>

// Include secrets file with credentials
#include "secrets.h"

// Version information
constexpr char FIRMWARE_VERSION[] = "1.0.0";

// I2C Configuration
constexpr int SDA_PIN = 14;
constexpr int SCL_PIN = 13;

// Serial Monitor Configuration
constexpr unsigned long SERIAL_BAUD = 115200UL;

// NFC Configuration
constexpr unsigned int NFC_SCAN_DELAY = 2000;  // 2 seconds between scans
struct NFCConfig {
    static const uint8_t DEFAULT_KEY[6]; // Just declare it here
    static constexpr uint8_t DATA_BLOCK = 4;
};

// ThingsBoard Configuration
constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;
// Access token is defined in secrets.h
constexpr uint16_t MAX_MESSAGE_SEND_SIZE = 256U;
constexpr uint16_t MAX_MESSAGE_RECEIVE_SIZE = 256U;
constexpr uint16_t DEFAULT_MAX_STACK_SIZE = 1024U;
constexpr uint8_t MAX_RPC_SUBSCRIPTIONS = 3U;
constexpr uint8_t MAX_RPC_RESPONSE = 5U;

// RPC Callback Configuration
const std::unordered_map<std::string, std::vector<std::string>> RPC_CALLBACK = {
    {"METHODS", {"CHECK_IN_OUT"}},
    {"KEYS", {"REQUEST", "ROOM"}}
};

// Debug configuration
#ifdef DEBUG_MODE
constexpr bool ENABLE_DEBUG_LOGS = true;
#else
constexpr bool ENABLE_DEBUG_LOGS = false;
#endif

#endif // CONFIG_H