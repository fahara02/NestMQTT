#include "MQTTStateMachine.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
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

StateMachine::~StateMachine() {
  // Cleanup if necessary
}

void StateMachine::deserializeTransitions(const char *filename) {
  File file = LittleFS.open(filename, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  size_t fileSize = file.size();
  DynamicJsonDocument doc(fileSize);

  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.print("Failed to read file, using default configuration: ");
    Serial.println(error.c_str());
    return;
  }

  JsonArray transitions = doc["transitions"].as<JsonArray>();

  transition_table.clear();
  for (JsonObject transition : transitions) {
    State current_state =
        stringToState(transition["current_state"].as<const char *>());
    Event event = stringToEvent(transition["event"].as<const char *>());
    State next_state =
        stringToState(transition["next_state"].as<const char *>());

    ActionFunction action = []() {};
    GuardFunction guard = []() { return true; };

    if (!transition["action"].isNull()) {
      action = []() { Serial.println("Action executed"); };
    }

    if (!transition["guard"].isNull()) {
      guard = []() -> bool {
        Serial.println("Guard checked");
        return true;
      };
    }

    transition_table.push_back(
        {current_state, event, next_state, action, guard});
  }
}

void StateMachine::handleEvent(Event event) {
  Serial.printf("Handling event: %s from state: %s\n", eventToString(event),
                stateToString(current_state));

  if (event == Event::SYSTEM_FAULT) {
    handleSystemFaultEvent();
    return;
  }

  if (event == Event::BROKER_DOWN) {
    setState(State::disconnected);
    return;
  }

  if (event == Event::RETRY) {
    handleRetryEvent();
    return;
  }

  for (const auto &transition : transition_table) {
    if (transition.current_state == current_state &&
        transition.event == event) {
      if (transition.guard()) {
        if (event == Event::DISCONNECTED && current_state != State::reconnect) {
          retry_count = 0;
        }

        setState(transition.next_state);
        transition.action();
        return;
      }
    }
  }
}

void StateMachine::handleRetryEvent() {
  Serial.printf("Retry count before increment: %d\n", retry_count.load());
  if (retry_count.load() >= max_retries) {
    Serial.printf("Max retries reached! %d\n", retry_count.load() + 1);
    setState(State::timeout);
  } else {
    retry_count++;
    Serial.printf("Retry count after increment: %d\n", retry_count.load());
    setState(State::reconnect);
  }
}

void StateMachine::handleSystemFaultEvent() { setState(State::hibernate); }

StateMachine::State StateMachine::getCurrentState() const {
  return current_state.load();
}

void StateMachine::setState(State new_state) {
  State old_state = current_state.load();
  current_state.store(new_state);
  logStateTransition(old_state, new_state, Event::NONE);
  saveState(new_state);
}

void StateMachine::logStateTransition(State from, State to, Event event) {
  Serial.printf("Transitioned from state: %s to state: %s on event: %s\n",
                stateToString(from), stateToString(to), eventToString(event));
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

StateMachine::State StateMachine::stringToState(const char *str) {
  if (strcmp(str, "disconnected") == 0)
    return State::disconnected;
  if (strcmp(str, "connectingTcp1") == 0)
    return State::connectingTcp1;
  if (strcmp(str, "connectingTcp2") == 0)
    return State::connectingTcp2;
  if (strcmp(str, "connectingMqtt") == 0)
    return State::connectingMqtt;
  if (strcmp(str, "connected") == 0)
    return State::connected;
  if (strcmp(str, "mqtt_ok") == 0)
    return State::mqtt_ok;
  if (strcmp(str, "disconnectingMqtt1") == 0)
    return State::disconnectingMqtt1;
  if (strcmp(str, "disconnectingMqtt2") == 0)
    return State::disconnectingMqtt2;
  if (strcmp(str, "disconnectingTcp1") == 0)
    return State::disconnectingTcp1;
  if (strcmp(str, "disconnectingTcp2") == 0)
    return State::disconnectingTcp2;
  if (strcmp(str, "reconnect") == 0)
    return State::reconnect;
  if (strcmp(str, "timeout") == 0)
    return State::timeout;
  if (strcmp(str, "hibernate") == 0)
    return State::hibernate;
  return State::disconnected;
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
  case Event::RETRY_MQTT_OK:
    return "RETRY_MQTT_OK";
  case Event::RETRY_TCP1_OK:
    return "RETRY_TCP1_OK";
  case Event::RETRY_TCP2_OK:
    return "RETRY_TCP2_OK";
  case Event::RETRY_OK:
    return "RETRY_OK";
  case Event::MAX_RETRIES:
    return "MAX_RETRIES";
  case Event::SYSTEM_FAULT:
    return "SYSTEM_FAULT";
  case Event::BROKER_DOWN:
    return "BROKER_DOWN";
  case Event::BAD_PROTOCOL:
    return "BAD_PROTOCOL";
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
  return Event::NONE;
}

void StateMachine::serializeTransitions(const char *filename) {
  File file = LittleFS.open(filename, "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  DynamicJsonDocument doc(1024);

  JsonArray transitions = doc.createNestedArray("transitions");

  for (const auto &transition : transition_table) {
    JsonObject transitionObj = transitions.createNestedObject();
    transitionObj["current_state"] = stateToString(transition.current_state);
    transitionObj["event"] = eventToString(transition.event);
    transitionObj["next_state"] = stateToString(transition.next_state);
  }

  if (serializeJson(doc, file) == 0) {
    Serial.println("Failed to write to file");
  }

  file.close();
}

void StateMachine::saveState(State state) {
  File file = LittleFS.open("/current_state.json", "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  DynamicJsonDocument doc(1024);
  doc["current_state"] = stateToString(state);

  if (serializeJson(doc, file) == 0) {
    Serial.println("Failed to write to file");
  }

  file.close();
}

StateMachine::State StateMachine::loadState() {
  File file = LittleFS.open("/current_state.json", "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return State::disconnected;
  }

  size_t fileSize = file.size();
  DynamicJsonDocument doc(fileSize);

  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.print("Failed to read file, using default configuration: ");
    Serial.println(error.c_str());
    return State::disconnected;
  }

  const char *stateStr = doc["current_state"].as<const char *>();
  return stringToState(stateStr);
}

} // namespace MQTTCore