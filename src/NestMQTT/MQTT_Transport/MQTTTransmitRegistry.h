#ifndef MQTT_TRANSMIT_REGISTRY_H_
#define MQTT_TRANSMIT_REGISTRY_H_

#include "MQTTBuffer.h"
#include "MQTTCore.h"
#include <set>
#include <stdint.h>

namespace MQTTTransport {

typedef time_t mqtt_pal_time_t;
enum QueuedMessageState {
  MQTT_QUEUED_UNSENT,
  MQTT_QUEUED_AWAITING_ACK,
  MQTT_QUEUED_COMPLETE
};

struct QueuedPacket {
  uint8_t *start;
  uint16_t packet_id;
  size_t packet_size;
  QueuedMessageState packet_state;
  mqtt_pal_time_t packet_time_sent;
  MQTTCore::ControlPacketType packetType;
};

struct transmit_registry {
  Buffer<QueuedPacket> packet_queue;
  uint16_t pid_lfsr;
  std::set<uint16_t> used_packet_ids;
};

uint16_t __transmit_next_pid(transmit_registry *treg);
} // namespace MQTTTransport

void testPacketIDGeneration();

#endif // MQTT_TRANSMIT_REGISTRY_H_
