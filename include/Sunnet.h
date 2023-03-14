#ifndef __SUNNET_H__
#define __SUNNET_H__
#include "BaseMsg.h"
#include "Service.h"
#include <cstdint>
#include <memory>
#include <pthread.h>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>
class Worker;
namespace std {
class thread;
}
class Sunnet {
public:
  static Sunnet *inst;
  Sunnet();
  Sunnet(Sunnet &&) = default;
  Sunnet(const Sunnet &) = default;
  Sunnet &operator=(Sunnet &&) = default;
  Sunnet &operator=(const Sunnet &) = default;
  ~Sunnet();
  void Start();
  void Wait();
  uint32_t NewService(std::shared_ptr<std::string> type);
  void KillService(uint32_t id);

  void Send(uint32_t toService, std::shared_ptr<BaseMsg> msg);
  void PushGlobalQueue(std::shared_ptr<Service> service);
  std::shared_ptr<Service> PopGlobalQueu();

  static std::shared_ptr<BaseMsg> MakeMsg(uint32_t source, char *buff, int len);

  void CheckAndWakeUp();
  void WorkerWait();

public:
  std::unordered_map<uint32_t, std::shared_ptr<Service>> services;
  uint32_t maxId = 0;
  pthread_rwlock_t servicesLock;

private:
  int WORK_NUM = 3;
  std::vector<Worker *> workers;
  std::vector<std::thread *> workerThreads;

  void startWorker();

  std::shared_ptr<Service> GetService(uint32_t id);

  std::queue<std::shared_ptr<Service>> globalQueue;
  int globalQueueLength = 0;
  pthread_spinlock_t globalLock;
  // 休眠与唤醒
  pthread_mutex_t sleepMtx;
  pthread_cond_t sleepCond;
  int sleepCount = 0;
};

#endif // !__SUNNET_H__
