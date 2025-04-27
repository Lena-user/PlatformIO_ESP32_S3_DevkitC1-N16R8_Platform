#include "main.h"

// Global variables DEFINITION
WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
Server_Side_RPC<MAX_RPC_SUBSCRIPTIONS, MAX_RPC_RESPONSE> rpc;

// Create array of API implementations
const std::array<IAPI_Implementation *, 1U> apis = {&rpc};

// Initialize ThingsBoard instance with the maximum needed buffer size
ThingsBoard tb(
    mqttClient,
    MAX_MESSAGE_RECEIVE_SIZE,
    MAX_MESSAGE_SEND_SIZE,
    DEFAULT_MAX_STACK_SIZE,
    apis);

// Initialize NFC reader with configured pins
PN532_NFC nfcReader(SDA_PIN, SCL_PIN);

// Queue and semaphore handles
QueueHandle_t writeQueue = NULL;
SemaphoreHandle_t nfcMutex = NULL;

// Task handles
TaskHandle_t serverTaskHandle = NULL;
TaskHandle_t wifiTaskHandle = NULL;
TaskHandle_t readTaskHandle = NULL;
TaskHandle_t writeTaskHandle = NULL;

// State variables from config
bool subscribed = false;

// Array of door attribute keys for ThingsBoard
const char *DOOR_KEYS[] = {
    "door_Room_1",
    "door_Room_2",
    "door_Room_3",
    "door_Room_4"};

const char *STATUS_KEYS[] = {
    "status_Room_1",
    "status_Room_2",
    "status_Room_3",
    "status_Room_4"};

RoomManager roomManager(tb);