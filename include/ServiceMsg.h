#pragma once

#include "BaseMsg.h"
#include <cstddef>
#include <cstdint>
#include <memory>

class ServiceMsg : public BaseMsg {
public:
  uint32_t source;
  std::shared_ptr<char> buff;
  size_t size;
};
