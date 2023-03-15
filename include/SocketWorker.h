#ifndef __SOCKET_WORKER__
#define __SOCKET_WORKER__
#include <memory>
#include <sys/epoll.h>
class Conn;

class SocketWorker {

public:
  void Init();
  void operator()();

  void AddEvent(int fd);
  void RemoveEvent(int fd);
  void ModifyEvent(int fd, bool epollOutput);

private:
  int epollfd;

  void onEvent(epoll_event ev);
  void onAccept(std::shared_ptr<Conn> con);
  void onRw(std::shared_ptr<Conn> con, bool isRead, bool isWrite);
  void onError(std::shared_ptr<Conn> con);
};
#endif // !__SOCKET_WORKER__
