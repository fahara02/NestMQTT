#ifndef MQTT_SUBSCRIPTION_H_
#define MQTT_SUBSCRIPTION_H_

#include <cstring>
#include <iostream>
#include <stddef.h>
#include <utility>

#include "MQTTError.h"

using namespace MQTTCore;

constexpr size_t MAX_TOPIC_LENGTH = 32;  // Maximum length of topic name
constexpr size_t MAX_TOPICS = 10;        // Maximum number of topics

namespace MQTTPacket {

enum Subscription_task { UNSUBSCRIBE = 0, SUBSCRIBE = 1 };

struct SubscribeItem {
  char topic[MAX_TOPIC_LENGTH];
  uint8_t qos;

  // Constructor
  SubscribeItem(const char* topicName = "", uint8_t qualityOfService = 0)
      : qos(qualityOfService) {
    strncpy(topic, topicName, MAX_TOPIC_LENGTH - 1);
    topic[MAX_TOPIC_LENGTH - 1] = '\0';  // Ensure null-terminated
  }

  bool operator==(const char* otherTopic) const {
    return strncmp(topic, otherTopic, MAX_TOPIC_LENGTH) == 0;
  }
};

// Struct for SUBSCRIBE and UNSUBSCRIBE packet parameters
struct Subscription {
  SubscribeItem list[MAX_TOPICS];  // Fixed-size array
  size_t numberTopics;

  // Default constructor
  Subscription() : numberTopics(0) {}

  // Constructor for multiple topics using a list
  Subscription(size_t topics, const SubscribeItem* itemList)
      : numberTopics(topics) {
    if (topics > MAX_TOPICS) {
      throw std::runtime_error("Too many topics specified");
    }
    for (size_t i = 0; i < topics; ++i) {
      list[i] = itemList[i];
    }
  }

  // Constructor for a single topic and qos
  Subscription(const char* topic, uint8_t qos = 0) : numberTopics(1) {
    list[0] = SubscribeItem(topic, qos);
  }

  // Constructor for exactly two topics, no default values
  Subscription(const char* topic1, uint8_t qos1, const char* topic2,
               uint8_t qos2)
      : numberTopics(2) {
    list[0] = SubscribeItem(topic1, qos1);
    list[1] = SubscribeItem(topic2, qos2);
  }

  // Constructor for more than two topics
  template <typename... Args>
  Subscription(const char* topic1, uint8_t qos1, const char* topic2,
               uint8_t qos2, Args&&... args)
      : numberTopics((sizeof...(Args) / 2) + 2) {
    static_assert(sizeof...(Args) % 2 == 0,
                  "Each topic must have a corresponding QoS");
    static_assert((sizeof...(Args) + 2) / 2 <= MAX_TOPICS,
                  "Too many topics specified");

    list[0] = SubscribeItem(topic1, qos1);
    list[1] = SubscribeItem(topic2, qos2);

    size_t i = 2;
    fillItems(i, std::forward<Args>(args)...);
  }

  // Constructor for more than two topics for unsubscribe case
  template <typename... Args>
  Subscription(const char* topic1, const char* topic2, Args&&... args)
      : numberTopics((sizeof...(Args)) + 2) {
    static_assert(sizeof...(Args) <= MAX_TOPICS - 2,
                  "Too many topics specified");

    list[0] = SubscribeItem(topic1, 0);
    list[1] = SubscribeItem(topic2, 0);

    size_t i = 2;
    fillItems(i, std::forward<Args>(args)...);
  }

  // Copy constructor
  Subscription(const Subscription& other) : numberTopics(other.numberTopics) {
    for (size_t i = 0; i < other.numberTopics; ++i) {
      list[i] = other.list[i];
    }
  }

  // Assignment operator
  Subscription& operator=(const Subscription& other) {
    if (this != &other) {
      numberTopics = other.numberTopics;
      for (size_t i = 0; i < other.numberTopics; ++i) {
        list[i] = other.list[i];
      }
    }
    return *this;
  }

private:
  // Helper function to fill SubscribeItem array
  template <typename... Args>
  void fillItems(size_t& index, const char* topic, uint8_t qos,
                 Args&&... args) {
    if (index < MAX_TOPICS) {  // Ensure we do not exceed bounds
      strncpy(list[index].topic, topic,
              MAX_TOPIC_LENGTH - 1);  // Ensure null-terminated
      list[index].topic[MAX_TOPIC_LENGTH - 1]
          = '\0';  // Null-terminate to ensure safety
      list[index].qos = qos;
      ++index;
      fillItems(index, std::forward<Args>(args)...);
    }
  }

  template <typename... Args>
  void fillItems(size_t& index, const char* topic, Args&&... args) {
    strncpy(list[index].topic, topic,
            MAX_TOPIC_LENGTH - 1);  // Ensure null-terminated
    list[index].topic[MAX_TOPIC_LENGTH - 1]
        = '\0';  // Null-terminate to ensure safety
    ++index;
    fillItems(index, std::forward<Args>(args)...);
  }

  // Base case for fillItems
  void fillItems(size_t&) {}
};
}  // namespace MQTTPacket

#endif