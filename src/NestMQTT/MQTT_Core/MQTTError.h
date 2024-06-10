#ifndef MQTT_ERROR_H_
#define MQTT_ERROR_H_

#include <cassert>
#include <limits>
#include <string>
#include <unordered_map>

namespace MQTTCore {

enum class MQTTErrors {
  UNKNOWN = std::numeric_limits<int>::min(),
  NULLPTR,
  CONTROL_FORBIDDEN_TYPE,
  CONTROL_INVALID_FLAGS,
  CONTROL_WRONG_TYPE,
  CONNECT_NULL_CLIENT_ID,
  CONNECT_NULL_WILL_MESSAGE,
  CONNECT_FORBIDDEN_WILL_QOS,
  CONNACK_FORBIDDEN_FLAGS,
  CONNACK_FORBIDDEN_CODE,
  PUBLISH_FORBIDDEN_QOS,
  SUBSCRIBE_TOO_MANY_TOPICS,
  SUBSCRIBE_UNEVEN_TOPIC_QOS,
  STRING_LENGTH_ERROR,
  MALFORMED_RESPONSE,
  MALFORMED_REMAINING_LENGTH,
  UNSUBSCRIBE_TOO_MANY_TOPICS,
  RESPONSE_INVALID_CONTROL_TYPE,
  CLIENT_NOT_CONNECTED,
  SEND_BUFFER_IS_FULL,
  SOCKET_ERROR,
  MALFORMED_REQUEST,
  RECV_BUFFER_TOO_SMALL,
  ACK_OF_UNKNOWN,
  NOT_IMPLEMENTED,
  CONNECTION_REFUSED,
  SUBSCRIBE_FAILED,
  CONNECTION_CLOSED,
  OK = 1
};

class MQTTError {
public:
  MQTTError(MQTTErrors error) : errorCode(error) {
    auto it = error_strings.find(error);
    if (it != error_strings.end()) {
      errorMessage = it->second.c_str();
    } else {
      // If the error code is not found, use the string representation of
      // UNKNOWN error
      errorMessage = error_strings.at(MQTTErrors::UNKNOWN).c_str();
    }
  }

  MQTTErrors getErrorCode() const { return errorCode; }
  const char *what() const noexcept { return errorMessage; }

private:
  MQTTErrors errorCode;
  const char *errorMessage;
  const std::unordered_map<MQTTErrors, std::string> error_strings = {
      {MQTTErrors::UNKNOWN, "Unknown error"},
      {MQTTErrors::NULLPTR, "NULL pointer error"},
      {MQTTErrors::CONTROL_FORBIDDEN_TYPE, "Forbidden control type"},
      {MQTTErrors::CONTROL_INVALID_FLAGS, "Invalid control flags"},
      {MQTTErrors::CONTROL_WRONG_TYPE, "Wrong control type"},
      {MQTTErrors::CONNECT_NULL_CLIENT_ID, "NULL client ID in connect"},
      {MQTTErrors::CONNECT_NULL_WILL_MESSAGE, "NULL will message in connect"},
      {MQTTErrors::CONNECT_FORBIDDEN_WILL_QOS,
       "Forbidden QoS for will message in connect"},
      {MQTTErrors::CONNACK_FORBIDDEN_FLAGS,
       "Forbidden flags in CONNACK packet"},
      {MQTTErrors::CONNACK_FORBIDDEN_CODE, "Forbidden code in CONNACK packet"},
      {MQTTErrors::PUBLISH_FORBIDDEN_QOS, "Forbidden QoS in publish packet"},
      {MQTTErrors::SUBSCRIBE_TOO_MANY_TOPICS,
       "Too many topics in subscribe packet"},
      {MQTTErrors::SUBSCRIBE_UNEVEN_TOPIC_QOS,
       "Topic and Qos must come in pairs"},
      {MQTTErrors::STRING_LENGTH_ERROR, "String length error"},
      {MQTTErrors::MALFORMED_RESPONSE, "Malformed response"},
      {MQTTErrors::MALFORMED_REMAINING_LENGTH, "Malformed Remaining Length"},
      {MQTTErrors::UNSUBSCRIBE_TOO_MANY_TOPICS,
       "Too many topics in unsubscribe packet"},
      {MQTTErrors::RESPONSE_INVALID_CONTROL_TYPE,
       "Invalid control type in response"},
      {MQTTErrors::CLIENT_NOT_CONNECTED, "Client not connected"},
      {MQTTErrors::SEND_BUFFER_IS_FULL, "Send buffer is full"},
      {MQTTErrors::SOCKET_ERROR, "Socket error"},
      {MQTTErrors::MALFORMED_REQUEST, "Malformed request"},
      {MQTTErrors::RECV_BUFFER_TOO_SMALL, "Receive buffer too small"},
      {MQTTErrors::ACK_OF_UNKNOWN, "ACK of unknown packet"},
      {MQTTErrors::NOT_IMPLEMENTED, "Feature not implemented"},
      {MQTTErrors::CONNECTION_REFUSED, "Connection refused"},
      {MQTTErrors::SUBSCRIBE_FAILED, "Subscribe failed"},
      {MQTTErrors::CONNECTION_CLOSED, "Connection closed"},
      {MQTTErrors::OK, "OK"}};
  // static const std::unordered_map<MQTTErrors, std::string> error_strings;
};

} // namespace MQTTCore

#endif /* MQTT_ERROR_H_ */
