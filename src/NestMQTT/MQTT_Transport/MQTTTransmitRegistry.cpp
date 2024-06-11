#include "MQTTTransmitRegistry.h"
#include "Arduino.h"

namespace MQTTTransport {

uint16_t __transmit_next_pid(transmit_registry *treg) {
  if (treg->pid_lfsr == 0) {
    treg->pid_lfsr = 163u; // Initialize to a non-zero value
  }

  /* LFSR taps taken from:
   * https://en.wikipedia.org/wiki/Linear-feedback_shift_register */
  int pid_exists = 0;
  do {
    unsigned lsb = treg->pid_lfsr & 1;
    (treg->pid_lfsr) >>= 1;
    if (lsb) {
      treg->pid_lfsr ^= 0xB400u;
    }

    /* check that the PID is unique */
    pid_exists = 0;
    for (const auto &queuedPacket : treg->packet_queue) {
      if (queuedPacket.packet_id == treg->pid_lfsr) {
        pid_exists = 1;
        break;
      }
    }
  } while (pid_exists);
  // Store the generated packet ID in the QueuedPacket

  return treg->pid_lfsr;
}

} // namespace MQTTTransport

void testPacketIDGeneration() {
  MQTTTransport::transmit_registry registry;
  registry.pid_lfsr = 0; // Ensure initial state

  // Initialize Serial communication
  Serial.begin(9600);
  while (!Serial)
    continue;

  // Test generating packet IDs
  for (int i = 0; i < 10; ++i) {
    // Retrieve last generated packet ID
    uint16_t lastPacketID = MQTTTransport::__transmit_next_pid(&registry);

    // Output result
    Serial.print("Packet ID added: ");
    Serial.println(lastPacketID);

    // Debug: Print pid_lfsr value
    Serial.print("pid_lfsr value after generation: ");
    Serial.println(registry.pid_lfsr);
  }
}
