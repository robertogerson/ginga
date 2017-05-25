/* Copyright (C) 2006-2017 PUC-Rio/Laboratorio TeleMidia

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
along with Ginga.  If not, see <http://www.gnu.org/licenses/>.  */

#include "ginga.h"
#include "Entity.h"

GINGA_NCL_BEGIN

set<Entity *> Entity::instances;

Entity::Entity (const string &id)
{
  this->id = id;
  typeSet.insert ("Entity");
  instances.insert (this);
}

Entity::~Entity ()
{
  set<Entity *>::iterator i;

  i = instances.find (this);
  if (i != instances.end ())
    instances.erase (i);
}

bool
Entity::hasInstance (Entity *instance, bool eraseFromList)
{
  set<Entity *>::iterator i;
  bool hasEntity = false;

  i = instances.find (instance);
  if (i != instances.end ())
    {
      if (eraseFromList)
        {
          instances.erase (i);
        }
      hasEntity = true;
    }

  return hasEntity;
}

bool
Entity::instanceOf (const string &s)
{
  if (!typeSet.empty ())
    {
      return (typeSet.find (s) != typeSet.end ());
    }
  else
    {
      return false;
    }
}

int
Entity::compareTo (Entity *otherEntity)
{
  string otherId;
  int cmp;

  otherId = (static_cast<Entity *> (otherEntity))->getId ();

  if (id == "")
    return -1;

  if (otherId == "")
    return 1;

  cmp = id.compare (otherId);
  switch (cmp)
    {
    case 0:
      return 0;
    default:
      if (cmp < 0)
        return -1;
      else
        return 1;
    }
}

string
Entity::getId ()
{
  return id;
}

void
Entity::setId (const string &someId)
{
  id = someId;
}

Entity *
Entity::getDataEntity ()
{
  return this;
}

GINGA_NCL_END
