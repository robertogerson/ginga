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

#ifndef FORMATTERCONVERTER_H_
#define FORMATTERCONVERTER_H_

#include "NclExecutionObjectSwitch.h"
#include "NclSwitchEvent.h"

#include "NclAttributionEvent.h"
#include "NclPresentationEvent.h"
#include "NclSelectionEvent.h"
#include "INclEventListener.h"
#include "NclFormatterEvent.h"
#include "NclPresentationEvent.h"

#include "NclFormatterCausalLink.h"
#include "NclFormatterLink.h"
#include "NclLinkAction.h"
#include "NclLinkCompoundAction.h"
#include "NclLinkSimpleAction.h"

#include "NclCascadingDescriptor.h"

#include "NclCompositeExecutionObject.h"
#include "NclApplicationExecutionObject.h"
#include "NclExecutionObject.h"

#include "RuleAdapter.h"

GINGA_FORMATTER_BEGIN

class FormatterLinkConverter;
class FormatterScheduler;

class FormatterConverter : public INclEventListener
{
public:
  explicit FormatterConverter (RuleAdapter *);
  virtual ~FormatterConverter ();

  void setHandlingStatus (bool handling);
  NclExecutionObject *getObjectFromNodeId (const string &id);

  void setLinkActionListener (INclLinkActionListener *actionListener);

  NclExecutionObject *getExecutionObjectFromPerspective (
      NclNodeNesting *perspec, GenericDescriptor *desc);

  set<NclExecutionObject *> *getSettingNodeObjects ();

  NclFormatterEvent *getEvent (NclExecutionObject *exeObj,
                               InterfacePoint *interfacePoint,
                               int ncmEventType,
                               const string &key);

  NclExecutionObject *
  processExecutionObjectSwitch (NclExecutionObjectSwitch *switchObject);

  NclFormatterEvent *insertContext (NclNodeNesting *contextPerspective,
                                    Port *port);

private:
  static int _dummyCount;
  map<string, NclExecutionObject *> _executionObjects;
  set<NclFormatterEvent *> _listening;
  set<NclExecutionObject *> _settingObjects;
  FormatterLinkConverter *_linkCompiler;
  INclLinkActionListener *_actionListener;
  RuleAdapter *_ruleAdapter;
  bool _handling;

  void addExecutionObject (NclExecutionObject *exeObj,
                           NclCompositeExecutionObject *parentObj);

  bool removeExecutionObject (NclExecutionObject *exeObj);

  NclCompositeExecutionObject *
  addSameInstance (NclExecutionObject *exeObj, ReferNode *referNode);

  NclCompositeExecutionObject *getParentExecutionObject (
      NclNodeNesting *perspective);

  NclExecutionObject *
  createExecutionObject (const string &id, NclNodeNesting *perspective,
                         NclCascadingDescriptor *descriptor);

  void compileExecutionObjectLinks (NclExecutionObject *exeObj, Node *dataObj,
                                    NclCompositeExecutionObject *parentObj);

  void processLink (Link *ncmLink,
                    Node *dataObject,
                    NclExecutionObject *exeObj,
                    NclCompositeExecutionObject *parentObj);

  void setActionListener (NclLinkAction *action);

  void resolveSwitchEvents (NclExecutionObjectSwitch *switchObject);

  NclFormatterEvent *insertNode (NclNodeNesting *perspective,
                                 InterfacePoint *interfacePoint,
                                 GenericDescriptor *descriptor);

  void eventStateChanged (NclFormatterEvent *someEvent, short transition,
                          short previousState) override;

  static Descriptor *createDummyDescriptor (Node *node);
  static NclCascadingDescriptor *createDummyCascadingDescriptor (Node *node);

  static NclCascadingDescriptor *
  getCascadingDescriptor (NclNodeNesting *nodePerspective,
                          GenericDescriptor *descriptor);

  static NclCascadingDescriptor *checkCascadingDescriptor (Node *node);
  static NclCascadingDescriptor *checkContextCascadingDescriptor (
      NclNodeNesting *nodePerspective,
      NclCascadingDescriptor *cascadingDescriptor, Node *ncmNode);

  static bool hasDescriptorPropName (const string &name);
  static bool isEmbeddedApp (NodeEntity *dataObject);
  static bool isEmbeddedAppMediaType (const string &mediaType);
};

GINGA_FORMATTER_END

#endif /*FORMATTERCONVERTER_H_*/
