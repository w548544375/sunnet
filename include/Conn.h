#pragma once

#include <cstdint>
class Conn {
public:
  enum TYPE {
    SERVER = 1,
    CLIENT = 2,
  };

  uint8_t type;
  int fd;
  uint32_t serviceId;
};
