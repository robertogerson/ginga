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

#include "RemoteDeviceManager.h"
#include "ActiveDeviceDomain.h"
#include "PassiveDeviceDomain.h"
#include "BaseDeviceDomain.h"

#include "util/functions.h"
using namespace ::br::pucrio::telemidia::util;

#include <iostream>
using namespace std;

using namespace ::br::pucrio::telemidia::ginga::core::multidevice;

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace multidevice {
	RemoteDeviceManager* RemoteDeviceManager::_instance = NULL;

	RemoteDeviceManager::RemoteDeviceManager() : Thread() {
		domainService = NULL;
		connecting    = false;
		running       = false;
		released      = false;
	}

	RemoteDeviceManager::~RemoteDeviceManager() {
		running = false;

		lock();
		connecting = false;

		if (domainService != NULL) {
			delete domainService;
			domainService = NULL;
		}

		unlock();
	}

	void RemoteDeviceManager::release() {
		running = false;
		connecting = false;
		released = true;

		/*
		if (_instance != NULL) {
			delete _instance;
			_instance = NULL;
		}
		*/
	}

	RemoteDeviceManager* RemoteDeviceManager::getInstance() {
		if (_instance == NULL) {
			_instance = new RemoteDeviceManager();
		}

		return _instance;
	}

	void RemoteDeviceManager::setDeviceDomain(IDeviceDomain* domain) {
		domainService = domain;
	}

	void RemoteDeviceManager::setDeviceInfo(
			int deviceClass, int width, int height, string base_device_ncl_path) {

		if (domainService != NULL) {
			domainService->setDeviceInfo(width, height, base_device_ncl_path);
			if ((!running)&&(!released)) {
				startThread();
			}
		}
	}

	int RemoteDeviceManager::getDeviceClass() {
		if (domainService == NULL) {
			return -1;
		}
		return domainService->getDeviceClass();
	}

	void RemoteDeviceManager::addListener(IRemoteDeviceListener* listener) {
		if (domainService != NULL) {
			domainService->addDeviceListener(listener);
		}
	}

	void RemoteDeviceManager::removeListener(IRemoteDeviceListener* listener) {
		if (domainService != NULL) {
			domainService->removeDeviceListener(listener);
		}
	}

	void RemoteDeviceManager::postEvent(
			int devClass,
			int eventType,
			char* event,
			int eventSize) {

		domainService->postEventTask(devClass, eventType, event, eventSize);
	}

	bool RemoteDeviceManager::postMediaContent(int destDevClass, string url) {
		return domainService->postMediaContentTask(destDevClass, url);
	}

	/*void RemoteDeviceManager::postNclMetadata(
			int devClass, vector<StreamData*>* streams) {

		domainService->postNclMetadata(devClass, streams);
	}*/

	void RemoteDeviceManager::run() {
		double rdmTimer;
		bool notifyWarning = true;

		rdmTimer   = 0;
		connecting = true;
		running    = true;

		lock();
		if (!running) {
			unlock();
			return;
		}

		while (running) {
			/*clog << "RemoteDeviceManager::run postConnectionRequestTask";
			clog << endl;*/

			//TODO: improve this loop
			if (domainService != NULL) {
				if (domainService->getDeviceClass() != 0) {
					domainService->postConnectionRequestTask();
				}
				domainService->checkDomainTasks();

			} else if (notifyWarning) {
				notifyWarning = false;
				clog << "RemoteDeviceManager::run Warning! domainService is ";
				clog << "NULL" << endl;
			}

			/*if (connecting) {
				if (domainService->isConnected()) {
					clog << "RemoteDeviceManager::run connected" << endl;
					connecting = false;

				} else if (rdmTimer == 0 ||
						((getCurrentTimeMillis() - rdmTimer) > 3000)) {

					rdmTimer = getCurrentTimeMillis();
					domainService->postConnectionRequestTask();
				}
			}*/
			//SystemCompat::uSleep(25000);
			SystemCompat::uSleep(80000);
		}
		unlock();

		clog << "RemoteDeviceManager::run All done!" << endl;
	}
}
}
}
}
}
}

extern "C" IRemoteDeviceManager* createRemoteDeviceManager(int devClass, bool useMulticast, int srvPort) {
	IDeviceDomain* domain = NULL;

	RemoteDeviceManager* rdm = RemoteDeviceManager::getInstance();

	if (devClass == IDeviceDomain::CT_BASE) {
		domain = new BaseDeviceDomain(useMulticast, srvPort);

	} else if (devClass == IDeviceDomain::CT_PASSIVE) {
		domain = new PassiveDeviceDomain(useMulticast, srvPort);

	} else if (devClass == IDeviceDomain::CT_ACTIVE) {
		domain = new ActiveDeviceDomain(useMulticast, srvPort);
	}

	rdm->setDeviceDomain(domain);
	return rdm;
}

extern "C" void destroyRemoteDeviceManager(
		::br::pucrio::telemidia::ginga::core::multidevice::
				IRemoteDeviceManager* rdm) {

	rdm->release();
}