#include "MQTTStateMachine.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <array>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <utility>

namespace MQTTCore {

StateMachine::StateMachine()
    : current_state(State::disconnected), retry_count(0) {
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed");
    return;
  }
  deserializeTransitions("/device_settings.json");
}

// Destructor for StateMachine class
StateMachine::~StateMachine() {
  // Cleanup if necessary
}

// Deserialize transitions from JSON file
void StateMachine::deserializeTransitions(const char *filename) {
  // Open the file
  File file = LittleFS.open(filename, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  // Estimate the size of the JSON document
  size_t fileSize = file.size();
  JsonDocument doc;

  // Parse the JSON
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.print("Failed to read file, using default configuration: ");
    Serial.println(error.c_str());
    return;
  }

  JsonArray transitions = doc["transitions"].as<JsonArray>();

  size_t tr_id = 0;
  for (JsonObject transition : transitions) {
    if (tr_id >= transition_table.size())
      break;

    State current_state =
        stringToState(transition["current_state"].as<const char *>());
    Event event = stringToEvent(transition["event"].as<const char *>());
    State next_state =
        stringToState(transition["next_state"].as<const char *>());
    Event next_event =
        stringToEvent(transition["next_event"].as<const char *>());

    // Placeholder actions and guards
    ActionFunction action = []() {};
    GuardFunction guard = []() { return true; };

    // Check if action or guard are provided
    if (!transition["action"].isNull()) {
      // Replace with actual action implementation
      action = []() { Serial.println("Action executed"); };
    }

    if (!transition["guard"].isNull()) {
      // Replace with actual guard implementation
      guard = []() -> bool {
        Serial.println("Guard checked");
        return true;
      };
    }

    StateMachine::transition_table[tr_id++] = {current_state, event, next_state,
                                               action, guard};
  }
}

void StateMachine::handleEvent(Event event) {
  std::cout << "Handling event: " << eventToString(event)
            << " from state: " << stateToString(current_state) << std::endl;

  if (event == Event::SYSTEM_FAULT) {
    setState(State::hibernate);
    std::cout << "Transitioned to state: " << stateToString(current_state)
              << std::endl;
    return;
  }

  if (event == Event::BROKER_DOWN) {
    setState(State::disconnected);
    std::cout << "Transitioned to state: " << stateToString(current_state)
              << std::endl;
    return;
  }

  // Special handling for the RETRY event to ensure retry logic is applied
  if (event == Event::RETRY) {
    std::cout << "Retry count before increment: " << retry_count.load()
              << std::endl;
    if (retry_count.load() >= max_retries) {
      std::cout << "Max retries reached! " << retry_count.load() + 1
                << std::endl;
      setState(State::timeout);
      std::cout << "Transitioned to state: " << stateToString(current_state)
                << std::endl;
      return;
    } else {
      retry_count++;
      std::cout << "Retry count after increment: " << retry_count.load()
                << std::endl;
      setState(State::reconnect);
    }
  }

  for (const auto &transition : transition_table) {
    if (transition.current_state == current_state &&
        transition.event == event) {
      if (transition.guard()) {
        if (event == Event::DISCONNECTED) {
          if (current_state.load() != State::reconnect) {
            retry_count = 0;
          }

          if (current_state.load() == State::mqtt_ok) {
            setState(State::reconnect);
          }
        }

        setState(transition.next_state);
        transition.action();
        std::cout << "Transitioned to state: " << stateToString(current_state)
                  << std::endl;
        return;
      }
    }
  }
}

StateMachine::State StateMachine::getCurrentState() const {
  return current_state.load();
}

void StateMachine::setState(State new_state) {
  current_state.store(new_state);
  saveState(new_state);
}

const char *StateMachine::stateToString(State state) {
  switch (state) {
  case State::disconnected:
    return "disconnected";
  case State::connectingTcp1:
    return "connectingTcp1";
  case State::connectingTcp2:
    return "connectingTcp2";
  case State::connectingMqtt:
    return "connectingMqtt";
  case State::connected:
    return "connected";
  case State::mqtt_ok:
    return "mqtt_ok";
  case State::disconnectingMqtt1:
    return "disconnectingMqtt1";
  case State::disconnectingMqtt2:
    return "disconnectingMqtt2";
  case State::disconnectingTcp1:
    return "disconnectingTcp1";
  case State::disconnectingTcp2:
    return "disconnectingTcp2";
  case State::reconnect:
    return "reconnect";
  case State::timeout:
    return "timeout";
  case State::hibernate:
    return "hibernate";
  default:
    return "unknown";
  }
}
StateMachine::State stringToState(const char *str) {
  if (strcmp(str, "disconnected") == 0)
    return StateMachine::State::disconnected;
  if (strcmp(str, "connectingTcp1") == 0)
    return StateMachine::State::connectingTcp1;
  if (strcmp(str, "connectingTcp2") == 0)
    return StateMachine::State::connectingTcp2;
  if (strcmp(str, "connectingMqtt") == 0)
    return StateMachine::State::connectingMqtt;
  if (strcmp(str, "connected") == 0)
    return StateMachine::State::connected;
  if (strcmp(str, "mqtt_ok") == 0)
    return StateMachine::State::mqtt_ok;
  if (strcmp(str, "disconnectingMqtt1") == 0)
    return StateMachine::State::disconnectingMqtt1;
  if (strcmp(str, "disconnectingMqtt2") == 0)
    return StateMachine::State::disconnectingMqtt2;
  if (strcmp(str, "disconnectingTcp1") == 0)
    return StateMachine::State::disconnectingTcp1;
  if (strcmp(str, "disconnectingTcp2") == 0)
    return StateMachine::State::disconnectingTcp2;
  if (strcmp(str, "reconnect") == 0)
    return StateMachine::State::reconnect;
  if (strcmp(str, "timeout") == 0)
    return StateMachine::State::timeout;
  if (strcmp(str, "hibernate") == 0)
    return StateMachine::State::hibernate;
  return StateMachine::State::disconnected; // Default to disconnected state
}
const char *StateMachine::eventToString(Event event) {
  switch (event) {
  case Event::NONE:
    return "NONE";
  case Event::ERROR:
    return "ERROR";
  case Event::CONNECTED:
    return "CONNECTED";
  case Event::DISCONNECTED:
    return "DISCONNECTED";
  case Event::SUBSCRIBED:
    return "SUBSCRIBED";
  case Event::UNSUBSCRIBED:
    return "UNSUBSCRIBED";
  case Event::PUBLISHED:
    return "PUBLISHED";
  case Event::DATA:
    return "DATA";
  case Event::BEFORE_CONNECT:
    return "BEFORE_CONNECT";
  case Event::DELETED:
    return "DELETED";
  case Event::RETRY:
    return "RETRY";
  case Event::RETRY_OK:
    return "RETRY OK";
  case Event::RETRY_TCP1_OK:
    return "RETRY TCP1 OK";
  case Event::RETRY_TCP2_OK:
    return "RETRY TCP2 OK";
  case Event::RETRY_MQTT_OK:
    return "RETRY MQTT OK";
  case Event::MAX_RETRIES:
    return "MAX_RETRIES";
  case Event::SYSTEM_FAULT:
    return "SYSTEM_FAULT";
  case Event::BROKER_DOWN:
    return "BROKER DOWN";
  case Event::BAD_PROTOCOL:
    return "BAD PROTOCOL";
  case Event::RESTART:
    return "RESTART";
  case Event::RESET:
    return "RESET";
  default:
    return "unknown";
  }
}
StateMachine::Event StateMachine::stringToEvent(const char *str) {
  if (strcmp(str, "NONE") == 0)
    return Event::NONE;
  if (strcmp(str, "ERROR") == 0)
    return Event::ERROR;
  if (strcmp(str, "CONNECTED") == 0)
    return Event::CONNECTED;
  if (strcmp(str, "DISCONNECTED") == 0)
    return Event::DISCONNECTED;
  if (strcmp(str, "SUBSCRIBED") == 0)
    return Event::SUBSCRIBED;
  if (strcmp(str, "UNSUBSCRIBED") == 0)
    return Event::UNSUBSCRIBED;
  if (strcmp(str, "PUBLISHED") == 0)
    return Event::PUBLISHED;
  if (strcmp(str, "DATA") == 0)
    return Event::DATA;
  if (strcmp(str, "BEFORE_CONNECT") == 0)
    return Event::BEFORE_CONNECT;
  if (strcmp(str, "DELETED") == 0)
    return Event::DELETED;
  if (strcmp(str, "RETRY") == 0)
    return Event::RETRY;
  if (strcmp(str, "RETRY_MQTT_OK") == 0)
    return Event::RETRY_MQTT_OK;
  if (strcmp(str, "RETRY_TCP1_OK") == 0)
    return Event::RETRY_TCP1_OK;
  if (strcmp(str, "RETRY_TCP2_OK") == 0)
    return Event::RETRY_TCP2_OK;
  if (strcmp(str, "RETRY_OK") == 0)
    return Event::RETRY_OK;
  if (strcmp(str, "MAX_RETRIES") == 0)
    return Event::MAX_RETRIES;
  if (strcmp(str, "SYSTEM_FAULT") == 0)
    return Event::SYSTEM_FAULT;
  if (strcmp(str, "BROKER_DOWN") == 0)
    return Event::BROKER_DOWN;
  if (strcmp(str, "BAD_PROTOCOL") == 0)
    return Event::BAD_PROTOCOL;
  if (strcmp(str, "RESTART") == 0)
    return Event::RESTART;
  if (strcmp(str, "RESET") == 0)
    return Event::RESET;
  return Event::NONE; // Default to NONE if not matched
}

} // namespace MQTTCore
