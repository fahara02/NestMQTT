#ifndef MQTT_CORE_H_
#define MQTT_CORE_H_
#include "Arduino.h"
#include <stddef.h>
#include <stdint.h>

#include "MQTTCodes.h"

namespace MQTTCore {

enum MQTT_Protocol_Version_t {
  MQTT_PROTOCOL_UNDEFINED = 0,
  MQTT_PROTOCOL_V_3_1,
  MQTT_PROTOCOL_V_3_1_1
};

enum MQTT_Transport_t {
  MQTT_TRANSPORT_UNKNOWN = 0x0,
  MQTT_TRANSPORT_OVER_TCP, /*!< MQTT over TCP, using scheme: ``mqtt`` */
  MQTT_TRANSPORT_OVER_SSL, /*!< MQTT over SSL, using scheme: ``mqtts`` */
  MQTT_TRANSPORT_OVER_WS,  /*!< MQTT over Websocket, using scheme:: ``ws`` */
  MQTT_TRANSPORT_OVER_WSS /*!< MQTT over Websocket Secure, using scheme: ``wss``
                           */
};

enum ControlPacketType {
  CONNECT = 1u,
  CONNACK = 2u,
  PUBLISH = 3u,
  PUBACK = 4u,
  PUBREC = 5u,
  PUBREL = 6u,
  PUBCOMP = 7u,
  SUBSCRIBE = 8u,
  SUBACK = 9u,
  UNSUBSCRIBE = 10u,
  UNSUBACK = 11u,
  PINGREQ = 12u,
  PINGRESP = 13u,
  DISCONNECT = 14u
};

struct mqtt_fixed_header {
  enum ControlPacketType control_type;
  int control_flags : 4;
  uint32_t remaining_length;
};

struct mqtt_response_connack {
  uint8_t session_present_flag;
  enum ConnackReturnCode return_code;
};

struct mqtt_response_publish {
  uint8_t dup_flag;
  uint8_t qos_level;
  uint8_t retain_flag;
  uint16_t topic_name_size;
  const void *topic_name;
  uint16_t packet_id;
  const void *application_message;
  size_t application_message_size;
};

struct mqtt_response_puback {
  uint16_t packet_id;
};

struct mqtt_response_pubrec {
  uint16_t packet_id;
};

struct mqtt_response_pubrel {
  uint16_t packet_id;
};

struct mqtt_response_pubcomp {
  uint16_t packet_id;
};

struct mqtt_response_suback {
  uint16_t packet_id;
  const uint8_t *_return_codes;
  size_t num_return_codes;
};

struct mqtt_response_unsuback {
  uint16_t packet_id;
};

struct mqtt_response {
  struct mqtt_fixed_header fixed_header;
  union {
    struct mqtt_response_connack connack;
    struct mqtt_response_publish publish;
    struct mqtt_response_puback puback;
    struct mqtt_response_pubrec pubrec;
    struct mqtt_response_pubrel pubrel;
    struct mqtt_response_pubcomp pubcomp;
    struct mqtt_response_suback suback;
    struct mqtt_response_unsuback unsuback;
    /*struct mqtt_response_pingresp pingresp;*/
  } decoded;
};

constexpr struct {
  const uint8_t RESERVED1 = 0;
  const uint8_t CONNECT = 1 << 4;
  const uint8_t CONNACK = 2 << 4;
  const uint8_t PUBLISH = 3 << 4;
  const uint8_t PUBACK = 4 << 4;
  const uint8_t PUBREC = 5 << 4;
  const uint8_t PUBREL = 6 << 4;
  const uint8_t PUBCOMP = 7 << 4;
  const uint8_t SUBSCRIBE = 8 << 4;
  const uint8_t SUBACK = 9 << 4;
  const uint8_t UNSUBSCRIBE = 10 << 4;
  const uint8_t UNSUBACK = 11 << 4;
  const uint8_t PINGREQ = 12 << 4;
  const uint8_t PINGRESP = 13 << 4;
  const uint8_t DISCONNECT = 14 << 4;
  const uint8_t RESERVED2 = 1 << 4;
} PacketType;

struct HeaderFlag {
  const uint8_t CONNECT_RESERVED = 0x00;
  const uint8_t CONNACK_RESERVED = 0x00;
  const uint8_t PUBLISH_DUP = 0x08;
  const uint8_t PUBLISH_QOS0 = 0x00;
  const uint8_t PUBLISH_QOS1 = 0x02;
  const uint8_t PUBLISH_QOS2 = 0x04;
  const uint8_t PUBLISH_QOSRESERVED = 0x06;
  const uint8_t PUBLISH_RETAIN = 0x01;
  const uint8_t PUBACK_RESERVED = 0x00;
  const uint8_t PUBREC_RESERVED = 0x00;
  const uint8_t PUBREL_RESERVED = 0x02;
  const uint8_t PUBCOMP_RESERVED = 0x00;
  const uint8_t SUBSCRIBE_RESERVED = 0x02;
  const uint8_t SUBACK_RESERVED = 0x00;
  const uint8_t UNSUBSCRIBE_RESERVED = 0x02;
  const uint8_t UNSUBACK_RESERVED = 0x00;
  const uint8_t PINGREQ_RESERVED = 0x00;
  const uint8_t PINGRESP_RESERVED = 0x00;
  const uint8_t DISCONNECT_RESERVED = 0x00;
  const uint8_t RESERVED2_RESERVED = 0x00;
};

constexpr struct {
  const uint8_t USERNAME = 0x80;
  const uint8_t PASSWORD = 0x40;
  const uint8_t WILL_RETAIN = 0x20;
  const uint8_t WILL_QOS0 = 0x00;
  const uint8_t WILL_QOS1 = 0x08;
  const uint8_t WILL_QOS2 = 0x10;
  const uint8_t WILL = 0x04;
  const uint8_t CLEAN_SESSION = 0x02;
  const uint8_t RESERVED = 0x00;
} ConnectFlag;

struct MessageProperties {

  bool dup;
  uint8_t qos;
  bool retain;
};

enum MQTT_Queued_Message_State_t {
  MQTT_QUEUED_UNSENT,
  MQTT_QUEUED_AWAITING_ACK,
  MQTT_QUEUED_COMPLETE
};

} // namespace MQTTCore

#endif