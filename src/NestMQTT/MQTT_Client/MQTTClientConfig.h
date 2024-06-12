#ifndef MQTT_CLIENT_CONFIG_H_
#define MQTT_CLIENT_CONFIG_H_
#include "MQTTCore.h"
namespace MQTTClientDetails {

struct lastWillSettings {
  const char *_username;
  const char *_password;
  const char *_lwt_topic;
  bool _willRetain;
  uint8_t _lwt_qos;
  const uint8_t *_lwt_msg;
  uint16_t _lwt_msg_len;
};
struct ConnectionSettings {
  const char *host;
  const char *uri;
  uint16_t _port;
  bool _useIp;
  IPAddress _ip;
  bool _cleanSession;
  bool disable_auto_reconnect;
  bool disable_keepalive;
  uint32_t reconnect_timeout_ms;
  uint32_t network_timeout_ms;
  uint32_t refresh_connection_after_ms;
  uint32_t message_retransmit_timeout;
  uint32_t _keepAlive;
};
struct SecureConnection_Settings {
  const char *cert_pem;
  size_t cert_len;
  const char *client_cert_pem;
  size_t client_cert_len;
  size_t client_key_len;

  const struct psk_key_hint *psk_hint_key;
  bool use_global_ca_store;
  esp_err_t (*crt_bundle_attach)(void *conf);

  const char **alpn_protos;
  const char *clientkey_password;
  int clientkey_password_len;
  bool skip_cert_common_name_check;
  bool use_secure_element;
};
struct MqttClientCfg {
  typedef void (*mqttClientHook)(void *);
  MQTTCore::MQTT_Protocol_Version_t _protocolVersion;
  MQTTCore::MQTT_Transport_t transport;
  bool set_null_client_id;
  lastWillSettings last_will_settings;
  ConnectionSettings connections_settings;
  SecureConnection_Settings secure_connection_settings;
  void *user_context;
  int task_prio;
  int task_stack;
  int buffer_size;
  void *ds_data;
  const char *path;
};

} // namespace MQTTClientDetails
#endif