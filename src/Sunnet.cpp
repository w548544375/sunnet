#include "Sunnet.h"
#include "BaseMsg.h"
#include "Conn.h"
#include "Service.h"
#include "ServiceMsg.h"
#include "SocketWorker.h"
#include "Worker.h"
#include "unistd.h"
#include <arpa/inet.h>
#include <assert.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unordered_map>
using namespace std;

Sunnet *Sunnet::inst;

Sunnet::Sunnet() { inst = this; }

Sunnet::~Sunnet() {
  pthread_rwlock_destroy(&servicesLock);
  pthread_spin_destroy(&globalLock);
  pthread_cond_destroy(&sleepCond);
  pthread_mutex_destroy(&sleepMtx);
  pthread_rwlock_destroy(&connLock);
  std::unordered_map<uint32_t, std::shared_ptr<Conn>>::iterator it;
  for (it = conns.begin(); it != conns.end(); it++) {
    close(it->first);
  }
  std::cout << "[Sunnet]~Sunnet()\n";
}

void Sunnet::Start() {
  cout << "Hello Sunnect" << endl;
  // 忽略复位信号
  signal(SIGPIPE, SIG_IGN);
  pthread_rwlock_init(&servicesLock, NULL);
  pthread_spin_init(&globalLock, PTHREAD_PROCESS_PRIVATE);
  pthread_cond_init(&sleepCond, NULL);
  pthread_mutex_init(&sleepMtx, NULL);
  assert(pthread_rwlock_init(&connLock, NULL) == 0);
  startWorker();
  startSocketWorker();
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

void Sunnet::startSocketWorker() {
  socketWorker = new SocketWorker();
  socketWorker->Init();
  socketThread = new thread(*socketWorker);
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

int Sunnet::AddConn(int fd, uint32_t serviceId, uint8_t type) {
  auto conn = make_shared<Conn>();
  conn->fd = fd;
  conn->serviceId = serviceId;
  conn->type = type;

  pthread_rwlock_wrlock(&connLock);
  conns.emplace(fd, conn);
  pthread_rwlock_unlock(&connLock);
  return fd;
}

std::shared_ptr<Conn> Sunnet::GetConn(int fd) {
  std::shared_ptr<Conn> conn = NULL;
  pthread_rwlock_rdlock(&connLock);
  std::unordered_map<uint32_t, std::shared_ptr<Conn>>::iterator iter =
      conns.find(fd);
  if (iter != conns.end()) {
    conn = iter->second;
  }
  pthread_rwlock_unlock(&connLock);
  return conn;
}

bool Sunnet::RemoveConn(int fd) {
  int result;
  pthread_rwlock_rdlock(&connLock);
  result = conns.erase(fd);
  pthread_rwlock_unlock(&connLock);
  return result == 1;
}

int Sunnet::Listen(uint32_t port, uint32_t serviceId) {
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd <= 0) {
    std::cout << "[Sunnet]Listen error,create listen socket error !"
              << std::endl;
    return -1;
  }
  std::cout << "[Sunnet]Listen fd is " << listenfd << std::endl;

  fcntl(listenfd, F_SETFL, O_NONBLOCK);
  // bind
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  int ret = bind(listenfd, (struct sockaddr *)&addr, sizeof(addr));
  if (ret == -1) {
    std::cout << "[Sunnet]Listen error,bind socket error:"
              << std::strerror(errno) << std::endl;
    return -1;
  }

  ret = listen(listenfd, 64);
  if (ret == -1) {
    std::cout << "[Sunnet]Listen error,listen socket error:"
              << std::strerror(errno) << std::endl;
    return -1;
  }

  AddConn(listenfd, serviceId, Conn::TYPE::SERVER);

  socketWorker->AddEvent(listenfd);

  return listenfd;
}

void Sunnet::CloseConn(int fd) {
  bool removed = RemoveConn(fd);
  if (removed) {
    socketWorker->RemoveEvent(fd);
  }
  close(fd);
  std::string eventRemoved =
      removed ? "Event Removed." : "Event did not removed!";
  std::cout << "[Sunnet]CloseConn," << eventRemoved << std::endl;
}

void Sunnet::ModifyEvent(int fd, bool epollout) {
  socketWorker->ModifyEvent(fd, epollout);
}
