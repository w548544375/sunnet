#ifndef __WORKER_H__
#define __WORKER_H__
#include <memory>
class Sunnet;
class Service;

class Worker {

public:
  int id;
  int eachNum;
  void operator()();

private:
  void checkAndPutGlobal(std::shared_ptr<Service> service);
};

#endif // !__WORKER_H__
