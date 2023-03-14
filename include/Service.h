#pragma once

#include "BaseMsg.h"
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
};
