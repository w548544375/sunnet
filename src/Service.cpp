#include "Service.h"
#include "ServiceMsg.h"
#include "Sunnet.h"
#include <cstddef>
#include <cstdio>
#include <ios>
#include <iostream>
#include <memory>
#include <pthread.h>
#include <string>

Service::Service() {
  // 初始化锁
  pthread_spin_init(&queueLock, PTHREAD_PROCESS_PRIVATE);
  pthread_spin_init(&isGlobalLock, PTHREAD_PROCESS_PRIVATE);
}

Service::~Service() {
  pthread_spin_destroy(&queueLock);
  pthread_spin_destroy(&isGlobalLock);
}

void Service::PushMsg(std::shared_ptr<BaseMsg> msg) {
  pthread_spin_lock(&queueLock);
  msgQueue.push(msg);
  pthread_spin_unlock(&queueLock);
}

std::shared_ptr<BaseMsg> Service::PopMsg() {
  std::shared_ptr<BaseMsg> msg = NULL;
  pthread_spin_lock(&queueLock);

  if (!msgQueue.empty()) {
    msg = msgQueue.front();
    msgQueue.pop();
  }

  pthread_spin_unlock(&queueLock);

  return msg;
}

void Service::OnInit() {
  std::cout << "[Service " << type->data() << id << "] OnInit" << std::endl;
}

void Service::OnExit() {
  std::cout << "[Service " << type->data() << id << "] OnExit" << std::endl;
}

void Service::OnMsg(std::shared_ptr<BaseMsg> msg) {
  if (msg->type == BaseMsg::TYPE::SERVICE) {
    auto m = std::dynamic_pointer_cast<ServiceMsg>(msg);
    std::cout << "[" << id << "]"
              << "msg from " << m->source << " content:" << m->buff
              << std::endl;
    // 给发送的服务返回消息
    // char *buf = new char[99999];
    // sprintf(buf, "%d recv message from %d,content is %s\n", id, m->source,
    //        m->buff.get());
    // auto retm = Sunnet::MakeMsg(id, buf, 99999);
    // Sunnet::inst->Send(m->source, retm);
    return;
  }
  std::cout << "[Service " << type->data() << id << "] OnMsg" << std::endl;
}

bool Service::ProcessMsg() {
  std::shared_ptr<BaseMsg> msg = this->PopMsg();
  if (msg) {
    OnMsg(msg);
    return true;
  }

  return false;
}

void Service::ProcessMsgs(int max) {
  for (int i = 0; i < max; i++) {
    bool succ = ProcessMsg();
    if (!succ) {
      break;
    }
  }
}

void Service::SetIsInGlobal(bool in) {
  pthread_spin_lock(&isGlobalLock);
  isInGlobal = in;
  pthread_spin_unlock(&isGlobalLock);
}
