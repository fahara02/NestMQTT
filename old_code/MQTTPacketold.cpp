// #include "MQTTPacket.h"
// #include "MQTTAsyncTask.h"
// #include "MQTTCore.h"
// #include "MQTTUtility.h"

// using namespace MQTTCore;

// namespace MQTTPacket {

// Packet::~Packet() { free(_packetData); }

// size_t Packet::available(size_t index) {
//   if (index >= _packetSize)
//     return 0;
//   if (!_getPayload)
//     return _packetSize - index;
//   return _chunkedAvailable(index);
// }

// const uint8_t *Packet::data(size_t index) const {
//   if (!_getPayload) {
//     if (!_packetData || index >= _packetSize)
//       return nullptr;
//     return &_packetData[index];
//   }
//   return _chunkedData(index);
// }

// size_t Packet::size() const { return _packetSize; }

// void Packet::setDup() {
//   if (!_packetData)
//     return;
//   if (static_cast<uint8_t>(packetType()) !=
//   static_cast<uint8_t>(PacketType::PUBLISH))
//     return;
//   if (_packetId == 0)
//     return;
//   _packetData[0] |= static_cast<uint8_t>(HeaderFlag::PUBLISH_DUP);
// }

// uint16_t Packet::packetId() const { return _packetId; }

// MQTTPacketType Packet::packetType() const {
//   if (_packetData)
//     return static_cast<MQTTPacketType>(_packetData[0] & 0xF0);
//   return static_cast<MQTTPacketType>(0);
// }

// bool Packet::removable() const {
//   if (_packetId == 0)
//     return true;
//   if (static_cast<uint8_t>(packetType()) ==
//   static_cast<uint8_t>(PacketType::PUBACK) ||
//      static_cast<uint8_t> (packetType() )==
//      static_cast<uint8_t>PacketType::PUBCOMP)
//     return true;
//   return false;
// }

// // CONNECT
// Packet::Packet(MQTTErrors &error, bool cleanSession, const char *username,
//                const char *password, const char *willTopic, bool willRetain,
//                uint8_t willQos, const uint8_t *willPayload,
//                uint16_t willPayloadLength, uint16_t keepAlive,
//                const char *clientId)
//     : _error(error), _packetId(0), _packetData(nullptr), _packetSize(0),
//       _payloadIndex(0), _payloadStartIndex(0), _payloadEndIndex(0),
//       _subscription{}, _subscriptionPtr(nullptr), _getPayload(nullptr) {
//   if (willPayload && willPayloadLength == 0) {
//     size_t length = strlen(reinterpret_cast<const char *>(willPayload));
//     willPayloadLength = (length > UINT16_MAX) ? UINT16_MAX : length;
//   }
//   if (!clientId || strlen(clientId) == 0) {
//     error = MQTTErrors::MALFORMED_PARAMETER;
//     return;
//   }

//   size_t remainingLength =
//       6 + 1 + 1 + 2 + 2 + strlen(clientId) +
//       (willTopic ? 2 + strlen(willTopic) + 2 + willPayloadLength : 0) +
//       (username ? 2 + strlen(username) : 0) +
//       (password ? 2 + strlen(password) : 0);

//   if (!_allocateMemory(remainingLength))
//     return;
//   if (!_packetData) {
//     error = MQTTErrors::OUT_OF_MEMORY;
//     return;
//   }

//   _packetData[0] = static_cast<uint8_t>(PacketType::CONNECT);
//   _packetData[1] = static_cast<uint8_t>(remainingLength);

//   size_t index = 2;
//   MQTTUtility::fillMQTTString(&_packetData[index], "MQTT", 4);
//   index += 4;
//   _packetData[index++] = 4;
//   _packetData[index++] = (cleanSession ? 0x02 : 0x00) |
//                          (willTopic ? 0x04 | (willQos & 0x03) << 3 |
//                                           (willRetain ? 0x20 : 0x00)
//                                     : 0x00) |
//                          (username ? 0x80 : 0x00) |
//                          (password ? 0x40 : 0x00);
//   MQTTUtility::fillTwoBytes(&_packetData[index], keepAlive);
//   index += 2;
//   MQTTUtility::fillMQTTString(&_packetData[index], clientId);
//   index += 2 + strlen(clientId);
//   if (willTopic) {
//     MQTTUtility::fillMQTTString(&_packetData[index], willTopic);
//     index += 2 + strlen(willTopic);
//     MQTTUtility::fillTwoBytes(&_packetData[index], willPayloadLength);
//     index += 2;
//     if (willPayloadLength)
//       memcpy(&_packetData[index], willPayload, willPayloadLength);
//     index += willPayloadLength;
//   }
//   if (username) {
//     MQTTUtility::fillMQTTString(&_packetData[index], username);
//     index += 2 + strlen(username);
//   }
//   if (password) {
//     MQTTUtility::fillMQTTString(&_packetData[index], password);
//     index += 2 + strlen(password);
//   }
//   error = MQTTErrors::SUCCESS;
// }

// // PUBLISH
// Packet::Packet(MQTTErrors &error, uint16_t packetId, const char *topic,
//                const uint8_t *payload, size_t payloadLength, uint8_t qos,
//                bool retain)
//     : _error(error), _packetId(packetId), _packetData(nullptr),
//       _packetSize(0), _payloadIndex(0), _payloadStartIndex(0),
//       _payloadEndIndex(0), _subscription{}, _subscriptionPtr(nullptr),
//       _getPayload(nullptr) {
//   size_t remainingLength = _fillPublishHeader(packetId, topic, payloadLength,
//                                               qos, retain);

//   if (!_allocateMemory(remainingLength))
//     return;
//   if (!_packetData) {
//     error = MQTTErrors::OUT_OF_MEMORY;
//     return;
//   }
//   size_t index = _payloadStartIndex;
//   if (payload && payloadLength) {
//     memcpy(&_packetData[index], payload, payloadLength);
//     index += payloadLength;
//   }
//   error = MQTTErrors::SUCCESS;
// }

// Packet::Packet(MQTTErrors &error, uint16_t packetId, const char *topic,
//                MQTTCore::onPayloadInternalCallback payloadCallback,
//                size_t payloadLength, uint8_t qos, bool retain)
//     : _error(error), _packetId(packetId), _packetData(nullptr),
//       _packetSize(0), _payloadIndex(0), _payloadStartIndex(0),
//       _payloadEndIndex(0), _subscription{}, _subscriptionPtr(nullptr),
//       _getPayload(payloadCallback) {
//   size_t remainingLength = _fillPublishHeader(packetId, topic, payloadLength,
//                                               qos, retain);

//   if (!_allocateMemory(remainingLength, false))
//     return;
//   if (!_packetData) {
//     error = MQTTErrors::OUT_OF_MEMORY;
//     return;
//   }
//   _payloadEndIndex = _packetSize;
//   error = MQTTErrors::SUCCESS;
// }

// size_t Packet::_fillPublishHeader(uint16_t packetId, const char *topic,
//                                   size_t remainingLength, uint8_t qos,
//                                   bool retain) {
//   size_t topicLength = strlen(topic);
//   remainingLength += 2 + topicLength + (qos > 0 ? 2 : 0);

//   if (!_allocateMemory(remainingLength))
//     return remainingLength;
//   if (!_packetData) {
//     _error = MQTTErrors::OUT_OF_MEMORY;
//     return remainingLength;
//   }

//   _packetData[0] = static_cast<uint8_t>(PacketType::PUBLISH) |
//                    static_cast<uint8_t>(retain ? HeaderFlag::PUBLISH_RETAIN
//                                               : HeaderFlag::CONNECT_RESERVED)
//                                               |
//                    static_cast<uint8_t>(qos == 1 ? HeaderFlag::PUBLISH_QOS1
//                                                  : qos == 2
//                                                        ?
//                                                        HeaderFlag::PUBLISH_QOS2
//                                                        :
//                                                        HeaderFlag::PUBLISH_QOS0);
//   size_t index = 1;
//   index += MQTTUtility::fillRemainingLength(&_packetData[index],
//                                             remainingLength - 1);
//   MQTTUtility::fillMQTTString(&_packetData[index], topic);
//   index += 2 + topicLength;
//   if (qos > 0) {
//     MQTTUtility::fillTwoBytes(&_packetData[index], packetId);
//     index += 2;
//   }
//   _payloadStartIndex = index;
//   _payloadEndIndex = _payloadStartIndex + remainingLength -
//   _payloadStartIndex; _error = MQTTErrors::SUCCESS; return remainingLength;
// }

// Packet::Packet(MQTTErrors &error, uint16_t packetId, const char *topic,
//                uint8_t qos)
//     : _error(error), _packetId(packetId), _packetData(nullptr),
//       _packetSize(0), _payloadIndex(0), _payloadStartIndex(0),
//       _payloadEndIndex(0), _subscription{}, _subscriptionPtr(nullptr),
//       _getPayload(nullptr) {
//   _createSubscribe(error, Subscription(topic, qos));
// }

// Packet::Packet(MQTTErrors &error, uint16_t packetId,
//                const Subscription &subscription)
//     : _error(error), _packetId(packetId), _packetData(nullptr),
//       _packetSize(0), _payloadIndex(0), _payloadStartIndex(0),
//       _payloadEndIndex(0), _subscription(subscription),
//       _subscriptionPtr(nullptr), _getPayload(nullptr) {
//   _createSubscribe(error, subscription);
// }

// Packet::Packet(MQTTErrors &error, uint16_t packetId,
//                std::vector<Subscription> &subscription)
//     : _error(error), _packetId(packetId), _packetData(nullptr),
//       _packetSize(0), _payloadIndex(0), _payloadStartIndex(0),
//       _payloadEndIndex(0), _subscriptionPtr(&subscription),
//       _getPayload(nullptr) {
//   if (subscription.empty()) {
//     error = MQTTErrors::MALFORMED_PARAMETER;
//     return;
//   }
//   size_t remainingLength = 2;
//   for (const auto &sub : subscription)
//     remainingLength += 2 + strlen(sub.topic) + 1;
//   if (!_allocateMemory(remainingLength))
//     return;
//   if (!_packetData) {
//     error = MQTTErrors::OUT_OF_MEMORY;
//     return;
//   }
//   _packetData[0] = static_cast<uint8_t>(PacketType::SUBSCRIBE) |
//                    static_cast<uint8_t>(HeaderFlag::SUBSCRIBE_RESERVED) |
//                    static_cast<uint8_t>(HeaderFlag::SUBSCRIBE_QOS1);
//   size_t index = 1;
//   index += MQTTUtility::fillRemainingLength(&_packetData[index],
//                                             remainingLength - 1);
//   MQTTUtility::fillTwoBytes(&_packetData[index], packetId);
//   index += 2;
//   for (const auto &sub : subscription) {
//     MQTTUtility::fillMQTTString(&_packetData[index], sub.topic);
//     index += 2 + strlen(sub.topic);
//     _packetData[index++] = sub.qos;
//   }
//   error = MQTTErrors::SUCCESS;
// }

// void Packet::_createSubscribe(MQTTErrors &error, const Subscription
// &subscription) {
//   if (!subscription.topic) {
//     error = MQTTErrors::MALFORMED_PARAMETER;
//     return;
//   }

//   size_t remainingLength = 2 + 2 + strlen(subscription.topic) + 1;

//   if (!_allocateMemory(remainingLength))
//     return;
//   if (!_packetData) {
//     error = MQTTErrors::OUT_OF_MEMORY;
//     return;
//   }

//   _packetData[0] = static_cast<uint8_t>(PacketType::SUBSCRIBE) |
//                    static_cast<uint8_t>(HeaderFlag::SUBSCRIBE_RESERVED) |
//                    static_cast<uint8_t>(HeaderFlag::SUBSCRIBE_QOS1);

//   size_t index = 1;
//   index += MQTTUtility::fillRemainingLength(&_packetData[index],
//                                             remainingLength - 1);
//   MQTTUtility::fillTwoBytes(&_packetData[index], _packetId);
//   index += 2;
//   MQTTUtility::fillMQTTString(&_packetData[index], subscription.topic);
//   index += 2 + strlen(subscription.topic);
//   _packetData[index++] = subscription.qos;

//   error = MQTTErrors::SUCCESS;
// }

// } // namespace MQTTPacket
