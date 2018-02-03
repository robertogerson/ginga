/* Copyright (C) 2006-2018 PUC-Rio/Laboratorio TeleMidia

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
along with Ginga.  If not, see <https://www.gnu.org/licenses/>.  */

#include "aux-ginga.h"
#include "aux-gl.h"
#include "Formatter.h"

#include "Context.h"
#include "Media.h"
#include "MediaSettings.h"
#include "Object.h"
#include "Switch.h"

#include "Parser.h"
#include "PlayerText.h"

#if defined WITH_LUA && WITH_LUA
#include "ParserLua.h" // for ncl-ltab support
#endif

GINGA_NAMESPACE_BEGIN

// Option defaults.
static GingaOptions opts_defaults = {
  800,   // width
  600,   // height
  false, // debug
  false, // experimental
  false, // opengl
  "",    // background ("" == none)
};

// Option data.
typedef struct GingaOptionData
{
  GType type; // option type
  int offset; // offset in GingaOption struct
  void *func; // update function
} OptionTab;

#define OPTS_ENTRY(name, type, func)                                       \
  {                                                                        \
    G_STRINGIFY (name),                                                    \
    {                                                                      \
      (type), offsetof (GingaOptions, name),                               \
          pointerof (G_PASTE (Formatter::setOption, func))                 \
    }                                                                      \
  }

// Option table.
static map<string, GingaOptionData> opts_table = {
  OPTS_ENTRY (background, G_TYPE_STRING, Background),
  OPTS_ENTRY (debug, G_TYPE_BOOLEAN, Debug),
  OPTS_ENTRY (experimental, G_TYPE_BOOLEAN, Experimental),
  OPTS_ENTRY (height, G_TYPE_INT, Size),
  OPTS_ENTRY (opengl, G_TYPE_BOOLEAN, OpenGL),
  OPTS_ENTRY (width, G_TYPE_INT, Size),
};

// Indexes option table.
static bool
opts_table_index (const string &key, GingaOptionData **result)
{
  map<string, GingaOptionData>::iterator it;
  if ((it = opts_table.find (key)) == opts_table.end ())
    return false;
  tryset (result, &it->second);
  return true;
}

// Compares the z-index and z-order of two media objects.
static int
zcmp (Media *a, Media *b)
{
  int z1, zo1, z2, zo2;

  g_assert_nonnull (a);
  g_assert_nonnull (b);

  z1 = zo1 = z2 = zo2 = 0;
  a->getZ (&z1, &zo1);
  b->getZ (&z2, &zo2);

  if (z1 < z2)
    return -1;
  if (z1 > z2)
    return 1;
  if (zo1 < zo2)
    return -1;
  if (zo1 > zo2)
    return 1;
  return 0;
}

// Public: External API.

GingaState
Formatter::getState ()
{
  return _state;
}

bool
Formatter::start (const string &file, string *errmsg)
{
  int w, h;
  string id;
  Event *evt;

  if (_state != GINGA_STATE_STOPPED)
    return false; // nothing to do

  // Parse document.
  g_assert_null (_doc);
  w = _opts.width;
  h = _opts.height;
  _doc = nullptr;
#if defined WITH_LUA && WITH_LUA
  if (xstrhassuffix (file, ".lua"))
    {
      _doc = ParserLua::parseFile (file, errmsg);
      if (unlikely (_doc == nullptr))
        return false;
    }
#endif
  if (_doc == nullptr)
    _doc = Parser::parseFile (file, w, h, errmsg);
  if (unlikely (_doc == nullptr))
    return false;

  g_assert_nonnull (_doc);
  _doc->setData ("formatter", (void *) this);

  Context *root = _doc->getRoot ();
  g_assert_nonnull (root);
  MediaSettings *settings = _doc->getSettings ();
  g_assert_nonnull (settings);

  // Initialize formatter variables.
  _docPath = file;
  _eos = false;
  _last_tick_total = 0;
  _last_tick_diff = 0;
  _last_tick_frameno = 0;

  // Run document.
  TRACE ("%s", file.c_str ());
  evt = root->getLambda ();
  g_assert_nonnull (evt);
  if (_doc->evalAction (evt, Event::START) == 0)
    return false;

  // Start settings.
  evt = _doc->getSettings ()->getLambda ();
  g_assert_nonnull (evt);
  g_assert (evt->transition (Event::START));

  // Refresh current focus.
  settings->updateCurrentFocus ("");

  _state = GINGA_STATE_PLAYING;
  return true;
}

bool
Formatter::start (const string &buf, size_t size, string *errmsg)
{
  Context *root;
  MediaSettings *settings;

  if (_state != GINGA_STATE_STOPPED)
    return false; // nothing to do

  _doc = Parser::parseBuffer (buf.c_str (), size, _opts.width, _opts.height,
                              errmsg);
  if (unlikely (_doc == nullptr))
    return false;

  g_assert_nonnull (_doc);
  _doc->setData ("formatter", (void *) this);

  // Initialize formatter variables.
  _docPath = "(buffer)";
  _eos = false;
  _last_tick_total = 0;
  _last_tick_diff = 0;
  _last_tick_frameno = 0;

  // Start root context (body).
  root = _doc->getRoot ();
  g_assert_nonnull (root);
  if (unlikely (_doc->evalAction (root->getLambda (), Event::START) == 0))
    return false;

  // start settings
  Event *evt = _doc->getSettings ()->getLambda ();
  g_assert_nonnull (evt);
  g_assert (evt->transition (Event::START));

  // Refresh current focus.
  settings = _doc->getSettings ();
  g_assert_nonnull (settings);
  settings->updateCurrentFocus ("");

  // Success.
  _state = GINGA_STATE_PLAYING;
  return true;
}

bool
Formatter::stop ()
{
  if (_state == GINGA_STATE_STOPPED)
    return false; // nothing to do

  delete _doc;
  _doc = nullptr;

  _state = GINGA_STATE_STOPPED;
  return true;
}

void
Formatter::resize (int width, int height)
{
  g_assert (width > 0 && height > 0);
  _opts.width = width;
  _opts.height = height;

  if (_state != GINGA_STATE_PLAYING)
    return; // nothing to do

  for (auto media : *_doc->getMedias ())
    {
      string top;
      string bottom;
      string left;
      string right;
      string width;
      string height;

      top = media->getProperty ("top");
      if (top != "")
        media->setProperty ("top", top);

      bottom = media->getProperty ("bottom");
      if (bottom != "")
        media->setProperty ("bottom", bottom);

      left = media->getProperty ("left");
      if (left != "")
        media->setProperty ("left", left);

      right = media->getProperty ("right");
      if (right != "")
        media->setProperty ("right", right);

      width = media->getProperty ("width");
      media->setProperty ("width", width);

      height = media->getProperty ("height");
      media->setProperty ("height", height);
    }
}

void
Formatter::redraw (cairo_t *cr)
{
  GList *zlist;
  GList *l;

  if (_state != GINGA_STATE_PLAYING)
    return; // nothing to do

  if (_opts.opengl) // clear screen opengl
    {
      GL::beginDraw ();
      GL::clear_scene (_opts.width, _opts.height);
    }
  else // clear screen cairo
    {
      cairo_save (cr);
      cairo_set_source_rgba (cr, 0, 0, 0, 1.0);
      cairo_rectangle (cr, 0, 0, _opts.width, _opts.height);
      cairo_fill (cr);
      cairo_restore (cr);
    }

  if (_background.alpha > 0)
    {
      if (_opts.opengl)
        {
          GL::draw_quad (0, 0, _opts.width, _opts.height,
                         (float) _background.red, (float) _background.green,
                         (float) _background.blue,
                         (float) _background.alpha);
        }
      else
        {
          cairo_save (cr);
          cairo_set_source_rgba (cr, _background.red, _background.green,
                                 _background.blue, _background.alpha);
          cairo_rectangle (cr, 0, 0, _opts.width, _opts.height);
          cairo_fill (cr);
          cairo_restore (cr);
        }
    }

  zlist = nullptr;
  for (auto &media : *_doc->getMedias ())
    zlist = g_list_insert_sorted (zlist, media, (GCompareFunc) zcmp);

  l = zlist;
  while (l != NULL)
    {
      GList *next = l->next;
      ((Media *) l->data)->redraw (cr);
      zlist = g_list_delete_link (zlist, l);
      l = next;
    }
  g_assert_null (zlist);

  if (_opts.debug)
    {
      static Color fg = {1., 1., 1., 1. };
      static Color bg = {0, 0, 0, 0};
      static Rect rect = {0, 0, 0, 0};
      string info;
      cairo_surface_t *debug;
      Rect ink;
      info = xstrbuild ("%s: #%lu %" GINGA_TIME_FORMAT " %.1ffps",
                        _docPath.c_str (), _last_tick_frameno,
                        GINGA_TIME_ARGS (_last_tick_total),
                        1 * GINGA_SECOND / (double) _last_tick_diff);
      rect.width = _opts.width;
      rect.height = _opts.height;
      debug = PlayerText::renderSurface (info, "monospace", "", "bold", "9",
                                         fg, bg, rect, "center", "", true,
                                         &ink);
      ink = {0, 0, rect.width, ink.height - ink.y + 4};
      cairo_save (cr);
      cairo_set_source_rgba (cr, 1., 0., 0., .5);
      cairo_rectangle (cr, 0, 0, ink.width, ink.height);
      cairo_fill (cr);
      cairo_set_source_surface (cr, debug, 0, 0);
      cairo_paint (cr);
      cairo_restore (cr);
      cairo_surface_destroy (debug);
    }
}

// Stops formatter if EOS has been seen.
#define _GINGA_CHECK_EOS(ginga)                                            \
  G_STMT_START                                                             \
  {                                                                        \
    Context *root;                                                         \
    root = _doc->getRoot ();                                               \
    g_assert_nonnull (root);                                               \
    if (root->isSleeping ())                                               \
      {                                                                    \
        (ginga)->setEOS (true);                                            \
      }                                                                    \
    if ((ginga)->getEOS ())                                                \
      {                                                                    \
        g_assert ((ginga)->_state == GINGA_STATE_PLAYING);                 \
        (ginga)->setEOS (false);                                           \
        g_assert ((ginga)->stop ());                                       \
        g_assert ((ginga)->_state == GINGA_STATE_STOPPED);                 \
      }                                                                    \
  }                                                                        \
  G_STMT_END

bool
Formatter::sendKey (const string &key, bool press)
{
  list<Object *> buf;

  _GINGA_CHECK_EOS (this);
  if (_state != GINGA_STATE_PLAYING)
    return false; // nothing to do

  // IMPORTANT: When propagating a key to the objects, we cannot traverse
  // the object set directly, as the reception of a key may cause this set
  // to be modified.  We thus need to create a buffer with the objects that
  // should receive the key, i.e., those that are not sleeping, and then
  // propagate the key only to the objects in this buffer.
  for (auto obj : *_doc->getObjects ())
    if (!obj->isSleeping ())
      buf.push_back (obj);
  for (auto obj : buf)
    obj->sendKey (key, press);

  return true;
}

bool
Formatter::sendTick (uint64_t total, uint64_t diff, uint64_t frame)
{
  list<Object *> buf;

  // natural end of the document
  if (_doc->getRoot ()->isOccurring () == false)
    this->setEOS (true);

  _GINGA_CHECK_EOS (this);

  if (_state != GINGA_STATE_PLAYING)
    return false; // nothing to do

  _last_tick_total = total;
  _last_tick_diff = diff;
  _last_tick_frameno = frame;

  // IMPORTANT: The same warning about propagation that appear in
  // Formatter::sendKeyEvent() applies here.  The difference is that ticks
  // should only be propagated to objects that are occurring.
  for (auto obj : *_doc->getObjects ())
    if (obj->isOccurring ())
      buf.push_back (obj);
  for (auto obj : buf)
    obj->sendTick (total, diff, frame);

  return true;
}

const GingaOptions *
Formatter::getOptions ()
{
  return &_opts;
}

#define OPT_ERR_UNKNOWN(name) ERROR ("unknown GingaOption '%s'", (name))
#define OPT_ERR_BAD_TYPE(name, typename)                                   \
  ERROR ("GingaOption '%s' is of type '%s'", (name), (typename))

#define OPT_GETSET_DEFN(Name, Type, GType)                                 \
  Type Formatter::getOption##Name (const string &name)                     \
  {                                                                        \
    GingaOptionData *opt;                                                  \
    if (unlikely (!opts_table_index (name, &opt)))                         \
      OPT_ERR_UNKNOWN (name.c_str ());                                     \
    if (unlikely (opt->type != (GType)))                                   \
      OPT_ERR_BAD_TYPE (name.c_str (), G_STRINGIFY (Type));                \
    return *((Type *) (((ptrdiff_t) &_opts) + opt->offset));               \
  }                                                                        \
  void Formatter::setOption##Name (const string &name, Type value)         \
  {                                                                        \
    GingaOptionData *opt;                                                  \
    if (unlikely (!opts_table_index (name, &opt)))                         \
      OPT_ERR_UNKNOWN (name.c_str ());                                     \
    if (unlikely (opt->type != (GType)))                                   \
      OPT_ERR_BAD_TYPE (name.c_str (), G_STRINGIFY (Type));                \
    *((Type *) (((ptrdiff_t) &_opts) + opt->offset)) = value;              \
    if (opt->func)                                                         \
      {                                                                    \
        ((void (*) (Formatter *, const string &, Type)) opt->func) (       \
            this, name, value);                                            \
      }                                                                    \
  }

OPT_GETSET_DEFN (Bool, bool, G_TYPE_BOOLEAN)
OPT_GETSET_DEFN (Int, int, G_TYPE_INT)
OPT_GETSET_DEFN (String, string, G_TYPE_STRING)

// Public: Internal API.

Formatter::Formatter (unused (int argc), unused (char **argv),
                      GingaOptions *opts)
    : Ginga (argc, argv, opts)
{
  const char *s;

  _state = GINGA_STATE_STOPPED;
  _opts = (opts) ? *opts : opts_defaults;
  _background = {0., 0., 0., 0. };

  _last_tick_total = 0;
  _last_tick_diff = 0;
  _last_tick_frameno = 0;
  _saved_G_MESSAGES_DEBUG
      = (s = g_getenv ("G_MESSAGES_DEBUG")) ? string (s) : "";

  _doc = nullptr;
  _docPath = "";
  _eos = false;

  // Initialize options.
  setOptionBackground (this, "background", _opts.background);
  setOptionDebug (this, "debug", _opts.debug);
  setOptionExperimental (this, "experimental", _opts.experimental);
  setOptionOpenGL (this, "opengl", _opts.opengl);
}

Formatter::~Formatter ()
{
  this->stop ();
}

Document *
Formatter::getDocument ()
{
  return _doc;
}

bool
Formatter::getEOS ()
{
  return _eos;
}

void
Formatter::setEOS (bool eos)
{
  _eos = eos;
}

// Public: Static.

void
Formatter::setOptionBackground (Formatter *self, const string &name,
                                string value)
{
  g_assert (name == "background");
  if (value == "")
    self->_background = {0., 0., 0., 0. };
  else
    self->_background = ginga::parse_color (value);
  TRACE ("%s:='%s'", name.c_str (), value.c_str ());
}

void
Formatter::setOptionDebug (Formatter *self, const string &name, bool value)
{
  g_assert (name == "debug");
  if (value)
    {
      const char *curr = g_getenv ("G_MESSAGES_DEBUG");
      if (curr != nullptr)
        self->_saved_G_MESSAGES_DEBUG = string (curr);
      g_assert (g_setenv ("G_MESSAGES_DEBUG", "all", true));
    }
  else
    {
      g_assert (g_setenv ("G_MESSAGES_DEBUG",
                          self->_saved_G_MESSAGES_DEBUG.c_str (), true));
    }
  TRACE ("%s:=%s", name.c_str (), strbool (value));
}

void
Formatter::setOptionExperimental (unused (Formatter *self),
                                  const string &name, bool value)
{
  g_assert (name == "experimental");
  TRACE ("%s:=%s", name.c_str (), strbool (value));
}

void
Formatter::setOptionOpenGL (unused (Formatter *self), const string &name,
                            bool value)
{
  g_assert (name == "opengl");
#if !(defined WITH_OPENGL && WITH_OPENGL)
  if (unlikely (value))
    ERROR ("Not compiled with OpenGL support");
#endif
  TRACE ("%s:=%s", name.c_str (), strbool (value));
}

void
Formatter::setOptionSize (Formatter *self, const string &name, int value)
{
  const GingaOptions *opts;
  g_assert (name == "width" || name == "height");
  opts = self->getOptions ();
  self->resize (opts->width, opts->height);
  TRACE ("%s:=%d", name.c_str (), value);
}

GINGA_NAMESPACE_END
