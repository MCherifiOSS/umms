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
#include "player-control-base.h"
#include "media-player-control.h"
#include "param-table.h"

static void media_player_control_init (MediaPlayerControl* iface);

G_DEFINE_TYPE_WITH_CODE (PlayerControlBase, player_control_base, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TYPE_MEDIA_PLAYER_CONTROL, media_player_control_init))

#define PLAYER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PLAYER_CONTROL_TYPE_BASE, PlayerControlBasePrivate))

#define GET_PRIVATE(o) ((PlayerControlBase *)o)->priv

#define TEARDOWN_ELEMENT(ele)                      \
    if (ele) {                                     \
      gst_element_set_state (ele, GST_STATE_NULL); \
      g_object_unref (ele);                        \
      ele = NULL;                                  \
    }

#define INVALID_PLANE_ID -1

static const gchar *gst_state[] = {
  "GST_STATE_VOID_PENDING",
  "GST_STATE_NULL",
  "GST_STATE_READY",
  "GST_STATE_PAUSED",
  "GST_STATE_PLAYING"
};


/* list of URIs that we consider to be live source. */
static gchar *live_src_uri[] = {"mms://", "mmsh://", "rtsp://",
    "mmsu://", "mmst://", "fd://", "myth://", "ssh://", "ftp://", "sftp://",
    NULL
                               };

#define IS_LIVE_URI(uri)                                                           \
({                                                                                 \
  gboolean ret = FALSE;                                                            \
  gchar ** src = live_src_uri;                                                     \
  while (*src) {                                                                   \
    if (!g_ascii_strncasecmp (uri, *src, strlen(*src))) {                          \
      ret = TRUE;                                                                  \
      break;                                                                       \
    }                                                                              \
    src++;                                                                         \
  }                                                                                \
  ret;                                                                             \
})

#define UMMS_MAX_SERV_CONNECTS 5
#define UMMS_SOCKET_DEFAULT_PORT 112131
#define UMMS_SOCKET_DEFAULT_ADDR NULL

static gboolean _query_buffering_percent (GstElement *pipe, gdouble *percent);
static void _source_changed_cb (GObject *object, GParamSpec *pspec, gpointer data);
static gboolean _stop_pipe (MediaPlayerControl *control);
static gboolean player_control_base_set_video_size (MediaPlayerControl *self, guint x, guint y, guint w, guint h);
static gboolean create_xevent_handle_thread (MediaPlayerControl *self);
static gboolean destroy_xevent_handle_thread (MediaPlayerControl *self);
static gboolean parse_uri_async (MediaPlayerControl *self, gchar *uri);
//static void release_resource (MediaPlayerControl *self);
static void uri_parser_bus_message_error_cb (GstBus *bus, GstMessage *message, PlayerControlBase  *self);
static void _set_proxy (PlayerControlBase *self);

static gboolean
player_control_base_set_uri (MediaPlayerControl *self,
                    const gchar           *uri)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (uri, FALSE);

  UMMS_DEBUG ("SetUri called: old = %s new = %s",
              priv->uri,
              uri);

  if (priv->uri) {
    _stop_pipe(self);
    g_free (priv->uri);
  }

  priv->uri = g_strdup (uri);
  g_object_set (priv->pipeline, "uri", uri, NULL);

  //reset flags and object specific to uri.
  TEARDOWN_ELEMENT (priv->uri_parse_pipe);
  TEARDOWN_ELEMENT (priv->source);

  priv->total_bytes = -1;
  priv->duration = -1;
  priv->seekable = -1;
  priv->buffering = FALSE;
  priv->buffer_percent = -1;
  priv->player_state = PlayerStateStopped;
  priv->pending_state = PlayerStateNull;

  priv->resource_prepared = FALSE;
  priv->uri_parsed = FALSE;
  priv->has_video = FALSE;
  priv->hw_viddec = 0;
  priv->has_audio = FALSE;
  priv->hw_auddec = FALSE;
  priv->is_live = IS_LIVE_URI(priv->uri);

  if (priv->tag_list)
    gst_tag_list_free(priv->tag_list);
  priv->tag_list = NULL;
  RESET_STR(priv->title);
  RESET_STR(priv->artist);

  //URI is one of metadatas whose change should be notified.
  media_player_control_emit_metadata_changed (self);
  return TRUE;
}
static gboolean setup_xwindow_target (PlayerControlBase *self, GHashTable *params)
{
  UMMS_DEBUG("virtual APIs default Impl: NULL");
  return TRUE;
}

static gboolean
unset_xwindow_target (PlayerControlBase *self)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);

  UMMS_DEBUG("virtual APIs default Impl: NULL");
  return TRUE;
}

static gboolean
set_target (PlayerControlBase *self, gint type, GHashTable *params)
{
  gboolean ret = TRUE;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);
  MediaPlayerControl *self_iface = (MediaPlayerControl *)self;

  UMMS_DEBUG("virtual APIs default Impl: NULL");
  return ret;

}

static gboolean
player_control_base_set_target (MediaPlayerControl *self, gint type, GHashTable *params)
{
  PlayerControlBaseClass *kclass = PLAYER_CONTROL_BASE_GET_CLASS(self);
  return kclass->set_target((PlayerControlBase *)self, type, params);
}

static gboolean
prepare_plane (MediaPlayerControl *self)
{
  GstElement *vsink_bin = NULL;
  gint plane;
  gboolean ret = TRUE;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);

  g_object_get (G_OBJECT(priv->pipeline), "video-sink", &vsink_bin, NULL);

  if (vsink_bin) {
    if (g_object_class_find_property (G_OBJECT_GET_CLASS (vsink_bin), "gdl-plane") == NULL) {
      UMMS_DEBUG ("vsink has no gdl-plane property, which means target type is not XWindow or ReservedType0");
      goto OUT;
    }

    g_object_get (G_OBJECT(vsink_bin), "gdl-plane", &plane, NULL);

    //request plane resource
    ResourceRequest req = {ResourceTypePlane, plane};
    Resource *res = NULL;

    res = umms_resource_manager_request_resource (priv->res_mngr, &req);

    if (!res) {
      UMMS_DEBUG ("Failed");
      ret = FALSE;
    } else if (plane != res->handle) {
      g_object_set (G_OBJECT(vsink_bin), "gdl-plane", res->handle, NULL);
      UMMS_DEBUG ("Plane changed '%d'==>'%d'", plane,  res->handle);
    } else {
      //Do nothing;
    }

    priv->res_list = g_list_append (priv->res_list, res);
  }

OUT:
  if (vsink_bin)
    gst_object_unref (vsink_bin);

  return ret;
}


static gboolean
activate_player_control (PlayerControlBase *self, GstState target_state)
{
  gboolean ret;
  PlayerState old_pending;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);

  UMMS_DEBUG("virtual APIs default Impl: NULL");
  return TRUE;

}

static gboolean
player_control_base_pause (MediaPlayerControl *self)
{
/*
  return activate_player_control (self, GST_STATE_PAUSED);
*/
  PlayerControlBaseClass *kclass = PLAYER_CONTROL_BASE_GET_CLASS(self);
  return kclass->activate_player_control((PlayerControlBase *)self, GST_STATE_PAUSED);

}

static gboolean
player_control_base_play (MediaPlayerControl *self)
{
/*
  return activate_player_control (self, GST_STATE_PLAYING);
*/
  PlayerControlBaseClass *kclass = PLAYER_CONTROL_BASE_GET_CLASS(self);
  return kclass->activate_player_control((PlayerControlBase *)self, GST_STATE_PLAYING);
}

static gboolean
request_resource (PlayerControlBase *self)
{
  UMMS_DEBUG("virtual APIs default Impl: NULL");
  return;
}

static void
release_resource (PlayerControlBase *self)
{
  UMMS_DEBUG("virtual APIs default Impl: NULL");
  return;
}
static gboolean
_stop_pipe (MediaPlayerControl *control)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (control);
  PlayerControlBaseClass *bclass = PLAYER_CONTROL_BASE_GET_CLASS (control);

  if (gst_element_set_state(priv->pipeline, GST_STATE_NULL) == GST_STATE_CHANGE_FAILURE) {
    UMMS_DEBUG ("Unable to set NULL state");
    return FALSE;
  }

  bclass->release_resource (PLAYER_CONTROL_BASE(control));

  UMMS_DEBUG ("gstreamer player_control stopped");
  return TRUE;
}

static gboolean
player_control_base_stop (MediaPlayerControl *self)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);
  PlayerState old_state;

  if (!_stop_pipe (self))
    return FALSE;

  old_state = priv->player_state;
  priv->player_state = PlayerStateStopped;
  priv->suspended = FALSE;

  if (old_state != priv->player_state) {
    media_player_control_emit_player_state_changed (self, old_state, priv->player_state);
  }
  media_player_control_emit_stopped (self);

  return TRUE;
}

static gboolean
player_control_base_set_video_size (MediaPlayerControl *self,
    guint x, guint y, guint w, guint h)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);
  GstElement *pipe = priv->pipeline;
  gboolean ret = FALSE;
  GstElement *vsink_bin = NULL;
  GstElement *tsink_bin = NULL;

  g_return_val_if_fail (pipe, FALSE);
  UMMS_DEBUG ("invoked");

  //We use ismd_vidrend_bin as video-sink, so we can set rectangle property.
  g_object_get (G_OBJECT(pipe), "video-sink", &vsink_bin, NULL);
  if (vsink_bin) {
    gchar *rectangle_des = NULL;

    UMMS_DEBUG ("video sink: %p, name: %s", vsink_bin, GST_ELEMENT_NAME(vsink_bin));
    if (g_object_class_find_property (G_OBJECT_GET_CLASS (vsink_bin), "rectangle")) {
      rectangle_des = g_strdup_printf ("%u,%u,%u,%u", x, y, w, h);
      UMMS_DEBUG ("set rectangle damension :'%s'", rectangle_des);
      g_object_set (G_OBJECT(vsink_bin), "rectangle", rectangle_des, NULL);
      g_free (rectangle_des);
      ret = TRUE;
    } else {
      UMMS_DEBUG ("video sink: %s has no 'rectangle' property",  GST_ELEMENT_NAME(vsink_bin));
      goto OUT;
    }
  } else {
    UMMS_DEBUG ("Get video-sink failed");
    goto OUT;
  }

  gint tsvalue;
  g_object_get (G_OBJECT(pipe), "text-sink", &tsink_bin, NULL);
  if (tsink_bin) {
    //Position Setting
    tsvalue = w;
    g_object_set (G_OBJECT(tsink_bin), "tsub-widthpad", tsvalue, NULL);

    tsvalue = h;
    g_object_set (G_OBJECT(tsink_bin), "tsub-heightpad", tsvalue, NULL);

    tsvalue = x;
    g_object_set (G_OBJECT(tsink_bin), "pstart-x", tsvalue, NULL);

    tsvalue = y;
    g_object_set (G_OBJECT(tsink_bin), "pstart-y", tsvalue, NULL);

    //Font Setting
#define SUBTITLE_FONT_CUSTOM (0)
    if (SUBTITLE_FONT_CUSTOM) {
      tsvalue = SUBTITLE_FONT_CUSTOM;
    } else if (w <= 320  && h <= 240) {
      tsvalue = 6;
    } else if (w <= 640  && h <= 480) {
      tsvalue = 10;
    } else if (w <= 720  && h <= 576) {
      tsvalue = 12;
    } else if (w <= 1024 && h <= 720) {
      tsvalue = 16;
    } else if (w <= 1280 && h <= 800) {
      tsvalue = 20;
    } else if (w <= 1920 && h <= 1280) {
      tsvalue = 26;
    } else if (w <= 2880 && h <= 1920) {
      tsvalue = 36;
    } else {
      tsvalue = 12;
    }

    g_object_set (G_OBJECT(tsink_bin), "tsub-fontsize", tsvalue, NULL);

    ret = TRUE;
  }

OUT:
  if (vsink_bin)
    gst_object_unref (vsink_bin);

  if (tsink_bin)
    gst_object_unref (vsink_bin);

  return ret;
}

static gboolean
player_control_base_get_video_size (MediaPlayerControl *self,
    guint *w, guint *h)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);
  GstElement *pipe = priv->pipeline;
  guint x[1], y[1];
  gboolean ret;
  GstElement *vsink_bin = NULL;

  g_return_val_if_fail (pipe, FALSE);
  UMMS_DEBUG ("invoked");
  g_object_get (G_OBJECT(pipe), "video-sink", &vsink_bin, NULL);
  if (vsink_bin) {
    gchar *rectangle_des = NULL;
    UMMS_DEBUG ("bin: %p, name: %s", vsink_bin, GST_ELEMENT_NAME(vsink_bin));
    g_object_get (G_OBJECT(vsink_bin), "rectangle", &rectangle_des, NULL);
    if(rectangle_des == NULL){
      UMMS_DEBUG("Not support get video size");
      return FALSE;
    }
    sscanf (rectangle_des, "%u,%u,%u,%u", x, y, w, h);
    UMMS_DEBUG ("got rectangle damension :'%u,%u,%u,%u'", *x, *y, *w, *h);
    ret = TRUE;
  } else {
    UMMS_DEBUG ("Get video-sink failed");
    ret = FALSE;
  }

  if (vsink_bin)
    gst_object_unref (vsink_bin);
  return ret;
}

static gboolean
player_control_base_is_seekable (MediaPlayerControl *self, gboolean *seekable)
{
  PlayerControlBasePrivate *priv = NULL;
  GstElement *pipe = NULL;
  gboolean res = FALSE;
  gint old_seekable;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (seekable != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);


  if (priv->uri == NULL)
    return FALSE;

  old_seekable = priv->seekable;

  if (priv->seekable == -1) {
    GstQuery *query;

    query = gst_query_new_seeking (GST_FORMAT_TIME);
    if (gst_element_query (pipe, query)) {
      gst_query_parse_seeking (query, NULL, &res, NULL, NULL);
      UMMS_DEBUG ("seeking query says the stream is%s seekable", (res) ? "" : " not");
      priv->seekable = (res) ? 1 : 0;
    } else {
      UMMS_DEBUG ("seeking query failed, set seekable according to is_live flag");
      priv->seekable = priv->is_live ? FALSE : TRUE;
    }
    gst_query_unref (query);
  }

  if (priv->seekable != -1) {
    *seekable = (priv->seekable != 0);
  }

  UMMS_DEBUG ("stream is%s seekable", (*seekable) ? "" : " not");
  return TRUE;
}

static gboolean
player_control_base_set_position (MediaPlayerControl *self, gint64 in_pos)
{
  GstElement *pipe;
  PlayerControlBasePrivate *priv;
  gboolean ret;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);


  ret = gst_element_seek (pipe, 1.0,
        GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
        GST_SEEK_TYPE_SET, in_pos * GST_MSECOND,
        GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
  UMMS_DEBUG ("Seeking to %" GST_TIME_FORMAT " %s", GST_TIME_ARGS (in_pos * GST_MSECOND), ret ? "succeeded" : "failed");
  if (ret)
    media_player_control_emit_seeked (self);
  return ret;
}

static gboolean
player_control_base_get_position (MediaPlayerControl *self, gint64 *cur_pos)
{
  PlayerControlBasePrivate *priv = NULL;
  GstElement *pipe = NULL;
  GstFormat fmt;
  gint64 cur = 0;
  gboolean ret = TRUE;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (cur_pos != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  fmt = GST_FORMAT_TIME;
  if (gst_element_query_position (pipe, &fmt, &cur)) {
    UMMS_DEBUG ("current position = %lld (ms)", *cur_pos = cur / GST_MSECOND);
  } else {
    UMMS_DEBUG ("Failed to query position");
    ret = FALSE;
  }
  return ret;
}

static gboolean
player_control_base_set_playback_rate (MediaPlayerControl *self, gdouble in_rate)
{
  GstElement *pipe;
  PlayerControlBasePrivate *priv;
  gboolean ret = TRUE;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("set playback rate to %f ", in_rate);
  ret = gst_element_seek (pipe, in_rate,
         GST_FORMAT_TIME, GST_SEEK_FLAG_NONE,
         GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE,
         GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
         
  if (ret)
    media_player_control_emit_seeked (self);
}

static gboolean
player_control_base_get_playback_rate (MediaPlayerControl *self, gdouble *out_rate)
{
  GstElement *pipe;
  PlayerControlBasePrivate *priv;
  GstQuery *query;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("invoked");
  query = gst_query_new_segment (GST_FORMAT_TIME);

  if (gst_element_query (pipe, query)) {
    gst_query_parse_segment (query, out_rate, NULL, NULL, NULL);
    UMMS_DEBUG ("current rate = %f", *out_rate);
  } else {
    UMMS_DEBUG ("Failed to query segment");
  }
  gst_query_unref (query);

  return TRUE;
}

static gboolean
player_control_base_set_volume (MediaPlayerControl *self, gint vol)
{
  GstElement *pipe;
  PlayerControlBasePrivate *priv;
  gdouble volume;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("invoked");

  volume = (gdouble)(CLAMP (vol, 0, 100))/100;
  UMMS_DEBUG ("set volume to = %f", volume);
  gst_stream_volume_set_volume (GST_STREAM_VOLUME (pipe),
      GST_STREAM_VOLUME_FORMAT_LINEAR,
      volume);

  return TRUE;
}

static gboolean
player_control_base_get_volume (MediaPlayerControl *self, gint *volume)
{
  GstElement *pipe;
  PlayerControlBasePrivate *priv;
  gdouble vol;

  *volume = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("invoked");

  vol = gst_stream_volume_get_volume (GST_STREAM_VOLUME (pipe),
        GST_STREAM_VOLUME_FORMAT_LINEAR);

  *volume = (gint)(vol * 100 + 0.5);
  UMMS_DEBUG ("cur volume=%f(double), %d(int)", vol, *volume);

  return TRUE;
}

static gboolean
player_control_base_get_media_size_time (MediaPlayerControl *self, gint64 *media_size_time)
{
  GstElement *pipe;
  PlayerControlBasePrivate *priv;
  gint64 duration;
  gboolean ret = TRUE;
  GstFormat fmt = GST_FORMAT_TIME;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("invoked");

  if (gst_element_query_duration (pipe, &fmt, &duration)) {
    *media_size_time = duration / GST_MSECOND;
    UMMS_DEBUG ("media size = %lld (ms)", *media_size_time);
  } else {
    UMMS_DEBUG ("query media_size_time failed");
    *media_size_time = -1;
    ret = FALSE;
  }

  return ret;
}


static gboolean
player_control_base_get_media_size_bytes (MediaPlayerControl *self, gint64 *media_size_bytes)
{
  GstElement *source;
  PlayerControlBasePrivate *priv;
  gint64 length = 0;
  GstFormat fmt = GST_FORMAT_BYTES;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  source = priv->source;
  g_return_val_if_fail (GST_IS_ELEMENT (source), FALSE);

  UMMS_DEBUG ("invoked");

  if (!gst_element_query_duration (source, &fmt, &length)) {

    // Fall back to querying the source pads manually.
    // See also https://bugzilla.gnome.org/show_bug.cgi?id=638749
    GstIterator* iter = gst_element_iterate_src_pads(source);
    gboolean done = FALSE;
    length = 0;
    while (!done) {
      gpointer data;

      switch (gst_iterator_next(iter, &data)) {
        case GST_ITERATOR_OK: {
          GstPad* pad = GST_PAD_CAST(data);
          gint64 padLength = 0;
          if (gst_pad_query_duration(pad, &fmt, &padLength)
               && padLength > length)
            length = padLength;
          gst_object_unref(pad);
          break;
        }
        case GST_ITERATOR_RESYNC:
          gst_iterator_resync(iter);
          break;
        case GST_ITERATOR_ERROR:
          // Fall through.
        case GST_ITERATOR_DONE:
          done = TRUE;
          break;
      }
    }
    gst_iterator_free(iter);
  }

  *media_size_bytes = length;
  UMMS_DEBUG ("Total bytes = %lld", length);

  return TRUE;
}

static gboolean
player_control_base_has_video (MediaPlayerControl *self, gboolean *has_video)
{
  GstElement *pipe;
  PlayerControlBasePrivate *priv;
  gint n_video;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("invoked");

  g_object_get (G_OBJECT (pipe), "n-video", &n_video, NULL);
  UMMS_DEBUG ("'%d' videos in stream", n_video);
  *has_video = (n_video > 0) ? (TRUE) : (FALSE);

  return TRUE;
}

static gboolean
player_control_base_has_audio (MediaPlayerControl *self, gboolean *has_audio)
{
  GstElement *pipe;
  PlayerControlBasePrivate *priv;
  gint n_audio;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("invoked");

  g_object_get (G_OBJECT (pipe), "n-audio", &n_audio, NULL);
  UMMS_DEBUG ("'%d' audio tracks in stream", n_audio);
  *has_audio = (n_audio > 0) ? (TRUE) : (FALSE);

  return TRUE;
}

static gboolean
player_control_base_support_fullscreen (MediaPlayerControl *self, gboolean *support_fullscreen)
{
  //We are using ismd_vidrend_bin, so this function always return TRUE.
  *support_fullscreen = TRUE;
  return TRUE;
}

static gboolean
player_control_base_is_streaming (MediaPlayerControl *self, gboolean *is_streaming)
{
  GstElement *pipe;
  PlayerControlBasePrivate *priv;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("invoked");

  g_return_val_if_fail (priv->uri, FALSE);
  /*For now, we consider live source to be streaming source , hence unseekable.*/
  *is_streaming = priv->is_live;
  UMMS_DEBUG ("uri:'%s' is %s streaming source", priv->uri, (*is_streaming) ? "" : "not");
  return TRUE;
}

static gboolean
player_control_base_get_player_state (MediaPlayerControl *self,
    gint *state)
{
  PlayerControlBasePrivate *priv;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (state != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);

  *state = priv->player_state;
  return TRUE;
}

static gboolean
player_control_base_get_buffered_bytes (MediaPlayerControl *self,
    gint64 *buffered_bytes)
{
  gint64 total_bytes;
  gdouble percent;

  g_return_val_if_fail (self != NULL, FALSE);
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);

  if (!_query_buffering_percent(priv->pipeline, &percent)) {
    return FALSE;
  }

  if (priv->total_bytes == -1) {
    player_control_base_get_media_size_bytes (self, &total_bytes);
    priv->total_bytes = total_bytes;
  }

  *buffered_bytes = (priv->total_bytes * percent) / 100;

  UMMS_DEBUG ("Buffered bytes = %lld", *buffered_bytes);

  return TRUE;
}

static gboolean
_query_buffering_percent (GstElement *pipe, gdouble *percent)
{
  GstQuery* query = gst_query_new_buffering(GST_FORMAT_PERCENT);

  if (!gst_element_query(pipe, query)) {
    gst_query_unref(query);
    UMMS_DEBUG ("failed");
    return FALSE;
  }

  gint64 start, stop;
  gdouble fillStatus = 100.0;

  gst_query_parse_buffering_range(query, 0, &start, &stop, 0);
  gst_query_unref(query);

  if (stop != -1) {
    fillStatus = 100.0 * stop / GST_FORMAT_PERCENT_MAX;
  }

  UMMS_DEBUG ("[Buffering] Download buffer filled up to %f%%", fillStatus);
  *percent = fillStatus;

  return TRUE;
}

static gboolean
player_control_base_get_buffered_time (MediaPlayerControl *self, gint64 *buffered_time)
{
  gint64 duration;
  gdouble percent;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (self != NULL, FALSE);

  if (!_query_buffering_percent(priv->pipeline, &percent)) {
    return FALSE;
  }

  if (priv->duration == -1) {
    player_control_base_get_media_size_time (self, &duration);
    priv->duration = duration;
  }

  *buffered_time = (priv->duration * percent) / 100;
  UMMS_DEBUG ("duration=%lld, buffered_time=%lld", priv->duration, *buffered_time);

  return TRUE;
}

static gboolean
player_control_base_get_current_video (MediaPlayerControl *self, gint *cur_video)
{
  PlayerControlBasePrivate *priv = NULL;
  GstElement *pipe = NULL;
  gint c_video = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  g_object_get (G_OBJECT (pipe), "current-video", &c_video, NULL);
  UMMS_DEBUG ("the current video stream is %d", c_video);

  *cur_video = c_video;

  return TRUE;
}


static gboolean
player_control_base_get_current_audio (MediaPlayerControl *self, gint *cur_audio)
{
  PlayerControlBasePrivate *priv = NULL;
  GstElement *pipe = NULL;
  gint c_audio = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  g_object_get (G_OBJECT (pipe), "current-audio", &c_audio, NULL);
  UMMS_DEBUG ("the current audio stream is %d", c_audio);

  *cur_audio = c_audio;

  return TRUE;
}


static gboolean
player_control_base_set_current_video (MediaPlayerControl *self, gint cur_video)
{
  PlayerControlBasePrivate *priv = NULL;
  GstElement *pipe = NULL;
  gint n_video = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  /* Because the playbin2 set_property func do no check the return value,
     we need to get the total number and check valid for cur_video ourselves.*/
  g_object_get (G_OBJECT (pipe), "n-video", &n_video, NULL);
  UMMS_DEBUG ("The total video numeber is %d, we want to set to %d", n_video, cur_video);
  if ((cur_video < 0) || (cur_video >= n_video)) {
    UMMS_DEBUG ("The video we want to set is %d, invalid one.", cur_video);
    return FALSE;
  }

  g_object_set (G_OBJECT (pipe), "current-video", cur_video, NULL);

  return TRUE;
}


static gboolean
player_control_base_set_current_audio (MediaPlayerControl *self, gint cur_audio)
{
  PlayerControlBasePrivate *priv = NULL;
  GstElement *pipe = NULL;
  gint n_audio = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  /* Because the playbin2 set_property func do no check the return value,
     we need to get the total number and check valid for cur_audio ourselves.*/
  g_object_get (G_OBJECT (pipe), "n-audio", &n_audio, NULL);
  UMMS_DEBUG ("The total audio numeber is %d, we want to set to %d", n_audio, cur_audio);
  if ((cur_audio < 0) || (cur_audio >= n_audio)) {
    UMMS_DEBUG ("The audio we want to set is %d, invalid one.", cur_audio);
    return FALSE;
  }

  g_object_set (G_OBJECT (pipe), "current-audio", cur_audio, NULL);

  return TRUE;
}


static gboolean
player_control_base_get_video_num (MediaPlayerControl *self, gint *video_num)
{
  PlayerControlBasePrivate *priv = NULL;
  GstElement *pipe = NULL;
  gint n_video = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  g_object_get (G_OBJECT (pipe), "n-video", &n_video, NULL);
  UMMS_DEBUG ("the video number of the stream is %d", n_video);

  *video_num = n_video;

  return TRUE;
}


static gboolean
player_control_base_get_audio_num (MediaPlayerControl *self, gint *audio_num)
{
  PlayerControlBasePrivate *priv = NULL;
  GstElement *pipe = NULL;
  gint n_audio = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  g_object_get (G_OBJECT (pipe), "n-audio", &n_audio, NULL);
  UMMS_DEBUG ("the audio number of the stream is %d", n_audio);

  *audio_num = n_audio;

  return TRUE;
}

static gboolean
set_subtitle_uri (PlayerControlBase *self, gchar *sub_uri)
{
  UMMS_DEBUG("virtual APIs default Impl: NULL");
  return TRUE;
}

static gboolean
player_control_base_set_subtitle_uri (MediaPlayerControl *self, gchar *sub_uri)
{

  g_return_val_if_fail (self != NULL, FALSE);

  PlayerControlBaseClass *kclass = PLAYER_CONTROL_BASE_GET_CLASS(self);

  kclass->set_subtitle_uri(PLAYER_CONTROL_BASE(self), sub_uri);
  
  return TRUE;
}


static gboolean
player_control_base_get_subtitle_num (MediaPlayerControl *self, gint *sub_num)
{
  PlayerControlBasePrivate *priv = NULL;
  GstElement *pipe = NULL;
  gint n_sub = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  g_object_get (G_OBJECT (pipe), "n-text", &n_sub, NULL);
  UMMS_DEBUG ("the subtitle number of the stream is %d", n_sub);

  *sub_num = n_sub;

  return TRUE;
}


static gboolean
player_control_base_get_current_subtitle (MediaPlayerControl *self, gint *cur_sub)
{
  PlayerControlBasePrivate *priv = NULL;
  GstElement *pipe = NULL;
  gint c_sub = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  g_object_get (G_OBJECT (pipe), "current-text", &c_sub, NULL);
  UMMS_DEBUG ("the current subtitle stream is %d", c_sub);

  *cur_sub = c_sub;

  return TRUE;
}


static gboolean
player_control_base_set_current_subtitle (MediaPlayerControl *self, gint cur_sub)
{
  PlayerControlBasePrivate *priv = NULL;
  GstElement *pipe = NULL;
  gint n_sub = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  /* Because the playbin2 set_property func do no check the return value,
     we need to get the total number and check valid for cur_sub ourselves.*/
  g_object_get (G_OBJECT (pipe), "n-text", &n_sub, NULL);
  UMMS_DEBUG ("The total subtitle numeber is %d, we want to set to %d", n_sub, cur_sub);
  if ((cur_sub < 0) || (cur_sub >= n_sub)) {
    UMMS_DEBUG ("The subtitle we want to set is %d, invalid one.", cur_sub);
    return FALSE;
  }

  g_object_set (G_OBJECT (pipe), "current-text", cur_sub, NULL);

  return TRUE;
}


static gboolean
player_control_base_set_proxy (MediaPlayerControl *self, GHashTable *params)
{
  PlayerControlBasePrivate *priv = NULL;
  GValue *val = NULL;

  g_return_val_if_fail ((self != NULL) && (params != NULL), FALSE);
  priv = GET_PRIVATE (self);

  if (g_hash_table_lookup_extended (params, "proxy-uri", NULL, (gpointer)&val)) {
    RESET_STR (priv->proxy_uri);
    priv->proxy_uri = g_value_dup_string (val);
  }

  if (g_hash_table_lookup_extended (params, "proxy-id", NULL, (gpointer)&val)) {
    RESET_STR (priv->proxy_id);
    priv->proxy_id = g_value_dup_string (val);
  }

  if (g_hash_table_lookup_extended (params, "proxy-pw", NULL, (gpointer)&val)) {
    RESET_STR (priv->proxy_pw)
    priv->proxy_pw = g_value_dup_string (val);
  }

  UMMS_DEBUG ("proxy=%s, id=%s, pw=%s", priv->proxy_uri, priv->proxy_id, priv->proxy_pw);
  return TRUE;
}


static gboolean
player_control_base_set_buffer_depth (MediaPlayerControl *self, gint format, gint64 buf_val)
{
  PlayerControlBasePrivate *priv = NULL;
  GstElement *pipe = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  if (buf_val < 0) {
    UMMS_DEBUG("The buffer depth value is %lld, invalid one", buf_val);
    return FALSE;
  }

  if (format == BufferFormatByTime) {
    g_object_set (G_OBJECT (pipe), "buffer-duration", buf_val, NULL);
    UMMS_DEBUG("Set the buffer-duration to %lld", buf_val);
  } else if (format == BufferFormatByBytes) {
    g_object_set (G_OBJECT (pipe), "buffer-size", buf_val, NULL);
    UMMS_DEBUG("Set the buffer-size to %lld", buf_val);
  } else {
    UMMS_DEBUG("Pass the wrong format:%d to buffer depth setting", format);
    return FALSE;
  }

  return TRUE;
}


static gboolean
player_control_base_get_buffer_depth (MediaPlayerControl *self, gint format, gint64 *buf_val)
{
  PlayerControlBasePrivate *priv = NULL;
  GstElement *pipe = NULL;
  gint64 val;

  *buf_val = -1;

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  if (format == BufferFormatByTime) {
    g_object_get (G_OBJECT (pipe), "buffer-duration", &val, NULL);
    UMMS_DEBUG("Get the buffer-duration: %lld", val);
    *buf_val = val;
  } else if (format == BufferFormatByBytes) {
    g_object_get (G_OBJECT (pipe), "buffer-size", &val, NULL);
    UMMS_DEBUG("Get the buffer-size: %lld", val);
    *buf_val = val;
  } else {
    UMMS_DEBUG("Pass the wrong format:%d to buffer depth setting", format);
    return FALSE;
  }

  return TRUE;
}


static gboolean
player_control_base_set_mute (MediaPlayerControl *self, gint mute)
{
  GstElement *pipe;
  PlayerControlBasePrivate *priv;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("set mute to = %d", mute);
  gst_stream_volume_set_mute (GST_STREAM_VOLUME (pipe), mute);

  return TRUE;
}


static gboolean
player_control_base_is_mute (MediaPlayerControl *self, gint *mute)
{
  GstElement *pipe;
  PlayerControlBasePrivate *priv;
  gboolean is_mute;

  *mute = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  is_mute = gst_stream_volume_get_mute (GST_STREAM_VOLUME (pipe));
  UMMS_DEBUG("Get the mute %d", is_mute);
  *mute = is_mute;

  return TRUE;
}


static gboolean
player_control_base_set_scale_mode (MediaPlayerControl *self, gint scale_mode)
{
  GstElement *pipe;
  PlayerControlBasePrivate *priv;
  GstElement *vsink_bin;
  GParamSpec *pspec = NULL;
  GEnumClass *eclass = NULL;
  gboolean ret = TRUE;
  GValue val = { 0, };
  GEnumValue *eval;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  /* We assume that the video-sink is just ismd_vidrend_bin, because if not
     the scale mode is not supported yet in gst sink bins. */
  g_object_get (G_OBJECT(pipe), "video-sink", &vsink_bin, NULL);
  if (vsink_bin) {
    pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (vsink_bin), "scale-mode");
    if (pspec == NULL) {
      ret = FALSE;
      UMMS_DEBUG("can not get the scale-mode feature");
      goto OUT;
    }

    g_value_init (&val, pspec->value_type);
    g_object_get_property (G_OBJECT (vsink_bin), "scale-mode", &val);
    eclass = G_ENUM_CLASS (g_type_class_peek (G_VALUE_TYPE (&val)));
    if (eclass == NULL) {
      ret = FALSE;
      goto OUT;
    }

    switch (scale_mode) {
      case ScaleModeNoScale:
        eval = g_enum_get_value_by_nick (eclass, "none");
        break;
      case ScaleModeFill:
        eval = g_enum_get_value_by_nick (eclass, "scale2fit");
        break;
      case ScaleModeKeepAspectRatio:
        eval = g_enum_get_value_by_nick (eclass, "zoom2fit");
        break;
      case ScaleModeFillKeepAspectRatio:
        eval = g_enum_get_value_by_nick (eclass, "zoom2fill");
        break;
      default:
        UMMS_DEBUG("Invalid scale mode: %d", scale_mode);
        ret = FALSE;
        goto OUT;
    }

    if (eval == NULL) {
      ret = FALSE;
      goto OUT;
    }

    g_value_set_enum (&val, eval->value);
    g_object_set_property (G_OBJECT (vsink_bin), "scale-mode", &val);
    g_value_unset (&val);
  } else {
    UMMS_DEBUG("No a vidrend_bin set, scale not support now!");
    ret = FALSE;
  }

OUT:
  if (vsink_bin)
    gst_object_unref (vsink_bin);
  return ret;
}


static gboolean
player_control_base_get_scale_mode (MediaPlayerControl *self, gint *scale_mode)
{
  GstElement *pipe;
  PlayerControlBasePrivate *priv;
  GstElement *vsink_bin;
  gboolean ret = TRUE;
  GValue val = { 0, };
  GEnumValue *eval;
  GParamSpec *pspec = NULL;
  GEnumClass *eclass = NULL;

  *scale_mode = ScaleModeInvalid;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  /* We assume that the video-sink is just ismd_vidrend_bin, because if not
     the scale mode is not supported yet in gst sink bins. */
  g_object_get (G_OBJECT(pipe), "video-sink", &vsink_bin, NULL);
  if (vsink_bin) {
    pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (vsink_bin), "scale-mode");
    if (pspec == NULL) {
      ret = FALSE;
      UMMS_DEBUG("can not get the scale-mode feature");
      goto OUT;
    }

    g_value_init (&val, pspec->value_type);
    g_object_get_property (G_OBJECT (vsink_bin), "scale-mode", &val);
    eclass = G_ENUM_CLASS (g_type_class_peek (G_VALUE_TYPE (&val)));
    if (eclass == NULL) {
      ret = FALSE;
      goto OUT;
    }

    eval = g_enum_get_value (eclass, g_value_get_enum(&val));
    if (eval == NULL) {
      ret = FALSE;
      goto OUT;
    }

    if (strcmp(eval->value_nick, "none")) {
      *scale_mode = ScaleModeNoScale;
      goto OUT;
    } else if (strcmp(eval->value_nick, "scale2fit")) {
      *scale_mode = ScaleModeFill;
      goto OUT;
    } else if (strcmp(eval->value_nick, "zoom2fit")) {
      *scale_mode = ScaleModeKeepAspectRatio;
      goto OUT;
    } else if (strcmp(eval->value_nick, "zoom2fill")) {
      *scale_mode = ScaleModeFillKeepAspectRatio;
      goto OUT;
    } else {
      UMMS_DEBUG("Error scale mode");
      ret = FALSE;
    }
  } else {
    UMMS_DEBUG("No a vidrend_bin set, scale not support now!");
    ret = FALSE;
  }

OUT:
  if (vsink_bin)
    gst_object_unref (vsink_bin);
  return ret;
}

static gboolean
player_control_base_suspend (MediaPlayerControl *self)
{
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  PlayerControlBasePrivate *priv = GET_PRIVATE (self);

  if (priv->suspended)
    return TRUE;

  if (priv->player_state == PlayerStatePaused || priv->player_state == PlayerStatePlaying) {
    player_control_base_get_position (self, &priv->pos);
    player_control_base_stop (self);
  } else if (priv->player_state == PlayerStateStopped) {
    priv->pos = 0;
  }
  priv->suspended = TRUE;
  UMMS_DEBUG ("media_player_control_emit_suspended");
  media_player_control_emit_suspended (self);
  return TRUE;
}

//restore asynchronously.
//Set to pause here, and do actual retoring operation in bus_message_state_change_cb().
static gboolean
player_control_base_restore (MediaPlayerControl *self)
{
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  PlayerControlBasePrivate *priv = GET_PRIVATE (self);

  if (priv->player_state != PlayerStateStopped || !priv->suspended) {
    return FALSE;
  }

  return player_control_base_pause (self);
}


static gboolean
player_control_base_get_video_codec (MediaPlayerControl *self, gint channel, gchar ** video_codec)
{
  PlayerControlBasePrivate *priv = NULL;
  GstElement *pipe = NULL;
  int tol_channel;
  GstTagList * tag_list = NULL;
  gint size = 0;
  gchar * codec_name = NULL;
  int i;

  *video_codec = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;

  g_object_get (G_OBJECT (pipe), "n-video", &tol_channel, NULL);
  UMMS_DEBUG ("the video number of the stream is %d, want to get: %d",
              tol_channel, channel);

  if (channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipe, "get-video-tags", channel, &tag_list);
  if (tag_list == NULL) {
    UMMS_DEBUG ("No tags about stream: %d", channel);
    return TRUE;
  }

  if ((size = gst_tag_list_get_tag_size(tag_list, GST_TAG_VIDEO_CODEC)) > 0) {
    gchar *st = NULL;

    for (i = 0; i < size; ++i) {
      if (gst_tag_list_get_string_index (tag_list, GST_TAG_VIDEO_CODEC, i, &st) && st) {
        UMMS_DEBUG("Channel: %d provide the video codec named: %s", channel, st);
        if (codec_name) {
          codec_name = g_strconcat(codec_name, st, NULL);
        } else {
          codec_name = g_strdup(st);
        }
        g_free (st);
      }
    }

    UMMS_DEBUG("%s", codec_name);
  }

  if (codec_name)
    *video_codec = codec_name;

  if (tag_list)
    gst_tag_list_free (tag_list);

  return TRUE;
}


static gboolean
player_control_base_get_audio_codec (MediaPlayerControl *self, gint channel, gchar ** audio_codec)
{
  PlayerControlBasePrivate *priv = NULL;
  GstElement *pipe = NULL;
  int tol_channel;
  GstTagList * tag_list = NULL;
  gint size = 0;
  gchar * codec_name = NULL;
  int i;

  *audio_codec = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;

  g_object_get (G_OBJECT (pipe), "n-audio", &tol_channel, NULL);
  UMMS_DEBUG ("the audio number of the stream is %d, want to get: %d",
              tol_channel, channel);

  if (channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipe, "get-audio-tags", channel, &tag_list);
  if (tag_list == NULL) {
    UMMS_DEBUG ("No tags about stream: %d", channel);
    return TRUE;
  }

  if ((size = gst_tag_list_get_tag_size(tag_list, GST_TAG_AUDIO_CODEC)) > 0) {
    gchar *st = NULL;

    for (i = 0; i < size; ++i) {
      if (gst_tag_list_get_string_index (tag_list, GST_TAG_AUDIO_CODEC, i, &st) && st) {
        UMMS_DEBUG("Channel: %d provide the audio codec named: %s", channel, st);
        if (codec_name) {
          codec_name = g_strconcat(codec_name, st, NULL);
        } else {
          codec_name = g_strdup(st);
        }
        g_free (st);
      }
    }

    UMMS_DEBUG("%s", codec_name);
  }

  if (codec_name)
    *audio_codec = codec_name;

  if (tag_list)
    gst_tag_list_free (tag_list);

  return TRUE;
}

static gboolean
player_control_base_get_video_bitrate (MediaPlayerControl *self, gint channel, gint *video_rate)
{
  PlayerControlBasePrivate *priv = NULL;
  GstElement *pipe = NULL;
  int tol_channel;
  GstTagList * tag_list = NULL;
  guint32 bit_rate = 0;

  *video_rate = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;

  g_object_get (G_OBJECT (pipe), "n-video", &tol_channel, NULL);
  UMMS_DEBUG ("the video number of the stream is %d, want to get: %d",
              tol_channel, channel);

  if (channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipe, "get-video-tags", channel, &tag_list);
  if (tag_list == NULL) {
    UMMS_DEBUG ("No tags about stream: %d", channel);
    return TRUE;
  }

  if (gst_tag_list_get_uint(tag_list, GST_TAG_BITRATE, &bit_rate) && bit_rate > 0) {
    UMMS_DEBUG ("bit rate for channel: %d is %d", channel, bit_rate);
    *video_rate = bit_rate / 1000;
  } else if (gst_tag_list_get_uint(tag_list, GST_TAG_NOMINAL_BITRATE, &bit_rate) && bit_rate > 0) {
    UMMS_DEBUG ("nominal bit rate for channel: %d is %d", channel, bit_rate);
    *video_rate = bit_rate / 1000;
  } else {
    UMMS_DEBUG ("No bit rate for channel: %d", channel);
  }

  if (tag_list)
    gst_tag_list_free (tag_list);

  return TRUE;
}


static gboolean
player_control_base_get_audio_bitrate (MediaPlayerControl *self, gint channel, gint *audio_rate)
{
  PlayerControlBasePrivate *priv = NULL;
  GstElement *pipe = NULL;
  int tol_channel;
  GstTagList * tag_list = NULL;
  guint32 bit_rate = 0;

  *audio_rate = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;

  g_object_get (G_OBJECT (pipe), "n-audio", &tol_channel, NULL);
  UMMS_DEBUG ("the audio number of the stream is %d, want to get: %d",
              tol_channel, channel);

  if (channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipe, "get-audio-tags", channel, &tag_list);
  if (tag_list == NULL) {
    UMMS_DEBUG ("No tags about stream: %d", channel);
    return TRUE;
  }

  if (gst_tag_list_get_uint(tag_list, GST_TAG_BITRATE, &bit_rate) && bit_rate > 0) {
    UMMS_DEBUG ("bit rate for channel: %d is %d", channel, bit_rate);
    *audio_rate = bit_rate / 1000;
  } else if (gst_tag_list_get_uint(tag_list, GST_TAG_NOMINAL_BITRATE, &bit_rate) && bit_rate > 0) {
    UMMS_DEBUG ("nominal bit rate for channel: %d is %d", channel, bit_rate);
    *audio_rate = bit_rate / 1000;
  } else {
    UMMS_DEBUG ("No bit rate for channel: %d", channel);
  }

  if (tag_list)
    gst_tag_list_free (tag_list);

  return TRUE;
}


static gboolean
player_control_base_get_encapsulation(MediaPlayerControl *self, gchar ** encapsulation)
{
  PlayerControlBasePrivate *priv = NULL;
  GstElement *pipe = NULL;
  gchar *enca_name = NULL;

  *encapsulation = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;

  if (priv->tag_list) {
    gst_tag_list_get_string (priv->tag_list, GST_TAG_CONTAINER_FORMAT, &enca_name);
    if (enca_name) {
      UMMS_DEBUG("get the container name: %s", enca_name);
      *encapsulation = enca_name;
    } else {
      UMMS_DEBUG("no infomation about the container.");
    }
  }

  return TRUE;
}


static gboolean
player_control_base_get_audio_samplerate(MediaPlayerControl *self, gint channel, gint * sample_rate)
{
  GstElement *pipe = NULL;
  PlayerControlBasePrivate *priv = NULL;
  int tol_channel;
  GstCaps *caps = NULL;
  GstPad *pad = NULL;
  GstStructure *s = NULL;
  gboolean ret = TRUE;

  *sample_rate = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  /* We get this kind of infomation from the caps of inputselector. */
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;

  g_object_get (G_OBJECT (pipe), "n-audio", &tol_channel, NULL);
  UMMS_DEBUG ("the audio number of the stream is %d, want to get: %d",
              tol_channel, channel);

  if (channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipe, "get-audio-pad", channel, &pad);
  if (pad == NULL) {
    UMMS_DEBUG ("No pad of stream: %d", channel);
    return FALSE;
  }

  caps = gst_pad_get_negotiated_caps (pad);
  if (caps) {
    s = gst_caps_get_structure (caps, 0);
    ret = gst_structure_get_int (s, "rate", sample_rate);
    gst_caps_unref (caps);
  }

  if (pad)
    gst_object_unref (pad);

  return ret;
}


static gboolean
player_control_base_get_video_framerate(MediaPlayerControl *self, gint channel,
    gint * frame_rate_num, gint * frame_rate_denom)
{
  GstElement *pipe = NULL;
  PlayerControlBasePrivate *priv = NULL;
  int tol_channel;
  GstCaps *caps = NULL;
  GstPad *pad = NULL;
  GstStructure *s = NULL;
  gboolean ret = TRUE;

  *frame_rate_num = 0;
  *frame_rate_denom = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  /* We get this kind of infomation from the caps of inputselector. */
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;

  g_object_get (G_OBJECT (pipe), "n-video", &tol_channel, NULL);
  UMMS_DEBUG ("the video number of the stream is %d, want to get: %d",
              tol_channel, channel);

  if (channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipe, "get-video-pad", channel, &pad);
  if (pad == NULL) {
    UMMS_DEBUG ("No pad of stream: %d", channel);
    return FALSE;
  }

  caps = gst_pad_get_negotiated_caps (pad);
  if (caps) {
    s = gst_caps_get_structure (caps, 0);
    ret = gst_structure_get_fraction(s, "framerate", frame_rate_num, frame_rate_denom);
    gst_caps_unref (caps);
  }

  if (pad)
    gst_object_unref (pad);

  return ret;
}


static gboolean
player_control_base_get_video_resolution(MediaPlayerControl *self, gint channel, gint * width, gint * height)
{
  GstElement *pipe = NULL;
  PlayerControlBasePrivate *priv = NULL;
  int tol_channel;
  GstCaps *caps = NULL;
  GstPad *pad = NULL;
  GstStructure *s = NULL;

  *width = 0;
  *height = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  /* We get this kind of infomation from the caps of inputselector. */
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  
  g_object_get (G_OBJECT (pipe), "n-video", &tol_channel, NULL);
  UMMS_DEBUG ("the video number of the stream is %d, want to get: %d",
              tol_channel, channel);

  if (channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipe, "get-video-pad", channel, &pad);
  if (pad == NULL) {
    UMMS_DEBUG ("No pad of stream: %d", channel);
    return FALSE;
  }

  caps = gst_pad_get_negotiated_caps (pad);
  if (caps) {
    s = gst_caps_get_structure (caps, 0);
    gst_structure_get_int (s, "width", width);
    gst_structure_get_int (s, "height", height);
    gst_caps_unref (caps);
  }

  if (pad)
    gst_object_unref (pad);

  return TRUE;
}


static gboolean
player_control_base_get_video_aspect_ratio(MediaPlayerControl *self, gint channel,
    gint * ratio_num, gint * ratio_denom)
{
  GstElement *pipe = NULL;
  PlayerControlBasePrivate *priv = NULL;
  int tol_channel;
  GstCaps *caps = NULL;
  GstPad *pad = NULL;
  GstStructure *s = NULL;
  gboolean ret = TRUE;

  *ratio_num = 0;
  *ratio_denom = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  /* We get this kind of infomation from the caps of inputselector. */
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;

  g_object_get (G_OBJECT (pipe), "n-video", &tol_channel, NULL);
  UMMS_DEBUG ("the video number of the stream is %d, want to get: %d",
              tol_channel, channel);

  if (channel >= tol_channel || channel < 0) {
    UMMS_DEBUG ("Invalid Channel: %d", channel);
    return FALSE;
  }

  g_signal_emit_by_name (pipe, "get-video-pad", channel, &pad);
  if (pad == NULL) {
    UMMS_DEBUG ("No pad of stream: %d", channel);
    return FALSE;
  }

  caps = gst_pad_get_negotiated_caps (pad);
  if (caps) {
    s = gst_caps_get_structure (caps, 0);
    ret = gst_structure_get_fraction(s, "pixel-aspect-ratio", ratio_num, ratio_denom);
    gst_caps_unref (caps);
  }

  if (pad)
    gst_object_unref (pad);

  return ret;
}


static gboolean
player_control_base_get_protocol_name(MediaPlayerControl *self, gchar ** prot_name)
{
  GstElement *pipe = NULL;
  PlayerControlBasePrivate *priv = NULL;
  gchar * uri = NULL;

  *prot_name = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;

  g_object_get (G_OBJECT (pipe), "uri", &uri, NULL);

  if (!uri) {
    UMMS_DEBUG("Pipe %"GST_PTR_FORMAT" has no uri now!", pipe);
    return FALSE;
  }

  if (!gst_uri_is_valid(uri)) {
    UMMS_DEBUG("uri: %s  is invalid", uri);
    g_free(uri);
    return FALSE;
  }

  UMMS_DEBUG("Pipe %"GST_PTR_FORMAT" has no uri is %s", pipe, uri);

  *prot_name = gst_uri_get_protocol(uri);
  UMMS_DEBUG("Get the protocol name is %s", *prot_name);

  g_free(uri);
  return TRUE;
}


static gboolean
player_control_base_get_current_uri(MediaPlayerControl *self, gchar ** uri)
{
  PlayerControlBasePrivate *priv = NULL;
  GstElement *pipe = NULL;
  gchar * s_uri = NULL;

  *uri = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_object_get (G_OBJECT (pipe), "uri", &s_uri, NULL);

  if (!s_uri) {
    UMMS_DEBUG("Pipe %"GST_PTR_FORMAT" has no uri now!", pipe);
    return FALSE;
  }

  if (!gst_uri_is_valid(s_uri)) {
    UMMS_DEBUG("uri: %s  is invalid", s_uri);
    g_free(s_uri);
    return FALSE;
  }

  *uri = s_uri;
  return TRUE;
}

static gboolean
player_control_base_get_title(MediaPlayerControl *self, gchar ** title)
{
  PlayerControlBasePrivate *priv = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  *title = g_strdup (priv->title);
  UMMS_DEBUG ("title = %s", *title);

  return TRUE;
}

static gboolean
player_control_base_get_artist(MediaPlayerControl *self, gchar ** artist)
{
  PlayerControlBasePrivate *priv = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  *artist = g_strdup (priv->artist);
  UMMS_DEBUG ("artist = %s", *artist);
  return TRUE;
}

static void
media_player_control_init (MediaPlayerControl *iface)
{
  MediaPlayerControlClass *klass = (MediaPlayerControlClass *)iface;

  media_player_control_implement_set_uri (klass,
      player_control_base_set_uri);
  media_player_control_implement_set_target (klass,
      player_control_base_set_target);
  media_player_control_implement_play (klass,
      player_control_base_play);
  media_player_control_implement_pause (klass,
      player_control_base_pause);
  media_player_control_implement_stop (klass,
      player_control_base_stop);
  media_player_control_implement_set_video_size (klass,
      player_control_base_set_video_size);
  media_player_control_implement_get_video_size (klass,
      player_control_base_get_video_size);
  media_player_control_implement_is_seekable (klass,
      player_control_base_is_seekable);
  media_player_control_implement_set_position (klass,
      player_control_base_set_position);
  media_player_control_implement_get_position (klass,
      player_control_base_get_position);
  media_player_control_implement_set_playback_rate (klass,
      player_control_base_set_playback_rate);
  media_player_control_implement_get_playback_rate (klass,
      player_control_base_get_playback_rate);
  media_player_control_implement_set_volume (klass,
      player_control_base_set_volume);
  media_player_control_implement_get_volume (klass,
      player_control_base_get_volume);
  media_player_control_implement_get_media_size_time (klass,
      player_control_base_get_media_size_time);
  media_player_control_implement_get_media_size_bytes (klass,
      player_control_base_get_media_size_bytes);
  media_player_control_implement_has_video (klass,
      player_control_base_has_video);
  media_player_control_implement_has_audio (klass,
      player_control_base_has_audio);
  media_player_control_implement_support_fullscreen (klass,
      player_control_base_support_fullscreen);
  media_player_control_implement_is_streaming (klass,
      player_control_base_is_streaming);
  media_player_control_implement_get_player_state (klass,
      player_control_base_get_player_state);
  media_player_control_implement_get_buffered_bytes (klass,
      player_control_base_get_buffered_bytes);
  media_player_control_implement_get_buffered_time (klass,
      player_control_base_get_buffered_time);
  media_player_control_implement_get_current_video (klass,
      player_control_base_get_current_video);
  media_player_control_implement_get_current_audio (klass,
      player_control_base_get_current_audio);
  media_player_control_implement_set_current_video (klass,
      player_control_base_set_current_video);
  media_player_control_implement_set_current_audio (klass,
      player_control_base_set_current_audio);
  media_player_control_implement_get_video_num (klass,
      player_control_base_get_video_num);
  media_player_control_implement_get_audio_num (klass,
      player_control_base_get_audio_num);
  media_player_control_implement_set_proxy (klass,
      player_control_base_set_proxy);
  media_player_control_implement_set_subtitle_uri (klass,
      player_control_base_set_subtitle_uri);
  media_player_control_implement_get_subtitle_num (klass,
      player_control_base_get_subtitle_num);
  media_player_control_implement_set_current_subtitle (klass,
      player_control_base_set_current_subtitle);
  media_player_control_implement_get_current_subtitle (klass,
      player_control_base_get_current_subtitle);
  media_player_control_implement_set_buffer_depth (klass,
      player_control_base_set_buffer_depth);
  media_player_control_implement_get_buffer_depth (klass,
      player_control_base_get_buffer_depth);
  media_player_control_implement_set_mute (klass,
      player_control_base_set_mute);
  media_player_control_implement_is_mute (klass,
      player_control_base_is_mute);
  media_player_control_implement_suspend (klass,
      player_control_base_suspend);
  media_player_control_implement_restore (klass,
      player_control_base_restore);
  media_player_control_implement_set_scale_mode (klass,
      player_control_base_set_scale_mode);
  media_player_control_implement_get_scale_mode (klass,
      player_control_base_get_scale_mode);
  media_player_control_implement_get_video_codec (klass,
      player_control_base_get_video_codec);
  media_player_control_implement_get_audio_codec (klass,
      player_control_base_get_audio_codec);
  media_player_control_implement_get_video_bitrate (klass,
      player_control_base_get_video_bitrate);
  media_player_control_implement_get_audio_bitrate (klass,
      player_control_base_get_audio_bitrate);
  media_player_control_implement_get_encapsulation (klass,
      player_control_base_get_encapsulation);
  media_player_control_implement_get_audio_samplerate (klass,
      player_control_base_get_audio_samplerate);
  media_player_control_implement_get_video_framerate (klass,
      player_control_base_get_video_framerate);
  media_player_control_implement_get_video_resolution (klass,
      player_control_base_get_video_resolution);
  media_player_control_implement_get_video_aspect_ratio (klass,
      player_control_base_get_video_aspect_ratio);
  media_player_control_implement_get_protocol_name (klass,
      player_control_base_get_protocol_name);
  media_player_control_implement_get_current_uri (klass,
      player_control_base_get_current_uri);
  media_player_control_implement_get_title (klass,
      player_control_base_get_title);
  media_player_control_implement_get_artist (klass,
      player_control_base_get_artist);
}

static void
player_control_base_get_property (GObject    *object,
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
player_control_base_set_property (GObject      *object,
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
player_control_base_dispose (GObject *object)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (object);
  PlayerControlBaseClass *kclass = PLAYER_CONTROL_BASE_GET_CLASS(object);
  int i;

  _stop_pipe ((MediaPlayerControl *)object);

  TEARDOWN_ELEMENT (priv->source);
  TEARDOWN_ELEMENT (priv->uri_parse_pipe);
  TEARDOWN_ELEMENT (priv->pipeline);

  if (priv->target_type == XWindow) {
    kclass->unset_xwindow_target (PLAYER_CONTROL_BASE(object));
  }

  if (priv->disp) {
    XCloseDisplay (priv->disp);
    priv->disp = NULL;
  }

  if (priv->tag_list)
    gst_tag_list_free(priv->tag_list);
  priv->tag_list = NULL;

  G_OBJECT_CLASS (player_control_base_parent_class)->dispose (object);
}

static void
player_control_base_finalize (GObject *object)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (object);

  RESET_STR(priv->uri);
  RESET_STR(priv->title);
  RESET_STR(priv->artist);
  RESET_STR(priv->proxy_uri);
  RESET_STR(priv->proxy_id);
  RESET_STR(priv->proxy_pw);

  G_OBJECT_CLASS (player_control_base_parent_class)->finalize (object);
}

static void player_control_base_do_action(PlayerControlBase *self)
{
  g_warning("virtual func: %s is default\n", __FUNCTION__);
  return;
}

static void
player_control_base_class_init (PlayerControlBaseClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  UMMS_DEBUG("called");

  g_type_class_add_private (klass, sizeof (PlayerControlBasePrivate));

  object_class->get_property = player_control_base_get_property;
  object_class->set_property = player_control_base_set_property;
  object_class->dispose = player_control_base_dispose;
  object_class->finalize = player_control_base_finalize;
  /*virtual APis*/
  klass->activate_player_control = activate_player_control;
  klass->set_target = set_target;
  klass->set_proxy = _set_proxy;
  klass->release_resource = release_resource;
  klass->request_resource = request_resource;
  klass->unset_xwindow_target = unset_xwindow_target;
  klass->setup_xwindow_target = setup_xwindow_target;
  klass->set_subtitle_uri = set_subtitle_uri;
}

static void
bus_message_state_change_cb (GstBus     *bus,
    GstMessage *message,
    PlayerControlBase  *self)
{

  GstState old_state, new_state;
  PlayerState old_player_state;
  gpointer src;
  gboolean seekable;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);

  src = GST_MESSAGE_SRC (message);
  if (src != priv->pipeline)
    return;

  gst_message_parse_state_changed (message, &old_state, &new_state, NULL);

  UMMS_DEBUG ("state-changed: old='%s', new='%s'", gst_state[old_state], gst_state[new_state]);

  old_player_state = priv->player_state;
  if (new_state == GST_STATE_PAUSED) {
    priv->player_state = PlayerStatePaused;
    if (old_player_state == PlayerStateStopped && priv->suspended) {
      UMMS_DEBUG ("restoring suspended execution, pos = %lld", priv->pos);
      player_control_base_is_seekable (MEDIA_PLAYER_CONTROL(self), &seekable);
      if (seekable) {
        player_control_base_set_position (MEDIA_PLAYER_CONTROL(self), priv->pos);
      }
      player_control_base_play(MEDIA_PLAYER_CONTROL(self));
      priv->suspended = FALSE;
      UMMS_DEBUG ("media_player_control_emit_restored");
      media_player_control_emit_restored (self);
    }
  } else if (new_state == GST_STATE_PLAYING) {
    priv->player_state = PlayerStatePlaying;
  } else {
    if (new_state < old_state)//down state change to GST_STATE_READY
      priv->player_state = PlayerStateStopped;
  }

  if (priv->pending_state == priv->player_state)
    priv->pending_state = PlayerStateNull;

  if (priv->player_state != old_player_state) {
    UMMS_DEBUG ("emit state changed, old=%d, new=%d", old_player_state, priv->player_state);
    media_player_control_emit_player_state_changed (self, old_player_state, priv->player_state);
  }
}

static void
bus_message_get_tag_cb (GstBus *bus, GstMessage *message, PlayerControlBase  *self)
{
  gpointer src;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);
  GstPad * src_pad = NULL;
  GstTagList * tag_list = NULL;
  gchar * pad_name = NULL;
  gchar * element_name = NULL;
  gchar * title = NULL;
  gchar * artist = NULL;
  gboolean metadata_changed = FALSE;

  src = GST_MESSAGE_SRC (message);

  if (message->type != GST_MESSAGE_TAG) {
    UMMS_DEBUG("not a tag message in a registered tag signal, strange");
    return;
  }

  gst_message_parse_tag_full (message, &src_pad, &tag_list);
  if (src_pad) {
    pad_name = g_strdup (GST_PAD_NAME (src_pad));
    UMMS_DEBUG("The pad name is %s", pad_name);
  }

  if (message->src) {
    element_name = g_strdup (GST_ELEMENT_NAME (message->src));
    UMMS_DEBUG("The element name is %s", element_name);
  }

  
  //cache the title
  if (gst_tag_list_get_string_index (tag_list, GST_TAG_TITLE, 0, &title)) {
    UMMS_DEBUG("Element: %s, provide the title: %s", element_name, title);
    RESET_STR(priv->title);
    priv->title = title;
    metadata_changed = TRUE;
  }

  //cache the artist
  if (gst_tag_list_get_string_index (tag_list, GST_TAG_ARTIST, 0, &artist)) {
    UMMS_DEBUG("Element: %s, provide the artist: %s", element_name, artist);
    RESET_STR(priv->artist);
    priv->artist = artist;
    metadata_changed = TRUE;
  }

  //only care about artist and title
  if (metadata_changed) {
    media_player_control_emit_metadata_changed (self);
  }

  //tag_list will be freed in gst_tag_list_merge(), so we don't need to free it by ourself
  priv->tag_list =
    gst_tag_list_merge(priv->tag_list, tag_list, GST_TAG_MERGE_REPLACE);

  if (src_pad)
    g_object_unref(src_pad);
  if (pad_name)
    g_free(pad_name);
  if (element_name)
    g_free(element_name);
}


static void
bus_message_eos_cb (GstBus     *bus,
                    GstMessage *message,
                    PlayerControlBase  *self)
{
  UMMS_DEBUG ("message::eos received on bus");
  media_player_control_emit_eof (self);
}


static void
bus_message_error_cb (GstBus     *bus,
    GstMessage *message,
    PlayerControlBase  *self)
{
  GError *error = NULL;

  UMMS_DEBUG ("message::error received on bus");

  gst_message_parse_error (message, &error, NULL);
  _stop_pipe (MEDIA_PLAYER_CONTROL(self));

  media_player_control_emit_error (self, UMMS_ENGINE_ERROR_FAILED, error->message);

  UMMS_DEBUG ("Error emitted with message = %s", error->message);

  g_clear_error (&error);
}

static void
bus_message_buffering_cb (GstBus *bus,
    GstMessage *message,
    PlayerControlBase *self)
{
  const GstStructure *str;
  gboolean res;
  gint buffer_percent;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);

  str = gst_message_get_structure (message);
  if (!str)
    return;

  res = gst_structure_get_int (str, "buffer-percent", &buffer_percent);
  if (!(buffer_percent % 25))
    UMMS_DEBUG ("buffering...%d%% ", buffer_percent);

  if (res) {
    priv->buffer_percent = buffer_percent;

    if (priv->buffer_percent == 100) {

      UMMS_DEBUG ("Done buffering");
      priv->buffering = FALSE;

      if (priv->pending_state == PlayerStatePlaying)
        gst_element_set_state (priv->pipeline, GST_STATE_PLAYING);

      media_player_control_emit_buffered (self);
    } else if (!priv->buffering) {
      
      if (priv->player_state == PlayerStatePlaying) {
        priv->pending_state = PlayerStatePlaying;//Use this to restore playing when buffering done.
      }
      priv->buffering = TRUE;
      UMMS_DEBUG ("Set pipeline to paused for buffering data");

      if (!priv->is_live)
        gst_element_set_state (priv->pipeline, GST_STATE_PAUSED);

      media_player_control_emit_buffering (self);
    }
  }
}

static GstBusSyncReply
bus_sync_handler (GstBus *bus,
                  GstMessage *message,
                  PlayerControlBase *player_control)
{
  PlayerControlBasePrivate *priv;
  GstElement *vsink;
  GError *err = NULL;
  GstMessage *msg = NULL;

  if ( GST_MESSAGE_TYPE( message ) != GST_MESSAGE_ELEMENT )
    return( GST_BUS_PASS );

  if ( ! gst_structure_has_name( message->structure, "prepare-gdl-plane" ) )
    return( GST_BUS_PASS );

  g_return_val_if_fail (player_control, GST_BUS_PASS);
  g_return_val_if_fail (PLAYER_CONTROL_IS_BASE (player_control), GST_BUS_PASS);
  priv = GET_PRIVATE (player_control);

  vsink =  GST_ELEMENT(GST_MESSAGE_SRC (message));
  UMMS_DEBUG ("sync-handler received on bus: prepare-gdl-plane, source: %s", GST_ELEMENT_NAME(vsink));

  if (!prepare_plane ((MediaPlayerControl *)player_control)) {
    //Since we are in streame thread, let the vsink to post the error message. Handle it in bus_message_error_cb().
    err = g_error_new_literal (UMMS_RESOURCE_ERROR, UMMS_RESOURCE_ERROR_NO_RESOURCE, "Plane unavailable");
    msg = gst_message_new_error (GST_OBJECT_CAST(priv->pipeline), err, "No resource");
    gst_element_post_message (priv->pipeline, msg);
    g_error_free (err);
  }

//  media_player_control_emit_request_window (player_control);

  if (message)
    gst_message_unref (message);
  return( GST_BUS_DROP );
}

static void
video_tags_changed_cb (GstElement *playbin2, gint stream_id, gpointer user_data) /* Used as tag change monitor. */
{
  PlayerControlBase * priv = (PlayerControlBase *) user_data;
  media_player_control_emit_video_tag_changed(priv, stream_id);
}

static void
audio_tags_changed_cb (GstElement *playbin2, gint stream_id, gpointer user_data) /* Used as tag change monitor. */
{
  PlayerControlBase * priv = (PlayerControlBase *) user_data;
  media_player_control_emit_audio_tag_changed(priv, stream_id);
}

static void
text_tags_changed_cb (GstElement *playbin2, gint stream_id, gpointer user_data) /* Used as tag change monitor. */
{
  PlayerControlBase * priv = (PlayerControlBase *) user_data;
  media_player_control_emit_text_tag_changed(priv, stream_id);
}

/* GstPlayFlags flags from playbin2 */
typedef enum {
  GST_PLAY_FLAG_VIDEO         = (1 << 0),
  GST_PLAY_FLAG_AUDIO         = (1 << 1),
  GST_PLAY_FLAG_TEXT          = (1 << 2),
  GST_PLAY_FLAG_VIS           = (1 << 3),
  GST_PLAY_FLAG_SOFT_VOLUME   = (1 << 4),
  GST_PLAY_FLAG_NATIVE_AUDIO  = (1 << 5),
  GST_PLAY_FLAG_NATIVE_VIDEO  = (1 << 6),
  GST_PLAY_FLAG_DOWNLOAD      = (1 << 7),
  GST_PLAY_FLAG_BUFFERING     = (1 << 8),
  GST_PLAY_FLAG_DEINTERLACE   = (1 << 9)
} GstPlayFlags;

static void
player_control_base_init (PlayerControlBase *self)
{
  PlayerControlBasePrivate *priv;
  GstBus *bus;
  GstPlayFlags flags;
  PlayerControlBaseClass *kclass = PLAYER_CONTROL_BASE_GET_CLASS(self);

  self->priv = PLAYER_PRIVATE (self);
  priv = self->priv;

  UMMS_DEBUG("");


  priv->pipeline = gst_element_factory_make ("playbin2", "pipeline");

  g_signal_connect(priv->pipeline, "notify::source", G_CALLBACK(_source_changed_cb), self);

  bus = gst_pipeline_get_bus (GST_PIPELINE (priv->pipeline));

  gst_bus_add_signal_watch (bus);

  g_signal_connect_object (bus, "message::error",
      G_CALLBACK (bus_message_error_cb),
      self,
      0);

  g_signal_connect_object (bus, "message::buffering",
      G_CALLBACK (bus_message_buffering_cb),
      self,
      0);

  g_signal_connect_object (bus, "message::eos",
      G_CALLBACK (bus_message_eos_cb),
      self,
      0);

  g_signal_connect_object (bus,
      "message::state-changed",
      G_CALLBACK (bus_message_state_change_cb),
      self,
      0);

  g_signal_connect_object (bus,
      "message::tag",
      G_CALLBACK (bus_message_get_tag_cb),
      self,
      0);

  gst_bus_set_sync_handler(bus, (GstBusSyncHandler) bus_sync_handler,
      self);

  gst_object_unref (GST_OBJECT (bus));

  g_signal_connect (priv->pipeline,
                    "video-tags-changed", G_CALLBACK (video_tags_changed_cb), self);

  g_signal_connect (priv->pipeline,
                    "audio-tags-changed", G_CALLBACK (audio_tags_changed_cb), self);

  g_signal_connect (priv->pipeline,
                    "text-tags-changed", G_CALLBACK (text_tags_changed_cb), self);

  /*
   *Use GST_PLAY_FLAG_DOWNLOAD flag to enable Gstreamer Download buffer mode,
   *so that we can query the buffered time/bytes, and further the time rangs.
   */

  g_object_get (priv->pipeline, "flags", &flags, NULL);
  flags |= GST_PLAY_FLAG_DOWNLOAD;
  g_object_set (priv->pipeline, "flags", flags, NULL);

  priv->player_state = PlayerStateNull;
  priv->suspended = FALSE;
  priv->pos = 0;
  priv->res_mngr = umms_resource_manager_new ();
  priv->res_list = NULL;

  priv->tag_list = NULL;

  //Setup default target.
  /*
#define FULL_SCREEN_RECT "0,0,0,0"
  kclass->setup_ismd_vbin (self, FULL_SCREEN_RECT, UPP_A);
*/
  priv->target_type = ReservedType0;
  priv->target_initialized = TRUE;
}

PlayerControlBase *
player_control_base_new (void)
{
  return g_object_new (PLAYER_CONTROL_TYPE_BASE, NULL);
}

static void
_set_proxy (PlayerControlBase *self)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);
  g_return_if_fail (priv->source);
  if (g_object_class_find_property (G_OBJECT_GET_CLASS (priv->source), "proxy") == NULL)
    return;

  UMMS_DEBUG ("Setting proxy. proxy=%s, id=%s, pw=%s", priv->proxy_uri, priv->proxy_id, priv->proxy_pw);
  g_object_set (priv->source, "proxy", priv->proxy_uri,
                "proxy-id", priv->proxy_id,
                "proxy-pw", priv->proxy_pw, NULL);
}

static void
_source_changed_cb (GObject *object, GParamSpec *pspec, gpointer data)
{
  GstElement *source;
  PlayerControlBasePrivate *priv = GET_PRIVATE (data);
  PlayerControlBaseClass *kclass = PLAYER_CONTROL_BASE_GET_CLASS(data);

  g_object_get(priv->pipeline, "source", &source, NULL);
  gst_object_replace((GstObject**) &priv->source, (GstObject*) source);
  UMMS_DEBUG ("source changed");
  kclass->set_proxy ((PlayerControlBase *)data);

  return;
}


