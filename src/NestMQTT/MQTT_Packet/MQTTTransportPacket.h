#ifndef MQTT_TRANSPORT_PACKETS_H_
#define MQTT_TRANSPORT_PACKETS_H_

#include <cstring>
#include <iostream>
#include <stddef.h>
#include <utility>

#include "MQTTError.h"

using namespace MQTTCore;

constexpr size_t MAX_TOPIC_LENGTH = 32; // Maximum length of topic name
constexpr size_t MAX_TOPICS = 10;       // Maximum number of topics

namespace MQTTPacket {

struct SubscribeItem {
  char topic[MAX_TOPIC_LENGTH];
  uint8_t qos;
};

// Struct for SUBSCRIBE packet parameters
struct Subscription {
  SubscribeItem list[MAX_TOPICS]; // Fixed-size array
  size_t numberTopics;

  Subscription(size_t topics, const SubscribeItem *itemList)
      : numberTopics(topics) {
    if (topics > MAX_TOPICS) {
      // throw std::runtime_error("Too many topics specified");
    }
    for (size_t i = 0; i < topics; ++i) {
      strncpy(list[i].topic, itemList[i].topic, MAX_TOPIC_LENGTH - 1);
      list[i].topic[MAX_TOPIC_LENGTH - 1] = '\0'; // Ensure null-terminated
      list[i].qos = itemList[i].qos;
    }
  }

  // Constructor overload for a single topic and qos
  Subscription(const char *topic, uint8_t qos) : numberTopics(1) {
    list[0].qos = qos;
    strncpy(list[0].topic, topic, MAX_TOPIC_LENGTH - 1);
    list[0].topic[MAX_TOPIC_LENGTH - 1] = '\0'; // Ensure null-terminated
  }

  // Constructor overload for two topics and qoses
  Subscription(const char *topic1, uint8_t qos1, const char *topic2,
               uint8_t qos2)
      : numberTopics(2) {
    list[0].qos = qos1;
    strncpy(list[0].topic, topic1, MAX_TOPIC_LENGTH - 1);
    list[0].topic[MAX_TOPIC_LENGTH - 1] = '\0'; // Ensure null-terminated
    list[1].qos = qos2;
    strncpy(list[1].topic, topic2, MAX_TOPIC_LENGTH - 1);
    list[1].topic[MAX_TOPIC_LENGTH - 1] = '\0'; // Ensure null-terminated
  }

  // Constructor overload for more than two topics and qoses
  template <typename... Args>
  Subscription(const char *topic1, const char *topic2, Args &&...args)
      : numberTopics((sizeof...(Args) / 2) + 2) {
    static_assert(sizeof...(Args) % 2 == 0,
                  "Each topic must have a corresponding QoS");
    static_assert((sizeof...(Args) + 2) / 2 <= MAX_TOPICS,
                  "Too many topics specified");

    list[0].qos = 0; // Default initialization
    strncpy(list[0].topic, topic1, MAX_TOPIC_LENGTH - 1);
    list[0].topic[MAX_TOPIC_LENGTH - 1] = '\0'; // Ensure null-terminated
    list[1].qos = 0;                            // Default initialization
    strncpy(list[1].topic, topic2, MAX_TOPIC_LENGTH - 1);
    list[1].topic[MAX_TOPIC_LENGTH - 1] = '\0'; // Ensure null-terminated

    size_t i = 2;
    fillItems(i, std::forward<Args>(args)...);
  }

private:
  // Helper function to fill SubscribeItem array
  template <typename... Args>
  void fillItems(size_t &index, const char *topic, uint8_t qos,
                 Args &&...args) {
    strncpy(list[index].topic, topic,
            MAX_TOPIC_LENGTH - 1); // Ensure null-terminated
    list[index].topic[MAX_TOPIC_LENGTH - 1] =
        '\0'; // Null-terminate to ensure safety
    list[index].qos = qos;
    ++index;
    fillItems(index, std::forward<Args>(args)...);
  }

  // Base case for fillItems
  void fillItems(size_t &) {}
};

} // namespace MQTTPacket

#endif // TRANSPORT_PACKETS_H_
