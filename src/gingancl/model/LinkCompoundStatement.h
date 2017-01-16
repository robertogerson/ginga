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

#ifndef _LINKCOMPOUNDSTATEMENT_H_
#define _LINKCOMPOUNDSTATEMENT_H_

#include "LinkStatement.h"

#include "FormatterEvent.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::model::event;

#include "ncl/connectors/CompoundStatement.h"
using namespace ::br::pucrio::telemidia::ncl::connectors;

#include <vector>
using namespace std;

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace ncl {
namespace model {
namespace link {
	class LinkCompoundStatement : public LinkStatement {
		protected:
			vector<LinkStatement*>* statements;
			bool negated;
			short op;

		public:
			LinkCompoundStatement(short op);
			virtual ~LinkCompoundStatement();
			short getOperator();
			void addStatement(LinkStatement* statement);
			vector<LinkStatement*>* getStatements();
			bool isNegated();
			void setNegated(bool neg);

		protected:
			bool returnEvaluationResult(bool result);

		public:
			virtual vector<FormatterEvent*>* getEvents();
			virtual bool evaluate();
	};
}
}
}
}
}
}
}

#endif //_LINKCOMPOUNDSTATEMENT_H_