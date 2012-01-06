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
#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
/* for the volume property */
#include <gst/interfaces/streamvolume.h>
#include <gst/app/gstappsink.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <glib/gprintf.h>
#include "umms-common.h"
#include "umms-debug.h"
#include "umms-error.h"
#include "umms-resource-manager.h"
#include "dvb-player-control-generic.h"
#include "media-player-control.h"
#include "param-table.h"

G_DEFINE_TYPE (DvbPlayerControlGeneric,dvb_player_control_generic, DVB_PLAYER_CONTROL_TYPE_BASE);

#define PLAYER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),DVB_PLAYER_CONTROL_TYPE_BASE, DvbPlayerControlBasePrivate))

#define GET_PRIVATE(o) ((DvbPlayerControlBase *)o)->priv

/*
 * URI pattern: 
 * dvb://?program-number=x&type=x&modulation=x&trans-mod=x&bandwidth=x&frequency=x&code-rate-lp=x&
 * code-rate-hp=x&guard=x&hierarchy=x
 */

static GstElement *create_video_bin (void);
static GstElement *create_audio_bin (void);

/*virtual Apis: */
static gboolean setup_xwindow_target (DvbPlayerControlBase *self, GHashTable *params)
{
  GValue *val;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);

  val = g_hash_table_lookup (params, "window-id");
  if (!val) {
    UMMS_DEBUG ("no window-id");
    return FALSE;
  }

  priv->video_win_id = (Window)g_value_get_int (val);
  priv->target_type = XWindow;

  return TRUE;
}

static gboolean
set_target (DvbPlayerControlBase *self, gint type, GHashTable *params)
{
  gboolean ret = TRUE;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);
  UMMS_DEBUG("derived one");

  if (priv->player_state != PlayerStateNull && priv->player_state != PlayerStateStopped) {
    UMMS_DEBUG ("Ignored, can only set target at PlayerStateNull or PlayerStateStopped");
    return FALSE;
  }

  if (!priv->pipeline) {
    UMMS_DEBUG ("Engine not loaded, reason may be SetUri not invoked");
    return FALSE;
  }

  switch (type) {
    case XWindow:
      ret = setup_xwindow_target (self, params);
      break;
    case DataCopy:
    case Socket:
    case ReservedType0:
    default:
      ret = FALSE;
      break;
  }

  if (ret) {
    priv->target_type = type;
  }
  return ret;
}

static gboolean
set_volume (MediaPlayerControl *self, gint vol)
{
  GstElement *pipeline;
  DvbPlayerControlBasePrivate *priv;
  gdouble volume;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);

  UMMS_DEBUG ("invoked");

  volume = CLAMP ((((gdouble)vol) / 100), 0.0, 1.0);
  UMMS_DEBUG ("set volume to = %f", volume);
  gst_stream_volume_set_volume (GST_STREAM_VOLUME (pipeline),
      GST_STREAM_VOLUME_FORMAT_CUBIC,
      volume);

  return TRUE;
}

static gboolean
get_volume (MediaPlayerControl *self, gint *volume)
{
  GstElement *pipeline;
  DvbPlayerControlBasePrivate *priv;
  gdouble vol;

  *volume = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);

  UMMS_DEBUG ("invoked");

  vol = gst_stream_volume_get_volume (GST_STREAM_VOLUME (pipeline),
        GST_STREAM_VOLUME_FORMAT_CUBIC);

  *volume = vol * 100;
  UMMS_DEBUG ("cur volume=%f(double), %d(int)", vol, *volume);

  return TRUE;
}

static gboolean
set_mute (MediaPlayerControl *self, gint mute) 
{
  GstElement *pipeline;
  DvbPlayerControlBasePrivate *priv;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);

  UMMS_DEBUG ("set mute to = %d", mute);
  gst_stream_volume_set_mute (GST_STREAM_VOLUME (pipeline), mute);

  return TRUE;
}

static gboolean
is_mute (MediaPlayerControl *self, gint *mute) 
{
  GstElement *pipeline;
  DvbPlayerControlBasePrivate *priv;
  gboolean is_mute;

  *mute = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);

  is_mute = gst_stream_volume_get_mute (GST_STREAM_VOLUME (pipeline));
  UMMS_DEBUG("Get the mute %d", is_mute);
  *mute = is_mute;

  return TRUE;
}

static gboolean
set_video_size (DvbPlayerControlBase *self,
    guint x, guint y, guint w, guint h)

{
  UMMS_DEBUG("derived set_video_size one");
  return TRUE;
}

static gboolean
get_video_size (DvbPlayerControlBase *self, guint *w, guint *h)
{
  UMMS_DEBUG("derived get_video_size one");
  return TRUE;
}

static gboolean
set_scale_mode (MediaPlayerControl *self, gint scale_mode) 
{
  UMMS_DEBUG("derived one");
  return TRUE;
}


static gboolean
get_scale_mode (MediaPlayerControl *self, gint *scale_mode)
{
  UMMS_DEBUG("derived one");
  return TRUE;
}


static void
dvb_player_control_generic_get_property (GObject    *object,
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
dvb_player_control_generic_set_property (GObject      *object,
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
dvb_player_control_generic_dispose (GObject *object)
{
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (object);
  G_OBJECT_CLASS (dvb_player_control_generic_parent_class)->dispose (object);
}

static void
dvb_player_control_generic_finalize (GObject *object)
{
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (object);
  G_OBJECT_CLASS (dvb_player_control_generic_parent_class)->finalize (object);
}

static void autoplug_multiqueue (DvbPlayerControlBase *player,GstPad *srcpad,gchar *name)
{
  DvbPlayerControlBaseClass *kclass = DVB_PLAYER_CONTROL_BASE_GET_CLASS(player);

  if (g_str_has_prefix (name, "video")) {
    kclass->autoplug_pad (player, srcpad, CHAIN_TYPE_VIDEO);
  } else if (g_str_has_prefix (name, "audio")) {
    kclass->autoplug_pad (player, srcpad, CHAIN_TYPE_AUDIO);
  } else {
    //FIXME: do we need to handle flustsdemux's subpicture/private pad?
  }
  return;
}


static void
element_link (GstElement *pipeline, DvbPlayerControlBase *self)
{
  DvbPlayerControlBasePrivate *priv;
  GstElement *source = NULL;
  GstElement *front_queue = NULL;
  GstElement *tsdemux = NULL;
  GstElement *vsink = NULL;
  GstElement *asink = NULL;
  DvbPlayerControlBaseClass *kclass = DVB_PLAYER_CONTROL_BASE_GET_CLASS(self);

  self->priv = PLAYER_PRIVATE (self);
  priv = self->priv;

/*frontend pipeline: dvbsrc --> queue  --> flutsdemux*/
#ifdef DVB_SRC
  priv->source = source = gst_element_factory_make ("dvbsrc", "dvb-src");
  if (!source) {
    UMMS_DEBUG ("Creating dvbsrc failed");
    goto failed;
  }
#else
  priv->source = source = gst_element_factory_make ("filesrc", NULL);
  if (!source) {
    UMMS_DEBUG ("Creating filesrc failed");
    goto failed;
  }
  g_object_set (priv->source, "location", "/home/zhiwen/tvb.ts", NULL); // Qing
  UMMS_DEBUG ("\n Need to load /home/zhiwen/tvb.ts ");

#endif
  front_queue   = gst_element_factory_make ("queue", "front-queue");
  priv->tsdemux = tsdemux = gst_element_factory_make ("flutsdemux", NULL);
  
  priv->asink = asink = create_audio_bin();
  priv->vsink = vsink = create_video_bin();

  if (!front_queue || !tsdemux || !asink || !vsink) {
    UMMS_DEBUG ("front_queue(%p), tsdemux(%p), asink(%p)", \
                front_queue, tsdemux, asink, vsink);
    UMMS_DEBUG ("Creating element failed\n");
    goto failed;
  }
  else{
    UMMS_DEBUG ("\n OK @ front_queue(%p), tsdemux(%p), asink(%p)", \
                front_queue, tsdemux, asink, vsink);
  }
  //we need ref these element for later using.
  gst_object_ref_sink (source);
  gst_object_ref_sink (tsdemux);
  gst_object_ref_sink (asink);
  gst_object_ref_sink (vsink);

  g_signal_connect (tsdemux, "pad-added", G_CALLBACK (kclass->pad_added_cb), self);

  /* Add and link frontend elements*/
  gst_bin_add_many (GST_BIN (pipeline),source, front_queue, tsdemux, NULL);
  gst_element_link_many (source, front_queue, tsdemux, NULL);

  //TODO:Create socket thread

  priv->player_state = PlayerStateNull;
  priv->res_mngr = umms_resource_manager_new ();
  priv->res_list = NULL;
  priv->tag_list = NULL;

  priv->seekable = -1;
  priv->resource_prepared = FALSE;
  priv->has_video = FALSE;
  priv->hw_viddec = 0;
  priv->has_audio = FALSE;
  priv->hw_auddec = FALSE;
  priv->title = NULL;
  priv->artist = NULL;

  //URI is one of metadatas whose change should be notified.
  media_player_control_emit_metadata_changed (self);
  return;

failed:
  TEARDOWN_ELEMENT(pipeline);
  TEARDOWN_ELEMENT(source);
  TEARDOWN_ELEMENT(front_queue);
  TEARDOWN_ELEMENT(tsdemux);
  TEARDOWN_ELEMENT(vsink);
  TEARDOWN_ELEMENT(asink);
  return;
}

static void
dvb_player_control_generic_init (DvbPlayerControlGeneric *self)
{
  GstElement *pipeline = NULL;
  DvbPlayerControlBaseClass *kclass = DVB_PLAYER_CONTROL_BASE_GET_CLASS((DvbPlayerControlBase *)self);

  pipeline = kclass->pipeline_init((DvbPlayerControlBase *)self);

  if(pipeline==NULL){
    UMMS_DEBUG("dvb_player_control_generic_init pipeline error.");
    return;
  }

  element_link(pipeline,(DvbPlayerControlBase *)self);
  return;
}

static void
dvb_player_control_generic_class_init (DvbPlayerControlGenericClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
 

  g_type_class_add_private (klass, sizeof (DvbPlayerControlBasePrivate));

  object_class->get_property = dvb_player_control_generic_get_property;
  object_class->set_property = dvb_player_control_generic_set_property;
  object_class->dispose = dvb_player_control_generic_dispose;
  object_class->finalize = dvb_player_control_generic_finalize;

  DvbPlayerControlBaseClass *parent_class = DVB_PLAYER_CONTROL_BASE_CLASS(klass);
  /*derived one*/
  parent_class->set_target = set_target;
  parent_class->setup_xwindow_target = setup_xwindow_target;

  parent_class->set_volume = set_volume;
  parent_class->get_volume = get_volume;
  parent_class->set_mute = set_mute;
  parent_class->is_mute = is_mute;
  parent_class->set_scale_mode = set_scale_mode;
  parent_class->get_scale_mode = get_scale_mode;
  parent_class->get_video_size = get_video_size;
  parent_class->set_video_size = set_video_size;
  parent_class->autoplug_multiqueue = autoplug_multiqueue; // Differ Implement

}

static GstElement *create_video_bin (void)
{
  GstElement *video_convert = NULL;
  GstElement *video_scale = NULL;
  GstElement *video_sink = NULL;
  GstElement *video_bin = NULL;
  GstPad *sinkpad = NULL;

  video_convert = gst_element_factory_make ("ffmpegcolorspace", NULL);
  video_scale = gst_element_factory_make ("videoscale", NULL);
  video_sink = gst_element_factory_make ("autovideosink", NULL);
  video_bin = gst_bin_new ("video-bin");

  if (!video_convert || !video_scale || !video_sink || !video_bin) {
    UMMS_DEBUG ("Creating video elements failed, \
        video_convert(%p), video_scale(%p), video_sink(%p), video_bin(%p)",
        video_convert, video_scale, video_sink, video_bin);
    goto make_failed;
  }

  gst_bin_add_many (GST_BIN (video_bin), video_convert, video_scale, video_sink, NULL);
  if (!gst_element_link_many (video_convert, video_scale, video_sink, NULL)) {
    UMMS_DEBUG ("linking video elements failed");
    goto link_failed;
  }

  sinkpad = gst_element_get_static_pad (video_convert, "sink");
  gst_element_add_pad (video_bin, gst_ghost_pad_new ("sink", sinkpad));
  gst_object_unref (GST_OBJECT (sinkpad));

  return video_bin;

make_failed:
  if (video_convert)
    gst_object_unref (video_convert);
  if (video_scale)
    gst_object_unref (video_scale);
  if (video_sink)
    gst_object_unref (video_sink);

link_failed:
  if (video_bin)
    gst_object_unref (video_bin);
  return NULL;
}

static GstElement *create_audio_bin (void)
{
  GstElement *audio_convert = NULL;
  GstElement *audio_resample = NULL;
  GstElement *audio_sink = NULL;
  GstElement *audio_bin = NULL;
  GstPad *sinkpad = NULL;

  audio_convert = gst_element_factory_make ("audioconvert", NULL);
  audio_resample = gst_element_factory_make ("audioresample", NULL);
  audio_sink = gst_element_factory_make ("autoaudiosink", NULL);
  audio_bin = gst_bin_new ("audio-bin");

  if (!audio_convert || !audio_resample || !audio_sink || !audio_bin) {
    UMMS_DEBUG ("Creating audio elements failed, \
        audio_convert(%p), audio_resample(%p), audio_sink(%p), audio_bin(%p)",
        audio_convert, audio_resample, audio_sink, audio_bin);
    goto make_failed;
  }

  gst_bin_add_many (GST_BIN (audio_bin), audio_convert, audio_resample, audio_sink, NULL);
  if (!gst_element_link_many (audio_convert, audio_resample, audio_sink, NULL)) {
    UMMS_DEBUG ("linking audio elements failed");
    goto link_failed;
  }

  sinkpad = gst_element_get_static_pad (audio_convert, "sink");
  gst_element_add_pad (audio_bin, gst_ghost_pad_new ("sink", sinkpad));
  gst_object_unref (GST_OBJECT (sinkpad));

  return audio_bin;

make_failed:
  if (audio_convert)
    gst_object_unref (audio_convert);
  if (audio_resample)
    gst_object_unref (audio_resample);
  if (audio_sink)
    gst_object_unref (audio_sink);

link_failed:
  if (audio_bin)
    gst_object_unref (audio_bin);
  return NULL;
}

DvbPlayerControlGeneric *
dvb_player_control_generic_new (void)
{
  UMMS_DEBUG("Called");
  return g_object_new (DVB_PLAYER_CONTROL_TYPE_GENERIC, NULL);
}

