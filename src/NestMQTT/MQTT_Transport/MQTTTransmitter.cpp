#include "MQTTTransmitter.h"

using namespace MQTTTransport;

template <typename... Args>
Transmitter::Transmitter(uint32_t time, Args &&...args)
    : _transmitTime(time), _transmitStatus{}, _registry{} {
  // Initial status update
  _transmitStatus.update(TransmitStatusUpdate::withLastClientActivity(time));
  // Optionally, add a packet during construction
  // addPacket(std::forward<Args>(args)...);
}

template <typename... Args>
bool Transmitter::addPacket(Args &&...args) {
  MQTTCore::MQTTErrors error(MQTTCore::MQTTErrors::OK);
  const uint16_t &packetID = generateUniquePacketID();
  updateLatestID(packetID);
  Transmitter::OutboundPacket outboundPacket(_transmitTime, error, packetID,
                                   std::forward<Args>(args)...);

  _registry.packet_queue.pushBack(
      QueuedPacket{nullptr, packetID, 0, MQTT_QUEUED_UNSENT, 0, DISCONNECT});
  if (error != MQTTCore::MQTTErrors::OK) {
    return false; // Failed to create packet
  }
  if (!transmitBuffer.pushBack(outboundPacket)) {
    // Failed to add packet to buffer
    return false;
  }
  return true;
}

template <typename... Args>
bool Transmitter::_addPacketFront(Args &&...args) {
  MQTTCore::MQTTErrors error(MQTTCore::MQTTErrors::OK);
  const uint16_t &packetID = generateUniquePacketID();
  updateLatestID(packetID);
  OutboundPacket outboundPacket(_transmitTime, error, packetID,
                                   std::forward<Args>(args)...);
  if (error != MQTTCore::MQTTErrors::OK) {
    return false; // Failed to create packet
  }
  if (!transmitBuffer.pushFront(outboundPacket)) {
    // Failed to add packet to buffer
    return false;
  }
  return true;
}



bool Transmitter::_advanceBuffer() {
  MQTT_SEMAPHORE_TAKE();

  OutboundPacket *outboundPacket= transmitBuffer.getCurrent();

  if (!outboundPacket) {
    // No packet available, return false
    MQTT_SEMAPHORE_GIVE();
    return false;
  }

  MQTTPacket::Packet &packet = outboundPacket->packet;

  if (packet.isValid() &&
      _transmitStatus._bytesSent == packet.getPacketSize()) {
    if (packet.packetType() == ControlPacketType::DISCONNECT) {
      _transmitStatus.update(TransmitStatusUpdate::withDisconnectReason(
          DisconnectReason::USER_OK));
    }
    if (packet.removable()) {
      transmitBuffer.removeCurrent();
    } else {
      // Set 'dup' in case we have to retry
      if (packet.packetType() == ControlPacketType::PUBLISH) {
        packet.setDup();
      }
      transmitBuffer.next();
    }
    // Move to the next packet
    outboundPacket= transmitBuffer.getCurrent(); // Update pointer
    if (!outboundPacket ){
      MQTT_SEMAPHORE_GIVE();
      return false;
    } else {
      // Update packet reference to the next packet
      packet = outboundPacket->packet;
    }
    _transmitStatus.update(TransmitStatusUpdate::withBytesSent(0));
  }

  MQTT_SEMAPHORE_GIVE();

  return true;
}
const uint16_t &Transmitter::generateUniquePacketID() {
  _registry.pid_lfsr = __transmit_next_pid(&_registry);
  return _registry.pid_lfsr;
}

void Transmitter::updateLatestID(uint16_t packetID) {
  _packetID = packetID;
}

 uint16_t Transmitter::getPacketID() {
  return _packetID;
}
Transmitter::TransmitStatusUpdate::TransmitStatusUpdate()
    : bytesSent(nullptr), pingSent(nullptr), lastClientActivity(nullptr),
      lastServerActivity(nullptr), disconnectReason(nullptr) {}

Transmitter::TransmitStatusUpdate
Transmitter::TransmitStatusUpdate::withBytesSent(size_t bytesSent) {
  TransmitStatusUpdate update;
  update.bytesSent = new size_t(bytesSent);
  return update;
}

Transmitter::TransmitStatusUpdate
Transmitter::TransmitStatusUpdate::withPingSent(bool pingSent) {
  TransmitStatusUpdate update;
  update.pingSent = new bool(pingSent);
  return update;
}

Transmitter::TransmitStatusUpdate
Transmitter::TransmitStatusUpdate::withLastClientActivity(uint32_t lastClientActivity) {
  TransmitStatusUpdate update;
  update.lastClientActivity = new uint32_t(lastClientActivity);
  return update;
}

Transmitter::TransmitStatusUpdate
Transmitter::TransmitStatusUpdate::withLastServerActivity(uint32_t lastServerActivity) {
  TransmitStatusUpdate update;
  update.lastServerActivity = new uint32_t(lastServerActivity);
  return update;
}

Transmitter::TransmitStatusUpdate
Transmitter::TransmitStatusUpdate::withDisconnectReason(DisconnectReason disconnectReason) {
  TransmitStatusUpdate update;
  update.disconnectReason = new DisconnectReason(disconnectReason);
  return update;
}

Transmitter::TransmitStatusUpdate::~TransmitStatusUpdate() {
  delete bytesSent;
  delete pingSent;
  delete lastClientActivity;
  delete lastServerActivity;
  delete disconnectReason;
}

Transmitter::TransmitStatus::TransmitStatus()
    : _bytesSent(0), _pingSent(false), _lastClientActivity(0),
      _lastServerActivity(0), _disconnectReason(DisconnectReason::USER_OK) {}

void Transmitter::TransmitStatus::update(const TransmitStatusUpdate &update) {
  if (update.bytesSent)
    _bytesSent = *update.bytesSent;
  if (update.pingSent)
    _pingSent = *update.pingSent;
  if (update.lastClientActivity)
    _lastClientActivity = *update.lastClientActivity;
  if (update.lastServerActivity)
    _lastServerActivity = *update.lastServerActivity;
  if (update.disconnectReason)
    _disconnectReason = *update.disconnectReason;
}

template <typename... Args>
Transmitter::OutboundPacket::OutboundPacket(uint32_t t, MQTTCore::MQTTErrors &error,
                                                  uint16_t packetID, Args &&...args)
    : transmit_time(t),
      packet(error, packetID, std::forward<Args>(args)...) {}
