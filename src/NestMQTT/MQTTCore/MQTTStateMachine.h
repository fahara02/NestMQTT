#ifndef MQTT_STATE_MACHINE_H_
#define MQTT_STATE_MACHINE_H_

#include <array>
#include <atomic>
#include <cstddef>
#include <functional>
#include <utility>

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

  template <typename... Args>
  static std::array<Transition, sizeof...(Args)> TransitionTable() {
    return {Args::get_transition()...};
  }

  template <State Start, Event EventTrigger, State Next, Event ActionEvent>
  struct Row {
    static Transition get_transition() {
      return {Start, EventTrigger, Next, []() { /* Action */ },
              []() { return true; }};
    }
  };

  StateMachine();

  ~StateMachine();

  void handleEvent(Event event);

  State getCurrentState() const;
  void serializeTransitions(const char *filename);
  void deserializeTransitions(const char *filename);

  void saveState(State state) {
    // Save to persistent storage
  }

  State loadState() {
    // Load from persistent storage
    return State::disconnected;
  }

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
  std::array<Transition, 40> transition_table;
  // const std::array<Transition, 40> transition_table = TransitionTable<
  //     // hibernate  transitions
  //     Row<State::hibernate, Event::RESTART, State::disconnected,
  //     Event::NONE>, Row<State::disconnected, Event::SYSTEM_FAULT,
  //     State::hibernate,
  //         Event::NONE>,
  //     Row<State::timeout, Event::SYSTEM_FAULT, State::hibernate,
  //     Event::NONE>,

  //     // Initial transitions
  //     Row<State::disconnected, Event::ERROR, State::disconnected,
  //     Event::NONE>, Row<State::timeout, Event::ERROR, State::disconnected,
  //     Event::NONE>, Row<State::timeout, Event::RESET, State::disconnected,
  //     Event::NONE>, Row<State::disconnected, Event::BEFORE_CONNECT,
  //     State::connectingTcp1,
  //         Event::NONE>,
  //     // Connection phase transitions
  //     Row<State::connectingTcp1, Event::CONNECTED, State::connectingTcp2,
  //         Event::NONE>,
  //     Row<State::connectingTcp2, Event::CONNECTED, State::connectingMqtt,
  //         Event::NONE>,
  //     Row<State::connectingMqtt, Event::CONNECTED, State::connected,
  //         Event::NONE>,
  //     // MQTT SUCCESS
  //     Row<State::connected, Event::SUBSCRIBED, State::mqtt_ok, Event::NONE>,
  //     Row<State::connected, Event::UNSUBSCRIBED, State::mqtt_ok,
  //     Event::NONE>, Row<State::connected, Event::PUBLISHED, State::mqtt_ok,
  //     Event::NONE>, Row<State::connected, Event::DATA, State::mqtt_ok,
  //     Event::NONE>, Row<State::mqtt_ok, Event::SUBSCRIBED, State::mqtt_ok,
  //     Event::NONE>, Row<State::mqtt_ok, Event::UNSUBSCRIBED, State::mqtt_ok,
  //     Event::NONE>, Row<State::mqtt_ok, Event::PUBLISHED, State::mqtt_ok,
  //     Event::NONE>, Row<State::mqtt_ok, Event::DATA, State::mqtt_ok,
  //     Event::NONE>, Row<State::mqtt_ok, Event::DELETED, State::mqtt_ok,
  //     Event::NONE>,
  //     // BAD PROTOCOL
  //     Row<State::mqtt_ok, Event::BAD_PROTOCOL, State::connected,
  //     Event::NONE>, Row<State::mqtt_ok, Event::ERROR, State::connected,
  //     Event::NONE>,
  //     // Disconnection phase transitions
  //     Row<State::connected, Event::DISCONNECTED, State::disconnectingMqtt1,
  //         Event::NONE>,
  //     Row<State::disconnectingMqtt1, Event::DISCONNECTED,
  //         State::disconnectingMqtt2, Event::NONE>,
  //     Row<State::disconnectingMqtt2, Event::DISCONNECTED,
  //         State::disconnectingTcp1, Event::NONE>,
  //     Row<State::disconnectingTcp1, Event::DISCONNECTED,
  //         State::disconnectingTcp2, Event::NONE>,
  //     Row<State::disconnectingTcp2, Event::DISCONNECTED, State::disconnected,
  //         Event::NONE>,
  //     // Reconnect phase entry
  //     Row<State::mqtt_ok, Event::DISCONNECTED, State::reconnect,
  //     Event::NONE>, Row<State::disconnected, Event::RETRY, State::reconnect,
  //     Event::NONE>, Row<State::disconnectingTcp1, Event::RETRY,
  //     State::reconnect,
  //         Event::NONE>,
  //     Row<State::disconnectingTcp2, Event::RETRY, State::reconnect,
  //         Event::NONE>,
  //     Row<State::disconnectingMqtt1, Event::RETRY, State::reconnect,
  //         Event::NONE>,
  //     Row<State::disconnectingMqtt2, Event::RETRY, State::reconnect,
  //         Event::NONE>,
  //     Row<State::timeout, Event::RETRY, State::timeout, Event::NONE>,
  //     // Retry Success

  //     Row<State::reconnect, Event::RETRY_OK, State::connectingTcp1,
  //         Event::NONE>,
  //     Row<State::reconnect, Event::RETRY_TCP1_OK, State::connectingTcp2,
  //         Event::NONE>,
  //     Row<State::reconnect, Event::RETRY_TCP2_OK, State::connectingMqtt,
  //         Event::NONE>,
  //     Row<State::reconnect, Event::RETRY_MQTT_OK, State::connected,
  //         Event::NONE>,
  //     // Retry UNSUCCESS
  //     Row<State::reconnect, Event::RETRY, State::reconnect, Event::NONE>,
  //     Row<State::reconnect, Event::MAX_RETRIES, State::timeout, Event::NONE>,

  //     // default state Disconnected
  //     Row<State::disconnected, Event::DISCONNECTED, State::disconnected,
  //         Event::NONE>>();

  static const char *stateToString(State state);
  static const char *eventToString(Event event);

protected:
  static State stringToState(const char *str);
  static Event stringToEvent(const char *str);
};

} // namespace MQTTCore

#endif
