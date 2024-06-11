#ifndef MQTT_CALLBACKS_H_
#define MQTT_CALLBACKS_H_


#include <functional>
#include <string>
#include "MQTTCore.h"
#include "MQTTError.h"
#include "MQTTCodes.h"



namespace MQTTCore{

using OnConnectUserCallback = std::function<void(bool sessionPresent)>;
using OnDisconnectUserCallback = std::function<void(bool sessionPresent, DisconnectReason  reason)>;
using OnSubscribeUserCallback = std::function<void(uint16_t packetId, uint8_t qos)>;
using OnUnsubscribeUserCallback = std::function<void(uint16_t packetId, uint8_t qos)>;
using OnMessageUserCallback = std::function<void(const std::string& topic, const std::string& payload,  MessageProperties properties, size_t length, size_t index, size_t total)>;
using OnPublishUserCallback = std::function<void(uint16_t packetId)>;
using OnErrorUserCallback = std::function<void(uint16_t packetId, MQTTErrors error)>;



// internal callbacks
using OnObjectCreationCallback = std::function<void()>;
using OnConnAckInternalCallback = std::function<void(bool sessionPresent, ConnackReturnCode connectReturnCode)>;
using OnPingRespInternalCallback = std::function<void()>;
using OnSubAckInternalCallback = std::function<void(uint16_t packetId, const std::string status)>;
using OnUnsubAckInternalCallback = std::function<void(uint16_t packetId)>;
using OnMessageInternalCallback = std::function<void(const std::string& topic, const std::string& payload, uint8_t qos, bool dup, bool retain, size_t length, size_t index, size_t total, uint16_t packetId)>;
using OnPublishInternalCallback = std::function<void(uint16_t packetId, uint8_t qos)>;
using OnPubRelInternalCallback = std::function<void(uint16_t packetId)>;
using OnPubAckInternalCallback = std::function<void(uint16_t packetId)>;
using OnPubRecInternalCallback = std::function<void(uint16_t packetId)>;
using OnPubCompInternalCallback = std::function<void(uint16_t packetId)>;
using onPayloadInternalCallback =std::function<size_t(uint8_t* data, size_t maxSize, size_t index)> ;



typedef struct
{
    std::string topic;
    uint8_t qos;
    OnMessageUserCallback callback;
} OnMessageUserCallback_t;


}

#endif