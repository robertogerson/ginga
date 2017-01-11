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

#ifndef DocumentParser_H
#define DocumentParser_H

#include <iostream>
#include <string>
#include <map>
#include <vector>
using namespace std;

#include <xercesc/dom/DOM.hpp>
XERCES_CPP_NAMESPACE_USE

#include "util/functions.h"
using namespace ::br::pucrio::telemidia::util;

#include "XMLParsing.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace converter {
namespace framework {
  class DocumentParser {
	protected:
		string documentPath;
		string userCurrentPath;
		string documentUri;
		string iUriD;
		string fUriD;
		DOMDocument *documentTree;
		map<string, void*>* genericTable;

	public:
		DocumentParser();
		virtual ~DocumentParser();

	protected:
		virtual void initialize()=0;

	public:
		void* parse(string uri, string iUriD, string fUriD);
		void* parse(DOMElement* rootElement, string uri);

	protected:
		virtual void setDependencies();
		virtual void* parseRootElement(DOMElement *rootElement)=0;

	private:
		void initializeUserCurrentPath();
		string absoluteFile(string basePath, string filename);
		string getPath(string filename);

	public:
		string getIUriD();
		string getFUriD();
		string getUserCurrentPath();
		bool checkUriPrefix(string uri);
		bool isAbsolutePath(string path);
		bool isXmlStr(string location);

		string getAbsolutePath(string path);
		string getDocumentUri();
		string getDocumentPath();
		void setDocumentPath(string path);
		DOMDocument *getDocumentTree();
		void addObject(string tableName, string key, void* value);
		void* getObject(string tableName, string key);
		void removeObject(string tableName, string key);
		void addObjectGrouped(string tableName, string key, void* value);
		bool importDocument(DocumentParser* parser, string docLocation);
  };
}
}
}
}
}

#endif //DocumentParser_H