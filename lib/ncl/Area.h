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

#ifndef AREA_H
#define AREA_H

#include "Anchor.h"

GINGA_NCL_BEGIN

class Area : public Anchor
{
public:
  Area (const string &, GingaTime, GingaTime);
  GingaTime getBegin ();
  GingaTime getEnd ();

protected:
  GingaTime _begin;
  GingaTime _end;
};

class AreaLambda : public Area
{
public:
  AreaLambda (const string &id) : Area (id, 0, GINGA_TIME_NONE) {};
};

GINGA_NCL_END

#endif // AREA_H