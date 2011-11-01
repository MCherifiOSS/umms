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
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
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
#include "dvb-player-control-base.h"
#include "media-player-control.h"
#include "param-table.h"

static void media_player_control_init (MediaPlayerControl* iface);
static void socket_thread_join(MediaPlayerControl* dvd_player);

G_DEFINE_TYPE_WITH_CODE (DvbPlayerControlBase, dvb_player_control_base, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TYPE_MEDIA_PLAYER_CONTROL, media_player_control_init))

#define PLAYER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DVB_PLAYER_CONTROL_TYPE_BASE, DvbPlayerControlBasePrivate))

#define GET_PRIVATE(o) ((DvbPlayerControlBase *)o)->priv

static const gchar *gst_state[] = {
  "GST_STATE_VOID_PENDING",
  "GST_STATE_NULL",
  "GST_STATE_READY",
  "GST_STATE_PAUSED",
  "GST_STATE_PLAYING"
};


static const gchar *dvb_player_control_base_param_name[] = {
  "modulation",
  "trans-mode",
  "bandwidth",
  "frequency",
  "code-rate-lp",
  "code-rate-hp",
  "guard",
  "hierarchy"
};

static guint dvb_player_control_base_param_val[DVB_PLAYER_CONTROL_BASE_PARAMS_NUM] = {0, };

/*
 * URI pattern:
 * dvb://?program-number=x&type=x&modulation=x&trans-mod=x&bandwidth=x&frequency=x&code-rate-lp=x&
 * code-rate-hp=x&guard=x&hierarchy=x
 */

static gchar *dvb_player_control_base_type_name[] = {
  "DVB-T",
  "DVB-C",
  "DVB-S"
};

/*
static GstStaticCaps hw_h264_static_caps = GST_STATIC_CAPS (HW_H264_CAPS);
static GstStaticCaps hw_mpeg2_static_caps = GST_STATIC_CAPS (HW_MPEG2_CAPS);
static GstStaticCaps hw_mpeg4_static_caps = GST_STATIC_CAPS (HW_MPEG4_CAPS);
static GstStaticCaps hw_vc1_static_caps = GST_STATIC_CAPS (HW_VC1_CAPS);

static GstStaticCaps *hw_static_caps[HW_FORMAT_NUM] = {&hw_h264_static_caps,
    &hw_mpeg2_static_caps,
    &hw_mpeg4_static_caps,
    &hw_vc1_static_caps
                                                      };
*/

static gboolean
set_properties(DvbPlayerControlBase *player, const gchar *location)
{
  gint i;
  gboolean ret = FALSE;
  gchar **part = NULL;
  gchar **params = NULL;
  gchar *format = NULL;
  gint program_num;
  gint type;
  gboolean invalid_params = FALSE;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (player);

  g_return_val_if_fail (location, FALSE);
  UMMS_DEBUG ("location = '%s'", location);

  if (!priv->source || !priv->tsdemux)
    goto out;

  if (!(part = g_strsplit (location, "?", 0))) {
    UMMS_DEBUG ("splitting location failed");
    goto out;
  }

  if (!(params = g_strsplit (part[1], "&", 0))) {
    UMMS_DEBUG ("splitting params failed");
    goto out;
  }

  if ((0 == sscanf (params[1], "type=%d", &type))) {
    UMMS_DEBUG ("Invalid dvb type string: %s", params[1]);
    invalid_params = TRUE;
    goto out;
  }

  if (!(type >= 0 && type < DVB_PLAYER_CONTROL_BASE_PARAMS_NUM)) {
    UMMS_DEBUG ("Invalid dvb type: %d", type);
    invalid_params = TRUE;
    goto out;
  }

  UMMS_DEBUG ("Setting %s properties", dvb_player_control_base_type_name[type]);

  //setting dvbsrc properties
  for (i = 0; i < DVB_PLAYER_CONTROL_BASE_PARAMS_NUM; i++) {
    UMMS_DEBUG ("dvbt_param_name[%d]=%s", i, dvb_player_control_base_param_name[i]);
    format = g_strconcat (dvb_player_control_base_param_name[i], "=%u", NULL);
    sscanf (params[i+2], format, &dvb_player_control_base_param_val[i]);
    g_free (format);
    g_object_set (priv->source, dvb_player_control_base_param_name[i], dvb_player_control_base_param_val[i], NULL);
    UMMS_DEBUG ("Setting param: %s=%u", dvb_player_control_base_param_name[i], dvb_player_control_base_param_val[i]);
  }

  //setting program number
  sscanf (params[0], "program-number=%d", &program_num);
  g_object_set (priv->tsdemux, "program-number",  program_num, NULL);
  UMMS_DEBUG ("Setting program-number: %d", program_num);

  ret = TRUE;

out:
  if (part)
    g_strfreev (part);
  if (params)
    g_strfreev (params);

  if (invalid_params) {
    UMMS_DEBUG ("Incorrect params string");
  }

  if (!ret) {
    UMMS_DEBUG ("failed");
  }
  return ret;
}

static gboolean
dvb_player_control_base_parse_uri (DvbPlayerControlBase *player, const gchar *uri)
{
  gboolean ret = TRUE;
  gchar *protocol = NULL;
  gchar *location = NULL;

  g_return_val_if_fail (uri, FALSE);

  protocol = gst_uri_get_protocol (uri);

  if (strcmp (protocol, "dvb") != 0) {
    ret = FALSE;
  } else {
    location = gst_uri_get_location (uri);

    if (location != NULL) {
      ret = set_properties (player, location);
    } else
      ret = FALSE;
  }

  if (protocol)
    g_free (protocol);
  if (location)
    g_free (location);

  return ret;
}

static gboolean
dvb_player_control_base_set_uri (MediaPlayerControl *self,
                    const gchar           *uri)
{
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (uri, FALSE);

  UMMS_DEBUG ("SetUri called: old = %s new = %s",
              priv->uri,
              uri);

  if (priv->player_state != PlayerStateNull) {
    UMMS_DEBUG ("Failed, can only set uri on Null state");
    return FALSE;
  }

  if (priv->uri) {
    g_free (priv->uri);
  }

  priv->uri = g_strdup (uri);

#ifdef DVB_SRC
  return dvb_player_control_base_parse_uri (DVB_PLAYER_CONTROL_BASE (self), uri);
#else
  return TRUE;
#endif
}

static gboolean setup_xwindow_target (DvbPlayerControlBase *self, GHashTable *params)
{
  UMMS_DEBUG("virtual APIs default Impl: NULL");
  return TRUE;
}

static gboolean
unset_xwindow_target (MediaPlayerControl *self)
{
  UMMS_DEBUG("virtual APIs default Impl: NULL");
  return TRUE;
}

static gboolean
unset_target (MediaPlayerControl *self)
{
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);

  if (!priv->target_initialized)
    return TRUE;

  /*
   *For DataCopy and ReservedType0, nothing need to do here.
   */
  switch (priv->target_type) {
    case XWindow:
      unset_xwindow_target (self);
      break;
    case DataCopy:
      break;
    case Socket:
      g_assert_not_reached ();
      break;
    case ReservedType0:
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  priv->target_type = TargetTypeInvalid;
  priv->target_initialized = FALSE;
  return TRUE;
}

static gboolean
set_target (DvbPlayerControlBase *self, gint type, GHashTable *params)
{
  UMMS_DEBUG("virtual APIs default Impl: NULL");
  return TRUE;
}

static gboolean
dvb_player_control_base_set_target (MediaPlayerControl *self, gint type, GHashTable *params)
{
  DvbPlayerControlBaseClass *kclass = DVB_PLAYER_CONTROL_BASE_GET_CLASS(self);
  return kclass->set_target((DvbPlayerControlBase *)self, type, params);
}

static gboolean
prepare_plane (MediaPlayerControl *self)
{
  GstElement *vsink_bin;
  gint plane;
  gboolean ret = TRUE;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);

  vsink_bin = priv->vsink;
  if (vsink_bin) {
    if (g_object_class_find_property (G_OBJECT_GET_CLASS (vsink_bin), "gdl-plane") == NULL) {
      UMMS_DEBUG ("vsink has no gdl-plane property, which means target type is not XWindow or ReservedType0");
      return ret;
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

  return ret;
}


static gboolean request_resource (MediaPlayerControl *self)
{
  UMMS_DEBUG("virtual APIs default Impl: NULL");
  return TRUE;
}

static void release_resource (DvbPlayerControlBase *self)
{
  UMMS_DEBUG("virtual APIs default Impl: NULL");
  return;
}

static gboolean
dvb_player_control_base_play (MediaPlayerControl *self)
{
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE(self);
  DvbPlayerControlBaseClass *kclass = DVB_PLAYER_CONTROL_BASE_GET_CLASS(self);

  if (kclass->request_resource(self)) {
    if (gst_element_set_state(priv->pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
      UMMS_DEBUG ("Set pipeline to paused failed");
      return FALSE;
    }
  }
  return TRUE;
}

static gboolean stop_recording (DvbPlayerControlBase *self)
{
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);

  UMMS_DEBUG ("Begin");

  if (!priv->tsfilesink) {
    UMMS_DEBUG ("We don't have filesink, can't remove it");
    return FALSE;
  }

  gst_bin_remove (GST_BIN(priv->pipeline), priv->tsfilesink);
  TEARDOWN_ELEMENT (priv->tsfilesink);

  gst_element_release_request_pad (priv->tsdemux, priv->request_pad);
  gst_object_unref (priv->request_pad);
  priv->request_pad = NULL;

  priv->recording = FALSE;
  UMMS_DEBUG ("End");
  return TRUE;
}

static gboolean
_stop_pipe (DvbPlayerControlBase *control)
{
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (control);
  DvbPlayerControlBaseClass *kclass = DVB_PLAYER_CONTROL_BASE_GET_CLASS(control);

  if (priv->recording) {
    stop_recording (control);
  }

  if (gst_element_set_state(priv->pipeline, GST_STATE_NULL) == GST_STATE_CHANGE_FAILURE) {
    UMMS_DEBUG ("Unable to set NULL state");
    return FALSE;
  }

  kclass->release_resource (control);

  UMMS_DEBUG ("gstreamer engine stopped");
  return TRUE;
}

static gboolean
dvb_player_control_base_stop (MediaPlayerControl *self)
{
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);
  PlayerState old_state;

  if (!_stop_pipe (self))
    return FALSE;

  old_state = priv->player_state;
  priv->player_state = PlayerStateStopped;

  if (old_state != priv->player_state) {
    media_player_control_emit_player_state_changed (self, old_state, priv->player_state);
  }
  media_player_control_emit_stopped (self);

  return TRUE;
}


static gboolean
dvb_player_control_base_is_seekable (MediaPlayerControl *self, gboolean *seekable)
{
  DvbPlayerControlBasePrivate *priv = NULL;
  GstElement *pipeline = NULL;
  gboolean res = FALSE;
  gint old_seekable;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (seekable != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipeline = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipeline), FALSE);


  if (priv->uri == NULL)
    return FALSE;

  old_seekable = priv->seekable;

  if (priv->seekable == -1) {
    GstQuery *query;

    query = gst_query_new_seeking (GST_FORMAT_TIME);
    if (gst_element_query (pipeline, query)) {
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
set_volume (DvbPlayerControlBase *self, gint vol)
{
  UMMS_DEBUG("virtual APIs default Impl: NULL");
  return TRUE;
}

static gboolean
dvb_player_control_base_set_volume (MediaPlayerControl *self, gint vol)
{
  DvbPlayerControlBaseClass *kclass = DVB_PLAYER_CONTROL_BASE_GET_CLASS(self);
  return kclass->set_volume((DvbPlayerControlBase *)self, vol);
}

static gboolean
get_volume (DvbPlayerControlBase *self, gint *vol)
{
  UMMS_DEBUG("virtual APIs default Impl: NULL");
  return TRUE;
}

static gboolean
dvb_player_control_base_get_volume (MediaPlayerControl *self, gint *vol)
{
  DvbPlayerControlBaseClass *kclass = DVB_PLAYER_CONTROL_BASE_GET_CLASS(self);
  return kclass->get_volume((DvbPlayerControlBase *)self, vol);

}

static gboolean
dvb_player_control_base_support_fullscreen (MediaPlayerControl *self, gboolean *support_fullscreen)
{
  //We are using ismd_vidrend_bin, so this function always return TRUE.
  *support_fullscreen = TRUE;
  return TRUE;
}

static gboolean
dvb_player_control_base_get_player_state (MediaPlayerControl *self,
    gint *state)
{
  DvbPlayerControlBasePrivate *priv;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (state != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);

  *state = priv->player_state;
  return TRUE;
}

static gboolean
set_mute (DvbPlayerControlBase *self, gint mute)
{
  UMMS_DEBUG("virtual APIs default Impl: NULL");
  return TRUE;
}

static gboolean
dvb_player_control_base_set_mute (MediaPlayerControl *self, gint mute)
{
  DvbPlayerControlBaseClass *kclass = DVB_PLAYER_CONTROL_BASE_GET_CLASS(self);
  return kclass->set_mute((DvbPlayerControlBase *)self, mute);
}

static gboolean
is_mute (DvbPlayerControlBase *self, gint *mute)
{
  UMMS_DEBUG("virtual APIs default Impl: NULL");
  return TRUE;
}

static gboolean
dvb_player_control_base_is_mute (MediaPlayerControl *self, gint *mute)
{
  DvbPlayerControlBaseClass *kclass = DVB_PLAYER_CONTROL_BASE_GET_CLASS(self);
  return kclass->is_mute((DvbPlayerControlBase *)self, mute);

}


static gboolean
set_scale_mode (DvbPlayerControlBase *self, gint scale_mode)
{
  UMMS_DEBUG("virtual APIs default Impl: NULL");
  return TRUE;
}

static gboolean
dvb_player_control_base_set_scale_mode (MediaPlayerControl *self, gint scale_mode)
{
  DvbPlayerControlBaseClass *kclass = DVB_PLAYER_CONTROL_BASE_GET_CLASS(self);
  return kclass->set_scale_mode((DvbPlayerControlBase *)self, scale_mode);

}

static gboolean
get_scale_mode (DvbPlayerControlBase *self, gint *scale_mode)
{
  UMMS_DEBUG("virtual APIs default Impl: NULL");
  return TRUE;
}

static gboolean
dvb_player_control_base_get_scale_mode (MediaPlayerControl *self, gint *scale_mode)
{
  DvbPlayerControlBaseClass *kclass = DVB_PLAYER_CONTROL_BASE_GET_CLASS(self);
  return kclass->get_scale_mode((DvbPlayerControlBase *)self, scale_mode);
}

static void
no_more_pads_cb (GstElement *element, gpointer data)
{
  DvbPlayerControlBase *player = (DvbPlayerControlBase *)data;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (player);
  GstStateChangeReturn ret;

  /*set pipeline to playing, so that sinks start to render data*/
  UMMS_DEBUG ("Set pipe to playing");
  if ((ret = gst_element_set_state (priv->pipeline, GST_STATE_PLAYING)) == GST_STATE_CHANGE_FAILURE) {
    UMMS_DEBUG ("Settint pipeline to playing failed");
  }
  return;
}

static gboolean
set_video_size (DvbPlayerControlBase *self,
    guint x, guint y, guint w, guint h)

{
  UMMS_DEBUG("virtual APIs default Impl: NULL");
  return TRUE;
}

static gboolean
dvb_player_control_base_set_video_size (MediaPlayerControl *self,
    guint x, guint y, guint w, guint h)

{
  DvbPlayerControlBaseClass *kclass = DVB_PLAYER_CONTROL_BASE_GET_CLASS(self);
  return kclass->set_video_size((DvbPlayerControlBase *)self,x,y,w,h );

}

static gboolean
get_video_size (DvbPlayerControlBase *self,
    guint *w, guint *h)
{
  UMMS_DEBUG("virtual APIs default Impl: NULL");
  return TRUE;
}


static gboolean
dvb_player_control_base_get_video_size (MediaPlayerControl *self,
    guint *w, guint *h)
{
  DvbPlayerControlBaseClass *kclass = DVB_PLAYER_CONTROL_BASE_GET_CLASS(self);
  return kclass->get_video_size((DvbPlayerControlBase *)self,w,h );
}



static gboolean
dvb_player_control_base_get_protocol_name(DvbPlayerControlBase *self, gchar ** prot_name)
{
  *prot_name = "dvb";
  return TRUE;
}

static gboolean
dvb_player_control_base_get_current_uri(MediaPlayerControl *self, gchar ** uri)
{
  DvbPlayerControlBasePrivate *priv = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  *uri = priv->uri;
  return TRUE;
}

static gboolean start_recording (DvbPlayerControlBase *self, gchar *location)
{

  gboolean ret = FALSE;
  GstPad *srcpad = NULL;
  GstPad *sinkpad = NULL;
  GstElement *filesink = NULL;
  gchar *file_location = NULL;

  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);

  if ((priv->player_state != PlayerStatePlaying) || priv->tsfilesink || !priv->tsdemux) {
    goto out;
  }

  if (!(filesink = gst_element_factory_make ("filesink", NULL))) {
    UMMS_DEBUG ("Creating filesink failed");
    goto out;
  }
  gst_object_ref_sink (filesink);

#define DEFAULT_FILE_LOCATION "/tmp/record.ts"
  if (location && location[0] != '\0')
    file_location = location;
  else
    file_location = DEFAULT_FILE_LOCATION;

  //validate the location 
  FILE *fd = fopen (file_location, "wb");
  if (fd == NULL) {
    UMMS_DEBUG ("Could not open file \"%s\" for writing.", file_location);
    goto out;
  } else {
    fclose (fd);
  }

  UMMS_DEBUG ("location: '%s'", file_location);
  g_object_set (filesink, "location", file_location, NULL);

  gst_bin_add (GST_BIN(priv->pipeline), filesink);

  if (!(sinkpad = gst_element_get_static_pad (filesink, "sink"))) {
    UMMS_DEBUG ("Getting program pad failed");
    goto failed;
  }

  if (!(srcpad = gst_element_get_request_pad (priv->tsdemux, "program"))) {
    UMMS_DEBUG ("Getting program pad failed");
    goto failed;
  }


  if (GST_PAD_LINK_OK != gst_pad_link (srcpad, sinkpad)) {
    UMMS_DEBUG ("Linking filesink failed");
    goto failed;
  }

  if (GST_STATE_CHANGE_FAILURE == gst_element_set_state (filesink, GST_STATE_PLAYING)) {
    UMMS_DEBUG ("Setting filesink to playing failed");
    goto failed;
  }


  ret = TRUE;
  priv->recording = TRUE;
  priv->tsfilesink = filesink;
  priv->request_pad = srcpad;

out:
  if (sinkpad)
    gst_object_unref (sinkpad);

  if (!ret) {
    TEARDOWN_ELEMENT (filesink);
    UMMS_DEBUG ("failed!!!");
  }

  return ret;

failed:
  gst_bin_remove (GST_BIN(priv->pipeline), filesink);
  goto out;
}


static gboolean
dvb_player_control_base_record (MediaPlayerControl *self, gboolean to_record, gchar *location)
{
  DvbPlayerControlBasePrivate *priv = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);

  if (to_record == priv->recording) {
    return TRUE;
  }

  if (to_record)
    return start_recording (self, location);
  else
    return stop_recording (self);
}

static gboolean
dvb_player_control_base_get_pat (MediaPlayerControl *self, GPtrArray **pat)
{
  GValueArray *pat_info = NULL;
  GPtrArray *pat_out = NULL;
  DvbPlayerControlBasePrivate *priv = NULL;
  gboolean ret = FALSE;
  gint i;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);
  g_return_val_if_fail (pat, FALSE);

  priv = GET_PRIVATE (self);

  if (!priv->tsdemux)
    goto out;

  pat_out = g_ptr_array_new ();
  if (!pat_out) {
    goto out;
  }

  g_object_get (priv->tsdemux, "pat-info", &pat_info, NULL);

  if (!pat_info) {
    UMMS_DEBUG ("no pat-info");
    goto out;
  }


  for (i = 0; i < pat_info->n_values; i++) {
    GValue *val = NULL;
    GObject *entry = NULL;
    GHashTable *ht = NULL;
    GValue *val_out = NULL;
    guint program_num = 0;
    guint pid = 0;


    //get program-number and pid
    val = g_value_array_get_nth (pat_info, i);
    entry = g_value_get_object (val);
    g_object_get (entry, "program-number", &program_num, "pid", &pid, NULL);
    UMMS_DEBUG ("program-number : %u, pid : %u", program_num, pid);

    //fill the output
    ht = g_hash_table_new (NULL, NULL);
    val_out = g_new0(GValue, 1);
    g_value_init (val_out, G_TYPE_UINT);
    g_value_set_uint (val_out, program_num);
    g_hash_table_insert (ht, "program-number", val_out);

    val_out = g_new0(GValue, 1);
    g_value_init (val_out, G_TYPE_UINT);
    g_value_set_uint (val_out, pid);
    g_hash_table_insert (ht, "pid", val_out);

    g_ptr_array_add (pat_out, ht);
  }
  ret = TRUE;
  *pat = pat_out;

out:
  if (!ret && pat_out) {
    g_ptr_array_free (pat_out, FALSE);
  }

  if (pat_info)
    g_value_array_free (pat_info);

  return ret;
}


static gboolean
dvb_player_control_base_get_pmt (MediaPlayerControl *self, guint *program_num, guint *pcr_pid, GPtrArray **stream_info)
{
  DvbPlayerControlBasePrivate *priv;
  gboolean ret = FALSE;
  GHashTable *stream_info_out;
  gint i;

  GObject *pmt_info;
  GValueArray *stream_info_array = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);
  g_return_val_if_fail (program_num, FALSE);
  g_return_val_if_fail (pcr_pid, FALSE);
  g_return_val_if_fail (stream_info, FALSE);

  priv = GET_PRIVATE (self);

  if (!priv->tsdemux)
    goto out;

  g_object_get (priv->tsdemux, "pmt-info", &pmt_info, NULL);
  if (!pmt_info)
    goto out;

  //get all the pmt specific info we need
  g_object_get (pmt_info, "program-number", program_num, "pcr-pid", pcr_pid, "stream-info", &stream_info_array,  NULL);
  if (!stream_info_array) {
    UMMS_DEBUG ("getting stream-info failed");
    goto out;
  }

  //elementary stream info
  *stream_info = g_ptr_array_new ();
  for (i = 0; i < stream_info_array->n_values; i++) {
    guint pid;
    guint stream_type;
    GValue *val_tmp;
    GObject *stream_info_tmp;

    val_tmp = g_value_array_get_nth (stream_info_array, i);
    stream_info_tmp = g_value_get_object (val_tmp);
    g_object_get (stream_info_tmp, "pid", &pid, "stream-type", &stream_type, NULL);

    stream_info_out = g_hash_table_new (NULL, NULL);
    val_tmp = g_new0 (GValue, 1);
    g_value_init (val_tmp, G_TYPE_UINT);
    g_value_set_uint (val_tmp, pid);
    g_hash_table_insert (stream_info_out, "pid", val_tmp);
    val_tmp = g_new0 (GValue, 1);
    g_value_init (val_tmp, G_TYPE_UINT);
    g_value_set_uint (val_tmp, stream_type);
    g_hash_table_insert (stream_info_out, "stream-type", val_tmp);

    g_ptr_array_add (*stream_info, stream_info_out);
  }

  ret =  TRUE;

out:
  if (pmt_info)
    g_object_unref (pmt_info);
  if (stream_info_array)
    g_value_array_free (stream_info_array);
  return ret;
}

static gboolean
dvb_player_control_base_get_associated_data_channel (MediaPlayerControl *self, gchar **ip, gint *port)
{
  DvbPlayerControlBasePrivate *priv = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (IS_MEDIA_PLAYER_CONTROL(self), FALSE);
  g_return_val_if_fail (ip, FALSE);
  g_return_val_if_fail (port, FALSE);

  priv = GET_PRIVATE (self);

  if (priv->ip) {
    *ip = g_strdup(priv->ip);
  } else {
    *ip = NULL;
  }

  *port = priv->port;
  return TRUE;
}


static void
media_player_control_init (MediaPlayerControl *iface)
{
  MediaPlayerControlClass *klass = (MediaPlayerControlClass *)iface;

  media_player_control_implement_set_uri (klass,
      dvb_player_control_base_set_uri);
  media_player_control_implement_set_target (klass,
      dvb_player_control_base_set_target);
  media_player_control_implement_play (klass,
      dvb_player_control_base_play);
  media_player_control_implement_stop (klass,
      dvb_player_control_base_stop);
  media_player_control_implement_set_video_size (klass,
      dvb_player_control_base_set_video_size);
  media_player_control_implement_get_video_size (klass,
      dvb_player_control_base_get_video_size);
  media_player_control_implement_is_seekable (klass,
      dvb_player_control_base_is_seekable);
  media_player_control_implement_set_volume (klass,
      dvb_player_control_base_set_volume);
  media_player_control_implement_get_volume (klass,
      dvb_player_control_base_get_volume);
  media_player_control_implement_support_fullscreen (klass,
      dvb_player_control_base_support_fullscreen);
  media_player_control_implement_get_player_state (klass,
      dvb_player_control_base_get_player_state);
  media_player_control_implement_set_mute (klass,
      dvb_player_control_base_set_mute);
  media_player_control_implement_is_mute (klass,
      dvb_player_control_base_is_mute);
  media_player_control_implement_set_scale_mode (klass,
      dvb_player_control_base_set_scale_mode);
  media_player_control_implement_get_scale_mode (klass,
      dvb_player_control_base_get_scale_mode);
  media_player_control_implement_get_protocol_name (klass,
      dvb_player_control_base_get_protocol_name);
  media_player_control_implement_get_current_uri (klass,
      dvb_player_control_base_get_current_uri);
  media_player_control_implement_record (klass,
      dvb_player_control_base_record);
  media_player_control_implement_get_pat (klass,
      dvb_player_control_base_get_pat);
  media_player_control_implement_get_pmt (klass,
      dvb_player_control_base_get_pmt);
  media_player_control_implement_get_associated_data_channel (klass,
      dvb_player_control_base_get_associated_data_channel);
}

static void
dvb_player_control_base_get_property (GObject    *object,
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
dvb_player_control_base_set_property (GObject      *object,
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
dvb_player_control_base_dispose (GObject *object)
{
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (object);
  int i;

  UMMS_DEBUG ("Begin");
  _stop_pipe ((DvbPlayerControlBase *)object);

  TEARDOWN_ELEMENT(priv->source);
  TEARDOWN_ELEMENT(priv->tsdemux);
  TEARDOWN_ELEMENT(priv->vsink);
  TEARDOWN_ELEMENT(priv->asink);
  TEARDOWN_ELEMENT(priv->pipeline);

  if (priv->listen_thread) {
    socket_thread_join((MediaPlayerControl *)object);
    priv->listen_thread = NULL;
  }

  for (i = 0; i < SOCK_MAX_SERV_CONNECTS; i++) {
    if (priv->serv_fds[i] != -1) {
      close(priv->serv_fds[i]);
    }
    priv->serv_fds[i] = -1;
  }

  if (priv->socks_lock) {
    g_mutex_free (priv->socks_lock);
    priv->socks_lock = NULL;
  }

  if (priv->ip && priv->ip != SOCK_SOCKET_DEFAULT_ADDR) {
    g_free(priv->ip);
    priv->ip = NULL;
  }

  if (priv->target_type == XWindow) {
    unset_xwindow_target ((MediaPlayerControl *)object);
  }

  if (priv->disp) {
    XCloseDisplay (priv->disp);
    priv->disp = NULL;
  }

  if (priv->tag_list)
    gst_tag_list_free(priv->tag_list);
  priv->tag_list = NULL;

  G_OBJECT_CLASS (dvb_player_control_base_parent_class)->dispose (object);
  UMMS_DEBUG ("End");
}

static void
dvb_player_control_base_finalize (GObject *object)
{
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (object);

  RESET_STR(priv->uri);
  RESET_STR(priv->title);
  RESET_STR(priv->artist);

  G_OBJECT_CLASS (dvb_player_control_base_parent_class)->finalize (object);
}


static void
bus_message_state_change_cb (GstBus     *bus,
    GstMessage *message,
    DvbPlayerControlBase  *self)
{

  GstState old_state, new_state;
  PlayerState old_player_state;
  gpointer src;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);

  src = GST_MESSAGE_SRC (message);
  if (src != priv->pipeline)
    return;

  gst_message_parse_state_changed (message, &old_state, &new_state, NULL);

  UMMS_DEBUG ("state-changed: old='%s', new='%s'", gst_state[old_state], gst_state[new_state]);

  old_player_state = priv->player_state;
  if (new_state == GST_STATE_PAUSED) {
    priv->player_state = PlayerStatePaused;
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
bus_message_error_cb (GstBus     *bus,
    GstMessage *message,
    DvbPlayerControlBase  *self)
{
  GError *error = NULL;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);

  UMMS_DEBUG ("message::error received on bus");

  gst_message_parse_error (message, &error, NULL);
  _stop_pipe (self);

  media_player_control_emit_error (self, UMMS_ENGINE_ERROR_FAILED, error->message);

  UMMS_DEBUG ("Error emitted with message = %s", error->message);

  g_clear_error (&error);
}

static void
bus_message_eos_cb (GstBus     *bus,
                    GstMessage *message,
                    DvbPlayerControlBase  *self)
{
  UMMS_DEBUG ("message::eos received on bus");
  media_player_control_emit_eof (self);
}

static void
bus_message_get_tag_cb (GstBus *bus, GstMessage *message, DvbPlayerControlBase  *self)
{
  gpointer src;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (self);
  GstPad * src_pad = NULL;
  GstTagList * tag_list = NULL;
  guint32 bit_rate;
  gchar * pad_name = NULL;
  gchar * element_name = NULL;
  gchar * title = NULL;
  gchar * artist = NULL;
  gboolean metadata_changed = FALSE;

  src = GST_MESSAGE_SRC (message);
 
  if(message->type != GST_MESSAGE_TAG) {
    UMMS_DEBUG("not a tag message in a registered tag signal, strange");
    return;
  }

  gst_message_parse_tag_full (message, &src_pad, &tag_list);
  if(src_pad) {
    pad_name = g_strdup (GST_PAD_NAME (src_pad));
    UMMS_DEBUG("The pad name is %s", pad_name);
  }

  if(message->src) {
    element_name = g_strdup (GST_ELEMENT_NAME (message->src));
    UMMS_DEBUG("The element name is %s", element_name);
  }

  priv->tag_list = 
      gst_tag_list_merge(priv->tag_list, tag_list, GST_TAG_MERGE_REPLACE);

  //cache the title
  if(gst_tag_list_get_string_index (tag_list, GST_TAG_TITLE, 0, &title)) {
    UMMS_DEBUG("Element: %s, provide the title: %s", element_name, title);
    RESET_STR(priv->title); 
    priv->title = title;
    metadata_changed = TRUE;
  }

  //cache the artist
  if(gst_tag_list_get_string_index (tag_list, GST_TAG_ARTIST, 0, &artist)) {
    UMMS_DEBUG("Element: %s, provide the artist: %s", element_name, artist);
    RESET_STR(priv->artist); 
    priv->artist = artist;
    metadata_changed = TRUE;
  }

  //only care about artist and title
  if (metadata_changed) {
    media_player_control_emit_metadata_changed (self);
  }

  if(src_pad)
    g_object_unref(src_pad);
  gst_tag_list_free (tag_list);
  if(pad_name)
    g_free(pad_name);
  if(element_name)
    g_free(element_name);

}

static GstBusSyncReply
bus_sync_handler (GstBus *bus,
                  GstMessage *message,
                  DvbPlayerControlBase *engine)
{
  DvbPlayerControlBasePrivate *priv;
  GstElement *vsink;
  GError *err = NULL;
  GstMessage *msg = NULL;

  if ( GST_MESSAGE_TYPE( message ) != GST_MESSAGE_ELEMENT )
    return( GST_BUS_PASS );

  if ( ! gst_structure_has_name( message->structure, "prepare-gdl-plane" ) )
    return( GST_BUS_PASS );

  g_return_val_if_fail (engine, GST_BUS_PASS);
  //g_return_val_if_fail (DVB_IS_GPLAYER (engine), GST_BUS_PASS);
  priv = GET_PRIVATE (engine);

  vsink =  GST_ELEMENT(GST_MESSAGE_SRC (message));
  UMMS_DEBUG ("sync-handler received on bus: prepare-gdl-plane, source: %s", GST_ELEMENT_NAME(vsink));

  if (!prepare_plane ((MediaPlayerControl *)engine)) {
    //Since we are in streame thread, let the vsink to post the error message. Handle it in bus_message_error_cb().
    err = g_error_new_literal (UMMS_RESOURCE_ERROR, UMMS_RESOURCE_ERROR_NO_RESOURCE, "Plane unavailable");
    msg = gst_message_new_error (GST_OBJECT_CAST(priv->pipeline), err, "No resource");
    gst_element_post_message (priv->pipeline, msg);
    g_error_free (err);
  }

  ///media_player_control_emit_request_window (engine);

  if (message)
    gst_message_unref (message);
  return( GST_BUS_DROP );
}

static void bus_group(GstElement *pipeline, DvbPlayerControlBase *self)
{
  GstBus *bus;
  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_signal_watch (bus);

  g_signal_connect_object (bus, "message::error",
      G_CALLBACK (bus_message_error_cb),
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

}


static void
dvb_player_control_base_init (DvbPlayerControlBase *self)
{
  UMMS_DEBUG("dvb_player_control_base_init empty! ");
  return ;
}

static void
socket_thread_join(MediaPlayerControl* dvd_player)
{
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (dvd_player);
  struct sockaddr_in server_addr;

  g_mutex_lock (priv->socks_lock);
  priv->sock_exit_flag = 1;

  if (priv->listen_fd != -1) { /* need to wakeup the listen thread. */
    struct hostent *host = gethostbyname("localhost");
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd != -1) {
      bzero(&server_addr, sizeof(server_addr));
      server_addr.sin_family = AF_INET;
      server_addr.sin_port = htons(priv->port);
      server_addr.sin_addr = *((struct in_addr*)host->h_addr);

      UMMS_DEBUG("try to wakeup the thread by connect");
      g_mutex_unlock (priv->socks_lock);
      connect (fd, (struct sockaddr*)(&server_addr), sizeof(struct sockaddr));
      close(fd);
    } else {
      g_mutex_unlock (priv->socks_lock);
      UMMS_DEBUG("socket create failed, can not wakeup the listen thread");
    } 
  } else {
    g_mutex_unlock (priv->socks_lock);//Just Unlock it.
  }

  g_thread_join(priv->listen_thread);
  UMMS_DEBUG("listen_thread has joined");
}

//decodable and sink elements
static void
update_elements_list (DvbPlayerControlBase *player)
{
  GList *res, *sinks;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (player);

  if (!priv->elements ||
       priv->elements_cookie !=
       gst_default_registry_get_feature_list_cookie ()) {
    if (priv->elements)
      gst_plugin_feature_list_free (priv->elements);
    res =
      gst_element_factory_list_get_elements
      (GST_ELEMENT_FACTORY_TYPE_DECODABLE, GST_RANK_MARGINAL);
    sinks =
      gst_element_factory_list_get_elements
      (GST_ELEMENT_FACTORY_TYPE_AUDIOVIDEO_SINKS, GST_RANK_MARGINAL);
    priv->elements = g_list_concat (res, sinks);
    priv->elements =
      g_list_sort (priv->elements, gst_plugin_feature_rank_compare_func);
    priv->elements_cookie = gst_default_registry_get_feature_list_cookie ();
  }
}

static gboolean link_sink (DvbPlayerControlBase *player, GstPad *pad)
{
  DvbPlayerControlBasePrivate *priv;
  GstStructure *s;
  const gchar *name;
  GstPad *sinkpad = NULL;
  GstCaps *caps   = NULL;
  gboolean ret = TRUE;

  // g_return_val_if_fail (DVB_IS_PLAYER (player), FALSE);
  g_return_val_if_fail (GST_IS_PAD (pad), FALSE);

  priv = GET_PRIVATE (player);

  caps = gst_pad_get_caps_reffed (pad);
  //  UMMS_DEBUG ("pad:%s, caps : %"GST_PTR_FORMAT, GST_PAD_NAME(pad), caps);

  s = gst_caps_get_structure (caps, 0);

  if (s) {
    GstElement *sink = NULL;
    GstElement *queue = NULL;
    name = gst_structure_get_name (s);
    g_printf ("caps name : %s\n", name);

    if (g_str_has_prefix (name, "video")) {
      sink = priv->vsink;
      g_print ("Found video decoded pad: %p, linking it\n", pad);

    } else if (g_str_has_prefix (name, "audio")) {
      g_print ("Found audio decoded pad: %p, linking it\n", pad);
      sink = priv->asink;

    } else {
      g_print ("Ignore this pad\n");
    }

    if (sink) {
      if (!(queue = gst_element_factory_make ("queue", NULL))) {
        UMMS_DEBUG ("Creating queue failed");
        goto out;
      }

      gst_bin_add (GST_BIN (priv->pipeline), queue);
      gst_bin_add (GST_BIN (priv->pipeline), sink);
      if (!gst_element_link (queue, sink)) {
        UMMS_DEBUG ("linking queue and sink failed");
        gst_element_set_state (queue, GST_STATE_NULL);
        gst_bin_remove (GST_BIN (priv->pipeline), queue);
        gst_element_set_state (sink, GST_STATE_NULL);
        gst_bin_remove (GST_BIN (priv->pipeline), sink);
        ret = FALSE;
        goto out;
      }

      sinkpad = gst_element_get_static_pad (queue, "sink");
      if (gst_pad_link (pad, sinkpad) != GST_PAD_LINK_OK) {
        g_print ("link pad(%s): sinkpad(%s) failed\n", GST_PAD_NAME(pad), GST_PAD_NAME (sinkpad));
        gst_element_set_state (queue, GST_STATE_NULL);
        gst_bin_remove (GST_BIN (priv->pipeline), queue);
        gst_element_set_state (sink, GST_STATE_NULL);
        gst_bin_remove (GST_BIN (priv->pipeline), sink);
        ret = FALSE;
        goto out;
      }

      gst_element_set_state (queue, GST_STATE_PAUSED);
      gst_element_set_state (sink, GST_STATE_PAUSED);
      g_print ("pad: %p linked\n", pad);
    }
  } else {
    g_print ("Getting caps structure failed\n");
    ret = FALSE;
    goto out;
  }

out:

  if (caps)
    gst_caps_unref (caps);

  if (sinkpad)
    gst_object_unref (sinkpad);

  return ret;
}

static GstPad *
get_sink_pad (GstElement * element)
{
  GstIterator *it;
  GstPad *pad = NULL;
  gpointer point;

  it = gst_element_iterate_sink_pads (element);

  if ((gst_iterator_next (it, &point)) == GST_ITERATOR_OK)
    pad = (GstPad *) point;

  gst_iterator_free (it);

  return pad;
}

static gboolean
autoplug_dec_element (DvbPlayerControlBase *player, GstElement * element)
{
  GList *pads;
  gboolean ret;

  UMMS_DEBUG ("Attempting to connect element %s further",
      GST_ELEMENT_NAME (element));

  for (pads = GST_ELEMENT_GET_CLASS (element)->padtemplates; pads;
      pads = g_list_next (pads)) {
    GstPadTemplate *templ = GST_PAD_TEMPLATE (pads->data);
    const gchar *templ_name;

    /* we are only interested in source pads */
    if (GST_PAD_TEMPLATE_DIRECTION (templ) != GST_PAD_SRC)
      continue;

    templ_name = GST_PAD_TEMPLATE_NAME_TEMPLATE (templ);
    UMMS_DEBUG ("got a source pad template %s", templ_name);

    /* We only care about always pad */
    switch (GST_PAD_TEMPLATE_PRESENCE (templ)) {
      case GST_PAD_ALWAYS:
      {
        /* get the pad that we need to autoplug */
        GstPad *pad = gst_element_get_static_pad (element, templ_name);
        if (!pad) {
          /* strange, pad is marked as always but it's not
           * there. Fix the element */
          UMMS_DEBUG ("could not get the pad for always template %s", templ_name);
          break;
        }

        UMMS_DEBUG ("got the pad for always template %s",
            templ_name);
        /* 
         * here is the pad, we need link it to sink. 
         * note that we just autoplug the first found always pad unless autoplugging failed.
         */
        ret = link_sink (player, pad);
        gst_object_unref (pad);

        if (ret) {
          return TRUE;
        }
        break;
      }
      case GST_PAD_SOMETIMES:
      case GST_PAD_REQUEST:
      {
        UMMS_DEBUG ("ignoring sometimes and request padtemplate %s", templ_name);
        break;
      }
    }
  }

  return FALSE;
}

/*
 * autoplug the pad with a decoder and proceed to link a sink
 */
static gboolean autoplug_pad(DvbPlayerControlBase *player, GstPad *pad, gint chain_type)
{
  GstCaps *caps = NULL;
  gboolean ret = TRUE;
  GList *compatible_elements = NULL;
  GList *walk = NULL;
  GList *chain_elements = NULL;//store the elements autoplugged from this pad.
  gboolean done = FALSE;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (player);

  UMMS_DEBUG ("autoplugging pad: %s, caps: ", GST_PAD_NAME (pad));
  caps = gst_pad_get_caps (pad);

  if (!caps || gst_caps_is_empty (caps)) {
    UMMS_DEBUG ("Unknow caps");
    ret = FALSE;
    goto out;
  }

  if (!gst_caps_is_fixed (caps)) {
    UMMS_DEBUG ("caps is not fixed, strange");
    goto out;
  }

  update_elements_list (player);

  compatible_elements =
    gst_element_factory_list_filter (priv->elements, caps, GST_PAD_SINK,
        FALSE);
  if (!compatible_elements) {
    UMMS_DEBUG ("no compatible element available for this pad:%s, caps : %"GST_PTR_FORMAT, GST_PAD_NAME(pad), caps);
    ret = FALSE;
    goto out;
  }

  gst_plugin_feature_list_debug (compatible_elements);

  //make and connect element
  for (walk = compatible_elements; walk; walk = walk->next) {
    GstElementFactory *factory;
    GstElement *element;
    GstPad *sinkpad;
    const gchar *klass;
    gboolean link_ret;

    factory = GST_ELEMENT_FACTORY_CAST (walk->data);

    /*
     * if it's a sink, we treat this pad as decoded pad,
     * and try to connnect this pad with our custom sink other than the auto-detected one
     */
    if (gst_element_factory_list_is_type (factory, GST_ELEMENT_FACTORY_TYPE_SINK)) {
      UMMS_DEBUG ("we found a \"decoded\" pad");

      klass = gst_element_factory_get_klass (factory);

      if (strstr (klass, "Audio") || strstr (klass, "Video")) {
        UMMS_DEBUG ("linking custom sink");
        link_ret = link_sink (player, pad);
      } else {
        /* unknown klass, skip this element */
        UMMS_DEBUG ("unknown sink klass %s found", klass);
        continue;
      }

      done = link_ret;
      break;
    }

    /*it is not sink, autoplug the dec element*/
    if ((element = gst_element_factory_create (factory, NULL)) == NULL) {
      UMMS_DEBUG ("Could not create an element from %s",
                  gst_plugin_feature_get_name (GST_PLUGIN_FEATURE (factory)));
      continue;
    }

    if ((gst_element_set_state (element,
         GST_STATE_READY)) == GST_STATE_CHANGE_FAILURE) {
      UMMS_DEBUG ("Couldn't set %s to READY",
                  GST_ELEMENT_NAME (element));
      gst_object_unref (element);
      continue;
    }

    if (!(sinkpad = get_sink_pad (element))) {
      UMMS_DEBUG ("Element %s doesn't have a sink pad",
                  GST_ELEMENT_NAME (element));
      gst_element_set_state (element, GST_STATE_NULL);
      gst_object_unref (element);
      continue;
    }

    if (!(gst_bin_add (GST_BIN (priv->pipeline), element))) {
      UMMS_DEBUG ("Couldn't add %s to the bin",
                  GST_ELEMENT_NAME (element));
      gst_object_unref (sinkpad);
      gst_element_set_state (element, GST_STATE_NULL);
      gst_object_unref (element);
      continue;
    }

    if ((gst_pad_link (pad, sinkpad)) != GST_PAD_LINK_OK) {
      UMMS_DEBUG ("Link failed on pad %s:%s",
                  GST_DEBUG_PAD_NAME (sinkpad));
      gst_element_set_state (element, GST_STATE_NULL);
      gst_object_unref (sinkpad);
      gst_bin_remove (GST_BIN (priv->pipeline), element);
      continue;
    }
    gst_object_unref (sinkpad);
    UMMS_DEBUG ("linked on pad %s:%s", GST_DEBUG_PAD_NAME (pad));

    if (!autoplug_dec_element (player, element)) {
      gst_bin_remove (GST_BIN (priv->pipeline), element);
      continue;
    }

    /* Bring the element to the state of the parent */
    if ((gst_element_set_state (element,
         GST_STATE_PAUSED)) == GST_STATE_CHANGE_FAILURE) {

      UMMS_DEBUG ("Couldn't set %s to PAUSED",
                  GST_ELEMENT_NAME (element));

      /* Remove this element and the downstream one we just added.*/
      gst_element_set_state (element, GST_STATE_NULL);
      gst_bin_remove (GST_BIN (priv->pipeline), element);
      continue;
    }

    /*successful*/
    done = TRUE;
    break;
  }

  ret = done;

out:
  if (caps)
    gst_caps_unref (caps);

  if (compatible_elements)
    gst_plugin_feature_list_free (compatible_elements);
  return ret;
}

/*
 * link the pad to multiqueue and return corresponding src pad.
 * unref the returned pad after usage.
 */
static GstPad *link_multiqueue (DvbPlayerControlBase *player, GstPad *pad)
{
  GstPad *srcpad = NULL;
  GstPad *sinkpad = NULL;
  GstIterator *it = NULL;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (player);

  if (!priv->mq) {
    UMMS_DEBUG ("No multiqueue");
    return NULL;
  }

  if (!(sinkpad = gst_element_get_request_pad (priv->mq, "sink%d"))) {
    UMMS_DEBUG ("Couldn't get sinkpad from multiqueue");
    return NULL;
  }

  if ((gst_pad_link (pad, sinkpad) != GST_PAD_LINK_OK)) {
    UMMS_DEBUG ("Couldn't link demuxer and multiqueue");
    goto error;
  }

  it = gst_pad_iterate_internal_links (sinkpad);

  if (!it || (gst_iterator_next (it, (gpointer) & srcpad)) != GST_ITERATOR_OK
       || srcpad == NULL) {
    UMMS_DEBUG ("Couldn't get corresponding srcpad from multiqueue for sinkpad %" GST_PTR_FORMAT,
                sinkpad);
    goto error;
  }

out:
  if (sinkpad)
    gst_object_unref (sinkpad);

  if (it)
    gst_iterator_free (it);

  return srcpad;

error:
  gst_element_release_request_pad (priv->mq, sinkpad);
  goto out;
}

static void autoplug_multiqueue (DvbPlayerControlBase *player,GstPad *srcpad,gchar *name)
{
  UMMS_DEBUG ("Empty one");
  return;
}

static void
pad_added_cb (GstElement *element,
              GstPad     *pad,
              gpointer    data)
{
  GstPad *sinkpad = NULL;
  GstPad *srcpad  = NULL;
  DvbPlayerControlBase *player = (DvbPlayerControlBase *) data;
  DvbPlayerControlBasePrivate *priv = GET_PRIVATE (player);
  gchar *name = gst_pad_get_name (pad);
  DvbPlayerControlBaseClass *kclass = DVB_PLAYER_CONTROL_BASE_GET_CLASS(player);

  UMMS_DEBUG ("flutsdemux pad: %s added", name);
  if (!g_strcmp0 (name, "rawts")) {
    g_print ("pad(%s) added\n", GST_PAD_NAME(pad));
  } else if (!g_strcmp0 (name, "program")) {
    g_print ("pad(%s) added\n", GST_PAD_NAME(pad));
  } else {
    UMMS_DEBUG ("autoplug elementary stream pad: %s", name);
    if (!priv->mq) {
      priv->mq = gst_element_factory_make ("multiqueue", NULL);
      if (!priv->mq) {
        UMMS_DEBUG ("Making multiqueue failed");
        goto out;
      }
      gst_bin_add (GST_BIN (priv->pipeline), priv->mq);
      gst_element_set_state (priv->mq, GST_STATE_PAUSED);
    }

    if (!(srcpad = link_multiqueue (player, pad))) {
      UMMS_DEBUG ("Linking multiqueue failed");
      goto out;
    }

    kclass->autoplug_multiqueue(player,srcpad,name);
  }

out:
  if (srcpad)
    gst_object_unref (srcpad);
  if (sinkpad)
    gst_object_unref (sinkpad);
  if (name)
    g_free (name);
}

static void dvb_player_control_base_class_init 
(DvbPlayerControlBaseClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (DvbPlayerControlBasePrivate));

  object_class->get_property = dvb_player_control_base_get_property;
  object_class->set_property = dvb_player_control_base_set_property;
  object_class->dispose = dvb_player_control_base_dispose;
  object_class->finalize = dvb_player_control_base_finalize;

  /*virtual APis*/
  klass->set_target = set_target;
  klass->request_resource = request_resource;
  klass->release_resource = release_resource;
  klass->setup_xwindow_target = setup_xwindow_target;

  klass->set_volume = set_volume;
  klass->get_volume = get_volume;
  klass->set_mute = set_mute;
  klass->is_mute = is_mute;
  klass->set_scale_mode = set_scale_mode;
  klass->get_scale_mode = get_scale_mode;
  klass->set_video_size = set_video_size;
  klass->get_video_size = get_video_size;
  klass->bus_group = bus_group;
  klass->link_sink = link_sink;
  klass->pad_added_cb = pad_added_cb;
  klass->autoplug_pad = autoplug_pad;
}

DvbPlayerControlBase *
dvb_player_control_base_new (void)
{
  return g_object_new (DVB_PLAYER_CONTROL_TYPE_BASE, NULL);
}
