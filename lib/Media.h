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

#ifndef GINGA_MEDIA_H
#define GINGA_MEDIA_H

#include "Object.h"
#include "Player.h"

GINGA_NAMESPACE_BEGIN

class Media : public Object
{
public:

  /**
   * @brief Creates a new media object.
   * @see Object::Object().
   */
  Media (Document *doc,
         Composition *parent,
         const string &id);
  /**
   * @brief Destroys media object.
   * @see Object::~Object().
   */
  ~Media ();

  /**
   * @brief Gets a string representation of media object.
   * @see Object::toString().
   */
  string toString () override;

  /**
   * @brief Gets the type of media object.
   * @return Object::MEDIA.
   * @see Object::getType().
   */
  Object::Type getType () override;

  // TODO

  void setProperty (const string &, const string &, Time dur = 0) override;
  void sendKey (const string &, bool) override;
  void sendTick (Time, Time, Time) override;
  bool beforeTransition (Event *, Event::Transition) override;
  bool afterTransition (Event *, Event::Transition) override;

  // Media:
  virtual bool isFocused ();
  virtual bool getZ (int *, int *);
  virtual void redraw (cairo_t *);

protected:
  Player *_player; // underlying player

  void doStop () override;
};

GINGA_NAMESPACE_END

#endif // GINGA_MEDIA_H
