#ifndef MQTT_RECEIVER_H_
#define MQTT_RECEIVER_H_
#include <stdint.h>
namespace MQTTTransport {

class Receiver {

public:
  // Constructor
  template <typename... Args> Receiver(uint32_t time, Args &&...args) : {}

  // Destructor
  ~Receiver() {}
};

} // namespace MQTTTransport
#endif