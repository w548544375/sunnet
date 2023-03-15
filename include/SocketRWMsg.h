#pragma once

#include "BaseMsg.h"
class SocketRWMsg : public BaseMsg {
public:
  int fd;
  bool isRead;
  bool isWrite;
};
