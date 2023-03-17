#pragma once
#include <lua.hpp>
#include <memory>

struct DataPackage {
  short type;
  std::shared_ptr<char> buff;
  int size;
};

class LuaAPI {
public:
  static void Register(lua_State *state);
  static int NewService(lua_State *state);
  static int KillService(lua_State *state);
  static int Send(lua_State *state);
  static int Listen(lua_State *state);
  static int CloseConn(lua_State *state);
  static int Write(lua_State *state);

  static int NewDataPackage(lua_State *L);
  static int DataPackageGetSize(lua_State *L);
  static int DataPackageGetBuff(lua_State *L);
};
