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

#ifndef DESCRIPTOR_BASE_H
#define DESCRIPTOR_BASE_H

#include "Base.h"
#include "Descriptor.h"

GINGA_NCL_BEGIN

class DescriptorBase : public Base
{
public:
  DescriptorBase (const string &);
  virtual ~DescriptorBase ();

  void addDescriptor (Descriptor *);
  Descriptor *getDescriptor (const string &);
};

GINGA_NCL_END

#endif // DESCRIPTOR_BASE_H