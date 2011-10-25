/*
 * UMMS (Unified Multi Media Service) provides a set of DBus APIs to support
 * playing Audio and Video as well as DVB playback.
 *
 * Authored by Zhiwen Wu <zhiwen.wu@intel.com>
 *             Junyan He <junyan.he@intel.com>
 * Copyright (c) 2011 Intel Corp.
 *
 * UMMS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * UMMS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with UMMS; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
/* for the volume property */
#include <gst/interfaces/streamvolume.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <glib/gprintf.h>
#include "umms-common.h"
#include "umms-debug.h"
#include "umms-error.h"
#include "umms-resource-manager.h"
#include "engine-tv.h"
#include "media-player-control.h"
#include "param-table.h"

G_DEFINE_TYPE (EngineTv, engine_tv, ENGINE_TYPE_COMMON);

#define GET_PRIVATE(o) ((ENGINE_COMMON(o))->priv)

/*virtual Apis: */
#if 0
static gboolean
set_target (MediaPlayerControl *self, gint type, GHashTable *params)

static gboolean
activate_engine (EngineCommon *self, GstState target_state)
#endif

static void
engine_tv_get_property (GObject    *object,
    guint       property_id,
    GValue     *value,
    GParamSpec *pspec)
{
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
engine_tv_set_property (GObject      *object,
    guint         property_id,
    const GValue *value,
    GParamSpec   *pspec)
{
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
engine_tv_dispose (GObject *object)
{
  EngineCommonPrivate *priv = GET_PRIVATE (object);
  G_OBJECT_CLASS (engine_tv_parent_class)->dispose (object);
}

static void
engine_tv_finalize (GObject *object)
{
  EngineCommonPrivate *priv = GET_PRIVATE (object);

  G_OBJECT_CLASS (engine_tv_parent_class)->finalize (object);
}

static void
engine_tv_class_init (EngineTvClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  UMMS_DEBUG("Called");

  g_type_class_add_private (klass, sizeof (EngineCommonPrivate));

  object_class->get_property = engine_tv_get_property;
  object_class->set_property = engine_tv_set_property;
  object_class->dispose = engine_tv_dispose;
  object_class->finalize = engine_tv_finalize;

  EngineCommonClass *parent_class = ENGINE_COMMON_CLASS(klass);
  /*derived one*/
#if 0
  parent_class->activate_engine = activate_engine;
  parent_class->set_target = set_target;
#endif

}

static void
engine_tv_init (EngineTv *self)
{
  EngineCommonPrivate *priv;
  GstBus *bus;

  UMMS_DEBUG("Called");
}

EngineTv *
engine_tv_new (void)
{
  UMMS_DEBUG("Called");
  return g_object_new (ENGINE_TYPE_TV, NULL);
}

