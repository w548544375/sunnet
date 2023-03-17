#pragma once
#include "BaseMsg.h"
#include "ServiceMsg.h"
#include "SocketAcceptMsg.h"
#include "SocketRWMsg.h"
#include "lua.hpp"
#include <cstdint>
#include <memory>
#include <pthread.h>
#include <queue>
#include <string>
#include <thread>
class Service {
public:
  std::uint32_t id;
  std::shared_ptr<std::string> type;
  bool isExist = false;
  std::queue<std::shared_ptr<BaseMsg>> msgQueue;
  pthread_spinlock_t queueLock;

public:
  Service();
  ~Service();

  void OnInit();
  void OnMsg(std::shared_ptr<BaseMsg> msg);
  void OnExit();

  void PushMsg(std::shared_ptr<BaseMsg> msg);
  void ProcessMsgs(int max);
  bool ProcessMsg();

  void SetIsInGlobal(bool in);
  bool IsInGlobal() const { return isInGlobal; };
  pthread_spinlock_t isGlobalLock;

  bool isInGlobal = false;

private:
  std::shared_ptr<BaseMsg> PopMsg();

  void OnServiceMsg(std::shared_ptr<ServiceMsg> msg);
  void OnAcceptMsg(std::shared_ptr<SocketAcceptMsg> msg);
  void OnRWMsg(std::shared_ptr<SocketRWMsg> msg);

  // RW msg 细分
  void OnSocketData(int fd, const char *buff, int len);
  void OnSocketWritable(int fd);
  void OnSocketClose(int fd);

  lua_State *luaState;
};
