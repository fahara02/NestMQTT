#ifndef MQTT_CONSTANTS_H_
#define MQTT_CONSTANTS_H_
#include <stdint.h>

namespace MQTTCore {

constexpr const char PROTOCOL[] = "MQTT";
constexpr const uint8_t PROTOCOL_LEVEL = 0b00000100;

constexpr int UTF8_STRING_MAX_LENGTH = 65535;
constexpr int MQTT_TOPIC_MAX_LENGTH = 128;
constexpr int MQTT_CLIENT_ID_MAX_LENGTH = 23 + 1;
constexpr int MAX_ALLOWED_TOPICS = 10; // Maximum number of topics
constexpr int MAX_ALLOWED_RETRIES = 5;
constexpr int TX_BUFFER_MAX_SIZE_BYTE = 1440;

} // namespace MQTTCore

#endif