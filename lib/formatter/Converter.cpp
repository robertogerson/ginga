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

#include "ginga-internal.h"
#include "Converter.h"

#include "NclActions.h"
#include "NclLinkStatement.h"
#include "NclLinkAssessment.h"

GINGA_FORMATTER_BEGIN

int Converter::_dummyCount = 0;

static const string SEPARATOR = "/";

Converter::Converter (RuleAdapter *ruleAdapter)
{
  this->_actionListener = nullptr;
  this->_ruleAdapter = ruleAdapter;
  this->_handling = false;
}

Converter::~Converter ()
{
  for (NclEvent *evt: _listening)
    {
      if (NclEvent::hasInstance (evt, false))
        {
          evt->removeListener (this);
        }
    }

  for (auto i = _exeObjects.begin (); i != _exeObjects.end (); )
    {
      ExecutionObject *object = i->second;

      if (!removeExecutionObject (object))
        {
          i = _exeObjects.erase (i);
        }
      else
        {
          ++i;
        }
    }

  _exeObjects.clear ();
  _settingsObjects.clear ();
}

void
Converter::setLinkActionListener (INclActionListener *actListener)
{
  this->_actionListener = actListener;
}

ExecutionObject *
Converter::getExecutionObjectFromPerspective (
    NclNodeNesting *perspective, Descriptor *descriptor)
{
  ExecutionObjectContext *parentObj;
  ExecutionObject *exeObj;

  string id = perspective->getId () + SEPARATOR;

  auto i = _exeObjects.find (id);
  if (i != _exeObjects.end ())
    {
      exeObj = i->second;
      return exeObj;
    }

  parentObj = getParentExecutionObject (perspective);
  exeObj = createExecutionObject (id, perspective, descriptor);
  g_assert_nonnull (exeObj);

  addExecutionObject (exeObj, parentObj);

  return exeObj;
}

NclEvent *
Converter::getEvent (ExecutionObject *exeObj,
                     Anchor *interfacePoint,
                     EventType ncmEventType,
                     const string &key)
{
  string id;
  NclEvent *event;
  string type;

  xstrassign (type, "%d", (int) ncmEventType);

  if (key == "")
    {
      id = interfacePoint->getId () + "_" + type;
    }
  else
    {
      id = interfacePoint->getId () + "_" + type + "_" + key;
    }

  event = exeObj->getEvent (id);
  if (event != nullptr)
    {
      return event;
    }

  auto switchObj = cast (ExecutionObjectSwitch *, exeObj);
  auto cObj = cast (ExecutionObjectContext *, exeObj);

  if (switchObj)
    {
      event = new SwitchEvent (
            id, switchObj, interfacePoint, ncmEventType, key);
    }
  else if (ncmEventType == EventType::PRESENTATION)
    {
      event = new PresentationEvent (
            id, exeObj, (Area *)interfacePoint);
    }
  else if (cObj)
    {
      // TODO: eventos internos da composicao estao sendo tratados nos elos.
      if (ncmEventType == EventType::ATTRIBUTION)
        {
          auto propAnchor = cast (Property *, interfacePoint);
          if (propAnchor)
            {
              event = new AttributionEvent (
                    id, exeObj,
                    (Property *)interfacePoint,
                    _ruleAdapter->getSettings ());
            }
          else
            {
              WARNING ("NCM event type is attribution, but interface point "
                       "isn't");

              event = new AttributionEvent (
                    id, exeObj, nullptr, _ruleAdapter->getSettings ());
            }
        }
    }
  else
    {
      switch (ncmEventType)
        {
        case EventType::ATTRIBUTION:
          {
            auto propAnchor = cast (Property *, interfacePoint);
            if (propAnchor)
              {
                event = new AttributionEvent (
                      id, exeObj, propAnchor,
                      _ruleAdapter->getSettings ());
              }
            else
              {
                WARNING ("NCM event type is attribution, but interface point "
                         "isn't.");

                auto intervalAnchor
                    = cast (Area *, interfacePoint);
                if (intervalAnchor)
                  {
                    WARNING ("It was supposed to be a PRESENTATION EVENT");

                    // TODO: find the correct way to solve this
                    event = new PresentationEvent (
                          id, exeObj,
                          intervalAnchor);
                  }

                return nullptr;
              }
          }
          break;

        case EventType::SELECTION:
          {
            event = new SelectionEvent (
                  id, exeObj, (Area *)interfacePoint);

            if (key != "")
              {
                ((SelectionEvent *)event)->setSelectionCode (key);
              }
          }
          break;

        default:
          g_assert_not_reached ();
        }
    }


  if (event != nullptr)
    {
      exeObj->addEvent (event);
    }
  else
    {
      ERROR ("Returning a nullptr event for '%s'", id.c_str ());
    }

  return event;
}

ExecutionObjectContext *
Converter::addSameInstance (ExecutionObject *exeObj,
                            Refer *referNode)
{
  vector<Node *> *ncmPerspective = referNode->getPerspective ();
  NclNodeNesting *referPerspective = new NclNodeNesting (ncmPerspective);

  ExecutionObjectContext *referParentObject
      = getParentExecutionObject (referPerspective);

  if (referParentObject != nullptr)
    {
      exeObj->addParentObject (
            referNode,
            referParentObject,
            referPerspective->getNode (referPerspective->getNumNodes () - 2));

      referParentObject->addExecutionObject (exeObj);

      // A new entry for the execution object is inserted using
      // the refer node id.  As a consequence, links referring to the
      // refer node will generate events in the execution object.
      // Descriptor *desc = exeObj->getDescriptor ();

      // string objectId;
      // if (desc)
      //   {
      //     objectId = (referPerspective->getId () + SEPARATOR
      //                 + exeObj->getDescriptor ()->getId ());
      //   }
      // else
      //   {
      string objectId = referPerspective->getId ();
        // }
      _exeObjects[objectId] = exeObj;
    }

  delete ncmPerspective;
  delete referPerspective;

  return referParentObject;
}

void
Converter::addExecutionObject (ExecutionObject *exeObj,
                               ExecutionObjectContext *parentObj)
{
  _exeObjects[exeObj->getId ()] = exeObj;

  if (parentObj)
    {
      parentObj->addExecutionObject (exeObj);
    }

  // Hanlde settings nodes.
  Node *dataObject = exeObj->getNode ();
  auto contentNode = cast (Media *, dataObject);

  if (contentNode != nullptr && contentNode->isSettings ())
    _settingsObjects.insert (exeObj);

  auto referNode = cast (Refer *, dataObject);
  if (referNode)
    {

      Media *media = referNode->getReferred ();
      g_assert_nonnull (media);
      if (media->isSettings ())
        _settingsObjects.insert (exeObj);
    }

  NclNodeNesting *nodePerspective = exeObj->getNodePerspective ();
  Node *headNode = nodePerspective->getHeadNode ();

  auto nodeEntity = cast (Media *, dataObject);
  auto headComposition = cast (Composition *, headNode);
  if (headComposition != nullptr && nodeEntity != nullptr)
    {
      const set<Refer *> *sameInstances
        = nodeEntity->getInstSameInstances ();
      g_assert_nonnull (sameInstances);

      for (Refer *referNode: *(sameInstances))
        {
          TRACE ("'%s' instSame of '%s'",
                 exeObj->getId ().c_str(),
                 referNode->getId ().c_str());
          addSameInstance (exeObj, referNode);
        }
    }

  delete nodePerspective;

  // Compile execution object links
  for (Node *node : exeObj->getNodes ())
    {
      auto parent = cast (ExecutionObjectContext*, 
            exeObj->getParentObject (node));

      g_assert_nonnull (parent);

      compileExecutionObjectLinks (exeObj, node, parent);
    }
}

bool
Converter::removeExecutionObject (ExecutionObject *exeObj)
{
  bool removed = false;

  auto i = _exeObjects.find (exeObj->getId ());

  if (i != _exeObjects.end ())
    {
      _exeObjects.erase (i);
      removed = true;
    }

  if (_settingsObjects.count (exeObj))
    {
      _settingsObjects.erase (_settingsObjects.find (exeObj));
      removed = true;
    }

  if (removed)
    {
      delete exeObj;
    }

  return removed;
}

ExecutionObjectContext *
Converter::getParentExecutionObject (NclNodeNesting *perspective)
{
  NclNodeNesting *parentPerspective;

  if (perspective->getNumNodes () > 1)
    {
      parentPerspective = perspective->copy ();
      parentPerspective->removeAnchorNode ();

      auto cObj = cast (ExecutionObjectContext*, 
            this->getExecutionObjectFromPerspective (
              parentPerspective, nullptr));

      g_assert_nonnull (cObj);

      delete parentPerspective;

      return cObj;
    }

  return nullptr;
}

ExecutionObject *
Converter::createExecutionObject (
    const string &id, NclNodeNesting *perspective,
    Descriptor *descriptor)
{
  Node *node;
  NclNodeNesting *nodePerspective;
  ExecutionObject *exeObj;
  PresentationEvent *compositeEvt;


  node = perspective->getAnchorNode ();

  Node *nodeEntity = cast (Node *, node->derefer ());
  g_assert_nonnull (nodeEntity);

  // solve execution object cross reference coming from refer nodes with
  // new instance = false
  auto contentNode = cast (Media *, nodeEntity);
  if (contentNode != nullptr && !contentNode->isSettings ())
    {
      auto referNode = cast (Refer *, node);
      if (referNode)
        {
          nodePerspective
            = new NclNodeNesting (nodeEntity->getPerspective ());

          // verify if both nodes are in the same base.
          if (nodePerspective->getHeadNode ()
              == perspective->getHeadNode ())
            {
              exeObj = getExecutionObjectFromPerspective (nodePerspective,
                                                          descriptor);
              if (exeObj == nullptr)
                {
                  g_assert_nonnull (nodeEntity);
                  exeObj  = new ExecutionObject
                    (id, nodeEntity, _actionListener);
                }
            }
          else
            {
              g_assert_nonnull (nodeEntity);
              exeObj = new ExecutionObject
                (id, nodeEntity, _actionListener);
            }

          delete nodePerspective;

          if (exeObj != nullptr)
            {
              return exeObj;
            }
        }
    }

  auto switchNode = cast (Switch *, nodeEntity);
  if (switchNode)
    {
      string s;
      g_assert_nonnull (node);
      exeObj = new ExecutionObjectSwitch (id, node,
                                          _actionListener);
      xstrassign (s, "%d", (int) EventType::PRESENTATION);
      compositeEvt = new PresentationEvent (
            nodeEntity->getLambda ()->getId () + "_" + s,
            exeObj,
            (Area *)(nodeEntity->getLambda ()));

      exeObj->addEvent (compositeEvt);
      // to monitor the switch presentation and clear the selection after
      // each execution
      compositeEvt->addListener (this);
      _listening.insert (compositeEvt);
    }
  else if (instanceof (Composition* , nodeEntity))
    {
      string s;
      g_assert_nonnull (node);
      exeObj = new ExecutionObjectContext (id, node, _actionListener);

      xstrassign (s, "%d", (int) EventType::PRESENTATION);
      compositeEvt = new PresentationEvent (
            nodeEntity->getLambda ()->getId () + "_" + s,
            exeObj,
            (Area *)(nodeEntity->getLambda ()));

      exeObj->addEvent (compositeEvt);

      // to monitor the presentation and remove object at stops
      // compositeEvent->addEventListener(this);
    }
  else
    {
      g_assert_nonnull (node);
      exeObj = new ExecutionObject (id, node, _actionListener);
    }

  return exeObj;
}

void
Converter::processLink (Link *ncmLink,
                        Node *dataObject,
                        ExecutionObject *executionObject,
                        ExecutionObjectContext *parentObject)
{
  Node *nodeEntity = nullptr;
  const set<Refer *> *sameInstances;
  bool contains = false;

  if (executionObject->getNode () != nullptr)
    {
      nodeEntity
          = cast (Node *, executionObject->getNode ());
    }

  // Since the link may be removed in a deepest compilation it is necessary to
  // check that the link was not compiled.
  if (parentObject->containsUncompiledLink (ncmLink))
    {
      auto causalLink = cast (Link *, ncmLink);
      if (causalLink)
        {
          if (nodeEntity != nullptr && instanceof (Media *, nodeEntity))
            {
              sameInstances = cast (Media *, nodeEntity)
                ->getInstSameInstances ();
              for (Refer *referNode: *sameInstances)
                {
                  contains = causalLink->contains (referNode, true);
                  if (contains)
                    {
                      break;
                    }
                }
            }

          // Checks if execution object is part of link conditions.
          if (causalLink->contains (dataObject, true)
              || contains)
            {
              // Compile causal link.
              parentObject->removeLinkUncompiled (ncmLink);
              NclFormatterLink *formatterLink
                  = createLink (causalLink, parentObject);

              if (formatterLink != NULL)
                {
                  setActionListener (
                        ((NclFormatterLink *)formatterLink)
                        ->getAction ());

                  parentObject->setLinkCompiled (formatterLink);
                  TRACE ("link compiled '%s'",
                         ncmLink->getId ().c_str());
                }
            }
          else
            {
              WARNING ("cannot process ncmLink '%s' inside '%s' "
                       "because '%s' does not contain '%s' src",
                       ncmLink->getId ().c_str(),
                       parentObject->getId ().c_str(),
                       ncmLink->getId ().c_str(),
                       dataObject->getId ().c_str());
            }
        }
      else
        {
          WARNING ("cannot process ncmLink '%s' inside '%s' "
                   "because it isn't a causal link",
                   ncmLink->getId ().c_str (),
                   parentObject->getId ().c_str ());
        }
    }
  else
    {
      WARNING ("cannot process ncmLink '%s' inside '%s' "
               "because link may be removed in a deepest compilation.",
               ncmLink->getId ().c_str (),
               parentObject->getId ().c_str());
    }
}

void
Converter::compileExecutionObjectLinks (
    ExecutionObject *exeObj, Node *dataObject,
    ExecutionObjectContext *parentObj)
{
  set<Link *> *uncompiledLinks;
  ExecutionObjectContext *compObj;
  Node *execDataObject;

  exeObj->setCompiled (true);

  if (parentObj == nullptr)
    {
      return;
    }

  execDataObject = exeObj->getNode ();
  if (execDataObject != dataObject)
    {
      compObj = parentObj->getParentFromDataObject (execDataObject);
      if (compObj != nullptr && compObj != parentObj)
        {
          compileExecutionObjectLinks (exeObj, execDataObject,
                                       compObj);
        }
    }

  uncompiledLinks = parentObj->getUncompiledLinks ();
  if (!uncompiledLinks->empty ())
    {
      set<Link *> *dataLinks = uncompiledLinks;

      for ( Link *ncmLink : *dataLinks)
        {
          processLink (ncmLink, dataObject, exeObj, parentObj);
        }

      delete dataLinks;

      compileExecutionObjectLinks (
            exeObj, dataObject,
            (ExecutionObjectContext *)(parentObj->getParentObject ()));
    }
  else
    {
      ExecutionObject *object;

      delete uncompiledLinks;

      while (parentObj != nullptr)
        {
          object = parentObj;
          parentObj
              = (ExecutionObjectContext *)(parentObj
                                           ->getParentObject ());

          compileExecutionObjectLinks (object, dataObject, parentObj);
        }
    }
}

void
Converter::setActionListener (NclAction *action)
{
  auto simpleAction = cast (NclSimpleAction *, action);
  auto compoundAction = cast (NclCompoundAction *, action);

  if (simpleAction)
    {
      simpleAction->setSimpleActionListener (_actionListener);
    }
  else if (compoundAction)
    {
      vector<NclSimpleAction *> actions;

      compoundAction->setCompoundActionListener (_actionListener);
      compoundAction->getSimpleActions (actions);

      for (NclSimpleAction *a: actions)
        {
          setActionListener (a);
        }
    }
  else
    {
      g_assert_not_reached();
    }
}

ExecutionObject *
Converter::processExecutionObjectSwitch (
    ExecutionObjectSwitch *switchObject)
{

  Node *selectedNode;
  NclNodeNesting *selectedPerspective;
  string id;
  map<string, ExecutionObject *>::iterator i;
  ExecutionObject *selectedObject;

  auto switchNode = cast (Switch *, switchObject->getNode ());
  g_assert_nonnull (switchNode);

  selectedNode = _ruleAdapter->adaptSwitch (switchNode);
  if (selectedNode == NULL)
    {
      WARNING ("Cannot process '%s'. Selected NODE is nullptr.",
               switchObject->getId ().c_str());

      return nullptr;
    }

  selectedPerspective = switchObject->getNodePerspective ();
  selectedPerspective->insertAnchorNode (selectedNode);

  id = selectedPerspective->getId () + SEPARATOR;

  i = _exeObjects.find (id);
  if (i != _exeObjects.end ())
    {
      selectedObject = i->second;
      switchObject->select (selectedObject);
      resolveSwitchEvents (switchObject);
      delete selectedPerspective;

      return selectedObject;
    }

  selectedObject = createExecutionObject (id, selectedPerspective, nullptr);

  delete selectedPerspective;

  if (selectedObject == nullptr)
    {
      WARNING ("Cannot process '%s' because select object is NULL.",
               switchObject->getId ().c_str ());
      return nullptr;
    }

  addExecutionObject (selectedObject, switchObject);
  switchObject->select (selectedObject);
  resolveSwitchEvents (switchObject);

  return selectedObject;
}

void
Converter::resolveSwitchEvents (
    ExecutionObjectSwitch *switchObject)
{
  ExecutionObject *selectedObject;
  ExecutionObject *endPointObject;
  Node *selectedNode;
  vector<NclEvent *> events;
  vector<NclEvent *>::iterator i;
  SwitchEvent *switchEvent;
  Anchor *interfacePoint;
  vector<Node *> nestedSeq;
  NclNodeNesting *nodePerspective;
  NclEvent *mappedEvent;

  selectedObject = switchObject->getSelectedObject ();
  if (selectedObject == nullptr)
    {
      WARNING ("Selected object is nullptr");
      return;
    }

  selectedNode = selectedObject->getNode ();
  selectedNode = cast (Node *, selectedNode);
  g_assert_nonnull (selectedNode);

  if (events.empty ())
    {
      WARNING ("Can't find events.");
    }

  for (NclEvent *event: switchObject->getEvents ())
    {
      mappedEvent = nullptr;
      switchEvent = cast (SwitchEvent *, event);
      g_assert_nonnull (switchEvent);

      interfacePoint = switchEvent->getInterface ();
      auto lambdaAnchor = cast (AreaLambda *, interfacePoint);
      if (lambdaAnchor)
        {
          mappedEvent = getEvent (
                selectedObject, selectedNode->getLambda (),
                switchEvent->getType (), switchEvent->getKey ());
        }
      else
        {
          auto switchPort = cast (SwitchPort *, interfacePoint);
          g_assert_nonnull (switchPort);

          for (Port *mapping: *(switchPort->getPorts ()))
            {
              if (mapping->getNode () == selectedNode)
                {
                  nodePerspective
                      = switchObject->getNodePerspective ();

                  nestedSeq = mapping->getMapNodeNesting ();
                  nodePerspective->append (&nestedSeq);

                  endPointObject
                      = getExecutionObjectFromPerspective (
                        nodePerspective, nullptr);

                  if (endPointObject != nullptr)
                    {
                      mappedEvent = getEvent (
                            endPointObject,
                            mapping->getFinalInterface (),
                            switchEvent->getType (),
                            switchEvent->getKey ());
                    }

                  delete nodePerspective;

                  break;
                }
            }
        }

      if (mappedEvent != nullptr)
        {
          switchEvent->setMappedEvent (mappedEvent);
          TRACE ("Setting '%s' as mapped event of '%s'.",
                 mappedEvent->getId ().c_str(),
                 switchEvent->getId ().c_str());
        }
      else
        {
          WARNING ("Can't set a mapped event for '%s'.",
                   switchEvent->getId ().c_str());
        }
    }
}

NclEvent *
Converter::insertNode (NclNodeNesting *perspective,
                       Anchor *interfacePoint,
                       Descriptor *descriptor)
{
  ExecutionObject *executionObject;
  NclEvent *event;
  EventType eventType;

  event = nullptr;
  executionObject = getExecutionObjectFromPerspective (perspective,
                                                       descriptor);

  if (executionObject != nullptr)
    {
      if (!(instanceof (Property *, interfacePoint)))
        {
          eventType = EventType::PRESENTATION;
        }
      else
        {
          eventType = EventType::ATTRIBUTION;
        }

      // get the event corresponding to the node anchor
      event = getEvent (executionObject, interfacePoint, eventType, "");
    }

  return event;

}

NclEvent *
Converter::insertContext (NclNodeNesting *contextPerspective,
                          Port *port)
{
  vector<Node *> nestedSeq;
  NclNodeNesting *perspective;
  NclEvent *newEvent;
  bool error = false;

  if (contextPerspective == nullptr || port == nullptr)
    {
      error = true;
    }

  if (!(instanceof (Area *, port->getFinalInterface ())
        || instanceof (AreaLabeled *, port->getFinalInterface ())
        || instanceof (Property *, port->getFinalInterface ())
        || instanceof (SwitchPort *, port->getFinalInterface ()))
      || !(instanceof (Context *,
                       contextPerspective->getAnchorNode ())))
    {
      error = true;
    }

  if (error)
    {
      WARNING ("Can't find a valid interface point in '%s'.",
               contextPerspective->getId ().c_str());

      return nullptr;
    }
  else
    {
      nestedSeq = port->getMapNodeNesting ();
      perspective = new NclNodeNesting (contextPerspective);
      perspective->append (&nestedSeq);

      newEvent = insertNode (perspective,
                             port->getFinalInterface (),
                             nullptr);
      delete perspective;

      return newEvent;
    }
}

void
Converter::eventStateChanged (NclEvent *event,
                              EventStateTransition transition,
                              arg_unused (EventState previousState))
{
  ExecutionObject *exeObj = event->getExecutionObject ();
  auto exeCompositeObj = cast (ExecutionObjectContext *, exeObj);
  auto exeSwitch = cast (ExecutionObjectSwitch *, exeObj);

  if (exeSwitch)
    {
      if (transition == EventStateTransition::STARTS)
        {
          for (NclEvent *e: exeSwitch->getEvents())
            {
              auto switchEvt = cast (SwitchEvent *, e);
              if (switchEvt)
                {
                  NclEvent *ev = switchEvt->getMappedEvent ();

                  if (ev == nullptr)
                    {
                      // there is only one way to start a switch with
                      // NULL mapped event: a instSame refernode inside
                      // it was started
                      processExecutionObjectSwitch (exeSwitch);

                      ev = switchEvt->getMappedEvent ();
                      if (ev != nullptr)
                        {
                          // now we know the event is mapped, we can start
                          // the
                          // switchport
                          e->start ();
                        }
                    }
                }
            }
        }

      if (transition == EventStateTransition::STOPS
          || transition == EventStateTransition::ABORTS)
        {
          exeSwitch->select (NULL);
        }
    }
  else if (exeCompositeObj)
    {
      if (transition == EventStateTransition::STOPS
          || transition == EventStateTransition::ABORTS)
        {
          removeExecutionObject (exeObj);
        }
    }
}

bool
Converter::hasDescriptorPropName (const string &name)
{
  static const set <string> words = { "left", "top", "width", "height",
                                      "right", "bottom", "explicitDur",
                                      "size", "bounds", "location",
                                      "zIndex" };

  return words.count(name);
}

NclFormatterLink *
Converter::createLink (Link *ncmLink,
                             ExecutionObjectContext *parentObj)
{
  Connector *connector;
  Condition *conditionExpression;
  Action *actionExp;
  NclLinkCondition *formatterCondition;
  NclAction *formatterAction;
  NclFormatterLink *formatterLink;
  NclAssignmentAction *action;
  string value;
  NclEvent *event;

  if (ncmLink == nullptr)
    {
      WARNING ("Cannot create formatter link inside '%s' NCM link is nullptr",
               parentObj->getId ().c_str ());
      return nullptr;
    }

  // compile link condition and verify if it is a trigger condition
  connector = cast (Connector *, ncmLink->getConnector ());
  g_assert_nonnull (connector);

  conditionExpression = connector->getCondition ();
  formatterCondition
      = createCondition ((TriggerExpression *)conditionExpression, ncmLink,
                         parentObj);

  if (formatterCondition == nullptr
      || !(instanceof (NclLinkTriggerCondition *, formatterCondition)))
    {
      WARNING ("Cannot create formatter link inside '%s' from ncmLinkId '%s'"
               "with an unknown condition.",
               parentObj->getId ().c_str (),
               ncmLink->getId ().c_str ());

      if (formatterCondition != nullptr)
        {
          delete formatterCondition;
        }

      return nullptr;
    }

  // compile link action
  actionExp = connector->getAction ();
  formatterAction = createAction (actionExp, ncmLink, parentObj);

  if (formatterAction == nullptr)
    {
      WARNING ("Cannot create formatter link inside '%s' from ncmLinkID "
               "'%s' with a nullptr action.",
               parentObj->getId ().c_str (),
               ncmLink->getId ().c_str ());

      return nullptr;
    }

  // create formatter causal link
  formatterLink = new NclFormatterLink (
        (NclLinkTriggerCondition *)formatterCondition, formatterAction,
        ncmLink, (ExecutionObjectContext *)parentObj);

  if (instanceof (NclLinkCompoundTriggerCondition *, formatterCondition))
    {
      vector<NclAction *> acts
          = formatterAction->getImplicitRefRoleActions ();

      for (NclAction *linkAction : acts)
        {
          action = cast (NclAssignmentAction *, linkAction);
          g_assert_nonnull (action);

          value = action->getValue ();
          if (value != "" && value.substr (0, 1) == "$")
            {
              event = action->getEvent ();
              setImplicitRefAssessment (
                    value.substr (1, value.length ()), ncmLink, event);
            }

          value = action->getDuration ();
          if (value != "" && value.substr (0, 1) == "$")
            {
              event = action->getEvent ();
              setImplicitRefAssessment
                (value.substr (1, value.length ()), ncmLink, event);
            }
        }
    }

  return formatterLink;
}

void
Converter::setImplicitRefAssessment (const string &roleId,
                                     Link *ncmLink,
                                     NclEvent *event)
{
  NclNodeNesting *refPerspective;
  ExecutionObject *refObject;
  string value;

  auto attributionEvt = cast (AttributionEvent *, event);
  if (attributionEvt)
    {
      for (Bind *bind: *(ncmLink->getBinds ()))
        {
          value = bind->getRole ()->getLabel ();
          if (roleId == value)
            {
              Anchor *refInterface = bind->getInterface ();
              auto propAnchor = cast (Property *, refInterface);
              if (propAnchor)
                {
                  vector<Node *> *ncmPerspective
                      = bind->getNode ()->getPerspective ();
                  refPerspective = new NclNodeNesting (ncmPerspective);

                  delete ncmPerspective;

                  refObject = this->getExecutionObjectFromPerspective (
                        refPerspective, bind->getDescriptor ());

                  delete refPerspective;

                  NclEvent *refEvent
                      = this->getEvent (refObject,
                                        propAnchor,
                                        EventType::ATTRIBUTION,
                                        "");

                  attributionEvt->setImplicitRefAssessmentEvent (roleId,
                                                                 refEvent);

                  break;
                }
            }
        }
    }
}

NclAction *
Converter::createAction (Action *actionExp,
                         Link *ncmLink,
                         ExecutionObjectContext *parentObj)
{
  vector<Bind *> binds;
  size_t i, size;
  string delayObject;
  NclSimpleAction *simpleAction;
  NclCompoundAction *compoundAction;

  if (actionExp == nullptr)
    {
      WARNING ("ActionExpression is nullptr.");
      return nullptr;
    }

  auto sae = cast (SimpleAction *, actionExp);
  auto cae = cast (CompoundAction *, actionExp);
  if (sae) // SimpleAction
    {
      binds = ncmLink->getBinds (sae);
      size = binds.size ();
      if (size == 1)
        {
          return createSimpleAction (sae, binds[0], ncmLink,
                                     parentObj);
        }
      else if (size > 1)
        {
          compoundAction
            = new NclCompoundAction ();

          for (i = 0; i < size; i++)
            {
              simpleAction = createSimpleAction (sae, binds[i], ncmLink, parentObj);

              if (simpleAction == NULL)
                {
                  WARNING ("Cannot create compound action: invalid "
                           "action(s)");
                  delete compoundAction;
                  return nullptr;
                }
              compoundAction->addAction (simpleAction);
            }

          return compoundAction;
        }
      else
        {
          WARNING ("Cannot create action of link '%s' because number of"
                   "binds is = %lu.",
                   ncmLink->getId ().c_str (),
                   size);
          return nullptr;
        }

    }
  else if (cae) // CompundAction
    {
      return createCompoundAction (0, cae->getActions (), ncmLink,
                                   parentObj);
    }
  else
    {
      g_assert_not_reached ();
    }

  WARNING ("Cannot create action of link '%s'.  Returning nullptr.",
           ncmLink->getId ().c_str ());

  return nullptr;
}

NclLinkCondition *
Converter::createCondition (
    Condition *ncmExp, Link *ncmLink,
    ExecutionObjectContext *parentObj)
{
  auto triggerExp = cast (TriggerExpression *, ncmExp);
  auto statment = cast (Statement *, ncmExp);
  if (triggerExp)
    {
      return createCondition (triggerExp, ncmLink, parentObj);
    }
  else if (statment)
    {
      return createStatement (statment, ncmLink, parentObj);
    }

  g_assert_not_reached ();
}

NclLinkCompoundTriggerCondition *
Converter::createCompoundTriggerCondition (
    short op, GingaTime delay,
    const vector<Condition *> *ncmChildConditions, Link *ncmLink,
    ExecutionObjectContext *parentObj)
{
  NclLinkCompoundTriggerCondition *condition;
  NclLinkCondition *childCondition;

  if (op == CompoundCondition::OP_AND)
    {
      condition = new NclLinkAndCompoundTriggerCondition ();
    }
  else
    {
      condition = new NclLinkCompoundTriggerCondition ();
    }

  if (delay > 0)
    {
      condition->setDelay (delay);
    }

  for (auto cond: *ncmChildConditions)
    {
      childCondition = createCondition (cond, ncmLink, parentObj);
      condition->addCondition (childCondition);
    }

  return condition;
}

NclLinkCondition *
Converter::createCondition (
    TriggerExpression *condition, Link *ncmLink,
    ExecutionObjectContext *parentObj)
{
  NclLinkCompoundTriggerCondition *compoundCondition;
  NclLinkTriggerCondition *simpleCondition;

  auto ste = cast (SimpleCondition *, condition);
  auto cte = cast (CompoundCondition *, condition);

  if (ste) // SimpleCondition
    {
      vector<Bind *> binds = ncmLink->getBinds (ste);
      size_t size = binds.size ();
      if (size == 1)
        {
          return createSimpleCondition (ste, binds[0], ncmLink,
                                        parentObj);
        }
      else if (size > 1)
        {
          if (ste->isConjunction ())
            {
              compoundCondition
                = new NclLinkAndCompoundTriggerCondition ();
            }
          else
            {
              compoundCondition
                = new NclLinkCompoundTriggerCondition ();
            }

          for (size_t i = 0; i < size; i++)
            {
              simpleCondition = createSimpleCondition (ste, binds[i], ncmLink, parentObj);

              compoundCondition->addCondition (simpleCondition);
            }
          return compoundCondition;
        }
      else
        {
          WARNING ("Cannot create condition of link '%s' because number "
                   "of binds is %lu",
                   ncmLink->getId ().c_str(), size);

          return nullptr;
        }
    }
  else if (cte) // CompoundCondition
    {
      string delayObject = condition->getDelay ();
      GingaTime delay = compileDelay (ncmLink, delayObject, nullptr);

      return createCompoundTriggerCondition (cte->getOperator (), delay,
                                             cte->getConditions (), ncmLink,
                                             parentObj);
    }
  else
    {
      g_assert_not_reached ();
    }

  WARNING ("Cannot create condition of link '%s'.  Returning nullptr.",
           ncmLink->getId ().c_str ());

  return nullptr;
}

NclLinkAssessmentStatement *
Converter::createAssessmentStatement (
    AssessmentStatement *assessmentStatement, Bind *bind, Link *ncmLink,
    ExecutionObjectContext *parentObj)
{
  NclLinkAttributeAssessment *mainAssessment;
  NclLinkAssessment *otherAssessment;
  NclLinkAssessmentStatement *statement;
  string paramValue;
  Parameter *connParam, *param;
  vector<Bind *> otherBinds;

  mainAssessment = createAttributeAssessment (
        assessmentStatement->getMainAssessment (), bind, ncmLink,
        parentObj);

  auto valueAssessment = cast (ValueAssessment *, 
        assessmentStatement->getOtherAssessment ());

  auto attrAssessment = cast (AttributeAssessment *, 
        assessmentStatement->getOtherAssessment ());

  if (valueAssessment)
    {
      paramValue = valueAssessment->getValue ();
      if (paramValue[0] == '$')
        { // instanceof("Parameter")
          connParam = new Parameter (
                paramValue.substr (1, paramValue.length () - 1), "");

          param = bind->getParameter (connParam->getName ());
          if (param == nullptr)
            {
              param = ncmLink->getParameter (connParam->getName ());
            }

          if (param != nullptr)
            {
              paramValue = param->getValue ();
            }
        }

      otherAssessment = new NclLinkValueAssessment (paramValue);
    }
  else if (attrAssessment)
    {
      otherBinds = ncmLink->getBinds (attrAssessment);
      if (!otherBinds.empty ())
        {
          otherAssessment = createAttributeAssessment (
                attrAssessment, otherBinds[0], ncmLink, parentObj);
        }
      else
        {
          otherAssessment = createAttributeAssessment (
                attrAssessment, nullptr, ncmLink, parentObj);
        }
    }
  else
    {
      g_assert_not_reached ();
    }

  statement = new NclLinkAssessmentStatement (
        assessmentStatement->getComparator (), mainAssessment,
        otherAssessment);

  return statement;
}

NclLinkStatement *
Converter::createStatement (
    Statement *statementExpression, Link *ncmLink,
    ExecutionObjectContext *parentObj)
{
  int size;
  NclLinkStatement *statement;

  auto as = cast (AssessmentStatement *, statementExpression);
  auto cs = cast (CompoundStatement *, statementExpression);
  if (as) // AssessmentStatement
    {
      vector<Bind *> binds = ncmLink->getBinds (as->getMainAssessment ());
      size = (int) binds.size ();
      if (size == 1)
        {
          statement = createAssessmentStatement (as, binds[0], ncmLink, parentObj);
        }
      else
        {
          WARNING ("Cannot create statement of link '%s' because number "
                   "of binds is %d.",
                   ncmLink->getId ().c_str(), size);

          return nullptr;
        }
    }
  else if (cs) // CompoundStatement
    {
      statement = new NclLinkCompoundStatement (cs->isConjunction (),
                                                cs->isNegated ());

      for (auto child: *cs->getStatements ())
        {
          NclLinkStatement *childStatement
              = createStatement (child, ncmLink, parentObj);

          ((NclLinkCompoundStatement *)statement)
              ->addStatement (childStatement);
        }
    }
  else
    {
      g_assert_not_reached ();
    }

  return statement;
}

NclLinkAttributeAssessment *
Converter::createAttributeAssessment (
    arg_unused (AttributeAssessment *attributeAssessment), Bind *bind, Link *ncmLink,
    ExecutionObjectContext *parentObj)
{
  NclEvent *event = createEvent (bind, ncmLink, parentObj);

  return new NclLinkAttributeAssessment
    (event, AttributeType::NODE_PROPERTY);
}

NclSimpleAction *
Converter::createSimpleAction (
    SimpleAction *sae, Bind *bind, Link *ncmLink,
    ExecutionObjectContext *parentObj)
{
  NclEvent *event;
  SimpleAction::Type actionType;
  EventType eventType;
  NclSimpleAction *action;
  Parameter *connParam;
  Parameter *param;
  string paramValue;
  int repeat;
  GingaTime delay;

  action = nullptr;
  event = createEvent (bind, ncmLink, parentObj);

  actionType = sae->getActionType ();
  if (event != nullptr)
    {
      eventType = bind->getRole ()->getEventType ();
      event->setType (eventType);
    }
  else
    {
      ERROR ("Trying to create a simple action with a nullptr event");
    }

  switch (actionType)
    {
    case SimpleAction::START:
      if (eventType == EventType::PRESENTATION)
        {
          action = new NclSimpleAction (event, actionType);

          // repeat
          paramValue = sae->getRepeat ();
          if (paramValue == "")
            {
              repeat = 0;
            }
          else if (paramValue[0] == '$')
            {
              connParam = new Parameter (
                    paramValue.substr (1, paramValue.length () - 1), "");

              param = bind->getParameter (connParam->getName ());
              if (param == nullptr)
                {
                  param = ncmLink->getParameter (connParam->getName ());
                }

              if (param == nullptr)
                {
                  repeat = 0;
                }
              else
                {
                  repeat = xstrtoint (param->getValue (), 10);
                }
            }
          else
            {
              repeat = xstrtoint (paramValue, 10);
            }

          // repeatDelay
          paramValue = sae->getRepeatDelay ();
          delay = compileDelay (ncmLink, paramValue, bind);

          action->setRepetitions (repeat, delay);
        }
      else if (eventType == EventType::ATTRIBUTION)
        {
          paramValue = sae->getValue ();
          if (paramValue != "" && paramValue[0] == '$')
            {
              connParam = new Parameter (
                    paramValue.substr (1, paramValue.length () - 1), "");

              param = bind->getParameter (connParam->getName ());
              if (param == nullptr)
                {
                  param = ncmLink->getParameter (connParam->getName ());
                }

              delete connParam;
              connParam = nullptr;

              if (param != nullptr)
                {
                  paramValue = param->getValue ();
                }
              else
                {
                  paramValue = "";
                }
            }

          string paramDur;

          paramDur = sae->getDuration ();
          if (paramDur[0] == '$')
            {
              connParam = new Parameter
                (paramDur.substr (1, paramDur.length () - 1), "");

              param = bind->getParameter (connParam->getName ());
              if (param == nullptr)
                {
                  param = ncmLink->getParameter (connParam->getName ());
                }

              delete connParam;
              connParam = nullptr;

              if (param != nullptr)
                {
                  paramDur = param->getValue ();
                }
            }

          action = new NclAssignmentAction (event, actionType,
                                            paramValue, paramDur);
        }
      else
        {
          ERROR ("Unknown event type '%d' for action type '%d'.",
                 (int) eventType, actionType);
        }
      break;

    case SimpleAction::STOP:
    case SimpleAction::PAUSE:
    case SimpleAction::RESUME:
    case SimpleAction::ABORT:
      action = new NclSimpleAction (event, actionType);
      break;

    default:
      g_assert_not_reached ();
    }

  g_assert_nonnull (action);

  paramValue = sae->getDelay ();
  delay = compileDelay (ncmLink, paramValue, bind);
  action->setDelay (delay);

  return action;
}

NclCompoundAction *
Converter::createCompoundAction (
    GingaTime delay, const vector<Action *> *ncmChildActions,
    Link *ncmLink, ExecutionObjectContext *parentObj)
{
  NclCompoundAction *action;
  NclAction *childAction;

  action = new NclCompoundAction ();
  if (delay > 0)
    {
      action->setDelay (delay);
    }

  if (ncmChildActions != nullptr)
    {
      for (Action *ncmChildAction: *ncmChildActions)
        {
          childAction = createAction (ncmChildAction, ncmLink, parentObj);

          if (childAction != nullptr)
            {
              action->addAction (childAction);
            }
          else
            {
              if (instanceof (SimpleAction *, ncmChildAction))
                {
                  WARNING ("Can't create simple action type '%d'.",
                           ((SimpleAction *)ncmChildAction)->getActionType ());
                }
              else if (instanceof (CompoundAction *, ncmChildAction))
                {
                  WARNING ("Can't create inner compoud action.");
                }
            }
        }
    }

  return action;
}

NclLinkTriggerCondition *
Converter::createSimpleCondition (
    SimpleCondition *simpleCondition, Bind *bind, Link *ncmLink,
    ExecutionObjectContext *parentObj)
{
  NclEvent *event;
  GingaTime delay;
  string delayObject;
  NclLinkTriggerCondition *condition;

  event = createEvent (bind, ncmLink, parentObj);
  condition = new NclLinkTransitionTriggerCondition (
        event, simpleCondition->getTransition (), bind);

  delayObject = simpleCondition->getDelay ();
  delay = compileDelay (ncmLink, delayObject, bind);
  if (delay > 0)
    {
      condition->setDelay (delay);
    }
  return condition;
}

NclEvent *
Converter::createEvent (Bind *bind, Link *ncmLink,
                        ExecutionObjectContext *parentObject)
{
  NclNodeNesting *endPointNodeSequence;
  NclNodeNesting *endPointPerspective;
  Node *parentNode;
  ExecutionObject *executionObject;
  Anchor *interfacePoint;
  string key;
  NclEvent *event = nullptr;
  vector<Node *> seq;

  endPointPerspective = parentObject->getNodePerspective ();

  parentNode = endPointPerspective->getAnchorNode ();

  Node *node = bind->getNode ();
  g_assert_nonnull (node);

  interfacePoint = bind->getInterface ();

  //seq = bind->getNodeNesting ();
  seq.push_back (node);
  if (interfacePoint != nullptr
      && instanceof (Port *, interfacePoint)
      && !(instanceof (SwitchPort *, interfacePoint)))
    {
      for (auto inner: ((Port *) interfacePoint)->getMapNodeNesting ())
        seq.push_back (inner);
    }

  endPointNodeSequence = new NclNodeNesting (&seq);
  if (endPointNodeSequence->getAnchorNode ()
      != endPointPerspective->getAnchorNode ()
      && endPointNodeSequence->getAnchorNode () != parentNode)
    {
      endPointPerspective->append (endPointNodeSequence);
    }

  delete endPointNodeSequence;

  executionObject = getExecutionObjectFromPerspective (
        endPointPerspective, bind->getDescriptor ());

  if (executionObject == nullptr)
    {

      WARNING ("Can't find execution object for perspective '%s'.",
               endPointPerspective->getId ().c_str ());

      delete endPointPerspective;
      return nullptr;
    }


  if (interfacePoint == nullptr)
    {
      // TODO: This is an error, the formatter then return the main event
      WARNING ("Can't find an interface point for '%s' bind '%s'.",
               endPointPerspective->getId ().c_str (),
               bind->getRole ()->getLabel ().c_str ());
      delete endPointPerspective;

      return executionObject->getWholeContentPresentationEvent ();
    }

  if (instanceof (Composition *, node)
      && instanceof (Port *, interfacePoint))
    {
      Composition *comp = cast (Composition *, node);
      Port *port = cast (Port *, interfacePoint);
      interfacePoint = comp->getMapInterface (port);
    }

  key = getBindKey (ncmLink, bind);
  event = getEvent (executionObject, interfacePoint,
                    bind->getRole ()->getEventType (), key);

  delete endPointPerspective;
  return event;
}

GingaTime
Converter::getDelayParameter (Link *ncmLink,
                              Parameter *connParam,
                              Bind *ncmBind)
{
  Parameter *parameter;
  string param;

  parameter = nullptr;
  if (ncmBind != nullptr)
    {
      parameter = ncmBind->getParameter (connParam->getName ());
    }

  if (parameter == nullptr)
    {
      parameter = ncmLink->getParameter (connParam->getName ());
    }

  if (parameter == nullptr)
    {
      return 0;
    }
  else
    {
      param = parameter->getValue ();
      if (param == "")
        {
          return 0;
        }
      else
        {
          return (GingaTime)(xstrtod (param) * GINGA_NSECOND);
        }
    }
}

string
Converter::getBindKey (Link *ncmLink, Bind *ncmBind)
{
  Role *role;
  string keyValue;
  Parameter *param, *auxParam;
  string key;

  role = ncmBind->getRole ();
  if (role == nullptr)
    {
      return "";
    }

  if (auto sc = cast (SimpleCondition *, role))
    {
      keyValue = sc->getKey ();
    }
  else if (auto attrAssessment = cast (AttributeAssessment *, role))
    {
      keyValue = attrAssessment->getKey ();
    }
  else
    {
      return "";
    }

  if (keyValue == "")
    {
      key = "";
    }
  else if (keyValue[0] == '$')
    { // instanceof Parameter
      param
          = new Parameter (keyValue.substr (1, keyValue.length () - 1), "");

      auxParam = ncmBind->getParameter (param->getName ());
      if (auxParam == nullptr)
        {
          auxParam = ncmLink->getParameter (param->getName ());
        }

      if (auxParam != nullptr)
        {
          key = auxParam->getValue ();
        }
      else
        {
          key = "";
        }

      delete param;
    }
  else
    {
      key = keyValue;
    }

  return key;
}

GingaTime
Converter::compileDelay (Link *ncmLink,
                         const string &delayObject,
                         Bind *bind)
{
  GingaTime delay;
  string::size_type pos;
  Parameter *param;
  string delayValue;

  if (delayObject == "")
    {
      delay = 0;
    }
  else
    {
      pos = delayObject.find ("$");
      if (pos != std::string::npos && pos == 0)
        {
          // instanceof Parameter
          delayValue = delayObject.substr (1, delayObject.length () - 1);
          param = new Parameter (delayValue, "");
          delay = getDelayParameter (ncmLink, param, bind);
          delete param;
        }
      else
        {
          delay = (GingaTime)(xstrtod (delayObject) * GINGA_NSECOND);
        }
    }

  return delay;
}

GINGA_FORMATTER_END