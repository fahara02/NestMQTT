#ifndef MQTT_TRANSPORT_H_
#define MQTT_TRANSPORT_H_

#include <stddef.h> 
#include <IPAddress.h>

namespace MQTTTransport{

class Transport {
 public:
  virtual bool connect(IPAddress ip, uint16_t port) = 0;
  virtual bool connect(const char* host, uint16_t port) = 0;
  virtual size_t write(const uint8_t* buf, size_t size) = 0;
  virtual int read(uint8_t* buf, size_t size) = 0;
  virtual void stop() = 0;
  virtual bool connected() = 0;
  virtual bool disconnected() = 0;
};




}



#endif