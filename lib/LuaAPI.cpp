/* Copyright (C) 2006-2018 PUC-Rio/Laboratorio TeleMidia

This file is part of Ginga (Ginga-NCL).

Ginga is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Ginga is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
License for more details.

You should have received a copy of the GNU General Public License
along with Ginga.  If not, see <https://www.gnu.org/licenses/>.  */

#include "LuaAPI.h"

#define CHUNK_INIT(name)                        \
  {G_STRINGIFY (LuaAPI_##name.lua),             \
   (const char *) LuaAPI::name##_lua,           \
   (size_t) LuaAPI::name##_lua_len}

LuaAPI::Chunk LuaAPI::_initMt             = CHUNK_INIT (initMt);
LuaAPI::Chunk LuaAPI::_Document_initMt    = CHUNK_INIT (Document_initMt);
LuaAPI::Chunk LuaAPI::_Object_initMt      = CHUNK_INIT (Object_initMt);
LuaAPI::Chunk LuaAPI::_Composition_initMt = CHUNK_INIT (Composition_initMt);
LuaAPI::Chunk LuaAPI::_Event_initMt       = CHUNK_INIT (Event_initMt);

static void
xloadbuffer (lua_State *L, const char *chunk, size_t len, const char *name)
{
  int err;
  err = luaL_loadbuffer (L, chunk, len, name);
  if (unlikely (err != LUA_OK))
    {
      luax_dump_stack (L);
      ERROR ("%s", lua_tostring (L, -1));
    }
}

static void
xpcall (lua_State *L, int nargs, int nresults)
{
  int err;

  err = lua_pcall (L, nargs, nresults, 0);
  if (unlikely (err != LUA_OK))
    {
      luax_dump_stack (L);
      ERROR ("%s", lua_tostring (L, -1));
    }
}

bool
LuaAPI::_loadLuaWrapperMt (lua_State *L, const char *name,
                           const luaL_Reg *const funcs[],
                           const Chunk *const chunks[])
{
  size_t i;

  luaL_getmetatable (L, name);
  if (!lua_isnil (L, -1))
    {
      lua_pop (L, 1);
      return false;
    }

  lua_pop (L, 1);
  luaL_newmetatable (L, name);

  if (funcs != NULL)
    {
      for (i = 0; funcs[i] != NULL; i++)
        luaL_setfuncs (L, funcs[i], 0);
    }

  if (chunks != NULL)
    {
      for (i = 0; chunks[i] != NULL; i++)
        {
          xloadbuffer (L, chunks[i]->text, chunks[i]->len, chunks[i]->name);
          lua_pushvalue (L, -2);
          xpcall (L, 1, 0);
        }
    }

  lua_pop (L, 1);
  return true;
}

void
LuaAPI::_attachLuaWrapper (lua_State *L, void *ptr)
{
  lua_pushvalue (L, LUA_REGISTRYINDEX);
  lua_insert (L, -2);
  lua_rawsetp (L, -2, ptr);
  lua_pop (L, 1);
}

void
LuaAPI::_detachLuaWrapper (lua_State *L, void *ptr)
{
  lua_pushnil (L);
  LuaAPI::_attachLuaWrapper (L, ptr);
}

void
LuaAPI::_pushLuaWrapper (lua_State *L, void *ptr)
{
  lua_pushvalue (L, LUA_REGISTRYINDEX);
  lua_rawgetp (L, -1, ptr);
  lua_remove (L, -2);
}

void
LuaAPI::_callLuaWrapper (lua_State *L, void *ptr, const char *name,
                         int nargs, int nresults)
{
  LuaAPI::_pushLuaWrapper (L, ptr);
  g_assert (luaL_getmetafield (L, -1, name) != LUA_TNIL);

  lua_insert (L, (-nargs) -2);
  lua_insert (L, (-nargs) -1);

  if (unlikely (lua_pcall (L, nargs + 1, nresults, 0) != LUA_OK))
    {
      luax_dump_stack (L);
      ERROR ("%s", lua_tostring (L, -1));
    }
}
