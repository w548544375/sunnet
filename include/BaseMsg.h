#pragma once

#include <cstdint>
class BaseMsg {

public:
  enum TYPE {
    SERVICE = 1,
    SOCKET_ACCEPT = 2,
    SOCKET_RW = 3,
  };
  char load[9999];
  uint8_t type;
  virtual ~BaseMsg(){};
};
