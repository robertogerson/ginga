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

#ifndef GINGA_LUA_API_H
#define GINGA_LUA_API_H

#include "aux-ginga.h"
GINGA_BEGIN_DECLS
#include "aux-lua.h"
GINGA_END_DECLS

#include "Object.h"
#include "Player.h"

GINGA_NAMESPACE_BEGIN

class Document;
class Context;
class Switch;
class Media;

/// The Lua interface to the internal model.
class LuaAPI
{
  // Document --------------------------------------------------------------
public:

  /// Attaches Lua wrapper to \p doc.
  static void Document_attachWrapper (lua_State *L, Document *doc);

  /// Detaches Lua wrapper from \p doc.
  static void Document_detachWrapper (lua_State *L, Document *doc);

  /// Checks if the value at index \p i of stack is a Document wrapper.
  static Document *Document_check (lua_State *L, int i);

  /// Pushes the Lua wrapper of document onto stack.
  static void Document_push (lua_State *L, Document *doc);

  /// Calls a method of the Lua wrapper of the given document.
  static void Document_call (lua_State *L, Document *doc,
                             const char *name, int nargs, int nresults);

private:
  static int __l_Document_gc (lua_State *L);
  static int _l_Document_createObject (lua_State *L);
  static int _l_Document_createStateMachine (lua_State *L);
  static int _l_Document_createPlayer (lua_State *L);
  static int _l_Document_isinteger (lua_State *L);

  // Object ----------------------------------------------------------------
public:

  /// Attaches Lua wrapper to \p obj.
  static void Object_attachWrapper (lua_State *L, Object *obj,
                                    Document *doc, Object::Type type,
                                    const string &id);

  /// Detaches Lua wrapper from \p obj.
  static void Object_detachWrapper (lua_State *L, Object *obj);

  /// Checks if the value at index \p i of stack is an Object wrapper.
  static Object *Object_check (lua_State *L, int i);

  /// Checks if the value at index \p i of stack is an Object::Type.
  static Object::Type Object_Type_check (lua_State *L, int i);

  /// Pushes the Lua wrapper of object onto stack.
  static void Object_push (lua_State *L, Object *obj);

  /// Pushes Object::Type (as string) onto stack.
  static void Object_Type_push (lua_State *L, Object::Type type);

  /// Calls a method of the Lua wrapper of the given object.
  static void Object_call (lua_State *L, Object *obj,
                           const char *name, int nargs, int nresults);
  // Composition -----------------------------------------------------------
public:

  /// Checks if the value at index \p i of stack is a Composition wrapper.
  static Composition *Composition_check (lua_State *L, int i);

  /// Calls a method of the Lua wrapper of the given composition.
  static void Composition_call (lua_State *L, Composition *comp,
                                const char *name, int nargs, int nresults);

  // Context ---------------------------------------------------------------

  /// Checks if the value at index \p i of stack is a Context wrapper.
  static Context *Context_check (lua_State *L, int i);

  /// Pushes the Lua wrapper of context onto stack.
  static void Context_push (lua_State *L, Context *ctx);

  /// Calls a method of the Lua wrapper of the given context.
  static void Context_call (lua_State *L, Context *ctx,
                            const char *name, int nargs, int nresults);

  // Switch ----------------------------------------------------------------
public:

  /// Checks if the value at index \p i of stack is a Switch wrapper.
  static Switch *Switch_check (lua_State *L, int i);

  /// Media ----------------------------------------------------------------

  /// Checks if the value at index \p i of stack is a Media wrapper.
  static Media *Media_check (lua_State *L, int i);

  /// Pushes the Lua wrapper of media onto stack.
  static void Media_push (lua_State *L, Media *media);

  /// Calls a method of the Lua wrapper of the given media.
  static void Media_call (lua_State *L, Media *media,
                          const char *name, int nargs, int nresults);

  // StateMachine ----------------------------------------------------------
public:

  /// Attaches Lua wrapper to \p sm.
  static void StateMachine_attachWrapper (lua_State *L, StateMachine *sm,
                                          Object *obj,
                                          StateMachine::Type type,
                                          const string &id);

  /// Detaches Lua wrapper from \p sm.
  static void StateMachine_detachWrapper (lua_State *L, StateMachine *sm);

  /// Checks if the value at index \p i of stack is an StateMachine wrapper.
  static StateMachine *StateMachine_check (lua_State *L, int i);

  /// Checks if the value at index \p i of stack is an StateMachine::Type.
  static StateMachine::Type StateMachine_Type_check (lua_State *L, int i);

  /// Checks if the value at index \p i of stack is an StateMachine::State.
  static StateMachine::State StateMachine_State_check (lua_State *L, int i);

  /// Checks if the value at index \p i of stack is an
  /// StateMachine::Transition.
  static StateMachine::Transition StateMachine_Transition_check
  (lua_State *L, int i);

  /// Pushes the Lua wrapper of state machine onto stack.
  static void StateMachine_push (lua_State *L, StateMachine *sm);

  /// Pushes StateMachine::Type (as string) onto stack.
  static void StateMachine_Type_push (lua_State *L,
                                      StateMachine::Type type);

  /// Pushes StateMachine::State (as string) onto stack.
  static void StateMachine_State_push (lua_State *L,
                                       StateMachine::State state);

  /// Pushes StateMachine::Transition (as string) onto stack.
  static void StateMachine_Transition_push (lua_State *L,
                                            StateMachine::Transition trans);

  /// Calls a method of the Lua wrapper of the given state machine.
  static void StateMachine_call (lua_State *L, StateMachine *sm,
                                 const char *name, int nargs, int nresults);

  // Player ----------------------------------------------------------------
public:

  /// Attaches Lua wrapper to \p player.
  static void Player_attachWrapper (lua_State *L, Player *player,
                                    Media *media);

  /// Detaches Lua wrapper from \p player.
  static void Player_detachWrapper (lua_State *L, Player *player);

  /// Checks if the value at index \p i of stack is a Player wrapper.
  static Player *Player_check (lua_State *L, int i);

  /// Checks if the value at index \p i of stack is a Player::State.
  static Player::State Player_State_check (lua_State *L, int i);

  /// Pushes the Lua wrapper of player onto stack.
  static void Player_push (lua_State *L, Player *player);

  /// Pushes Player::State (as string) onto stack.
  static void Player_State_push (lua_State *L, Player::State state);

  /// Calls a method of the Lua wrapper of the given player.
  static void Player_call (lua_State *L, Player *player,
                           const char *name, int nargs, int nresults);

private:
  static int l_Player_getState (lua_State *L);
  static int l_Player_getURI (lua_State *L);
  static int l_Player_setURI (lua_State *L);
  static int l_Player_getRect (lua_State *L);
  static int l_Player_setRect (lua_State *L);
  static int l_Player_getEOS (lua_State *L);
  static int l_Player_setEOS (lua_State *L);
  static int l_Player_start (lua_State *L);
  static int l_Player_pause (lua_State *L);
  static int l_Player_stop (lua_State *L);

  // Auxiliary -------------------------------------------------------------
public:

  /// Converts the value at index \p i of stack to a GValue.
  static bool GValue_to (lua_State *L, int i, GValue *value);

  /// Pushes the given GValue onto stack.
  static bool GValue_push (lua_State *L, const GValue *value);

private:

  /// Datatype for loadable Lua chunks.
  typedef struct
  {
    const char *name;
    const char *text;
    size_t len;
  } Chunk;

#define LUAAPI_CHUNK_DECL(name)                 \
  static Chunk _##name;                         \
  static unsigned char name##_lua[];            \
  static unsigned int name##_lua_len;

#define LUAAPI_CHUNK_DEFN(name)                 \
  LuaAPI::Chunk LuaAPI::_##name =               \
    {G_STRINGIFY (LuaAPI_##name.lua),           \
     (const char *) LuaAPI::name##_lua,         \
     (size_t) LuaAPI::name##_lua_len}

  LUAAPI_CHUNK_DECL (Behavior);
  LUAAPI_CHUNK_DECL (Composition_initMt);
  LUAAPI_CHUNK_DECL (Context_initMt);
  LUAAPI_CHUNK_DECL (Document_initMt);
  LUAAPI_CHUNK_DECL (Media_initMt);
  LUAAPI_CHUNK_DECL (Object_initMt);
  LUAAPI_CHUNK_DECL (Player_initMt);
  LUAAPI_CHUNK_DECL (StateMachine_initMt);
  LUAAPI_CHUNK_DECL (initMt);
  LUAAPI_CHUNK_DECL (traceMt);

  /// Registry keys for the metatables.
  static const char *_DOCUMENT;
  static const char *_CONTEXT;
  static const char *_SWITCH;
  static const char *_MEDIA;
  static const char *_STATE_MACHINE;
  static const char *_PLAYER;

  /// The functions to load in all metatables.
  static const struct luaL_Reg _funcs[];

  static int _l_debug (lua_State *L);
  static int _l_warn (lua_State *L);
  static int _l_error (lua_State *L);

  /// Loads the metatable of a Lua wrapper (if not already loaded).
  ///
  /// @param L Lua state
  /// @param name The name for the metatable in LUA_REGISTRY.
  /// @param funcs A `NULL`-terminated array of luaL_Reg entries to install
  ///              or NULL (no entries to install).
  /// @param chunks A `NULL`-terminated array of Chunk or NULL (no chunks to
  ///              load).  If the array is given, each Lua chunk in it array
  ///              is run immediately after the metatable is loaded; each
  ///              receives the metatable as the first argument.
  /// @return \c true if the metatable was loaded in this call, or \c false
  ///         otherwise (metatable already loaded).
  ///
  static bool _loadLuaWrapperMt (lua_State *L, const char *name,
                                 const luaL_Reg *const funcs[],
                                 const Chunk *const chunks[]);

  /// Pops a value from the stack and sets it as the Lua wrapper of \p ptr.
  static void _attachLuaWrapper (lua_State *L, void *ptr);

  /// Detaches Lua wrapper from \p ptr.
  static void _detachLuaWrapper (lua_State *L, void *ptr);

  /// Pushes the Lua wrapper of \p ptr onto stack.
  static void _pushLuaWrapper (lua_State *L, void *ptr);

  /// Calls a method of the Lua wrapper of \p ptr.
  ///
  /// @param L Lua state.
  /// @param ptr The pointer whose wrapper is to be called.
  /// @param name The name of the method to call.
  /// @param nargs The number of arguments to the method.
  /// @param nresults The number of results of the method.
  ///
  static void _callLuaWrapper (lua_State *L, void *ptr,
                               const char *name, int nargs, int nresults);
};

GINGA_NAMESPACE_END

#endif // GINGA_LUA_API_H