#include "SocketWorker.h"
#include "BaseMsg.h"
#include "Conn.h"
#include "ServiceMsg.h"
#include "SocketAcceptMsg.h"
#include "SocketRWMsg.h"
#include "Sunnet.h"
#include <cassert>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <sys/epoll.h>
#include <unistd.h>

void SocketWorker::Init() {
  std::cout << "[SockerWorker]Init" << std::endl;
  epollfd = epoll_create(1024);
  assert(epollfd > 0);
  std::cout << "[SocketWorker] Init ,created fd is " << epollfd << std::endl;
}

void SocketWorker::operator()() {
  while (true) {
    std::cout << "[SockerWorker] Run..." << std::endl;
    const int EVENTS_SIZE = 64;
    struct epoll_event events[EVENTS_SIZE];
    int count = epoll_wait(epollfd, events, EVENTS_SIZE, -1);
    for (int i = 0; i < count; i++) {
      struct epoll_event ev = events[i];
      onEvent(ev);
    }
  }
}

void SocketWorker::AddEvent(int fd) {
  struct epoll_event ev;
  ev.data.fd = fd;
  ev.events = EPOLLIN | EPOLLET;

  int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
  if (ret == -1) {
    std::cout << "[SocketWorker]AddEvent error " << std::strerror(errno)
              << std::endl;
  }
}

void SocketWorker::RemoveEvent(int fd) {
  int ret = epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
  if (ret == -1) {
    std::cout << "[SocketWorker]RemoveEvent error ,errono = "
              << std::strerror(errno) << std::endl;
  }
}

void SocketWorker::ModifyEvent(int fd, bool epollOutput) {
  struct epoll_event ev;
  ev.data.fd = fd;
  if (epollOutput) {
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
  } else {
    ev.events = EPOLLIN | EPOLLET;
  }
  int ret = epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
  if (ret == -1) {
    std::cout << "[SocketWorker]ModifyEvent error,errono = "
              << std::strerror(errno) << std::endl;
  }
}

void SocketWorker::onEvent(epoll_event ev) {
  int fd = ev.data.fd;
  std::shared_ptr<Conn> conn = Sunnet::inst->GetConn(fd);
  if (conn == NULL) {
    std::cout << "[SocketWorker]onEvent: fd not found!" << std::endl;
    return;
  }

  bool isRead = ev.events & EPOLLIN;
  bool isWrite = ev.events & EPOLLOUT;
  bool isError = ev.events & EPOLLERR;

  std::cout << "[SocketWorker]onEvent " << fd << " isRead:" << isRead
            << " isWrite:" << isWrite << " isError:" << isError << std::endl;
  if (conn->type == Conn::TYPE::SERVER) {
    if (isRead) {
      onAccept(conn);
    }
  } else {
    if (isRead || isWrite) {
      onRw(conn, isRead, isWrite);
    }
    if (isError) {
      onError(conn);
    }
  }
}

void SocketWorker::onAccept(std::shared_ptr<Conn> conn) {
  std::cout << "[SocketWorker]onAccept " << conn->fd << std::endl;
  int clientFd = conn->fd;
  fcntl(clientFd, F_SETFL, O_NONBLOCK);

  Sunnet::inst->AddConn(clientFd, conn->serviceId, conn->type);

  // 添加事件
  struct epoll_event ev;
  ev.data.fd = clientFd;
  ev.events = EPOLLET | EPOLLOUT;

  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientFd, &ev) == -1) {
    std::cout << "[SocketWorker]onAccept epoll_ctl failed: " << clientFd << " "
              << std::strerror(errno) << std::endl;
  }

  auto msg = std::make_shared<SocketAcceptMsg>();
  msg->listenFd = epollfd;
  msg->clientFd = clientFd;
  msg->type = BaseMsg::TYPE::SOCKET_ACCEPT;
  Sunnet::inst->Send(conn->serviceId, msg);
}

void SocketWorker::onRw(std::shared_ptr<Conn> conn, bool isRead, bool isWrite) {
  std::cout << "[SocketWorker]onRw " << conn->fd << " isRead:" << isRead
            << ",isWrite:" << isWrite << std::endl;
  auto msg = std::make_shared<SocketRWMsg>();
  msg->fd = conn->fd;
  msg->type = BaseMsg::TYPE::SOCKET_RW;
  msg->isWrite = isWrite;
  msg->isRead = isRead;
  Sunnet::inst->Send(conn->serviceId, msg);
}

void SocketWorker::onError(std::shared_ptr<Conn> con) {
  std::cout << "[SocketWorker]onError " << con->fd << std::endl;
}
