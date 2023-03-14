#include "Sunnet.h"
#include "BaseMsg.h"
#include "Service.h"
#include "ServiceMsg.h"
#include "Worker.h"
#include "unistd.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <pthread.h>
#include <thread>
using namespace std;

Sunnet *Sunnet::inst;

Sunnet::Sunnet() { inst = this; }

Sunnet::~Sunnet() {
  pthread_rwlock_destroy(&servicesLock);
  pthread_spin_destroy(&globalLock);
  pthread_cond_destroy(&sleepCond);
  pthread_mutex_destroy(&sleepMtx);
}

void Sunnet::Start() {
  cout << "Hello Sunnect" << endl;
  pthread_rwlock_init(&servicesLock, NULL);
  pthread_spin_init(&globalLock, PTHREAD_PROCESS_PRIVATE);
  pthread_cond_init(&sleepCond, NULL);
  pthread_mutex_init(&sleepMtx, NULL);
  startWorker();
}

void Sunnet::startWorker() {
  for (int i = 0; i < WORK_NUM; i++) {
    std::cout << "start worker thread " << i << std::endl;

    Worker *worker = new Worker();
    worker->id = i;
    worker->eachNum = 2 << i;

    std::thread *workThread = new std::thread(*worker);

    workers.push_back(worker);
    workerThreads.push_back(workThread);
  }
}

void Sunnet::Wait() {
  if (this->workerThreads[0]) {
    workerThreads[0]->join();
  }
}

uint32_t Sunnet::NewService(std::shared_ptr<std::string> type) {
  auto srv = make_shared<Service>();
  srv->type = type;
  pthread_rwlock_wrlock(&servicesLock);
  srv->id = maxId;
  maxId++;
  services.emplace(srv->id, srv);
  pthread_rwlock_unlock(&servicesLock);
  srv->OnInit();
  return srv->id;
}

shared_ptr<Service> Sunnet::GetService(uint32_t id) {
  shared_ptr<Service> srv = NULL;
  pthread_rwlock_rdlock(&servicesLock);
  unordered_map<uint32_t, shared_ptr<Service>>::iterator ite =
      services.find(id);
  if (ite != services.end()) {
    srv = ite->second;
  }
  pthread_rwlock_unlock(&servicesLock);
  return srv;
}

void Sunnet::KillService(uint32_t id) {
  auto srv = GetService(id);
  if (srv != NULL) {
    srv->OnExit();
    srv->isExist = true;
    pthread_rwlock_wrlock(&servicesLock);
    services.erase(id);
    pthread_rwlock_unlock(&servicesLock);
  }
}

void Sunnet::Send(uint32_t toService, std::shared_ptr<BaseMsg> msg) {
  auto srv = GetService(toService);
  if (!srv) {
    cout << "Send failed,[" << toService << "] does not exist." << endl;
    return;
  }
  srv->PushMsg(msg);
  bool isPush = false;
  pthread_spin_lock(&srv->isGlobalLock);
  if (!srv->IsInGlobal()) {
    PushGlobalQueue(srv);
    srv->isInGlobal = true;
    isPush = true;
  }
  pthread_spin_unlock(&srv->isGlobalLock);
  if (isPush) {
    CheckAndWakeUp();
  }
}

void Sunnet::PushGlobalQueue(std::shared_ptr<Service> service) {
  pthread_spin_lock(&globalLock);
  globalQueue.push(service);
  globalQueueLength++;
  pthread_spin_unlock(&globalLock);
}

shared_ptr<Service> Sunnet::PopGlobalQueu() {
  shared_ptr<Service> srv = NULL;
  pthread_spin_lock(&globalLock);
  if (!globalQueue.empty()) {
    srv = globalQueue.front();
    globalQueue.pop();
    globalQueueLength--;
  }
  pthread_spin_unlock(&globalLock);
  return srv;
}

shared_ptr<BaseMsg> Sunnet::MakeMsg(uint32_t source, char *buff, int len) {
  auto msg = make_shared<ServiceMsg>();
  msg->source = source;
  msg->buff = shared_ptr<char>(buff);
  msg->size = len;
  msg->type = BaseMsg::TYPE::SERVICE;
  return msg;
}

void Sunnet::WorkerWait() {
  pthread_mutex_lock(&sleepMtx);
  sleepCount++;
  pthread_cond_wait(&sleepCond, &sleepMtx);
  sleepCount--;
  pthread_mutex_unlock(&sleepMtx);
}

void Sunnet::CheckAndWakeUp() {
  if (sleepCount == 0) {
    return;
  }
  if (WORK_NUM - sleepCount <= globalQueueLength) {
    cout << "wake up" << endl;
    pthread_cond_signal(&sleepCond);
  }
}
