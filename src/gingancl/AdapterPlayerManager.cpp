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
#include "AdapterPlayerManager.h"

#include "AdapterSubtitlePlayer.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::adapters::text;

#include "AdapterPlainTxtPlayer.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::adapters::text;

#include "AdapterSsmlPlayer.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::adapters::tts;

#include "AdapterImagePlayer.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::adapters::image;

#include "AdapterMirrorPlayer.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::adapters::mirror;

#include "AdapterAVPlayer.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::adapters::av;

#include "AdapterLuaPlayer.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::adapters::application::
    lua;

#include "AdapterNCLPlayer.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::adapters::application::
    ncl;

#include "AdapterChannelPlayer.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::adapters::av;

#include "AdapterProgramAVPlayer.h"
using namespace ::br::pucrio::telemidia::ginga::ncl::adapters::av::tv;

// #include "AdapterTimePlayer.h"
// using namespace ::br::pucrio::telemidia::ginga::ncl::adapters::time;

BR_PUCRIO_TELEMIDIA_GINGA_NCL_ADAPTERS_BEGIN

AdapterPlayerManager::AdapterPlayerManager (NclPlayerData *data) : Thread ()
{
  nclPlayerData = data;

  editingCommandListener = NULL;
  epgFactoryAdapter = NULL;
  timeBaseProvider = NULL;

  Thread::mutexInit (&mutexPlayer, false);

  running = true;
  startThread ();
}

AdapterPlayerManager::~AdapterPlayerManager ()
{
  isDeleting = true;
  running = false;
  unlockConditionSatisfied ();

  clear ();
  clearDeletePlayers ();

  Thread::mutexLock (&mutexPlayer);
  Thread::mutexUnlock (&mutexPlayer);
  Thread::mutexDestroy (&mutexPlayer);

  clog << "AdapterPlayerManager::~AdapterPlayerManager all done" << endl;
}

bool
AdapterPlayerManager::hasPlayer (IAdapterPlayer *player)
{
  bool hasInstance = false;

  Thread::mutexLock (&mutexPlayer);
  if (playerNames.find (player) != playerNames.end ())
    {
      hasInstance = true;
    }
  Thread::mutexUnlock (&mutexPlayer);

  return hasInstance;
}

NclPlayerData *
AdapterPlayerManager::getNclPlayerData ()
{
  return nclPlayerData;
}

void
AdapterPlayerManager::setTimeBaseProvider (
    ITimeBaseProvider *timeBaseProvider)
{

  this->timeBaseProvider = timeBaseProvider;
}

ITimeBaseProvider *
AdapterPlayerManager::getTimeBaseProvider ()
{
  return timeBaseProvider;
}

void
AdapterPlayerManager::setVisible (string objectId, string visible,
                                  AttributionEvent *event)
{

  map<string, IAdapterPlayer *>::iterator i;
  AdapterFormatterPlayer *player;

  Thread::mutexLock (&mutexPlayer);
  i = objectPlayers.find (objectId);
  if (i != objectPlayers.end ())
    {
      player = (AdapterFormatterPlayer *)(i->second);
      player->setPropertyValue (event, visible);
      event->stop ();
    }
  Thread::mutexUnlock (&mutexPlayer);
}

bool
AdapterPlayerManager::removePlayer (void *exObject)
{
  ExecutionObject *object;
  bool removed = false;
  string objId;

  object = (ExecutionObject *)exObject;
  Thread::mutexLock (&mutexPlayer);
  if (ExecutionObject::hasInstance (object, false))
    {
      objId = object->getId ();
      removed = removePlayer (objId);
    }
  Thread::mutexUnlock (&mutexPlayer);

  return removed;
}

bool
AdapterPlayerManager::removePlayer (string objectId)
{
  map<string, IAdapterPlayer *>::iterator i;
  AdapterFormatterPlayer *player;

  i = objectPlayers.find (objectId);
  if (i != objectPlayers.end ())
    {
      player = (AdapterFormatterPlayer *)(i->second);
      if (!player->instanceOf ("AdapterProgramAVPlayer"))
        {
          deletePlayers[objectId] = player;
        }
      objectPlayers.erase (i);
      unlockConditionSatisfied ();
      return true;
    }

  return false;
}

void
AdapterPlayerManager::clear ()
{
  map<string, IAdapterPlayer *>::iterator i;

  Thread::mutexLock (&mutexPlayer);
  i = objectPlayers.begin ();
  while (i != objectPlayers.end ())
    {
      if (removePlayer (i->first))
        {
          i = objectPlayers.begin ();
        }
      else
        {
          ++i;
        }
    }
  objectPlayers.clear ();
  Thread::mutexUnlock (&mutexPlayer);
}

void
AdapterPlayerManager::setNclEditListener (IPlayerListener *listener)
{
  this->editingCommandListener = listener;
}

AdapterFormatterPlayer *
AdapterPlayerManager::initializePlayer (ExecutionObject *object)
{
  NodeEntity *entity;
  Content *content;
  string id;
  string buf;
  const char *mime;

  string classname;
  IAdapterPlayer *adapter = NULL;

  g_assert_nonnull (object);
  id = object->getId ();

  entity = (NodeEntity *)(object->getDataObject ()->getDataEntity ());
  g_assert_nonnull (entity);
  g_assert (entity->instanceOf ("ContentNode"));

  if (((ContentNode *)entity)->isSettingNode ())
    return NULL; // nothing to do

  content = entity->getContent ();
  g_assert (content != NULL);

  if (content->instanceOf ("ReferenceContent"))
    {
      string url;
      char *scheme;

      url = ((ReferenceContent *)(content))->getCompleteReferenceUrl ();
      scheme = g_uri_parse_scheme (url.c_str ());
      if (g_strcmp0 (scheme, "sbtvd-ts") == 0)
        {
          classname = "AdapterProgramAVPlayer";
          adapter = AdapterProgramAVPlayer::getInstance ();
          g_free (scheme);
          goto done;
        }
      else if (g_strcmp0 (scheme, "ncl-mirror") == 0)
        {
          classname = "AdapterMirrorPlayer";
          adapter = new AdapterMirrorPlayer ();
          g_free (scheme);
          goto done;
        }
      else
        {
          g_free (scheme);
        }
    }

  buf = ((ContentNode *)entity)->getNodeType ();
  mime = buf.c_str ();
  g_assert_nonnull (mime);

  if (g_str_has_prefix (mime, "image"))
    {
      classname = "AdapterImagePlayer";
      adapter = new AdapterImagePlayer ();
    }
  else if (g_str_has_prefix (mime, "audio")
           || g_str_has_prefix (mime, "video"))
    {
      classname = "AdapterAVPlayer";
      adapter = new AdapterAVPlayer ();
    }
#if WITH_BERKELIUM
  else if (g_strcmp0 (mime, "text/html"))
    {
      classname = "AdapterBerkeliumPlayer";
      adapter = new AdapterBerkeliumPlayer ();
    }
#endif
  else if (g_strcmp0 (mime, "text/plain") == 0)
    {
      classname = "AdapterPlainTxtPlayer";
      adapter = new AdapterPlainTxtPlayer ();
    }
  else if (g_strcmp0 (mime, "text/srt") == 0)
    {
      classname = "AdapterSubtitlePlayer";
      adapter = new AdapterSubtitlePlayer ();
    }
  else if (g_strcmp0 (mime, "application/x-ginga-NCLua") == 0
           || g_strcmp0 (mime, "application/x-ginga-EPGFactory") == 0)
    {
      classname = "AdapterLuaPlayer";
      adapter = new AdapterLuaPlayer ();
    }
  else if (g_strcmp0 (mime, "application/x-ncl-ncl") == 0
           || g_strcmp0 (mime, "application/x-ginga-ncl") == 0)
    {
      classname = "AdapterNCLPlayer";
      adapter = new AdapterNCLPlayer ();
    }
  else
    {
      g_warning ("unknown mime \"%s\", skipping object id=%s", mime,
                 id.c_str ());
      return NULL;
    }

done:
  adapter->setAdapterManager (this);
  objectPlayers[id] = adapter;
  playerNames[adapter] = classname;

  g_debug ("%s allocated for object id=%s) ", classname.c_str (),
           id.c_str ());

  return (AdapterFormatterPlayer *)adapter;
}

void *
AdapterPlayerManager::getObjectPlayer (void *eObj)
{
  map<string, IAdapterPlayer *>::iterator i;
  AdapterFormatterPlayer *player;
  string objId;
  ExecutionObject *execObj = (ExecutionObject *)eObj;

  Thread::mutexLock (&mutexPlayer);
  objId = execObj->getId ();
  i = objectPlayers.find (objId);
  if (i == objectPlayers.end ())
    {
      i = deletePlayers.find (objId);
      if (i == deletePlayers.end ())
        {
          player = initializePlayer (execObj);
        }
      else
        {
          player = (AdapterFormatterPlayer *)(i->second);
          deletePlayers.erase (i);
          objectPlayers[objId] = player;
        }
    }
  else
    {
      player = (AdapterFormatterPlayer *)(i->second);
    }
  Thread::mutexUnlock (&mutexPlayer);

  return player;
}

string
AdapterPlayerManager::getMimeTypeFromSchema (string url)
{
  string mime = "";

  if ((url.length () > 8 && url.substr (0, 8) == "https://")
      || (url.length () > 7 && url.substr (0, 7) == "http://")
      || (url.length () > 4 && url.substr (0, 4) == "www."))
    {

      clog << "AdapterPlayerManager::getMimeTypeFromSchema is ";
      clog << "considering HTML MIME." << endl;

      mime = ContentTypeManager::getInstance ()->getMimeType ("html");
    }
  else if ((url.length () > 6 && url.substr (0, 6) == "rtp://")
           || (url.length () > 7 && url.substr (0, 7) == "rtsp://"))
    {

      mime = ContentTypeManager::getInstance ()->getMimeType ("mpg");
    }

  return mime;
}

bool
AdapterPlayerManager::isEmbeddedApp (NodeEntity *dataObject)
{
  string mediaType = "";
  string url = "";
  string::size_type pos;
  Descriptor *descriptor;
  Content *content;

  // first, descriptor
  descriptor = (Descriptor *)(dataObject->getDescriptor ());
  if (descriptor != NULL && !descriptor->instanceOf ("DescriptorSwitch"))
    {
      mediaType = descriptor->getPlayerName ();
      if (mediaType == "AdapterLuaPlayer"
          || mediaType == "AdapterBerkeliumPlayer"
          || mediaType == "AdapterNCLPlayer")
        {

          return true;
        }
    }

  // second, media type
  if (dataObject->instanceOf ("ContentNode"))
    {
      mediaType = ((ContentNode *)dataObject)->getNodeType ();
      if (mediaType != "")
        {
          return isEmbeddedAppMediaType (mediaType);
        }
    }

  // finally, content file extension
  content = dataObject->getContent ();
  if (content != NULL)
    {
      if (content->instanceOf ("ReferenceContent"))
        {
          url = ((ReferenceContent *)(content))->getCompleteReferenceUrl ();

          if (url != "")
            {
              pos = url.find_last_of (".");
              if (pos != std::string::npos)
                {
                  pos++;
                  mediaType
                      = ContentTypeManager::getInstance ()->getMimeType (
                          url.substr (pos, url.length () - pos));

                  return isEmbeddedAppMediaType (mediaType);
                }
            }
        }
    }

  return false;
}

bool
AdapterPlayerManager::isEmbeddedAppMediaType (string mediaType)
{
  string upMediaType = upperCase (mediaType);

  if (upMediaType == "APPLICATION/X-GINGA-NCLUA"
      || upMediaType == "APPLICATION/X-GINGA-NCLET"
      || upMediaType == "APPLICATION/X-GINGA-NCL"
      || upMediaType == "APPLICATION/X-NCL-NCL"
      || upMediaType == "APPLICATION/X-NCL-NCLUA")
    {

      return true;
    }

  return false;
}

void
AdapterPlayerManager::timeShift (string direction)
{
  map<string, IAdapterPlayer *>::iterator i;
  AdapterFormatterPlayer *player;

  Thread::mutexLock (&mutexPlayer);
  i = objectPlayers.begin ();
  while (i != objectPlayers.end ())
    {
      player = (AdapterFormatterPlayer *)(i->second);
      player->timeShift (direction);
      ++i;
    }
  Thread::mutexUnlock (&mutexPlayer);
}

void
AdapterPlayerManager::clearDeletePlayers ()
{
  map<string, IAdapterPlayer *> dPlayers;

  map<string, IAdapterPlayer *>::iterator i;
  map<IAdapterPlayer *, string>::iterator j;
  IAdapterPlayer *player;
  string playerClassName = "";

  Thread::mutexLock (&mutexPlayer);
  i = deletePlayers.begin ();
  while (i != deletePlayers.end ())
    {
      player = i->second;

      j = playerNames.find (player);
      if (j != playerNames.end ())
        {
          playerClassName = j->second;
          playerNames.erase (j);
        }

      if (((AdapterFormatterPlayer *)player)->getObjectDevice () == 0)
        {
          dPlayers[playerClassName] = player;
        }

      ++i;
    }
  deletePlayers.clear ();
  Thread::mutexUnlock (&mutexPlayer);

  i = dPlayers.begin ();
  while (i != dPlayers.end ())
    {
      player = i->second;
      playerClassName = i->first;

      delete player;

      ++i;
    }
}

void
AdapterPlayerManager::run ()
{
  set<IAdapterPlayer *>::iterator i;

  while (running)
    {
      if (!isDeleting)
        {
          Thread::mutexLock (&mutexPlayer);
          if (deletePlayers.empty ())
            {
              Thread::mutexUnlock (&mutexPlayer);
              waitForUnlockCondition ();
            }
          else
            {
              Thread::mutexUnlock (&mutexPlayer);
            }
        }

      if (!running)
        {
          return;
        }

      if (isDeleting)
        {
          break;
        }

      Thread::mSleep (1000);
      if (running)
        {
          clearDeletePlayers ();
        }
    }

  clog << "AdapterPlayerManager::run all done" << endl;
}

BR_PUCRIO_TELEMIDIA_GINGA_NCL_ADAPTERS_END
