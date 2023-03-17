#include "LuaAPI.h"
#include "ServiceMsg.h"
#include "Sunnet.h"
#include "lauxlib.h"
#include "lua.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

int LuaAPI::NewDataPackage(lua_State *L) {
  int size = luaL_checkinteger(L, 1);
  luaL_argcheck(L, size > 0, 1, "size must be bigger than 0");

  const char *buf = luaL_checkstring(L, 2);

  size_t nbytes = sizeof(struct DataPackage); // + size * sizeof(unsigned char);
  DataPackage *dp = (DataPackage *)lua_newuserdatauv(L, nbytes, 0);
  char *newbuf = new char[size];
  memcpy(newbuf, &buf, size);
  dp->size = size;
  dp->buff = std::shared_ptr<char>(newbuf);
  luaL_getmetatable(L, "DataPackage");
  lua_setmetatable(L, -2);
  return 1;
}

int LuaAPI::DataPackageGetSize(lua_State *L) { return 1; }

int LuaAPI::DataPackageGetBuff(lua_State *L) { return 1; }

/***--------------------------------------------------------------------**/

void LuaAPI::Register(lua_State *L) {

  static luaL_Reg lualibs[] = {
      {"NewService", NewService},
      {"KillService", KillService},
      {"Send", Send},
  };

  luaL_newlib(L, lualibs);
  lua_setglobal(L, "Sunnet");

  static luaL_Reg packageFuns[] = {
      {"Size", DataPackageGetSize},
      {"Buff", DataPackageGetBuff},
      {NULL, NULL},
  };

  static luaL_Reg constructor[] = {{"new", NewDataPackage}, {NULL, NULL}};

  if (luaL_newmetatable(L, "DataPackage") != 0) {
    luaL_setfuncs(L, packageFuns, 0);
  }
}

int LuaAPI::NewService(lua_State *state) {
  int num = lua_gettop(state);
  if (lua_isstring(state, 1) == 0) {
    lua_pushinteger(state, -1);
    return 1;
  }

  size_t len = 0;
  const char *type = lua_tolstring(state, 1, &len);
  std::string str(type);
  auto t = std::make_shared<std::string>(str);
  uint32_t id = Sunnet::inst->NewService(t);
  lua_pushinteger(state, id);
  return 1;
}

int LuaAPI::KillService(lua_State *L) {
  if (lua_isnumber(L, 1) == 0) {
    lua_pushinteger(L, -1);
    return 0;
  }
  int srvId = lua_tointegerx(L, 1, NULL);
  Sunnet::inst->KillService(srvId);
  return 0;
}

int LuaAPI::Send(lua_State *L) {
  int num = lua_gettop(L);
  if (num != 3) {
    std::cout << "Send failed ,Send require 3 argments" << std::endl;
    return 0;
  }

  if (lua_isinteger(L, 1) == 0) {
    std::cout << "source must be interger" << std::endl;
    return 0;
  }

  int source = lua_tointeger(L, 1);

  if (lua_isinteger(L, 2) == 0) {
    std::cout << "target must be interger" << std::endl;
    return 0;
  }

  int target = lua_tointeger(L, 2);

  if (lua_isstring(L, 3) == 0) {
    std::cout << "buf must be string" << std::endl;
    return 0;
  }

  size_t len;
  const char *luaStr = lua_tolstring(L, 3, &len);

  char *str = new char[len];
  memcpy(str, luaStr, len);

  auto msg = std::make_shared<ServiceMsg>();
  msg->type = BaseMsg::TYPE::SERVICE;
  msg->source = source;
  msg->buff = std::shared_ptr<char>(str);
  msg->size = len;

  Sunnet::inst->Send(target, msg);
  return 0;
}
