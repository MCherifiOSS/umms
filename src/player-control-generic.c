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
#include "player-control-generic.h"
#include "media-player-control.h"
#include "param-table.h"

G_DEFINE_TYPE (PlayerControlGeneric, player_control_generic, PLAYER_CONTROL_TYPE_BASE);

#define GET_PRIVATE(o) ((PLAYER_CONTROL_BASE(o))->priv)

/*virtual Apis: */
static gboolean
set_target (PlayerControlBase *self, gint type, GHashTable *params)
{
  gboolean ret = TRUE;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);
  UMMS_DEBUG("Not Supportted\n");

  return ret;
}

static gboolean
activate_player_control (PlayerControlBase *self, GstState target_state)
{
  gboolean ret;
  PlayerState old_pending;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);

  UMMS_DEBUG("derived one");

  g_return_val_if_fail (priv->uri, FALSE);
  g_return_val_if_fail (target_state == GST_STATE_PAUSED || target_state == GST_STATE_PLAYING, FALSE);

  old_pending = priv->pending_state;
  priv->pending_state = ((target_state == GST_STATE_PAUSED) ? PlayerStatePaused : PlayerStatePlaying);

  if (gst_element_set_state(priv->pipeline, target_state) == GST_STATE_CHANGE_FAILURE) {
    UMMS_DEBUG ("set pipeline to %d failed", target_state);
    ret = FALSE;
    goto OUT;
  }

OUT:
  if (!ret) {
    priv->pending_state = old_pending;
  }
  return ret;
}

static void
player_control_generic_get_property (GObject    *object,
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
player_control_generic_set_property (GObject      *object,
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
player_control_generic_dispose (GObject *object)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (object);
  G_OBJECT_CLASS (player_control_generic_parent_class)->dispose (object);
}

static void
player_control_generic_finalize (GObject *object)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (object);

  G_OBJECT_CLASS (player_control_generic_parent_class)->finalize (object);
}

static void
player_control_generic_class_init (PlayerControlGenericClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  UMMS_DEBUG("Called");

  g_type_class_add_private (klass, sizeof (PlayerControlBasePrivate));

  object_class->get_property = player_control_generic_get_property;
  object_class->set_property = player_control_generic_set_property;
  object_class->dispose = player_control_generic_dispose;
  object_class->finalize = player_control_generic_finalize;

  PlayerControlBaseClass *parent_class = PLAYER_CONTROL_BASE_CLASS(klass);
  /*derived one*/
  parent_class->activate_player_control = activate_player_control;
  parent_class->set_target = set_target;

}

static void
player_control_generic_init (PlayerControlGeneric *self)
{
  PlayerControlBasePrivate *priv;
  GstBus *bus;

  UMMS_DEBUG("Called");
}

PlayerControlGeneric *
player_control_generic_new (void)
{
  UMMS_DEBUG("Called");
  return g_object_new (PLAYER_CONTROL_TYPE_GENERIC, NULL);
}

