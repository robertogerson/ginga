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

#ifndef _FORMATTERSCHEDULER_H_
#define _FORMATTERSCHEDULER_H_

#include "ctxmgmt/IContextListener.h"
using namespace ::ginga::ctxmgmt;

#include "model/ExecutionObject.h"
#include "model/NodeNesting.h"
#include "model/CompositeExecutionObject.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::model::components;

#include "model/ExecutionObjectSwitch.h"
#include "model/SwitchEvent.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::model::switches;

#include "model/AttributionEvent.h"
#include "model/IEventListener.h"
#include "model/FormatterEvent.h"
#include "model/PresentationEvent.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::model::event;

#include "model/LinkAssignmentAction.h"
#include "model/LinkSimpleAction.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::model::link;

#include "model/FormatterLayout.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::model::presentation;

#include "RuleAdapter.h"
#include "PresentationContext.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::adaptation::context;

#include "AdapterApplicationPlayer.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::adapters::application;

#include "AdapterFormatterPlayer.h"
#include "AdapterPlayerManager.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::adapters;

#include "ncl/SimpleAction.h"
#include "ncl/EventUtil.h"
using namespace ::ginga::ncl;

#include "ncl/CompositeNode.h"
#include "ncl/ContentNode.h"
#include "ncl/Node.h"
#include "ncl/NodeEntity.h"
using namespace ::ginga::ncl;

#include "ncl/Port.h"
#include "ncl/Anchor.h"
#include "ncl/ContentAnchor.h"
#include "ncl/PropertyAnchor.h"
#include "ncl/SwitchPort.h"
using namespace ::ginga::ncl;

#include "ncl/ReferNode.h"
using namespace ::ginga::ncl;

#include "FormatterFocusManager.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::focus;

#include "FormatterMultiDevice.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::multidevice;

#include "IFormatterSchedulerListener.h"
#include "ObjectCreationForbiddenException.h"

#include "AnimationController.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::animation;

BR_PUCRIO_TELEMIDIA_GINGA_NCL_BEGIN

class FormatterScheduler : public ILinkActionListener,
                           public IEventListener,
                           public IContextListener
{

private:
  RuleAdapter *ruleAdapter;
  AdapterPlayerManager *playerManager;
  PresentationContext *presContext;
  FormatterMultiDevice *multiDevPres;
  FormatterFocusManager *focusManager;

  void *compiler; // FormatterConverter*
  vector<IFormatterSchedulerListener *> schedulerListeners;
  vector<FormatterEvent *> documentEvents;
  map<FormatterEvent *, bool> documentStatus;
  set<void *> actions;

  bool running;

  set<string> typeSet;
  pthread_mutex_t mutexD;
  pthread_mutex_t mutexActions;

  set<FormatterEvent *> listening;
  pthread_mutex_t lMutex;

public:
  FormatterScheduler (AdapterPlayerManager *playerManager,
                      RuleAdapter *ruleAdapter,
                      FormatterMultiDevice *multiDevice,
                      void *compiler); // FormatterConverter

  virtual ~FormatterScheduler ();

  void addAction (void *action);
  void removeAction (void *action);

  bool setKeyHandler (bool isHandler);
  FormatterFocusManager *getFocusManager ();
  void *getFormatterLayout (void *descriptor, void *object);

private:
  bool isDocumentRunning (FormatterEvent *event);

  void setTimeBaseObject (ExecutionObject *object,
                          AdapterFormatterPlayer *objectPlayer,
                          string nodeId);

  static void printAction (string action, LinkCondition *condition,
                           LinkSimpleAction *linkAction);

public:
  void scheduleAction (void *condition, void *action);

private:
  void runAction (LinkCondition *condition, LinkSimpleAction *action);

  void runAction (FormatterEvent *event, LinkCondition *condition,
                  LinkSimpleAction *action);

  void runActionOverProperty (FormatterEvent *event,
                              LinkSimpleAction *action);

  void runActionOverApplicationObject (
      ApplicationExecutionObject *executionObject, FormatterEvent *event,
      AdapterFormatterPlayer *player, LinkSimpleAction *action);

  void runActionOverComposition (CompositeExecutionObject *compositeObject,
                                 LinkSimpleAction *action);

  void runActionOverSwitch (ExecutionObjectSwitch *switchObject,
                            SwitchEvent *event, LinkSimpleAction *action);

  void runSwitchEvent (ExecutionObjectSwitch *switchObject,
                       SwitchEvent *switchEvent,
                       ExecutionObject *selectedObject,
                       LinkSimpleAction *action);

  string solveImplicitRefAssessment (string propValue,
                                     AttributionEvent *event);

public:
  void startEvent (FormatterEvent *event);
  void stopEvent (FormatterEvent *event);
  void pauseEvent (FormatterEvent *event);
  void resumeEvent (FormatterEvent *event);

private:
  void initializeDefaultSettings ();
  void initializeDocumentSettings (Node *node);

public:
  void startDocument (FormatterEvent *documentEvent,
                      vector<FormatterEvent *> *entryEvents);

private:
  void removeDocument (FormatterEvent *documentEvent);

public:
  void stopDocument (FormatterEvent *documentEvent);
  void pauseDocument (FormatterEvent *documentEvent);
  void resumeDocument (FormatterEvent *documentEvent);
  void stopAllDocuments ();
  void pauseAllDocuments ();
  void resumeAllDocuments ();
  void eventStateChanged (void *someEvent, short transition,
                          short previousState);

  short getPriorityType ();
  void addSchedulerListener (IFormatterSchedulerListener *listener);
  void removeSchedulerListener (IFormatterSchedulerListener *listener);
  void receiveGlobalAttribution (string propertyName, string value);
};

BR_PUCRIO_TELEMIDIA_GINGA_NCL_END
#endif //_FORMATTERSCHEDULER_H_
