#ifndef MQTT_PACKET_H_
#define MQTT_PACKET_H_

#include <cstring>
#include <stddef.h>
#include <stdint.h>
#include <utility>

#include "MQTTCallbacks.h"
#include "MQTTConstants.h"
#include "MQTTError.h"
#include "MQTTTransportPacket.h"

namespace MQTTPacket {

// Packet class definition
class Packet {
private:
  MQTTErrors _error;
  uint16_t _packetId;
  uint8_t *_packetData;
  size_t _packetSize;

  // Variables for chunked payload handling
  size_t _payloadIndex;
  size_t _payloadStartIndex;
  size_t _payloadEndIndex;

  Subscription _subscription;
  Subscription *_subscriptionPtr;
  Unsubscription _unsubscription;
  Unsubscription *_unsubscriptionPtr;

  // Callback for getting payload
  MQTTCore::onPayloadInternalCallback _getPayload;

  bool _allocateMemory(size_t remainingLength, bool check = true);
  size_t _fillPublishHeader(uint16_t packetId, const char *topic,
                            size_t remainingLength, uint8_t qos, bool retain);

  void _createSubscribe(MQTTErrors &error, const Subscription &subscription);
  void _createUnsubscribe(MQTTErrors &error,
                          const Unsubscription &unsubscription);

public:
  // Constructor for CONNECT
  Packet(MQTTErrors &error, uint16_t packetId, bool cleanSession,
         const char *username, const char *password, const char *willTopic,
         bool willRetain, uint8_t willQos, const uint8_t *willPayload,
         uint16_t willPayloadLength, uint16_t keepAlive, const char *clientId);

  // Constructor for PUBLISH
  Packet(MQTTErrors &error, uint16_t packetId, const char *topic,
         const uint8_t *payload, size_t payloadLength, uint8_t qos,
         bool retain);
  Packet(MQTTErrors &error, uint16_t packetId, const char *topic,
         MQTTCore::onPayloadInternalCallback payloadCallback,
         size_t payloadLength, uint8_t qos, bool retain);

  // Constructor for SUBSCRIBE
  Packet(MQTTErrors &error, uint16_t packetId, const char *topic, uint8_t qos);
  Packet(MQTTErrors &error, uint16_t packetId,
         const Subscription &subscription);

  template <typename... Args>
  Packet(MQTTCore::MQTTErrors &error, uint16_t packetId, const char *topic1,
         uint8_t qos1, const char *topic2, uint8_t qos2, Args &&...args);

  // Constructor for UNSUBSCRIBE
  Packet(MQTTErrors &error, uint16_t packetId, const char *topic);
  template <typename... Args>
  Packet(MQTTErrors &error, // NOLINT(runtime/references)
         uint16_t packetId, const char *topic1, const char *topic2,
         Args &&...args);

  // Constructor for PUBACK, PUBREC, PUBREL, PUBCOMP
  Packet(MQTTErrors &error, uint16_t packetId, MQTTPacketType type);

  // Constructor for PING, DISCONNECT
  // Packet(MQTTErrors &error, MQTTPacketType type);

  // Destructor
  ~Packet();

  // Copy assignment operator
  Packet &operator=(const Packet &other);

  size_t size() const;
  uint16_t packetId() const;
  const uint8_t *data() const;
  const uint8_t *data(size_t index) const;
  MQTTCore::MQTTPacketType packetType() const;
  size_t available(size_t index);
  void setDup();
  bool removable() const;
  bool isEmpty() const;
  bool isValid() const;

  size_t _chunkedAvailable(size_t index);
  const uint8_t *_chunkedData(size_t index) const;
};

} // namespace MQTTPacket

#endif // MQTT_PACKET_H_
