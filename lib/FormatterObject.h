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

#ifndef FORMATTER_OBJECT_H
#define FORMATTER_OBJECT_H

#include "Formatter.h"
#include "FormatterAction.h"
#include "FormatterEvent.h"
#include "player/Player.h"

GINGA_NAMESPACE_BEGIN

class FormatterContext;
class FormatterMediaSettings;

class FormatterObject
{
public:
  FormatterObject (Formatter *, const string &, NclNode *);
  virtual ~FormatterObject ();

  NclNode *getNode ();
  string getId ();

  const vector <string> *getAliases ();
  bool hasAlias (const string &);
  bool addAlias (const string &);

  FormatterContext *getParent ();
  void initParent (FormatterContext *);

  const set<FormatterEvent *> *getEvents ();
  FormatterEvent *getEvent (NclEventType, NclAnchor *, const string &);
  FormatterEvent *getEventByAnchorId (NclEventType type, const string &,
                                      const string &);
  FormatterEvent *obtainEvent (NclEventType, NclAnchor *, const string &);
  bool addEvent (FormatterEvent *);

  FormatterEvent *obtainLambda ();
  bool isOccurring ();
  bool isPaused ();
  bool isSleeping ();

  bool isFocused ();
  string getProperty (const string &);
  void setProperty (const string &, const string &, GingaTime dur=0);

  bool getZ (int *, int *);
  void redraw (cairo_t *);
  void sendKeyEvent (const string &, bool);
  virtual void sendTickEvent (GingaTime, GingaTime, GingaTime);
  virtual bool exec (FormatterEvent *, NclEventState, NclEventState,
                     NclEventStateTransition);

protected:
  Formatter *_ginga;              // formatter handle
  FormatterScheduler *_scheduler; // formatter scheduler
  NclNode *_node;                 // NCL node
  string _id;                     // object id
  vector<string> _aliases;        // aliases
  FormatterContext *_parent;      // parent object
  Player *_player;                // associated player
  GingaTime _time;                // playback time
  set<FormatterEvent *> _events;  // object events

  // delayed actions
  vector<pair<FormatterAction *, GingaTime>> _delayed;
  vector<pair<FormatterAction *, GingaTime>> _delayed_new;

  void reset ();
  void resetDelayed ();
};

GINGA_NAMESPACE_END

#endif // FORMATTER_OBJECT_H