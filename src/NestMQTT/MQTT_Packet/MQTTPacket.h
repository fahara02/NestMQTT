#ifndef MQTT_PACKET_H_
#define MQTT_PACKET_H_

#include <cstring>
#include <stddef.h>
#include <stdint.h>
#include <utility>

#include "MQTTCallbacks.h"
#include "MQTTConstants.h"
#include "MQTTTransportPacket.h"

using namespace MQTTCore;

namespace MQTTPacket {

// Packet class definition
class Packet {
private:
  // Private member variables
  MQTTErrors &_error;
  uint16_t _packetId;
  uint8_t *_packetData = nullptr;
  size_t _packetSize = 0;

  // Variables for chunked payload handling
  size_t _payloadIndex = 0;
  size_t _payloadStartIndex = 0;
  size_t _payloadEndIndex = 0;

  // Callback for getting payload
  MQTTCore::onPayloadInternalCallback _getPayload = nullptr;

  Subscription _subscription;
  Subscription *_subscriptionPtr = nullptr;

  // Private member functions
  size_t requiredPacketSize(size_t remainingLength);
  bool _allocateMemory(size_t remainingLength, bool check = true);
  void _createSubscribe(MQTTErrors &error, const Subscription &subscription);

  Subscription CreateDefaultSubscription() {
    const char *topic = "*";
    uint8_t qos = 0;
    return Subscription(topic, qos);
  }

public:
  // Constructors
  explicit Packet(MQTTErrors &error, MQTTCore::ControlPacketType type)
      : _error(error), _packetId(0),
        _subscription(CreateDefaultSubscription()) {}

  Packet(MQTTErrors &error, uint16_t packetId, MQTTCore::ControlPacketType type)
      : _error(error), _packetId(packetId),
        _subscription(CreateDefaultSubscription()) {}

  Packet(MQTTErrors &error, uint16_t packetId, const char *topic, uint8_t qos)
      : _error(error), _packetId(packetId),
        _subscription(Subscription(topic, qos)) {}

  template <typename... Args>
  Packet(MQTTErrors &error, uint16_t packetId, const char *topic1, uint8_t qos1,
         const char *topic2, uint8_t qos2, Args &&...args)
      : _error(error), _packetId(packetId),
        _subscription(Subscription(topic1, qos1, topic2, qos2,
                                   std::forward<Args>(args)...)) {}

  Packet(MQTTErrors &error, uint16_t packetId, const Subscription &subscription)
      : _error(error), _packetId(packetId), _subscription(subscription) {}

  Packet(MQTTErrors &error, uint16_t packetId, const char *topic)
      : _error(error), _packetId(0),
        _subscription(CreateDefaultSubscription()) {}

  // Destructor
  ~Packet() = default;

  // Copy assignment operator
  Packet &operator=(const Packet &other) {
    if (this != &other) { // Check for self-assignment
      // Copy member variables from 'other' to 'this'
      _error = other._error;
      _packetId = other._packetId;
      _packetSize = other._packetSize;

      // Deep copy of _packetData
      if (_packetData != nullptr && _packetSize > 0) {
        delete[] _packetData; // Delete existing data if any
      }
      if (other._packetData != nullptr && other._packetSize > 0) {
        _packetData = new uint8_t[other._packetSize];
        std::memcpy(_packetData, other._packetData, other._packetSize);
      } else {
        _packetData = nullptr;
      }

      // Copy other member variables
      _payloadIndex = other._payloadIndex;
      _payloadStartIndex = other._payloadStartIndex;
      _payloadEndIndex = other._payloadEndIndex;
      _getPayload = other._getPayload;

      // Deep copy of Subscription object
      if (other._subscriptionPtr != nullptr) {
        _subscriptionPtr = new Subscription(*other._subscriptionPtr);
      } else {
        _subscriptionPtr = nullptr;
      }
    }
    return *this;
  }

  // Public member functions
  size_t size() const;
  uint16_t packetID() const;
  const uint8_t *data() const;
  const uint8_t *data(size_t index) const;
  MQTTCore::ControlPacketType packetType() const;
  size_t available(size_t index);
  void setDup();
  bool removable() const;
  bool isEmpty() const {
    return (_packetData == nullptr) || (_packetSize == 0);
  }
  bool isValid() const {
    // Check if the packet data pointer is not null and the packet size is
    // greater than 0
    return (_packetData != nullptr) && (_packetSize > 0);
  }

  MQTTErrors getPacketError();
};

} // namespace MQTTPacket

#endif // MQTT_PACKET_H_
