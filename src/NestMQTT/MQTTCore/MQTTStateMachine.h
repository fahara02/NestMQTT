#ifndef MQTT_STATE_MACHINE_H_
#define MQTT_STATE_MACHINE_H_

#include <array>
#include <atomic>
#include <cstddef>
#include <functional>
#include <iostream>
#include <utility>
#include <vector>


namespace MQTTCore {

class StateMachine {

public:
  enum class State {
    disconnected = 0,
    connectingTcp1 = 1,
    connectingTcp2 = 2,
    connectingMqtt = 3,
    connected = 4,
    mqtt_ok = 5,
    disconnectingMqtt1 = 6,
    disconnectingMqtt2 = 7,
    disconnectingTcp1 = 8,
    disconnectingTcp2 = 9,
    reconnect = 10,
    timeout = 11,
    hibernate = 12,
  };

  enum class Event {
    NONE = -1,
    ERROR = 0,
    CONNECTED,
    DISCONNECTED,
    SUBSCRIBED,
    UNSUBSCRIBED,
    PUBLISHED,
    DATA,
    BEFORE_CONNECT,
    DELETED,
    RETRY,
    RETRY_MQTT_OK,
    RETRY_TCP1_OK,
    RETRY_TCP2_OK,
    RETRY_OK,
    MAX_RETRIES,
    SYSTEM_FAULT,
    BROKER_DOWN,
    BAD_PROTOCOL,
    RESTART,
    RESET
  };

  using GuardFunction = std::function<bool()>;
  using ActionFunction = std::function<void()>;

  struct Transition {
    State current_state;
    Event event;
    State next_state;
    ActionFunction action;
    GuardFunction guard;
  };

  StateMachine();

  ~StateMachine();

  void handleEvent(Event event);

  State getCurrentState() const;

  void serializeTransitions(const char *filename);
  void deserializeTransitions(const char *filename);

  void saveState(State state);
  State loadState();

  void setState(State new_state);

#ifdef UNIT_TEST
  void setMockAction(ActionFunction mock_action) {
    this->mock_action = mock_action;
  }

  void setMockGuard(GuardFunction mock_guard) { this->mock_guard = mock_guard; }
#endif

private:
  std::atomic<State> current_state;
  std::atomic<int> retry_count;
  const int max_retries = 3;
  std::vector<Transition> transition_table;

  void handleRetryEvent();
  void handleSystemFaultEvent();
  void logStateTransition(State from, State to, Event event);

  static const char *stateToString(State state);
  static const char *eventToString(Event event);

protected:
  static State stringToState(const char *str);
  static Event stringToEvent(const char *str);
};

} // namespace MQTTCore

#endif