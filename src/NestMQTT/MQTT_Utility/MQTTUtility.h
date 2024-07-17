#ifndef MQTT_UTILITY_H_
#define MQTT_UTILITY_H_

#include "MQTTCodes.h"
#include "MQTTConstants.h"
#include "MQTTLog.h"
#include <cstdint>
#include <cstring>

using namespace MQTTCore;

class MQTTUtility {
public:
  static ConnackReturnCode mapToConnackReturnCode(uint8_t code) {
    switch (code) {
    case 0:
      return ConnackReturnCode::MQTT_CONNACK_ACCEPTED;
    case 1:
      return ConnackReturnCode::MQTT_CONNACK_REFUSED_BAD_USER_NAME_OR_PASSWORD;
    case 2:
      return ConnackReturnCode::MQTT_CONNACK_REFUSED_IDENTIFIER_REJECTED;
    case 3:
      return ConnackReturnCode::MQTT_CONNACK_REFUSED_NOT_AUTHORIZED;
    case 4:
      return ConnackReturnCode::MQTT_CONNACK_REFUSED_PROTOCOL_VERSION;
    case 5:
      return ConnackReturnCode::MQTT_CONNACK_REFUSED_SERVER_UNAVAILABLE;
    default:
      return ConnackReturnCode::MQTT_CONNACK_REFUSED_SERVER_UNAVAILABLE;
    }
  }

  const char *disconnectReasonToString(DisconnectReason reason) {
    switch (reason) {
    case DisconnectReason::USER_OK:
      return "No error";
    case DisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION:
      return "Unacceptable protocol version";
    case DisconnectReason::MQTT_IDENTIFIER_REJECTED:
      return "Identified rejected";
    case DisconnectReason::MQTT_SERVER_UNAVAILABLE:
      return "Server unavailable";
    case DisconnectReason::MQTT_MALFORMED_CREDENTIALS:
      return "Malformed credentials";
    case DisconnectReason::MQTT_NOT_AUTHORIZED:
      return "Not authorized";
    case DisconnectReason::TLS_BAD_FINGERPRINT:
      return "Bad fingerprint";
    case DisconnectReason::TCP_CONNECTION_LOST:
      return "TCP disconnected";
    default:
      return "";
    }
  }

  const char *subscribeReturncodeToString(SubscribeReturncode returnCode) {
    switch (returnCode) {
    case SubscribeReturncode::QOS0:
      return "QoS 0";
    case SubscribeReturncode::QOS1:
      return "QoS 1";
    case SubscribeReturncode::QOS2:
      return "QoS 2";
    case SubscribeReturncode::FAIL:
      return "Failed";
    default:
      return "";
    }
  }

  static size_t encodeString(const char *source, uint8_t *dest) {
    size_t length = std::strlen(source);
    if (length > UTF8_STRING_MAX_LENGTH) {
      mqtt_log(MQTTErrors::STRING_LENGTH_ERROR);
      return 0;
    }

    dest[0] = static_cast<uint8_t>((length >> 8) & 0xFF);
    dest[1] = static_cast<uint8_t>(length & 0xFF);
    std::memcpy(&dest[2], source, length);
    return 2 + length;
  }

  // Overload for encoding std::string
  static size_t encodeString(const std::string &source, uint8_t *dest) {
    return encodeString(source.c_str(), dest);
  }

  static int32_t decodeRemainingLength(const uint8_t *stream) {
    uint32_t multiplier = 1;
    int32_t remainingLength = 0;
    uint8_t currentByte = 0;
    uint8_t encodedByte;

    do {
      encodedByte = stream[currentByte++];
      remainingLength += (encodedByte & 127) * multiplier;
      if (multiplier > 128 * 128 * 128) {
        mqtt_log(MQTTErrors::MALFORMED_REMAINING_LENGTH);
        return -1;
      }
      multiplier *= 128;
    } while ((encodedByte & 128) != 0);

    return remainingLength;
  }

  static uint8_t remainingLengthFieldSize(uint32_t remainingLength) {
    if (remainingLength < 128)
      return 1;
    if (remainingLength < 16384)
      return 2;
    if (remainingLength < 2097152)
      return 3;
    if (remainingLength > 268435455)
      return 0;
    return 4;
  }

  static uint8_t encodeRemainingLength(uint32_t remainingLength,
                                       uint8_t *destination) {
    uint8_t currentByte = 0;
    uint8_t bytesNeeded = 0;

    do {
      uint8_t encodedByte = remainingLength % 128;
      remainingLength /= 128;
      // if there are more data to encode than 8 bit or 128, set the top bit of
      // this byte
      if (remainingLength > 0) {
        // encodedByte is the LSB and 128 is to set the continuation bit
        encodedByte = encodedByte | 128;
      }
      destination[currentByte++] = encodedByte;
      bytesNeeded++;
    } while (remainingLength > 0);

    return bytesNeeded;
  }
static void fillTwoBytes(uint8_t *data, uint16_t value) {
  data[0] = static_cast<uint8_t>((value >> 8) & 0xFF);
  data[1] = static_cast<uint8_t>(value & 0xFF);
}
  
static void fillMQTTString(uint8_t *data, const char *str) {
  size_t length = strlen(str);
  fillTwoBytes(data, static_cast<uint16_t>(length));
  memcpy(&data[2], str, length);
}



static size_t fillRemainingLength(uint8_t *data, size_t length) {
  size_t index = 0;
  do {
    uint8_t encodedByte = length % 128;
    length /= 128;
    if (length > 0)
      encodedByte |= 0x80;
    data[index++] = encodedByte;
  } while (length > 0);
  return index;
}










};

#endif