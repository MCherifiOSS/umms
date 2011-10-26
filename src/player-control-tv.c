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
#include "player-control-tv.h"
#include "media-player-control.h"
#include "param-table.h"

G_DEFINE_TYPE (PlayerControlTv, player_control_tv, PLAYER_CONTROL_TYPE_BASE);

#define GET_PRIVATE(o) ((PLAYER_CONTROL_BASE(o))->priv)

/*virtual Apis: */
#if 0
static gboolean
set_target (MediaPlayerControl *self, gint type, GHashTable *params)

static gboolean
activate_player_control (PlayerControlBase *self, GstState target_state)
#endif

static void
player_control_tv_get_property (GObject    *object,
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
player_control_tv_set_property (GObject      *object,
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
player_control_tv_dispose (GObject *object)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (object);
  G_OBJECT_CLASS (player_control_tv_parent_class)->dispose (object);
}

static void
player_control_tv_finalize (GObject *object)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (object);

  G_OBJECT_CLASS (player_control_tv_parent_class)->finalize (object);
}

static void
player_control_tv_class_init (PlayerControlTvClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  UMMS_DEBUG("Called");

  g_type_class_add_private (klass, sizeof (PlayerControlBasePrivate));

  object_class->get_property = player_control_tv_get_property;
  object_class->set_property = player_control_tv_set_property;
  object_class->dispose = player_control_tv_dispose;
  object_class->finalize = player_control_tv_finalize;

  PlayerControlBaseClass *parent_class = PLAYER_CONTROL_BASE_CLASS(klass);
  /*derived one*/
#if 0
  parent_class->activate_player_control = activate_player_control;
  parent_class->set_target = set_target;
#endif

}

static void
player_control_tv_init (PlayerControlTv *self)
{
  PlayerControlBasePrivate *priv;
  GstBus *bus;

  UMMS_DEBUG("Called");
}

PlayerControlTv *
player_control_tv_new (void)
{
  UMMS_DEBUG("Called");
  return g_object_new (PLAYER_CONTROL_TYPE_TV, NULL);
}

