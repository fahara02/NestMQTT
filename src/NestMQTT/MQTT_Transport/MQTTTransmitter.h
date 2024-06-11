#ifndef MQTT_TRANSMITTER_H_
#define MQTT_TRANSMITTER_H_

#include "MQTTAsyncTask.h"
#include "MQTTCore.h"
#include "MQTTError.h"
#include "MQTTPacket.h"
#include "MQTTTransmitRegistry.h"
#include <stdint.h>

using namespace MQTTCore;

namespace MQTTTransport {

class Transmitter {
public:
  // Constructor
  template <typename... Args>
  Transmitter(uint32_t time, Args &&...args)
      : _transmitTime(time), _transmitStatus{}, _registry{} {
    // Initial status update
    _transmitStatus.update(TransmitStatusUpdate::withLastClientActivity(time));
    // Optionally, add a packet during construction
    // addPacket(std::forward<Args>(args)...);
  }

  // Destructor
  ~Transmitter() {}

  template <typename... Args> bool addPacket(Args &&...args) {
    MQTTCore::MQTTErrors error(MQTTCore::MQTTErrors::OK);
    const uint16_t &packetID = generateUniquePacketID();
    updateLatestID(packetID);
    PacketForTransmit transmitPacket(_transmitTime, error, packetID,
                                     std::forward<Args>(args)...);

    _registry.packet_queue.pushBack(
        QueuedPacket{nullptr, packetID, 0, MQTT_QUEUED_UNSENT, 0, DISCONNECT});
    if (error != MQTTCore::MQTTErrors::OK) {
      return false; // Failed to create packet
    }
    if (!transmitBuffer.pushBack(transmitPacket)) {
      // Failed to add packet to buffer
      return false;
    }

    // Update status with the number of bytes sent
    // _transmitStatus.update(TransmitStatusUpdate::withBytesSent(transmitPacket.packet.getPacketSize()));

    return true;
  }

  template <typename... Args> bool _addPacketFront(Args &&...args) {
    MQTTCore::MQTTErrors error(MQTTCore::MQTTErrors::OK);
    const uint16_t &packetID = generateUniquePacketID();
    updateLatestID(packetID);
    PacketForTransmit transmitPacket(_transmitTime, error, packetID,
                                     std::forward<Args>(args)...);
    if (error != MQTTCore::MQTTErrors::OK) {
      return false; // Failed to create packet
    }
    if (!transmitBuffer.pushFront(transmitPacket)) {
      // Failed to add packet to buffer
      return false;
    }

    // Update status with the number of bytes sent
    // _transmitStatus.update(TransmitStatusUpdate::withBytesSent(transmitPacket.packet.getPacketSize()));

    return true;
  }

  bool _advanceBuffer() {
    MQTT_SEMAPHORE_TAKE();

    PacketForTransmit *transmitPacket = transmitBuffer.getCurrent();

    if (!transmitPacket) {
      // No packet available, return false
      MQTT_SEMAPHORE_GIVE();
      return false;
    }

    MQTTPacket::Packet &packet = transmitPacket->packet;

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
      transmitPacket = transmitBuffer.getCurrent(); // Update pointer
      if (!transmitPacket) {
        MQTT_SEMAPHORE_GIVE();
        return false;
      } else {
        // Update packet reference to the next packet
        packet = transmitPacket->packet;
      }
      _transmitStatus.update(TransmitStatusUpdate::withBytesSent(0));
    }

    MQTT_SEMAPHORE_GIVE();

    return true;
  }

  const uint16_t &generateUniquePacketID() {
    _registry.pid_lfsr = __transmit_next_pid(&_registry);
    return _registry.pid_lfsr;
  }

  void updateLatestID(uint16_t packetID) { _packetID = packetID; }
  const uint16_t getPacketID() { return _packetID; }

private:
  ControlPacketType parseControlPacketType(unsigned int value) {
    return static_cast<ControlPacketType>(value);
  }

  uint32_t _transmitTime;
  uint16_t _packetID;

  struct TransmitStatusUpdate {
    size_t *bytesSent;
    bool *pingSent;
    uint32_t *lastClientActivity;
    uint32_t *lastServerActivity;
    DisconnectReason *disconnectReason;

    TransmitStatusUpdate()
        : bytesSent(nullptr), pingSent(nullptr), lastClientActivity(nullptr),
          lastServerActivity(nullptr), disconnectReason(nullptr) {}

    static TransmitStatusUpdate withBytesSent(size_t bytesSent) {
      TransmitStatusUpdate update;
      update.bytesSent = new size_t(bytesSent);
      return update;
    }

    static TransmitStatusUpdate withPingSent(bool pingSent) {
      TransmitStatusUpdate update;
      update.pingSent = new bool(pingSent);
      return update;
    }

    static TransmitStatusUpdate
    withLastClientActivity(uint32_t lastClientActivity) {
      TransmitStatusUpdate update;
      update.lastClientActivity = new uint32_t(lastClientActivity);
      return update;
    }

    static TransmitStatusUpdate
    withLastServerActivity(uint32_t lastServerActivity) {
      TransmitStatusUpdate update;
      update.lastServerActivity = new uint32_t(lastServerActivity);
      return update;
    }

    static TransmitStatusUpdate
    withDisconnectReason(DisconnectReason disconnectReason) {
      TransmitStatusUpdate update;
      update.disconnectReason = new DisconnectReason(disconnectReason);
      return update;
    }

    ~TransmitStatusUpdate() {
      delete bytesSent;
      delete pingSent;
      delete lastClientActivity;
      delete lastServerActivity;
      delete disconnectReason;
    }
  };

  struct TransmitStatus {
    size_t _bytesSent;
    bool _pingSent;
    uint32_t _lastClientActivity;
    uint32_t _lastServerActivity;
    DisconnectReason _disconnectReason;

    // Default constructor
    TransmitStatus()
        : _bytesSent(0), _pingSent(false), _lastClientActivity(0),
          _lastServerActivity(0), _disconnectReason(DisconnectReason::USER_OK) {
    }

    // Update function
    void update(const TransmitStatusUpdate &update) {
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
  };

  // Nested struct for transmitting packets
  struct PacketForTransmit {
    uint32_t transmit_time;
    MQTTPacket::Packet packet;

    template <typename... Args>
    PacketForTransmit(uint32_t t, MQTTCore::MQTTErrors &error,
                      uint16_t packetID, Args &&...args)
        : transmit_time(t),
          packet(error, packetID, std::forward<Args>(args)...) {}
  };

  Buffer<PacketForTransmit> transmitBuffer;
  TransmitStatus _transmitStatus;
  transmit_registry _registry;
};

} // namespace MQTTTransport

#endif // MQTT_TRANSMITTER_H_
