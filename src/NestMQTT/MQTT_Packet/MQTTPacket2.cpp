#include "MQTTPacket.h"
#include "MQTTAsyncTask.h"
#include "MQTTCore.h"
#include "MQTTUtility.h"

using namespace MQTTCore;

namespace MQTTPacket {

Packet::~Packet() { free(_packetData); }

size_t Packet::available(size_t index) {
  if (index >= _packetSize)
    return 0;
  if (!_getPayload)
    return _packetSize - index;
  return _chunkedAvailable(index);
}

const uint8_t *Packet::data(size_t index) const {
  if (!_getPayload) {
    if (!_packetData || index >= _packetSize)
      return nullptr;
    return &_packetData[index];
  }
  return _chunkedData(index);
}

size_t Packet::size() const { return _packetSize; }

void Packet::setDup() {
  if (!_packetData)
    return;
  if (packetType() != MQTTCore::PacketType.PUBLISH)
    return;
  if (_packetId == 0)
    return;
  _packetData[0] |= 0x08;
}

uint16_t Packet::packetId() const { return _packetId; }

MQTTPacketType Packet::packetType() const {
  if (_packetData)
    return static_cast<MQTTPacketType>(_packetData[0] & 0xF0);
  return static_cast<MQTTPacketType>(0);
}

bool Packet::removable() const {
  if (_packetId == 0)
    return true;
  if (packetType() == MQTTCore::PacketType.PUBACK ||
      packetType() == MQTTCore::PacketType.PUBCOMP)
    return true;
  return false;
}

   size_t Packet::calculateRemainingLength(
                  const char *clientId ,
                  const char *username ,
                  const char *password ,
                  const char *willTopic ,
                  uint16_t willPayloadLength 

              ){
                     size_t remainingLength =
                 6 + 1 + 1 + 2 + 2 + (clientId ? strlen(clientId) : 0) +
               (willTopic ? 2 + strlen(willTopic) + 2 + willPayloadLength : 0) +
               (username ? 2 + strlen(username) : 0) +
                (password ? 2 + strlen(password) : 0);
               return remainingLength;

              }
   size_t Packet::calculateRemainingLength(

                  const char *Topic ,
                  uint16_t PayloadLength ,
                  uint16_t keepAlive ,
                  uint8_t qos 

              ){

size_t remainingLength = 2 + (Topic ? strlen(Topic) : 0) + (qos ? 2 : 0) + PayloadLength;
            return remainingLength;



              }
  size_t Packet::calculateRemainingLength(
                  SubscribeItem list,
                  size_t numberTopics,
                  uint8_t qos

              ){
                
  size_t remainingLength = 2; // Initial length for Packet Identifier

  // Calculate remaining length
  for (size_t i = 0; i < numberTopics; ++i) {
    remainingLength += 2 + strlen(list[i].topic) + 1;
  }
return remainingLength;


              }

// CONNECT
Packet::Packet(MQTTErrors &error, bool cleanSession, const char *username,
               const char *password, const char *willTopic, bool willRetain,
               uint8_t willQos, const uint8_t *willPayload,
               uint16_t willPayloadLength, uint16_t keepAlive,
               const char *clientId)
    : _error(error), _packetId(0), _packetData(nullptr), _packetSize(0),
      _payloadIndex(0), _payloadStartIndex(0), _payloadEndIndex(0),
      _subscription{}, _subscriptionPtr(nullptr), _getPayload(nullptr) {
  if (willPayload && willPayloadLength == 0) {
    size_t length = strlen(reinterpret_cast<const char *>(willPayload));
    willPayloadLength = (length > UINT16_MAX) ? UINT16_MAX : length;
  }
  if (!clientId || strlen(clientId) == 0) {
    error = MQTTErrors::MALFORMED_PARAMETER;
    return;
  }

  size_t remainingLength = calculateRemainingLength(clientId,username,password,willTopic,willPayloadLength);
      

  if (!_allocateMemory(remainingLength, false)) {
    error = MQTTErrors::OUT_OF_MEMORY;
    return;
  }

  size_t pos = 0;
  _packetData[pos++] =
      MQTTCore::PacketType.CONNECT | MQTTCore::HeaderFlag.CONNECT_RESERVED;
  pos += MQTTUtility::encodeRemainingLength(remainingLength, &_packetData[pos]);
  pos += MQTTUtility::encodeString(PROTOCOL, &_packetData[pos]);
  _packetData[pos++] = PROTOCOL_LEVEL;
  uint8_t connectFlags = 0;
  if (cleanSession)
    connectFlags |= MQTTCore::ConnectFlag.CLEAN_SESSION;
  if (username != nullptr)
    connectFlags |= MQTTCore::ConnectFlag.USERNAME;
  if (password != nullptr)
    connectFlags |= MQTTCore::ConnectFlag.PASSWORD;
  if (willTopic != nullptr) {
    connectFlags |= MQTTCore::ConnectFlag.WILL;
    if (willRetain)
      connectFlags |= MQTTCore::ConnectFlag.WILL_RETAIN;
    switch (willQos) {
              case 0:
      connectFlags |= MQTTCore::ConnectFlag.WILL_QOS0;
      break;
    case 1:
      connectFlags |= MQTTCore::ConnectFlag.WILL_QOS1;
      break;
    case 2:
      connectFlags |= MQTTCore::ConnectFlag.WILL_QOS2;
      break;
    }
  }
  _packetData[pos++] = connectFlags;
  _packetData[pos++] = keepAlive >> 8;
  _packetData[pos++] = keepAlive & 0xFF;
  // PAYLOAD
  // client ID
  pos += MQTTUtility::encodeString(clientId, &_packetData[pos]);
  // will
  if (willTopic != nullptr && willPayload != nullptr) {
    pos += MQTTUtility::encodeString(willTopic, &_packetData[pos]);
    _packetData[pos++] = willPayloadLength >> 8;
    _packetData[pos++] = willPayloadLength & 0xFF;
    memcpy(&_packetData[pos], willPayload, willPayloadLength);
    pos += willPayloadLength;
  }
  // credentials
  if (username != nullptr)
    pos += MQTTUtility::encodeString(username, &_packetData[pos]);
  if (password != nullptr)
    pos += MQTTUtility::encodeString(password, &_packetData[pos]);

  error = MQTTErrors::SUCCESS;
}
// PUBLISH
Packet::Packet(MQTTErrors &error, uint16_t packetId, const char *topic,
               const uint8_t *payload, size_t payloadLength, uint8_t qos,
               bool retain)
    : _packetId(packetId), _packetData(nullptr), _packetSize(0),
      _payloadIndex(0), _payloadStartIndex(0), _payloadEndIndex(0),
      _getPayload(nullptr) {
  
  size_t remainingLength = calculateRemainingLength(topic,payloadLength,0,qos);

  if (!_allocateMemory(remainingLength, false)) {
    error = MQTTErrors::OUT_OF_MEMORY;
    return;
  }

  size_t pos =
      _fillPublishHeader(packetId, topic, remainingLength, qos, retain);
  if (payloadLength) {
    memcpy(&_packetData[pos], payload, payloadLength);
    pos += payloadLength;
  }

  error = MQTTErrors::SUCCESS;
}
// PUBLISH
Packet::Packet(MQTTErrors &error, uint16_t packetId, const char *topic,
               MQTTCore::onPayloadInternalCallback payloadCallback,
               size_t payloadLength, uint8_t qos, bool retain)
    : _packetId(packetId), _packetData(nullptr), _packetSize(0),
      _payloadIndex(0), _payloadStartIndex(0), _payloadEndIndex(0),
      _getPayload(payloadCallback) {
 
   size_t remainingLength = calculateRemainingLength(topic,payloadLength,0,qos);

  if (qos == 0) {
    // remainingLength -= 2;
    _packetId = 0;
  }

  if (!_allocateMemory(remainingLength, false)) {
    error = MQTTErrors::OUT_OF_MEMORY;
    return;
  }

  size_t pos =
      _fillPublishHeader(packetId, topic, remainingLength, qos, retain);
  _payloadStartIndex = pos;
  _payloadEndIndex = _payloadStartIndex + payloadLength;

  error = MQTTErrors::SUCCESS;
}
// SUBSCRIBE
Packet::Packet(MQTTErrors &error, uint16_t packetId, const char *topic,
               uint8_t qos)
    : _packetId(packetId), _packetData(nullptr), _packetSize(0),
      _payloadIndex(0), _payloadStartIndex(0), _payloadEndIndex(0),
      _subscription(topic, qos), _subscriptionPtr(&_subscription),
      _unsubscription(), _unsubscriptionPtr(nullptr), _getPayload(nullptr) {
  _createSubscribe(error, _subscription);
}
// SUBSCRIBE
Packet::Packet(MQTTErrors &error, uint16_t packetId,
               const Subscription &subscription)
    : _packetId(packetId), _packetData(nullptr), _packetSize(0),
      _payloadIndex(0), _payloadStartIndex(0), _payloadEndIndex(0),
      _subscription(subscription), _subscriptionPtr(&_subscription),
      _unsubscription(), _unsubscriptionPtr(nullptr), _getPayload(nullptr) {
  _createSubscribe(error, subscription);
}
// SUBSCRIBE
template <typename... Args>
Packet::Packet(MQTTCore::MQTTErrors &error, uint16_t packetId,
               const char *topic1, uint8_t qos1, const char *topic2,
               uint8_t qos2, Args &&...args)
    : _packetId(packetId), _packetData(nullptr), _packetSize(0),
      _payloadIndex(0), _payloadStartIndex(0), _payloadEndIndex(0),
      _subscription(Subscription(topic1, qos1, topic2, qos2,
                                 std::forward<Args>(args)...)),
      _subscriptionPtr(&_subscription), _unsubscription(),
      _unsubscriptionPtr(nullptr), _getPayload(nullptr) {
  _createSubscribe(error, _subscription);
}

// UNSUBSCRIBE
Packet::Packet(MQTTErrors &error, uint16_t packetId, const char *topic)
    : _packetId(packetId), _packetData(nullptr), _packetSize(0),
      _payloadIndex(0), _payloadStartIndex(0), _payloadEndIndex(0),
      _subscription(), _subscriptionPtr(nullptr), _unsubscription(topic),
      _unsubscriptionPtr(&_unsubscription), _getPayload(nullptr) {
  _createUnsubscribe(error, _unsubscription);
}
// UNSUBSCRIBE
template <typename... Args>
Packet::Packet(MQTTErrors &error, // NOLINT(runtime/references)
               uint16_t packetId, const char *topic1, const char *topic2,
               Args &&...args)
    : _packetId(packetId), _packetData(nullptr), _packetSize(0),
      _payloadIndex(0), _payloadStartIndex(0), _payloadEndIndex(0),
      _subscription(), _subscriptionPtr(nullptr),
      _unsubscription(
          Unsubscription(topic1, topic2, std::forward<Args>(args)...)),
      _unsubscriptionPtr(&_unsubscription), _getPayload(nullptr) {
  _createUnsubscribe(error, _unsubscription);
}

// PUBACK, PUBREC, PUBREL, PUBCOMP
Packet::Packet(MQTTErrors &error, uint16_t packetId, MQTTPacketType type)
    : _packetId(packetId), _packetData(nullptr), _packetSize(0),
      _payloadIndex(0), _payloadStartIndex(0), _payloadEndIndex(0),
      _getPayload(nullptr) {
  size_t remainingLength = 2;

  if (!_allocateMemory(remainingLength, false)) {
    error = MQTTErrors::OUT_OF_MEMORY;
    return;
  }

  size_t pos = 0;
  _packetData[pos] = type;
  if (type == PacketType.PUBREL) {
    _packetData[pos++] |= HeaderFlag.PUBREL_RESERVED;
  } else {
    pos++;
  }
  pos += MQTTUtility::encodeRemainingLength(2, &_packetData[pos]);
  _packetData[pos++] = packetId >> 8;
  _packetData[pos] = packetId & 0xFF;

  error = MQTTErrors::SUCCESS;
}
// PING, DISCONNECT
Packet::Packet(MQTTErrors &error, MQTTPacketType type)
    : _packetId(0), _packetData(nullptr), _packetSize(0), _payloadIndex(0),
      _payloadStartIndex(0), _payloadEndIndex(0), _getPayload(nullptr) {
  size_t remainingLength = 0;

  if (!_allocateMemory(remainingLength, false)) {
    error = MQTTErrors::OUT_OF_MEMORY;
    return;
  }

  _packetData[0] = type;

  error = MQTTErrors::SUCCESS;
}
Packet &Packet::operator=(const Packet &other) {
  if (this != &other) {
    _packetId = other._packetId;
    _packetSize = other._packetSize;
    _packetData = static_cast<uint8_t *>(realloc(_packetData, _packetSize));
    if (_packetData) {
      memcpy(_packetData, other._packetData, _packetSize);
    }
  }
  return *this;
}

bool Packet::isEmpty() const {
  return (_packetData == nullptr || _packetSize == 0);
}

bool Packet::isValid() const {
  return (_packetData != nullptr && _packetSize > 0);
}

size_t Packet::_chunkedAvailable(size_t index) {
  // index vs size check done in 'available(index)'

  // index points to header or first payload byte
  if (index < _payloadIndex) {
    if (_packetSize > _payloadIndex && _payloadEndIndex != 0) {
      size_t copied =
          _getPayload(&_packetData[_payloadIndex],
                      std::min(static_cast<size_t>(TX_BUFFER_MAX_SIZE_BYTE),
                               _packetSize - _payloadStartIndex),
                      index);
      _payloadStartIndex = _payloadIndex;
      _payloadEndIndex = _payloadStartIndex + copied - 1;
    }

    // index points to payload unavailable
  } else if (index > _payloadEndIndex || _payloadStartIndex > index) {
    _payloadStartIndex = index;
    size_t copied =
        _getPayload(&_packetData[_payloadIndex],
                    std::min(static_cast<size_t>(TX_BUFFER_MAX_SIZE_BYTE),
                             _packetSize - _payloadStartIndex),
                    index);
    _payloadEndIndex = _payloadStartIndex + copied - 1;
  }

  // now index points to header or payload available
  return _payloadEndIndex - index + 1;
}

const uint8_t *Packet::_chunkedData(size_t index) const {
  return &_packetData[index];
}

bool Packet::_allocateMemory(size_t remainingLength, bool check) {

  if (check && MQTT_GET_FREE_MEMORY() < MQTT_MIN_FREE_MEMORY) {
    // emc_log_w("Packet buffer not allocated: low memory");
    return false;
  }
  _packetSize = 1 + MQTTUtility::remainingLengthFieldSize(remainingLength) +
                remainingLength;
  _packetData = reinterpret_cast<uint8_t *>(malloc(_packetSize));
  if (!_packetData) {
    _packetSize = 0;
    // emc_log_w("Alloc failed (l:%zu)", _size);
    return false;
  }
  // emc_log_i("Alloc (l:%zu)", _size);
  memset(_packetData, 0, _packetSize);
  return true;
}
size_t Packet::_fillPublishHeader(uint16_t packetId, const char *topic,
                                  size_t remainingLength, uint8_t qos,
                                  bool retain) {
  size_t index = 0;

  // FIXED HEADER
  _packetData[index] = MQTTCore::PacketType.PUBLISH;
  if (retain)
    _packetData[index] |= MQTTCore::HeaderFlag.PUBLISH_RETAIN;
  if (qos == 0) {
    _packetData[index++] |= MQTTCore::HeaderFlag.PUBLISH_QOS0;
  } else if (qos == 1) {
    _packetData[index++] |= MQTTCore::HeaderFlag.PUBLISH_QOS1;
  } else if (qos == 2) {
    _packetData[index++] |= MQTTCore::HeaderFlag.PUBLISH_QOS2;
  }
  index +=
      MQTTUtility::encodeRemainingLength(remainingLength, &_packetData[index]);

  // VARIABLE HEADER
  index += MQTTUtility::encodeString(topic, &_packetData[index]);
  if (qos > 0) {
    _packetData[index++] = packetId >> 8;
    _packetData[index++] = packetId & 0xFF;
  }

  return index;
}

void Packet::_createSubscribe(MQTTErrors &error,
                              const Subscription &subscription) {
  size_t numberTopics = subscription.numberTopics;
  size_t remainingLength = 2; // Initial length for Packet Identifier

  // Calculate remaining length
  for (size_t i = 0; i < numberTopics; ++i) {
    remainingLength += 2 + strlen(subscription.list[i].topic) + 1;
  }

  if (!_allocateMemory(remainingLength, false)) {
    error = MQTTErrors::OUT_OF_MEMORY;
    return;
  }

  size_t pos = 0;
  _packetData[pos++] =
      MQTTCore::PacketType.SUBSCRIBE | MQTTCore::HeaderFlag.SUBSCRIBE_RESERVED;
  pos += MQTTUtility::encodeRemainingLength(remainingLength, &_packetData[pos]);
  _packetData[pos++] = _packetId >> 8;
  _packetData[pos++] = _packetId & 0xFF;

  for (size_t i = 0; i < numberTopics; ++i) {
    pos += MQTTUtility::encodeString(subscription.list[i].topic,
                                     &_packetData[pos]);
    _packetData[pos++] = subscription.list[i].qos;
  }

  error = MQTTErrors::SUCCESS;
}

void Packet::_createUnsubscribe(MQTTErrors &error,
                                const Unsubscription &unsubscription) {
  size_t numberTopics = unsubscription.numberTopics;
  size_t remainingLength = 2; // Initial length for Packet Identifier

  // Calculate remaining length
  for (size_t i = 0; i < numberTopics; ++i) {
    remainingLength += 2 + strlen(unsubscription.list[i].topic);
  }

  if (!_allocateMemory(remainingLength, false)) {
    error = MQTTErrors::OUT_OF_MEMORY;
    return;
  }

  size_t pos = 0;
  _packetData[pos++] = MQTTCore::PacketType.UNSUBSCRIBE |
                       MQTTCore::HeaderFlag.UNSUBSCRIBE_RESERVED;
  pos += MQTTUtility::encodeRemainingLength(remainingLength, &_packetData[pos]);
  _packetData[pos++] = _packetId >> 8;
  _packetData[pos++] = _packetId & 0xFF;

  for (size_t i = 0; i < numberTopics; ++i) {
    pos += MQTTUtility::encodeString(unsubscription.list[i].topic,
                                     &_packetData[pos]);
  }

  error = MQTTErrors::SUCCESS;
}

} // namespace MQTTPacket
