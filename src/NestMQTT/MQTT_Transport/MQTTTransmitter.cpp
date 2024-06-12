#include "MQTTTransmitter.h"
#include "MQTTAsyncTask.h"
#include "MQTTPacket.h"
#include "MqttClient.h"

namespace MQTTTransport {

template <typename... Args>
Transmitter::Transmitter(MqttClient *client, Args &&...args)
    : _client(client), _transport(client->_transport),
      _clientCfg(client->_clientcfg), _transmitTime(0), _transmitStatus{} {
  // Initial status update
  _transmitStatus.update(
      TransmitStatusUpdate::withLastClientActivity(millis()));
}

void Transmitter::updateConfig(
    const MQTTClientDetails::MqttClientCfg &newConfig) {
  _clientCfg = newConfig;
  // You can perform any necessary actions here
}

bool Transmitter::sendConnectionRequest() {
  bool result = false;
  if (_client->getClientState() == StateMachine::State::disconnected) {
    MQTT_SEMAPHORE_TAKE();
    if (_addPacketFront(

            _clientCfg.connections_settings._cleanSession,
            _clientCfg.last_will_settings._username,
            _clientCfg.last_will_settings._password,
            _clientCfg.last_will_settings._lwt_topic,
            _clientCfg.last_will_settings._willRetain,
            _clientCfg.last_will_settings._lwt_qos,
            _clientCfg.last_will_settings._lwt_msg,
            _clientCfg.last_will_settings._lwt_msg_len,
            (uint16_t)(_clientCfg.connections_settings._keepAlive / 1000),
            _clientCfg.set_null_client_id ? nullptr : _clientCfg.path)) {
      result = true;
      _client->_statemachine.handleEvent(StateMachine::Event::CONNECTED);
    } else {
      MQTT_SEMAPHORE_GIVE();
    }
    MQTT_SEMAPHORE_GIVE();
  }
  return result;
}

int Transmitter::_sendPacket() {
  MQTT_SEMAPHORE_TAKE();
  OutboundPacket *packet = transmitBuffer.getCurrent();
  size_t haveWritten = 0;

  if (packet) {
    size_t wantToWrite = packet->packet.available(_transmitStatus._bytesSent);
    size_t haveWritten = _transport->write(
        packet->packet.data(_transmitStatus._bytesSent), wantToWrite);
    if (haveWritten == wantToWrite) {
      packet->transmit_time = millis();
      _transmitStatus.update(
          TransmitStatusUpdate::withLastClientActivity(millis()));
      _transmitStatus.update(TransmitStatusUpdate::withBytesSent(
          _transmitStatus._bytesSent += haveWritten));

      MQTT_SEMAPHORE_GIVE();
      return haveWritten;
    } else {
      MQTT_SEMAPHORE_GIVE();
      return 0;
    }
  } else {
    MQTT_SEMAPHORE_GIVE();
    return 0;
  }
}

template <typename... Args> bool Transmitter::addPacket(Args &&...args) {
  MQTTCore::MQTTErrors error(MQTTCore::MQTTErrors::OK);
  const uint16_t &packetID = generateUniquePacketID();
  updateLatestID(packetID);
  OutboundPacket transmitPacket(_transmitTime, error, packetID,
                                std::forward<Args>(args)...);

  _registry.packet_queue.pushBack(
      QueuedPacket{nullptr, packetID, 0, MQTT_QUEUED_UNSENT, 0, DISCONNECT});
  if (error != MQTTCore::MQTTErrors::OK) {
    return false; // Failed to create packet
  }
  if (!transmitBuffer.pushBack(transmitPacket)) {
    return false; // Failed to add packet to buffer
  }
  return true;
}

template <typename... Args> bool Transmitter::_addPacketFront(Args &&...args) {
  MQTTCore::MQTTErrors error(MQTTCore::MQTTErrors::OK);
  const uint16_t &packetID = generateUniquePacketID();
  updateLatestID(packetID);
  OutboundPacket transmitPacket(_transmitTime, error, packetID,
                                std::forward<Args>(args)...);
  if (error != MQTTCore::MQTTErrors::OK) {
    return false; // Failed to create packet
  }
  if (!transmitBuffer.pushFront(transmitPacket)) {
    return false; // Failed to add packet to buffer
  }
  return true;
}

bool Transmitter::_advanceBuffer() {
  OutboundPacket *transmitPacket = transmitBuffer.getCurrent();

  if (!transmitPacket) {
    return false;
  }

  MQTTPacket::Packet &packet = transmitPacket->packet;

  if (packet.isValid() && _transmitStatus._bytesSent == packet.size()) {
    if (packet.packetType() == ControlPacketType::DISCONNECT) {
      _transmitStatus.update(TransmitStatusUpdate::withDisconnectReason(
          DisconnectReason::USER_OK));
    }
    if (packet.removable()) {
      transmitBuffer.removeCurrent();
    } else {
      if (packet.packetType() == ControlPacketType::PUBLISH) {
        packet.setDup();
      }
      transmitBuffer.next();
    }
    transmitPacket = transmitBuffer.getCurrent();
    if (!transmitPacket) {
      return false;
    } else {
      packet = transmitPacket->packet;
    }
    _transmitStatus.update(TransmitStatusUpdate::withBytesSent(0));
  }

  return true;
}

const uint16_t &Transmitter::generateUniquePacketID() {
  _registry.pid_lfsr = __transmit_next_pid(&_registry);
  return _registry.pid_lfsr;
}

void Transmitter::updateLatestID(uint16_t packetID) { _packetID = packetID; }

uint16_t Transmitter::getPacketID() { return _packetID; }

ControlPacketType Transmitter::parseControlPacketType(unsigned int value) {
  return static_cast<ControlPacketType>(value);
}

// Definitions for TransmitStatusUpdate struct
Transmitter::TransmitStatusUpdate::TransmitStatusUpdate()
    : bytesSent(nullptr), pingSent(nullptr), lastClientActivity(nullptr),
      lastServerActivity(nullptr), disconnectReason(nullptr) {}

Transmitter::TransmitStatusUpdate
Transmitter::TransmitStatusUpdate::withBytesSent(size_t bytesSent) {
  Transmitter::TransmitStatusUpdate update;
  update.bytesSent = new size_t(bytesSent);
  return update;
}

Transmitter::TransmitStatusUpdate
Transmitter::TransmitStatusUpdate::withPingSent(bool pingSent) {
  Transmitter::TransmitStatusUpdate update;
  update.pingSent = new bool(pingSent);
  return update;
}

Transmitter::TransmitStatusUpdate
Transmitter::TransmitStatusUpdate::withLastClientActivity(
    uint32_t lastClientActivity) {
  Transmitter::TransmitStatusUpdate update;
  update.lastClientActivity = new uint32_t(lastClientActivity);
  return update;
}

Transmitter::TransmitStatusUpdate
Transmitter::TransmitStatusUpdate::withLastServerActivity(
    uint32_t lastServerActivity) {
  Transmitter::TransmitStatusUpdate update;
  update.lastServerActivity = new uint32_t(lastServerActivity);
  return update;
}

Transmitter::TransmitStatusUpdate
Transmitter::TransmitStatusUpdate::withDisconnectReason(
    DisconnectReason disconnectReason) {
  Transmitter::TransmitStatusUpdate update;
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

// Definitions for TransmitStatus struct
Transmitter::TransmitStatus::TransmitStatus()
    : _bytesSent(0), _pingSent(false), _lastClientActivity(0),
      _lastServerActivity(0), _disconnectReason(DisconnectReason::USER_OK) {}

void Transmitter::TransmitStatus::update(
    const Transmitter::TransmitStatusUpdate &update) {
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

// // Definitions for OutboundPacket struct
// template <typename... Args>
// Transmitter::OutboundPacket::OutboundPacket(uint32_t t,
//                                             MQTTCore::MQTTErrors &error,
//                                             uint16_t packetID, Args
//                                             &&...args)
//     : transmit_time(t), packet(error, packetID, std::forward<Args>(args)...)
//     {}

} // namespace MQTTTransport
