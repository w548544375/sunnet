#pragma once

#include <cstdint>
class BaseMsg {

public:
  enum TYPE {
    SERVICE = 1,
  };
  char load[9999];
  uint8_t type;
  virtual ~BaseMsg(){};
};
