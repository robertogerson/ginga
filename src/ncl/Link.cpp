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

#include "ginga.h"
#include "Link.h"

#include "ContextNode.h"

GINGA_PRAGMA_DIAG_IGNORE (-Wsign-conversion)

GINGA_NCL_BEGIN

Link::Link (const string &id, Connector *connector) : Entity (id)
{
  _roleBinds = new map<string, vector<Bind *> *>;
  _binds = new vector<Bind *>;
  this->_connector = connector;
  _parameters = new map<string, Parameter *>;
  _composition = NULL;
}

Link::~Link ()
{
  map<string, vector<Bind *> *>::iterator i;
  vector<Bind *>::iterator j;
  map<string, Parameter *>::iterator k;
  vector<Bind *> *rmBinds;

  if (_roleBinds != NULL)
    {
      i = _roleBinds->begin ();
      while (i != _roleBinds->end ())
        {
          rmBinds = i->second;
          rmBinds->clear ();
          delete rmBinds;
          ++i;
        }
      delete _roleBinds;
      _roleBinds = NULL;
    }

  if (_binds != NULL)
    {
      j = _binds->begin ();
      while (j != _binds->end ())
        {
          delete *j;
          ++j;
        }
      delete _binds;
      _binds = NULL;
    }

  if (_parameters != NULL)
    {
      k = _parameters->begin ();
      while (k != _parameters->end ())
        {
          delete k->second;
          ++k;
        }
      delete _parameters;
      _parameters = NULL;
    }
}

Bind *
Link::bind (Node *node, InterfacePoint *interfPt, Descriptor *desc,
            string roleId)
{
  Role *role;

  role = _connector->getRole (roleId);
  if (role == NULL)
    {
      clog << "Link::bind Warning! Can't find role '" << roleId;
      clog << "'" << endl;
      return NULL;
    }

  return bind (node, interfPt, desc, role);
}

Bind *
Link::bind (Node *node, InterfacePoint *interfPt, Descriptor *desc,
            Role *role)
{
  Bind *bind;
  vector<Bind *> *roleBindList;
  string label;

  label = role->getLabel ();
  if (_roleBinds->count (label) == 0)
    {
      roleBindList = new vector<Bind *>;
      (*_roleBinds)[label] = roleBindList;
    }
  else
    {
      roleBindList = (*_roleBinds)[label];
    }

  bind = new Bind (node, interfPt, desc, role);
  roleBindList->push_back (bind);
  _binds->push_back (bind);
  return bind;
}

bool
Link::isConsistent ()
{
  return true;
}

Bind *
Link::getBind (Node *node, InterfacePoint *interfPt,
               Descriptor *desc, Role *role)
{
  map<string, vector<Bind *> *>::iterator i;

  bool containsKey = false;
  for (i = _roleBinds->begin (); i != _roleBinds->end (); ++i)
    if (i->first == role->getLabel ())
      containsKey = true;

  if (!containsKey)
    return NULL;

  Bind *bind;
  vector<Bind *>::iterator bindIterator;

  vector<Bind *> *roleBindList;
  roleBindList = (*_roleBinds)[role->getLabel ()];

  for (bindIterator = roleBindList->begin ();
       bindIterator != roleBindList->end (); ++roleBindList)
    {
      bind = (*bindIterator);
      if ((bind->getNode () == node)
          && (bind->getInterfacePoint () == interfPt)
          && (bind->getDescriptor () == desc))
        return bind;
    }

  return NULL;
}

vector<Bind *> *
Link::getBinds ()
{
  return _binds;
}

Connector *
Link::getConnector ()
{
  return _connector;
}

void
Link::setParentComposition (ContextNode *composition)
{
  g_assert_nonnull (composition);
  _composition = composition;
}

ContextNode *
Link::getParentComposition ()
{
  return _composition;
}

unsigned int
Link::getNumBinds ()
{
  return (unsigned int) _binds->size ();
}

unsigned int
Link::getNumRoleBinds (Role *role)
{
  map<string, vector<Bind *> *>::iterator i;

  i = _roleBinds->find (role->getLabel ());
  if (i == _roleBinds->end ())
    {
      return 0;
    }

  return (unsigned int) i->second->size ();
}

vector<Bind *> *
Link::getRoleBinds (Role *role)
{
  map<string, vector<Bind *> *>::iterator i;

  i = _roleBinds->find (role->getLabel ());
  if (i == _roleBinds->end ())
    {
      return NULL;
    }

  return i->second;
}

bool
Link::isMultiPoint ()
{
  if (_binds->size () > 2)
    return true;
  else
    return false;
}

void
Link::setConnector (Connector *connector)
{
  this->_connector = connector;

  _roleBinds->clear ();
  _binds->clear ();
}

bool
Link::unBind (Bind *bind)
{
  vector<Bind *> *roleBindList;
  bool containsBind = false;

  vector<Bind *>::iterator it;
  for (it = _binds->begin (); it != _binds->end (); ++it)
    {
      if (bind == *it)
        {
          containsBind = true;
          _binds->erase (it);
          break;
        }
    }

  if (!containsBind)
    {
      return false;
    }

  if (_roleBinds->count (bind->getRole ()->getLabel ()) == 1)
    {
      roleBindList = (*_roleBinds)[bind->getRole ()->getLabel ()];
      vector<Bind *>::iterator i;
      for (i = roleBindList->begin (); i != roleBindList->end (); ++i)
        {
          if (*i == bind)
            {
              roleBindList->erase (i);
              break;
            }
        }
    }
  return true;
}

void
Link::addParameter (Parameter *parameter)
{
  if (parameter == NULL)
    return;

  (*_parameters)[parameter->getName ()] = parameter;
}

vector<Parameter *> *
Link::getParameters ()
{
  if (_parameters->empty ())
    return NULL;

  vector<Parameter *> *params;
  params = new vector<Parameter *>;

  map<string, Parameter *>::iterator i;
  for (i = _parameters->begin (); i != _parameters->end (); ++i)
    params->push_back (i->second);

  return params;
}

Parameter *
Link::getParameter (const string &name)
{
  if (_parameters->empty ())
    return NULL;

  map<string, Parameter *>::iterator i;
  for (i = _parameters->begin (); i != _parameters->end (); ++i)
    if (i->first == name)
      return (Parameter *)(i->second);

  return NULL;
}

void
Link::removeParameter (Parameter *parameter)
{
  if (_parameters->empty ())
    return;

  map<string, Parameter *>::iterator i;
  for (i = _parameters->begin (); i != _parameters->end (); ++i)
    {
      if (i->first == parameter->getName ())
        {
          _parameters->erase (i);
          return;
        }
    }
}

void
Link::updateConnector (Connector *newConnector)
{
  int i, size;
  Bind *bind;
  Role *newRole;

  if (this->_connector == NULL)
    {
      // TODO test if the set of roles is identical
      return;
    }

  size = (int) _binds->size ();
  for (i = 0; i < size; i++)
    {
      bind = (Bind *)(*_binds)[i];
      newRole = newConnector->getRole (bind->getRole ()->getLabel ());
      bind->setRole (newRole);
    }
  this->_connector = newConnector;
}

bool
Link::containsNode (Node *node, Descriptor *descriptor)
{
  return containsNode (node, descriptor, getBinds ());
}

bool
Link::containsNode (Node *node, Descriptor *descriptor,
                    vector<Bind *> *binds)
{
  Bind *bind;
  Node *bindNode;
  InterfacePoint *bindInterface;
  Descriptor *bindDescriptor;

  NodeEntity *nodeEntity;
  Descriptor *entityDesc;

  vector<Bind *>::iterator i;

  clog << "Link::containsNode searching '" << node->getId () << "' ";
  clog << "inside link '" << getId () << "' (" << binds->size ();
  clog << " condition binds)" << endl;

  i = binds->begin ();
  while (i != binds->end ())
    {
      bind = (*i);
      bindInterface = bind->getInterfacePoint ();
      if (bindInterface != NULL && instanceof (Port *, bindInterface))
        {
          bindNode = ((Port *)bindInterface)->getEndNode ();
        }
      else
        {
          bindNode = bind->getNode ();
        }

      clog << "Link::containsNode link '" << getId () << "' has '";
      clog << bindNode->getId () << "' with descriptor = '";
      if (descriptor != NULL)
        {
          clog << descriptor->getId () << "'" << endl;
        }
      else
        {
          clog << "NULL'" << endl;
        }

      if (node == bindNode)
        {
          bindDescriptor = bind->getDescriptor ();
          nodeEntity = cast (NodeEntity *, node);
          if (nodeEntity == nullptr)
            {
              g_assert (instanceof (ReferNode *, node));
              nodeEntity = cast (NodeEntity *, cast (ReferNode *, node)
                                 ->getReferredEntity ());
            }
          g_assert_nonnull (nodeEntity);

          if (bindDescriptor != NULL)
            {
              if (bindDescriptor == descriptor)
                {
                  return true;
                }
            }
          else if (descriptor == NULL)
            {
              return true;
            }
          else if (nodeEntity != NULL)
            {
              entityDesc = nodeEntity->getDescriptor ();
              if (entityDesc == descriptor)
                {
                  return true;
                }
              else if (descriptor != NULL
                       && descriptor->getId ().find ("dummy")
                              != std::string::npos)
                {
                  return true;
                }
              else if (entityDesc != NULL
                       && entityDesc->getId ().find ("dummy")
                              != std::string::npos)
                {
                  return true;
                }
            }
        }
      ++i;
    }
  return false;
}

GINGA_NCL_END
