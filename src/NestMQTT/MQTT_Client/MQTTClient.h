#ifndef MQTT_CLIENT_H_
#define MQTT_CLIENT_H_
#include "MQTTCallbacks.h"
#include "MQTTClientConfig.h"
#include "MQTTCore.h"
#include "MQTTReceiver.h"
#include "MQTTStateMachine.h"
#include "MQTTTransmitter.h"
#include "MQTTTransport.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <Arduino.h>
#include <atomic>
#include <random>
#include <utility>
#include <vector>

using namespace MQTTCore;
using namespace MQTTClientDetails;

class MqttClient {
  friend class MQTTTransport::Transmitter;
  friend class MQTTTransport::Receiver;

public:
  virtual ~MqttClient();
  bool connected() const;
  bool disconnected() const;
  bool connect();
  bool disconnect(bool force = false);
  const char *getClientId() const;
  void mqttloop();
  template <typename... Args>
  uint16_t subscribe(const char *topic, uint8_t qos, Args &&...args);
  template <typename... Args>
  uint16_t unsubscribe(const char *topic, Args &&...args);
  uint16_t publish(const char *topic, uint8_t qos, bool retain,
                   const uint8_t *payload, size_t length);
  uint16_t publish(const char *topic, uint8_t qos, bool retain,
                   const char *payload);
  uint16_t publish(const char *topic, uint8_t qos, bool retain,
                   MQTTCore::onPayloadInternalCallback callback, size_t length);

protected:
  SemaphoreHandle_t _xSemaphore;
  TaskHandle_t _taskHandle;

private:
  bool initiateConnectionRequest();
  const char *client_id;
  StateMachine::State _clientState;
  StateMachine _statemachine;
  MQTTClientDetails::MqttClientCfg _clientcfg;
  MQTTTransport::Transport *_transport;
  MQTTTransport::Transmitter *_tx;
  MQTTTransport::Receiver *_rx;
  void updateClientState() { _clientState = _statemachine.getCurrentState(); }

  std::vector<OnConnectUserCallback> _onConnectUserCallbacks;
  std::vector<OnDisconnectUserCallback> _onDisconnectUserCallbacks;
  std::vector<OnSubscribeUserCallback> _onSubscribeUserCallbacks;
  std::vector<OnUnsubscribeUserCallback> _onUnsubscribeUserCallbacks;
  std::vector<OnMessageUserCallback_t> _onMessageUserCallbacks;
  std::vector<OnPublishUserCallback> _onPublishUserCallbacks;
  std::vector<OnErrorUserCallback> _onErrorUserCallbacks;

  std::vector<OnConnAckInternalCallback> _onConnectInternalCallbacks;
  std::vector<OnPingRespInternalCallback> _onPingRespInternalCallbacks;
  std::vector<OnSubAckInternalCallback> _onSubAckInternalCallbacks;
  std::vector<OnUnsubAckInternalCallback> _onUnsubAckInternalCallbacks;
  std::vector<OnMessageInternalCallback> _onMessageInternalCallbacks;
  std::vector<OnPublishInternalCallback> _onPublishInternalCallbacks;
  std::vector<OnPubRelInternalCallback> _onPubRelInternalCallbacks;
  std::vector<OnPubAckInternalCallback> _onPubAckInternalCallbacks;
  std::vector<OnPubRecInternalCallback> _onPubRecInternalCallbacks;
  std::vector<OnPubCompInternalCallback> _onPubCompInternalCallbacks;
  std::vector<onPayloadInternalCallback> _onPayloadInternalCallbacks;

  void _onConnack();
  void _onPublish();
  void _onPuback();
  void _onPubrec();
  void _onPubrel();
  void _onPubcomp();
  void _onSuback();
  void _onUnsuback();

  char *generateRandomClientId() {
    std::random_device rd;
    std::mt19937 gen(rd());

    const std::string charset =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::uniform_int_distribution<> dis(0, charset.size() - 1);

    int length = std::uniform_int_distribution<>(1, 23)(
        gen); // Random length between 1 and 23

    // Allocate memory for the clientId including the null terminator
    char *clientId = new char[length + 1];

    for (int i = 0; i < length; ++i) {
      clientId[i] = charset[dis(gen)];
    }

    // Add null terminator at the end
    clientId[length] = '\0';

    return clientId;
  }

public:
  StateMachine::State getClientState() const { return _clientState; }
};

#endif // MQTT_CLIENT_H_
