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
#include "Object.h"

#include "Context.h"
#include "Media.h"
#include "Switch.h"

// Object type.
#define OBJECT_CONTEXT_STRING  "context"
#define OBJECT_SWITCH_STRING   "switch"
#define OBJECT_MEDIA_STRING    "media"

const char *
LuaAPI::_Object_getRegistryKey (Object *obj)
{
  g_return_val_if_fail (obj != NULL, NULL);

  switch (obj->getType ())
    {
    case Object::CONTEXT:
      return LuaAPI::_CONTEXT;
    case Object::SWITCH:
      return LuaAPI::_SWITCH;
    case Object::MEDIA:
      return LuaAPI::_MEDIA;
    default:
      g_assert_not_reached ();
    }
}

const struct luaL_Reg LuaAPI::_Object_funcs[] =
  {
   {"__getUnderlyingObject", LuaAPI::_l_Object_getUnderlyingObject},
   {"getType",               LuaAPI::_l_Object_getType},
   {"getDocument",           LuaAPI::_l_Object_getDocument},
   {"getParent",             LuaAPI::_l_Object_getParent},
   {"getId",                 LuaAPI::_l_Object_getId},
   {"getProperty",           LuaAPI::_l_Object_getProperty},
   {"setProperty",           LuaAPI::_l_Object_setProperty},
   {"getEvents",             LuaAPI::_l_Object_getEvents},
   {"getEvent",              LuaAPI::_l_Object_getEvent},
   {NULL, NULL},
  };

void
LuaAPI::Object_attachWrapper (lua_State *L, Object *obj)
{
  const char *name;

  g_return_if_fail (L != NULL);
  g_return_if_fail (obj != NULL);

  // Load and initialize metatable, if not loaded yet.
  name = LuaAPI::_Object_getRegistryKey (obj);

  // Create Lua wrapper for object.
  switch (obj->getType ())
    {
    case Object::CONTEXT:
      {
        static const struct luaL_Reg *const funcs[] =
          {
           _Object_funcs,
           NULL
          };

        static const Chunk *const chunks[] =
          {
           &LuaAPI::_initMt,
           &LuaAPI::_Object_initMt,
           &LuaAPI::_Composition_initMt,
           NULL
          };

        Context **wrapper;

        LuaAPI::_loadLuaWrapperMt (L, name, funcs, chunks);
        wrapper = (Context **) lua_newuserdata (L, sizeof (Context **));
        g_assert_nonnull (wrapper);
        *wrapper = (Context *) obj;
        g_assert_nonnull (*wrapper);
        break;
      }
    case Object::SWITCH:
      {
        static const struct luaL_Reg *const funcs[] =
          {
           _Object_funcs,
           NULL
          };

        static const Chunk *const chunks[] =
          {
           &LuaAPI::_initMt,
           &LuaAPI::_Object_initMt,
           &LuaAPI::_Composition_initMt,
           NULL
          };

        Switch **wrapper;

        LuaAPI::_loadLuaWrapperMt (L, name, funcs, chunks);
        wrapper = (Switch **) lua_newuserdata (L, sizeof (Switch **));
        g_assert_nonnull (wrapper);
        *wrapper = (Switch *) obj;
        g_assert_nonnull (*wrapper);
        break;
      }
    case Object::MEDIA:
      {
        static const struct luaL_Reg *const funcs[] =
          {
           _Object_funcs,
           NULL
          };

        static const Chunk *const chunks[] =
          {
           &LuaAPI::_initMt,
           &LuaAPI::_Object_initMt,
           NULL
          };

        Media **wrapper;

        LuaAPI::_loadLuaWrapperMt (L, name, funcs, chunks);
        wrapper = (Media **) lua_newuserdata (L, sizeof (Media **));
        g_assert_nonnull (wrapper);
        *wrapper = (Media *) obj;
        g_assert_nonnull (*wrapper);
        break;
      }
    default:
      g_assert_not_reached ();
    }

  luaL_setmetatable (L, name);

  // Set LUA_REGISTRY[obj]=wrapper.
  LuaAPI::_attachLuaWrapper (L, obj);

  // Call obj:_attachData().
  LuaAPI::_callLuaWrapper (L, obj, "_attachData", 0, 0);

  // Call _D:_addObject (obj).
  LuaAPI::_pushLuaWrapper (L, obj);
  LuaAPI::_callLuaWrapper (L, obj->getDocument (), "_addObject", 1, 0);
}

void
LuaAPI::Object_detachWrapper (lua_State *L, Object *obj)
{
  g_return_if_fail (L != NULL);
  g_return_if_fail (obj != NULL);

  // Call _D:_removeObject (obj).
  LuaAPI::_pushLuaWrapper (L, obj);
  LuaAPI::_callLuaWrapper (L, obj->getDocument (), "_removeObject", 1, 0);

  // Call obj:_detachData().
  LuaAPI::_callLuaWrapper (L, obj, "_detachData", 0, 0);

  // Set LUA_REGISTRY[obj] = nil.
  LuaAPI::_detachLuaWrapper (L, obj);
}

Object *
LuaAPI::Object_check (lua_State *L, int i)
{
  const char *name;

  luaL_checktype (L, i, LUA_TUSERDATA);
  if (unlikely (!lua_getmetatable (L, i)))
    {
      goto fail;
    }

  lua_getfield (L, -1, "__name");
  name = lua_tostring (L, -1);
  if (unlikely (name == NULL))
    {
      lua_pop (L, 1);
      goto fail;
    }

  lua_pop (L, 2);
  if (g_str_equal (name, LuaAPI::_CONTEXT))
    return LuaAPI::Context_check (L, i);
  else if (g_str_equal (name, LuaAPI::_SWITCH))
    return LuaAPI::Switch_check (L, i);
  else if (g_str_equal (name, LuaAPI::_MEDIA))
    return LuaAPI::Media_check (L, i);

 fail:
  luaL_argcheck (L, 0, i, "Ginga.Object expected");
  return NULL;
}

Object::Type
LuaAPI::Object_Type_check (lua_State *L, int i)
{
  static const char *types[] = {OBJECT_CONTEXT_STRING,
                                OBJECT_SWITCH_STRING,
                                OBJECT_MEDIA_STRING,
                                NULL};

  g_return_val_if_fail (L != NULL, Object::CONTEXT);

  switch (luaL_checkoption (L, i, NULL, types))
    {
    case 0:
      return Object::CONTEXT;
    case 1:
      return Object::SWITCH;
    case 2:
      return Object::MEDIA;
    default:
      g_assert_not_reached ();
    }
}

unsigned int
LuaAPI::Object_Type_Mask_check (lua_State *L, int i)
{
  unsigned int mask;

  g_return_val_if_fail (L != NULL, 0);

  if (lua_isnoneornil (L, i))
    return (unsigned int) -1;

  luaL_argcheck (L, lua_type (L, i) == LUA_TTABLE, i, "expected table");

  mask = 0;
  lua_getfield (L, i, OBJECT_CONTEXT_STRING);
  if (lua_toboolean (L, -1))
    mask &= Object::CONTEXT;
  lua_pop (L, 1);

  lua_getfield (L, i, OBJECT_SWITCH_STRING);
  if (lua_toboolean (L, -1))
    mask &= Object::SWITCH;
  lua_pop (L, 1);

  lua_getfield (L, i, OBJECT_MEDIA_STRING);
  if (lua_toboolean (L, -1))
    mask &= Object::MEDIA;
  lua_pop (L, 1);

  return mask;
}

void
LuaAPI::Object_push (lua_State *L, Object *obj)
{
  g_return_if_fail (L != NULL);
  g_return_if_fail (obj != NULL);

  LuaAPI::_pushLuaWrapper (L, obj);
}

void
LuaAPI::Object_Type_push (lua_State *L, Object::Type type)
{
  g_return_if_fail (L != NULL);

  switch (type)
    {
    case Object::CONTEXT:
      lua_pushliteral (L, OBJECT_CONTEXT_STRING);
      break;
    case Object::SWITCH:
      lua_pushliteral (L, OBJECT_SWITCH_STRING);
      break;
    case Object::MEDIA:
      lua_pushliteral (L, OBJECT_MEDIA_STRING);
      break;
    default:
      g_assert_not_reached ();
    }
}

void
LuaAPI::Object_Type_Mask_push (lua_State *L, unsigned int mask)
{
  g_return_if_fail (L != NULL);

  lua_newtable (L);
  if (mask & Object::CONTEXT)
    {
      lua_pushliteral (L, OBJECT_CONTEXT_STRING);
      lua_pushboolean (L, 1);
      lua_rawset (L, -3);
    }
  if (mask & Object::SWITCH)
    {
      lua_pushliteral (L, OBJECT_SWITCH_STRING);
      lua_pushboolean (L, 1);
      lua_rawset (L, -3);
    }
  if (mask & Object::MEDIA)
    {
      lua_pushliteral (L, OBJECT_MEDIA_STRING);
      lua_pushboolean (L, 1);
      lua_rawset (L, -3);
    }
}

int
LuaAPI::_l_Object_getUnderlyingObject (lua_State *L)
{
  lua_pushlightuserdata (L, LuaAPI::Object_check (L, 1));
  return 1;
}

int
LuaAPI::_l_Object_getType (lua_State *L)
{
  Object *obj;

  obj = LuaAPI::Object_check (L, 1);
  LuaAPI::Object_Type_push (L, obj->getType ());

  return 1;
}

int
LuaAPI::_l_Object_getDocument (lua_State *L)
{
  Object *obj;

  obj = LuaAPI::Object_check (L, 1);
  LuaAPI::_pushLuaWrapper (L, obj->getDocument ());

  return 1;
}

int
LuaAPI::_l_Object_getParent (lua_State *L)
{
  Object *obj;

  obj = LuaAPI::Object_check (L, 1);
  if (obj->getParent () != NULL)
    LuaAPI::_pushLuaWrapper (L, obj->getParent ());
  else
    lua_pushnil (L);

  return 1;
}

int
LuaAPI::_l_Object_getId (lua_State *L)
{
  Object *obj;

  obj = LuaAPI::Object_check (L, 1);
  lua_pushstring (L, obj->getId ().c_str ());

  return 1;
}

int
LuaAPI::_l_Object_getProperty (lua_State *L)
{
  Object *obj;
  const char *name;

  obj = LuaAPI::Object_check (L, 1);
  name = luaL_checkstring (L, 2);

  lua_pushstring (L, obj->getProperty (name).c_str ());

  return 1;
}

int
LuaAPI::_l_Object_setProperty (lua_State *L)
{
  Object *obj;
  const gchar *name;
  const gchar *value;
  lua_Integer dur;

  obj = LuaAPI::Object_check (L, 1);
  name = luaL_checkstring (L, 2);
  value = luaL_checkstring (L, 3);
  dur = luaL_optinteger (L, 4, 0);

  obj->setProperty (name, value, dur);

  return 0;
}

int
LuaAPI::_l_Object_getEvents (lua_State *L)
{
  Object *obj;
  set<Event *> events;
  lua_Integer i;

  obj = LuaAPI::Object_check (L, 1);
  obj->getEvents (&events);

  lua_newtable (L);
  i = 1;
  for (auto evt: events)
    {
      LuaAPI::_pushLuaWrapper (L, evt);
      lua_rawseti (L, -2, i++);
    }

  return 1;
}

int
LuaAPI::_l_Object_getEvent (lua_State *L)
{
  Object *obj;
  Event::Type type;
  const char *id;
  Event *evt;

  obj = LuaAPI::Object_check (L, 1);
  type = LuaAPI::Event_Type_check (L, 2);
  id = luaL_checkstring (L, 3);

  evt = obj->getEvent (type, id);
  if (evt == NULL)
    {
      lua_pushnil (L);
      return 1;
    }

  LuaAPI::_pushLuaWrapper (L, evt);
  return 1;
}
