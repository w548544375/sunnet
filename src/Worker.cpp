#include "Worker.h"
#include "Service.h"
#include "Sunnet.h"
#include <iostream>
#include <memory>
#include <pthread.h>
#include <unistd.h>

void Worker::operator()() {
  while (true) {
    std::shared_ptr<Service> srv = Sunnet::inst->PopGlobalQueu();
    if (!srv) {
      Sunnet::inst->WorkerWait();
    } else {
      srv->ProcessMsgs(eachNum);
      checkAndPutGlobal(srv);
    }
  }
}

void Worker::checkAndPutGlobal(std::shared_ptr<Service> service) {
  if (service->isExist) {
    return;
  }
  pthread_spin_lock(&service->queueLock);
  if (!service->msgQueue.empty()) {
    Sunnet::inst->PushGlobalQueue(service);
  } else {
    service->SetIsInGlobal(false);
  }
  pthread_spin_unlock(&service->queueLock);
}
