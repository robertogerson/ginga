/* Copyright (C) 1989-2017 PUC-Rio/Laboratorio TeleMidia

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

#ifndef _ANCHOREVENT_H_
#define _ANCHOREVENT_H_

#include "ncl/interfaces/ContentAnchor.h"
using namespace ::br::pucrio::telemidia::ncl::interfaces;

#include "FormatterEvent.h"

#include <string>
using namespace std;

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace ncl {
namespace model {
namespace event {
  class AnchorEvent : public FormatterEvent {
	protected:
		ContentAnchor* anchor;

	public:
		AnchorEvent(
		    string id,
		    void* executionObject,
		    ContentAnchor* anchor);

		virtual ~AnchorEvent();

		ContentAnchor* getAnchor();
		virtual bool start() {return FormatterEvent::start();};
		virtual bool stop() {return FormatterEvent::stop();};
  };
}
}
}
}
}
}
}

#endif //_ANCHOREVENT_H_