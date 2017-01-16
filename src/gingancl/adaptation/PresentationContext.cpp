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

#include "config.h"
#include "PresentationContext.h"
#include "ctxmgmt/ContextManager.h"
#include "ctxmgmt/SystemInfo.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace ncl {
namespace adaptation {
namespace context {
	ContextManager* PresentationContext::contextManager = NULL;

	PresentationContext::PresentationContext(GingaScreenID screenId) {
		createObserversVector();

		myScreen = screenId;

		if (contextManager == NULL) {
			PresentationContext::contextManager = ContextManager::getInstance();
		}

		contextManager->addContextListener(this);

		initializeUserContext();
		initializeSystemValues();

		globalVarListener = NULL;
		devListener = NULL;

		Thread::mutexInit(&attrMutex, NULL);
	}

	PresentationContext::~PresentationContext() {
		contextManager->removeContextListener(this);

		contextTable.clear();
	}

	void PresentationContext::setPropertyValue(
		    string property, string value) {

		string oldValue = "";
		map<string, string>::iterator i;

		Thread::mutexLock(&attrMutex);
		if ((property.length() >= 7 && property.substr(0, 7) == "system.") ||
				(property.length() >= 5 && property.substr(0, 5) == "user.")) {

			Thread::mutexUnlock(&attrMutex);
			return;
		}

		clog << "PresentationContext::setPropertyValue propName = '";
		clog << property << "' propValue = '" << value << "'" << endl;
		i = contextTable.find(property);

		if (i != contextTable.end()) {
			oldValue = i->second;
		}

		contextTable[property] = value;
		Thread::mutexUnlock(&attrMutex);

		if ((value != "") && (value != oldValue)) {
			notifyObservers(&property);
		}

		if (devListener != NULL) {
#if WITH_MULTIDEVICE
			//clog << "PresentationContext::set(devListener != NULL)" << endl;
			int myDevClass = -1;
			string evPayload = property + " = " + value;
			devListener->receiveRemoteEvent(myDevClass,
					DeviceDomain::FT_ATTRIBUTIONEVENT,
					evPayload);
#endif //WITH_MULTIDEVICE
		}

	}

	void PresentationContext::incPropertyValue(string propertyName) {
		string oldValue = "", newValue = "";
		map<string, string>::iterator i;

		i = contextTable.find(propertyName);
		if (i != contextTable.end()) {
			oldValue = i->second;
		}

		if (oldValue == "") {
			return;
		}

		newValue = itos(util::stof(oldValue) + 1);
		if ((newValue != "") && (newValue != oldValue)) {
			contextTable[propertyName] = newValue;
			notifyObservers(&propertyName);
		}
	}

	void PresentationContext::decPropertyValue(string propertyName) {
		string oldValue = "", newValue = "";
		map<string, string>::iterator i;

		i = contextTable.find(propertyName);
		if (i != contextTable.end()) {
			oldValue = i->second;
		}

		if (oldValue == "") {
			return;
		}

		newValue = itos(util::stof(oldValue) - 1);
		if ((newValue != "") && (newValue != oldValue)) {
			contextTable[propertyName] = newValue;
			notifyObservers(&propertyName);
		}
	}

	set<string>* PresentationContext::getPropertyNames() {
		set<string>* propertyNames = new set<string>;
		map<string, string>::iterator i;
		for (i = contextTable.begin(); i != contextTable.end(); ++i) {
			propertyNames->insert(i->first);
		}

		return propertyNames;
	}

	string PresentationContext::getPropertyValue(string attributeId) {
		string propValue;

		Thread::mutexLock(&attrMutex);
		if (contextTable.count(attributeId) == 0) {
			clog << "PresentationContext::getPropertyValue prop '";
			clog << attributeId << "' has a NULL value" << endl;
			Thread::mutexUnlock(&attrMutex);
			return "";
		}

		propValue = contextTable[attributeId];
		Thread::mutexUnlock(&attrMutex);

		clog << "PresentationContext::getPropertyValue prop '";
		clog << attributeId << "' == '" << propValue;
		clog << "'" << endl;

		return propValue;
	}

	void PresentationContext::initializeUserContext() {
		int currentUserId;
		map<string, string>* userProfile = NULL;

		currentUserId = PresentationContext::contextManager->getCurrentUserId();
		userProfile   = PresentationContext::contextManager->getUserProfile(
				currentUserId);

		if (userProfile == NULL) {
			PresentationContext::contextManager->addContextVar(
					currentUserId, "system.background-color", "000000");

			userProfile = PresentationContext::contextManager->getUserProfile(
					currentUserId);
		}

		contextTable.clear();
		contextTable.insert(userProfile->begin(), userProfile->end());

		delete userProfile;

		initializeUserInfo(currentUserId);
	}

	void PresentationContext::initializeUserInfo(int currentUserId) {
		GingaUser* user;

		user = PresentationContext::contextManager->getUser(currentUserId);
		if (user != NULL) {
			contextTable[USER_AGE]      = itos(user->getUserAge());
			contextTable[USER_LOCATION] = user->getUserLocation();
			contextTable[USER_GENRE]    = user->getUserGenre();
		}
	}

	void PresentationContext::initializeSystemValues() {
		SystemInfo* si;
		int w, h;

		//clog << "PresentationContext::initializeSystemValues " << endl;
		si = PresentationContext::contextManager->getSystemInfo();

		contextTable[SYSTEM_LANGUAGE]        = si->getSystemLanguage();
		contextTable[SYSTEM_CAPTION]         = si->getCaptionLanguage();
		contextTable[SYSTEM_SUBTITLE]        = si->getSubtitleLanguage();
		contextTable[SYSTEM_RETURN_BIT_RATE] = itos(si->getReturnBitRate());

		si->getScreenSize(myScreen, &w, &h);
		contextTable[SYSTEM_SCREEN_SIZE] = itos(w) + "," + itos(h);

		si->getScreenGraphicSize(myScreen, &w, &h);
		contextTable[SYSTEM_SCREEN_GRAPHIC_SIZE] = itos(w) + "," + itos(h);
		contextTable[SYSTEM_AUDIO_TYPE]          = si->getAudioType();
		contextTable[SYSTEM_CPU]                 = itos(si->getCPUClock());
		contextTable[SYSTEM_MEMORY]              = itos(si->getMemorySize());
		contextTable[SYSTEM_OPERATING_SYSTEM]    = si->getOperatingSystem();

		contextTable[SYSTEM_DEVNUMBER + "(0)"] = "0";
		contextTable[SYSTEM_DEVNUMBER + "(1)"] = "0";
		contextTable[SYSTEM_DEVNUMBER + "(2)"] = "0";

		contextTable[SYSTEM_CLASSTYPE + "(0)"] = "base";
		contextTable[SYSTEM_CLASSTYPE + "(1)"] = "passive";
		contextTable[SYSTEM_CLASSTYPE + "(2)"] = "active";

		contextTable[SYSTEM_INFO + "(0)"] = "0";
		contextTable[SYSTEM_INFO + "(1)"] = "1";
		contextTable[SYSTEM_INFO + "(2)"] = "2";

		contextTable[SYSTEM_CLASS_NUMBER] = "3";

		if (contextTable.count(DEFAULT_FOCUS_BORDER_COLOR) == 0) {
			contextTable[DEFAULT_FOCUS_BORDER_COLOR] = "blue";
		}

		if (contextTable.count(DEFAULT_SEL_BORDER_COLOR) == 0) {
			contextTable[DEFAULT_SEL_BORDER_COLOR] = "green";
		}

		if (contextTable.count(DEFAULT_FOCUS_BORDER_WIDTH) == 0) {
			contextTable[DEFAULT_FOCUS_BORDER_WIDTH] = "3";
		}

		if (contextTable.count(DEFAULT_FOCUS_BORDER_TRANSPARENCY) == 0) {
			contextTable[DEFAULT_FOCUS_BORDER_TRANSPARENCY] = "0";
		}
	}

	void PresentationContext::save() {
		string property;
		map<string, string>::iterator i;

		//contextManager->saveUsersAccounts();
		PresentationContext::contextManager->saveUsersProfiles();
	}

	void PresentationContext::setGlobalVarListener(IContextListener* listener) {
		globalVarListener = listener;
	}

	void PresentationContext::receiveGlobalAttribution(
			string propertyName, string value) {

		if (globalVarListener != NULL) {
			globalVarListener->receiveGlobalAttribution(propertyName, value);
		}
	}

#if WITH_MULTIDEVICE
	void PresentationContext::setRemoteDeviceListener(IRemoteDeviceListener* rdl) {
		devListener = rdl;
	}
#endif
}
}
}
}
}
}
}