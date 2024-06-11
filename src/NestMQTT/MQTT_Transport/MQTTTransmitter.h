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
  Transmitter(uint32_t time, Args &&...args);

  // Destructor
  ~Transmitter() {}
   int _sendPacket();
  template <typename... Args> bool addPacket(Args &&...args);
  template <typename... Args> bool _addPacketFront(Args &&...args);
  void _checkBuffer();
  bool _advanceBuffer();

  const uint16_t &generateUniquePacketID();
  void updateLatestID(uint16_t packetID);
  const uint16_t getPacketID();

private:
  ControlPacketType parseControlPacketType(unsigned int value);

  uint32_t _transmitTime;
  uint16_t _packetID;

  struct TransmitStatusUpdate {
    size_t *bytesSent;
    bool *pingSent;
    uint32_t *lastClientActivity;
    uint32_t *lastServerActivity;
    DisconnectReason *disconnectReason;

    TransmitStatusUpdate();

    static TransmitStatusUpdate withBytesSent(size_t bytesSent);
    static TransmitStatusUpdate withPingSent(bool pingSent);
    static TransmitStatusUpdate withLastClientActivity(uint32_t lastClientActivity);
    static TransmitStatusUpdate withLastServerActivity(uint32_t lastServerActivity);
    static TransmitStatusUpdate withDisconnectReason(DisconnectReason disconnectReason);

    ~TransmitStatusUpdate();
  };

  struct TransmitStatus {
    size_t _bytesSent;
    bool _pingSent;
    uint32_t _lastClientActivity;
    uint32_t _lastServerActivity;
    DisconnectReason _disconnectReason;

    TransmitStatus();
    void update(const TransmitStatusUpdate &update);
  };

  struct OutboundPacket{
    uint32_t transmit_time;
    MQTTPacket::Packet packet;

    template <typename... Args>
   OutboundPacket(uint32_t t, MQTTCore::MQTTErrors &error,
                      uint16_t packetID, Args &&...args);
  };

  Buffer<OutboundPacket> transmitBuffer;
  TransmitStatus _transmitStatus;
  transmit_registry _registry;
};

} // namespace MQTTTransport

#endif // MQTT_TRANSMITTER_H_
