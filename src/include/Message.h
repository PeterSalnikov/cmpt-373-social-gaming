# pragma once

#include <string_view>
#include "Task.h"


enum class MessageType {
  INPUT,
  PLAYER
};

// Messages to be sent to either a specific player or everyone
class Message : public Task {
public:

  Message(MessageType type, const std::string_view value) : type(type), value(value) { } 
  void run();

private:

  MessageType type;

  /* 
    we can just keep the raw value with { placeholders } 
    because GameInstance will handle it
  */
  std::string_view value;
};