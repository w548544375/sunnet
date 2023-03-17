#include "Service.h"
#include "BaseMsg.h"
#include "LuaAPI.h"
#include "ServiceMsg.h"
#include "SocketAcceptMsg.h"
#include "SocketRWMsg.h"
#include "Sunnet.h"
#include "lua.h"
#include <cstddef>
#include <cstdio>
#include <fcntl.h>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <pthread.h>
#include <string>
#include <unistd.h>

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
  luaState = luaL_newstate();

  luaL_openlibs(luaState);
  LuaAPI::Register(luaState);
  std::string fileName = "../service/" + *type + ".lua";
  std::fstream file;
  file.open(fileName, std::ios::in);
  if (!file) {
    fileName = "../service/" + *type + "/init.lua";
  }

  int isOk = luaL_dofile(luaState, fileName.data());
  if (isOk == 1) {
    std::cout << "run lua fail : " << lua_tostring(luaState, 1) << std::endl;
    return;
  }
  lua_getglobal(luaState, "OnInit");
  lua_pushinteger(luaState, id);
  isOk = lua_pcall(luaState, 1, 0, 0);
  if (isOk != 0) {
    std::cout << "call lua script OnInit failed :" << lua_tostring(luaState, -1)
              << std::endl;
  }
}

void Service::OnExit() {
  std::cout << "[Service " << type->data() << id << "] OnExit" << std::endl;
  lua_getglobal(luaState, "OnExit");
  int isOk = lua_pcall(luaState, 0, 0, 0);
  if (isOk != 0) {
    std::cout << "[Service]" << type.get() << " faile to invoke OnExit "
              << std::endl;
  }
}

void Service::OnMsg(std::shared_ptr<BaseMsg> msg) {
  if (msg->type == BaseMsg::TYPE::SOCKET_ACCEPT) {
    auto m = std::dynamic_pointer_cast<SocketAcceptMsg>(msg);
    std::cout << "[Service]OnMsg SOCKET_ACCEPT " << m->listenFd
              << " ,client :" << m->clientFd << std::endl;
    OnAcceptMsg(m);
  }
  if (msg->type == BaseMsg::TYPE::SOCKET_RW) {
    auto m = std::dynamic_pointer_cast<SocketRWMsg>(msg);
    std::cout << "[Service]OnMsg SOCKET_RW " << m->fd << "(isRead:" << m->isRead
              << ",isWrite:" << m->isWrite << ")" << std::endl;

    OnRWMsg(m);
    /**
    * if (m->isRead) {
      char buf[512];
      int len = read(m->fd, &buf, 512);
      if (len > 0) {
        std::cout << "[Service]OnMsg Recv Msg from client " << m->fd << ":"
                  << buf << std::endl;
        char resp[3] = {'l', 'p', 'y'};
        write(m->fd, &resp, 3);
      }
    }
    std::cout << "[Service]OnMsg " << m->fd << std::endl; */
  }
  if (msg->type == BaseMsg::TYPE::SERVICE) {
    auto m = std::dynamic_pointer_cast<ServiceMsg>(msg);
    std::cout << "[" << id << "]"
              << "msg from " << m->source << " content:" << m->buff
              << std::endl;
    OnServiceMsg(m);
    // 给发送的服务返回消息
    // char *buf = new char[99999];
    // sprintf(buf, "%d recv message from %d,content is %s\n", id, m->source,
    //        m->buff.get());
    // auto retm = Sunnet::MakeMsg(id, buf, 99999);
    // Sunnet::inst->Send(m->source, retm);
  }
}

void Service::OnAcceptMsg(std::shared_ptr<SocketAcceptMsg> msg) {}

void Service::OnRWMsg(std::shared_ptr<SocketRWMsg> msg) {
  int fd = msg->fd;
  const int BUFF_SIZE = 512;
  char buff[BUFF_SIZE];
  int len = 0;
  do {
    len = read(fd, &buff, BUFF_SIZE);
    if (len > 0) {
      OnSocketData(fd, buff, len);
    }
  } while (len == BUFF_SIZE);

  if (len <= 0 && errno != EAGAIN) {
    if (Sunnet::inst->GetConn(fd)) {
      OnSocketClose(fd);
      Sunnet::inst->CloseConn(fd);
    }
  }

  if (msg->isWrite) {
    if (Sunnet::inst->GetConn(fd)) {
      OnSocketWritable(fd);
    }
  }
}

void Service::OnSocketData(int fd, const char *buff, int len) {}

void Service::OnSocketWritable(int fd) {}

void Service::OnSocketClose(int fd) {}

void Service::OnServiceMsg(std::shared_ptr<ServiceMsg> msg) {
  lua_getglobal(luaState, "OnServiceMsg");
  lua_pushinteger(luaState, msg->source);
  lua_pushlstring(luaState, msg->buff.get(), msg->size);

  int isOk = lua_pcall(luaState, 2, 0, 0);
  if (isOk != 0) {
    std::cout << "[Service]" << type.get() << " failed to invoke OnServiceMsg "
              << lua_tostring(luaState, -1) << std::endl;
  }
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
