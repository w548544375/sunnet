#pragma once

#include "BaseMsg.h"

class SocketAcceptMsg : public BaseMsg {
public:
  int listenFd;
  int clientFd;
};
