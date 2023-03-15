#include "Sunnet.h"
#include <cstdint>
#include <memory>
#include <string>
#include <unistd.h>

void test() {
  auto pingType = std::make_shared<std::string>("ping");
  uint32_t ping1 = Sunnet::inst->NewService(pingType);
  uint32_t ping2 = Sunnet::inst->NewService(pingType);

  uint32_t pong = Sunnet::inst->NewService(pingType);

  auto ping1Msg = Sunnet::inst->MakeMsg(
      ping1, new char[6]{'p', 'i', 'n', 'g', '1', '\0'}, 5);
  auto ping2Msg = Sunnet::inst->MakeMsg(
      ping2, new char[6]{'p', 'i', 'n', 'g', '2', '\0'}, 5);
  auto pongMsg =
      Sunnet::inst->MakeMsg(pong, new char[5]{'p', 'o', 'n', 'g', '\0'}, 4);
  usleep(1000000);
  Sunnet::inst->Send(pong, ping1Msg);
  Sunnet::inst->Send(pong, ping2Msg);
  Sunnet::inst->Send(ping1, pongMsg);
}
void testSocket() {
  auto pingType = std::make_shared<std::string>("ping");
  uint32_t serv = Sunnet::inst->NewService(pingType);
  int fd = Sunnet::inst->Listen(7788, serv);
}

int main() {
  new Sunnet();
  Sunnet::inst->Start();
  //  test();
  testSocket();
  Sunnet::inst->Wait();
  return 0;
}
