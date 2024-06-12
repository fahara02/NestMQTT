#ifndef NEST_MQTT_LOG_H_
#define NEST_MQTT_LOG_H_

#include "MQTTError.h"
#include <iostream>
#include <sstream>
#include <string>


namespace MQTTCore {

enum class LogLevel { INFO, WARNING, ERROR };

static void mqtt_log(LogLevel level, const std::string &message) {
  std::ostringstream oss;

  oss << "[NestMQTT LOG] ";
  switch (level) {
  case LogLevel::INFO:
    oss << "\033[32m[I] "; // Green color
    break;
  case LogLevel::WARNING:
    oss << "\033[33m[W] "; // Yellow color
    break;
  case LogLevel::ERROR:
    oss << "\033[31m[E] "; // Red color
    break;
  }

  oss << " (" << __FILE__ << ":" << __LINE__ << ")"
      << "\033[0m"; // Reset color
  std::cout << oss.str() << std::endl;
}

static void mqtt_log(MQTTErrors error) {
  // std::ostringstream oss;

  // oss << "[NestMQTT LOG] \033[31m[E] "; // Red color for errors
  // auto it = error_strings.find(error);
  // if (it != error_strings.end()) {
  //     oss << it->second;
  // } else {
  //     oss << "Unknown error";
  // }

  // oss << " (" << __FILE__ << ":" << __LINE__ << ")" << "\033[0m"; // Reset
  // color std::cout << oss.str() << std::endl;
}

} // namespace MQTTCore

#endif