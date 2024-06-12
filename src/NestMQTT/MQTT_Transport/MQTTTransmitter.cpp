#include "MQTTTransmitter.h"
#include "MQTTAsyncTask.h"
#include "MQTTPacket.h"
#include "MqttClient.h" // Include the full definition here

namespace MQTTTransport {

template <typename... Args>
Transmitter::Transmitter(MqttClient *client, Args &&...args)
    : _client(client), _transport(client->_transport), _transmitTime(0),
      _transmitStatus{} {
  // Initial status update
  _transmitStatus.update(
      TransmitStatusUpdate::withLastClientActivity(millis()));
}

bool Transmitter::sendConnectionRequest(
    const MQTTClientDetails::MqttClientCfg &clientCfg) {
  bool result = false;
  if (_client->getClientState() == StateMachine::State::disconnected) {
    MQTT_SEMAPHORE_TAKE();
    if (_addPacketFront(clientCfg.connections_settings._cleanSession,
                        clientCfg.last_will_settings._username,
                        clientCfg.last_will_settings._password,
                        clientCfg.last_will_settings._lwt_topic,
                        clientCfg.last_will_settings._willRetain,
                        clientCfg.last_will_settings._lwt_qos,
                        clientCfg.last_will_settings._lwt_msg,
                        clientCfg.last_will_settings._lwt_msg_len,
                        (uint16_t)(clientCfg.connections_settings._keepAlive /
                                   1000), // 32b to 16b doesn't overflow because
                                          // it comes from 16b originally
                        clientCfg.set_null_client_id ? nullptr
                                                     : clientCfg.path)) {
      result = true;
      _client->_statemachine.handleEvent(StateMachine::Event::CONNECTED);

    } else {
      MQTT_SEMAPHORE_GIVE();
      // mqtt_log_e("Could not create CONNECT packet");
      // _onError(0, Error::OUT_OF_MEMORY);
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

// Other methods...

} // namespace MQTTTransport
