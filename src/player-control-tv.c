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

#define TEARDOWN_ELEMENT(ele)                      \
    if (ele) {                                     \
      gst_element_set_state (ele, GST_STATE_NULL); \
      g_object_unref (ele);                        \
      ele = NULL;                                  \
    }

#define INVALID_PLANE_ID -1

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

static void uri_parser_bus_message_error_cb (GstBus *bus, GstMessage *message, PlayerControlBase  *self);
static gboolean setup_ismd_vbin(PlayerControlBase *self, gchar *rect, gint plane);

#define HW_FORMAT_NUM 4
#define HW_H264_CAPS \
             "video/h264, " \
           "  format = (fourcc) H264, " \
           "  width = (int) [16,1920], " \
           "  height = (int) [16,1088]; " \
           "video/x-h264, " \
           "  format = (fourcc) X264, " \
           "  width = (int) [16,1920], " \
           "  height = (int) [16,1088]; "


#define HW_MPEG2_CAPS \
             "video/mpeg, " \
           "  format = (fourcc) {MPEG, MP2V}, " \
           "  width = (int) [16,1920], " \
           "  height = (int) [16,1088], " \
           "  mpegversion = (int) [1, 2], " \
           "  systemstream = (boolean) false, " \
           "  ispacketized = (boolean) false; "

#define HW_MPEG4_CAPS \
             "video/mpeg, " \
           "  mpegversion = (int) 4, " \
           "  systemstream = (boolean) false; " \
           "video/x-xvid;" \
           "video/x-divx, " \
           "  format = (fourcc) {MPEG, MP4V}, " \
           "  width = (int) [16,1920], " \
           "  height = (int) [16,1088], " \
           "  divxversion = (int) [4, 5] "

#define HW_VC1_CAPS \
             "video/x-wmv, " \
           "  wmvversion = (int)3, " \
           "  format = (fourcc) { WMV3, WVC1 }"

static GstStaticCaps hw_h264_static_caps = GST_STATIC_CAPS (HW_H264_CAPS);
static GstStaticCaps hw_mpeg2_static_caps = GST_STATIC_CAPS (HW_MPEG2_CAPS);
static GstStaticCaps hw_mpeg4_static_caps = GST_STATIC_CAPS (HW_MPEG4_CAPS);
static GstStaticCaps hw_vc1_static_caps = GST_STATIC_CAPS (HW_VC1_CAPS);

static GstStaticCaps *hw_static_caps[HW_FORMAT_NUM] = {&hw_h264_static_caps,
    &hw_mpeg2_static_caps,
    &hw_mpeg4_static_caps,
    &hw_vc1_static_caps
                                                      };
   

static gboolean
autoplug_continue_cb (GstElement * element, GstPad * pad, GstCaps * caps,
    MediaPlayerControl *control)
{
  gint i;
  const gchar *name = NULL;
  GstStructure *s = NULL;
  GstCaps *hw_caps = NULL;
  gboolean is_hw_vcaps = FALSE;
  gboolean ret = TRUE;
  PlayerControlBasePrivate *priv = GET_PRIVATE (control);

  UMMS_DEBUG ( "pad caps : %" GST_PTR_FORMAT,
               caps);

  //check software a/v caps.
  s = gst_caps_get_structure (caps, 0);
  name = gst_structure_get_name (s);

  if (!strncmp (name, "video", 5)) {
    priv->has_video = TRUE;
    //check HW video caps
    for (i = 0; i < HW_FORMAT_NUM; i++) {
      hw_caps = gst_static_caps_get (hw_static_caps[i]);

      if (gst_caps_can_intersect (hw_caps, caps)) {
        is_hw_vcaps = TRUE;
        priv->hw_viddec++;

        /*
         * Don't continue to autoplug this pad,
         * which avoids the failure of creating and plugin HW video decoder, and hence guarantees "no-more-pads"
         * signal will be emitted.
         *
         */
        ret = FALSE;
      }

      gst_caps_unref (hw_caps);

      if (is_hw_vcaps)
        goto hw_video_caps;
    }
  } else if (!strncmp (name, "audio", 5)) {
    priv->has_audio = TRUE;
    ret = FALSE;//Exposes this pad, and hence guarantees "no-more-pads" signal will be emitted.
    //FIXME: check the HW audio caps.
  } else if (!strncmp (name, "text", 4)) {
    priv->has_sub = TRUE;
  } else {
    //nothing
  }

  return ret;

hw_video_caps:
  UMMS_DEBUG ("new stream with caps(%" GST_PTR_FORMAT ") needs HW video decoder resource", caps);
  return ret;
}

//Unref returned clock after usage
static GstClock *
get_hw_clock(void)
{
  GstClock* clock = NULL;
  GstElement *vsink = NULL;

  UMMS_DEBUG ("Begin");
  vsink = gst_element_factory_make ("ismd_vidrend_sink", NULL);
  if (!vsink)
    return NULL;

  clock = gst_element_provide_clock (vsink);

  if (clock)
    g_object_ref (clock);

  g_object_unref (vsink);
  UMMS_DEBUG ("End");

  return clock;
}


static gboolean
uri_parsing_finished_cb (MediaPlayerControl * self)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);
  GstState target;

  UMMS_DEBUG("Begin");

  TEARDOWN_ELEMENT (priv->uri_parse_pipe);
  priv->uri_parsed = TRUE;

  UMMS_DEBUG("End, has_video=%d, num of hw_viddec=%d, has_audio=%d, num of hw_auddec=%d",
             priv->has_video, priv->hw_viddec, priv->has_audio, priv->hw_auddec);

  if (priv->pending_state >= PlayerStatePaused) {
    target = (priv->pending_state == PlayerStatePaused) ? (GST_STATE_PAUSED) : (GST_STATE_PLAYING);
    PlayerControlBaseClass *kclass = PLAYER_CONTROL_BASE_GET_CLASS(self);
    kclass->activate_player_control ((PlayerControlBase*)self, target);
  }

  return FALSE;
}

static void no_more_pads_cb (GstElement * uridecodebin, MediaPlayerControl * self)
{
  UMMS_DEBUG("Begin");
  g_idle_add ((GSourceFunc)uri_parsing_finished_cb, self);
  UMMS_DEBUG("End");
}


static void
_uri_parser_source_changed_cb (GObject *object, GParamSpec *pspec, gpointer data)
{
  GstElement *source;
  PlayerControlBasePrivate *priv = GET_PRIVATE (data);
  PlayerControlBaseClass *kclass = PLAYER_CONTROL_BASE_GET_CLASS(data);

  g_object_get(priv->uridecodebin, "source", &source, NULL);
  gst_object_replace((GstObject**) &priv->source, (GstObject*) source);
  UMMS_DEBUG ("source changed");
  kclass->set_proxy (PLAYER_CONTROL_BASE(data));

  return;
}

static void
uri_parser_bus_message_error_cb (GstBus     *bus,
    GstMessage *message,
    PlayerControlBase  *self)
{
  GError *error = NULL;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);

  UMMS_DEBUG ("message::URI parsing error received on bus");

  gst_message_parse_error (message, &error, NULL);

  TEARDOWN_ELEMENT (priv->uri_parse_pipe);
  media_player_control_emit_error (self, UMMS_ENGINE_ERROR_FAILED, error->message);

  UMMS_DEBUG ("URI Parsing error emitted with message = %s", error->message);

  g_clear_error (&error);
}

/*
 * Parse the av asset spacified by the uri.
 *
 * Called when set state from PlayerStateNull/PlayerStateStopped to PlayerStatePaused/PlayerStatePlaying.
 * Through this parsing, we can get the number of streams and their format, and can determine the
 * resources(HW clock, HW a/v decoder) needed when playing this av asset.
 *
 * Return TRUE if successfully initializeithe parsing action.
 *        FALSE if not.
 *
 * Note: This is a asynchronous invoking, so, requesting resources and setting pipeline to Pause/Playing will be
 * done in uri_parsing_finished_cb().
 */
static gboolean
parse_uri_async (MediaPlayerControl *self, gchar *uri)
{
  GstElement *uridecodebin = NULL;
  GstElement *uri_parse_pipe = NULL;
  GstBus *bus = NULL;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (uri, FALSE);

  if (priv->uri_parsed)
    return TRUE;
  if (priv->uri_parse_pipe)
    return TRUE;

  //use uridecodebin to automatically detect streams.
  uridecodebin = gst_element_factory_make ("uridecodebin", NULL);
  if (!uridecodebin) {
    UMMS_DEBUG ("Creating uridecodebin failed");
    return FALSE;
  }
  
  uri_parse_pipe = gst_pipeline_new ("uri-parse-pipe");
  if (!uri_parse_pipe) {
    UMMS_DEBUG ("Creating pipeline failed");
    gst_object_unref (uridecodebin);
    return FALSE;
  }

  priv->uri_parse_pipe = uri_parse_pipe;
  priv->uridecodebin = uridecodebin;
  g_signal_connect(uridecodebin, "notify::source", G_CALLBACK(_uri_parser_source_changed_cb), self);

  gst_bin_add (GST_BIN (uri_parse_pipe), uridecodebin);

  bus = gst_pipeline_get_bus (GST_PIPELINE (uri_parse_pipe));
  gst_bus_add_signal_watch (bus);
  g_signal_connect_object (bus, "message::error",
      G_CALLBACK (uri_parser_bus_message_error_cb),
      self,
      0);
  gst_object_unref (bus);

  g_object_set (G_OBJECT(uridecodebin), "uri", priv->uri, NULL);

  g_signal_connect (uridecodebin, "autoplug-continue",
                    G_CALLBACK (autoplug_continue_cb), self);

  g_signal_connect (uridecodebin, "no-more-pads",
                    G_CALLBACK (no_more_pads_cb), self);

  if (priv->is_live) {
    if (gst_element_set_state (uri_parse_pipe, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
      UMMS_DEBUG ("setting uri parse pipeline to  playing failed");
    }
  } else {
    if (gst_element_set_state (uri_parse_pipe, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
      UMMS_DEBUG ("setting uri parse pipeline to paused failed");
    }
  }

  return TRUE;
}

//both Xwindow and ReservedType0 target use ismd_vidrend_sink, so that need clock.
#define NEED_CLOCK(target_type) \
  ((target_type == XWindow || target_type == ReservedType0)?TRUE:FALSE)

#define REQUEST_RES(self, t, p, e_msg)                                        \
  do{                                                                         \
    PlayerControlBasePrivate *priv = GET_PRIVATE (self);                      \
    PlayerControlBaseClass *kclass = PLAYER_CONTROL_BASE_GET_CLASS (self);    \
    ResourceRequest req = {0,};                                               \
    Resource *res = NULL;                                                     \
    req.type = t;                                                             \
    req.preference = p;                                                       \
    res = umms_resource_manager_request_resource (priv->res_mngr, &req);      \
    if (!res) {                                                               \
      kclass->release_resource(self);                                         \
      media_player_control_emit_error (self,                                  \
                                             UMMS_RESOURCE_ERROR_NO_RESOURCE, \
                                             e_msg);                          \
      return FALSE;                                                           \
    }                                                                         \
    priv->res_list = g_list_append (priv->res_list, res);                     \
    }while(0)



static gboolean
request_resource (PlayerControlBase *self)
{
  gint i;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (priv->uri_parsed, FALSE);//uri should already be parsed.

  UMMS_DEBUG("virtual APIs derived Impl");
  if (priv->resource_prepared)
    return TRUE;

  /*
   * For live source, we create and set clock to pipeline manually,
   * ismd audio/video sink will not create and provide a clock.
   * So, just need one clock.
   */
  if (priv->is_live) {
    REQUEST_RES(self, ResourceTypeHwClock, INVALID_RES_HANDLE, "No HW clock resource");
  } else {
    if (priv->has_video && NEED_CLOCK(priv->target_type)) {
      REQUEST_RES(self, ResourceTypeHwClock, INVALID_RES_HANDLE, "No HW clock resource");
    }
    if (priv->has_audio) {
      REQUEST_RES(self, ResourceTypeHwClock, INVALID_RES_HANDLE, "No HW clock resource");
    }
  }

  for (i = 0; i < priv->hw_viddec; i++) {
    REQUEST_RES(self, ResourceTypeHwViddec, INVALID_RES_HANDLE, "No HW video decoder resource");
  }

  /*
  for (i=0; i<priv->hw_auddec; i++) {
    REQUEST_RES(self, ResourceTypeHwAuddec, INVALID_RES_HANDLE, "No HW video decoder resource");
  }
  */
  priv->resource_prepared = TRUE;
  return TRUE;
}

static void
release_resource (PlayerControlBase *self)
{
  GList *g;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);
  UMMS_DEBUG("virtual APIs derived Impl");

  for (g = priv->res_list; g; g = g->next) {
    Resource *res = (Resource *) (g->data);
    umms_resource_manager_release_resource (priv->res_mngr, res);
  }
  g_list_free (priv->res_list);
  priv->res_list = NULL;
  priv->resource_prepared = FALSE;

  return;
}

static gboolean
set_subtitle_uri (PlayerControlBase *self, gchar *sub_uri)
{
  PlayerControlBasePrivate *priv = NULL;
  GstElement *pipe = NULL;
  GstElement *sub_sink = NULL;

  UMMS_DEBUG("virtual APIs derived Impl");

  g_return_val_if_fail (self != NULL, FALSE);
  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  /* We first set the subtitle render element of playbin2 using ismd_subrend_bin.
     If failed, we just use the default subrender logic in playbin2. */
  sub_sink = gst_element_factory_make ("ismd_subrend_bin", NULL);
  if (sub_sink) {
    UMMS_DEBUG ("succeed to make the ismd_subrend_bin, set it to playbin2");
    g_object_set (priv->pipeline, "text-sink", sub_sink, NULL);
  } else {
    UMMS_DEBUG ("Unable to make the ismd_subrend_bin");
  }

  /* It seems that the subtitle URI need to set before activate the group, and
     can not dynamic change it of current group when playing.
     So calling this API when stream is playing may have no effect. */
  g_object_set (G_OBJECT (pipe), "suburi", sub_uri, NULL);

  return TRUE;
}



static gboolean
activate_player_control (PlayerControlBase *self, GstState target_state)
{
  gboolean ret;
  PlayerState old_pending;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);
  MediaPlayerControl *self_iface = (MediaPlayerControl *)self;
  PlayerControlBaseClass *kclass = PLAYER_CONTROL_BASE_GET_CLASS(self);

  UMMS_DEBUG("virtual APIs derived Impl");

  g_return_val_if_fail (priv->uri, FALSE);
  g_return_val_if_fail (target_state == GST_STATE_PAUSED || target_state == GST_STATE_PLAYING, FALSE);

  old_pending = priv->pending_state;
  priv->pending_state = ((target_state == GST_STATE_PAUSED) ? PlayerStatePaused : PlayerStatePlaying);

  if (!(ret = parse_uri_async(self_iface, priv->uri))) {
    goto OUT;
  }

  if (!priv->uri_parsed) {
    goto uri_not_parsed;
  }

  if ((ret = kclass->request_resource(PLAYER_CONTROL_BASE(self)))) {
    if (target_state == GST_STATE_PLAYING) {
      if (IS_LIVE_URI(priv->uri)) {
        /* For the special case of live source.
          Becasue our hardware decoder and sink need the special clock type and if the clock type is wrong,
          the hardware can not work well.  In file case, the provide_clock will be called after all the elements
          have been made and added in pause state, so the element which represent the hardware will provide the
          right clock. But in live source case, the state change from NULL to playing is continous and the provide_clock
          function may be called before hardware element has been made.
          So we need to set the clock of pipeline statically here.*/
        GstClock *clock = get_hw_clock ();
        if (clock) {
          gst_pipeline_use_clock (GST_PIPELINE_CAST(priv->pipeline), clock);
          g_object_unref (clock);
        } else {
          UMMS_DEBUG ("Can't get HW clock");
          media_player_control_emit_error (self_iface, UMMS_ENGINE_ERROR_FAILED, "Can't get HW clock for live source");
          ret = FALSE;
          goto OUT;
        }
      }
    }

    if (gst_element_set_state(priv->pipeline, target_state) == GST_STATE_CHANGE_FAILURE) {
      UMMS_DEBUG ("set pipeline to %d failed", target_state);
      ret = FALSE;
      goto OUT;
    }
  }


OUT:
  if (!ret) {
    priv->pending_state = old_pending;
  }
  return ret;

uri_not_parsed:
  UMMS_DEBUG ("uri parsing not finished");
  return FALSE;

}


static gboolean setup_gdl_plane_target (MediaPlayerControl *self, GHashTable *params)
{
  gchar *rect = NULL;
  gint  plane = INVALID_PLANE_ID;
  GValue *val = NULL;
  PlayerControlBaseClass *kclass = PLAYER_CONTROL_BASE_GET_CLASS(self);
  PlayerControlBase *pbase = PLAYER_CONTROL_BASE(self);

  UMMS_DEBUG ("setting up gdl plane target");
  val = g_hash_table_lookup (params, TARGET_PARAM_KEY_RECTANGLE);
  if (val) {
    rect = (gchar *)g_value_get_string (val);
    UMMS_DEBUG ("rectangle = '%s'", rect);
  }

  val = g_hash_table_lookup (params, TARGET_PARAM_KEY_PlANE_ID);
  if (val) {
    plane = g_value_get_int (val);
    UMMS_DEBUG ("gdl plane = '%d'", plane);
  }

  return setup_ismd_vbin (pbase, rect, plane);
  //return kclass->setup_ismd_vbin (pbase, rect, plane);
}

/*
 * To get the top level window according to subwindow.
 * Some prerequisites should be satisfied:
 * 1. The X server uses 29 bits(i.e. bit1--bit29) to represents a window id.
 * 2. The X server uses bit22--bit29 to represents a connection/client, it means X server supports 256 connections/clients at maximum.
 * 3. The top-level window and subwindow is requested from the same connection.
 * 1 and 2 are the case of xorg server.
 * 3 is always satisfied, unless subwindow is requested from a process which is not the same process hosting the top-level window.
*/
static Window get_top_level_win (MediaPlayerControl *self, Window sub_win)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);
  Display *disp = priv->disp;
  Window root_win, parent_win, cur_win;
  unsigned int num_children;
  Window *child_list = NULL;
  Window top_win = 0;
  gboolean done = FALSE;

  if (!disp)
    return 0;

  printf ("============%s:Begin, sub_win=%lx ==========\n", __FUNCTION__, sub_win);
#define CLIENT_MASK (0x1fe00000)
#define FROM_THE_SAME_PROC(w1,w2) ((w1&CLIENT_MASK) == (w2&CLIENT_MASK))

  cur_win = sub_win;
  while (!done) {
    if (!XQueryTree(disp, cur_win, &root_win, &parent_win, &child_list, &num_children)) {
      UMMS_DEBUG ("Can't query window tree.");
      return 0;
    }

    UMMS_DEBUG ("cur_win(%lx), parent_win(%lx)", cur_win, parent_win);
    if (child_list) XFree((char *)child_list);

    if (!FROM_THE_SAME_PROC(cur_win, parent_win)) {
      UMMS_DEBUG ("Got the top-level window(%lx)", cur_win);
      top_win = cur_win;
      done = TRUE;
      break;
    };
    cur_win = parent_win;
  }
  printf ("============%s:End==========\n", __FUNCTION__);
  return top_win;
}

static gboolean setup_datacopy_target (MediaPlayerControl *self, GHashTable *params)
{
  gchar *rect = NULL;
  GValue *val = NULL;
  GstElement *shmvbin = NULL;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);


  UMMS_DEBUG ("setting up datacopy target");
  shmvbin = gst_element_factory_make ("shmvidrendbin", NULL);
  if (!shmvbin) {
    UMMS_DEBUG ("Making \"shmvidrendbin\" failed");
    return FALSE;
  }

  val = g_hash_table_lookup (params, TARGET_PARAM_KEY_RECTANGLE);
  if (val) {
    rect = (gchar *)g_value_get_string (val);
    UMMS_DEBUG ("setting rectangle = '%s'", rect);
    g_object_set (shmvbin, "rectangle", rect, NULL);
  }

  g_object_set (priv->pipeline, "video-sink", shmvbin, NULL);
  UMMS_DEBUG ("Set \"shmvidrendbin\" to playbin2");
  return TRUE;
}

static int x_print_error(
  Display *dpy,
  XErrorEvent *event,
  FILE *fp)
{
  char buffer[BUFSIZ];
  char mesg[BUFSIZ];
  char number[32];
  const char *mtype = "XlibMessage";
  XGetErrorText(dpy, event->error_code, buffer, BUFSIZ);
  XGetErrorDatabaseText(dpy, mtype, "XError", "X Error", mesg, BUFSIZ);
  (void) fprintf(fp, "%s:  %s\n  ", mesg, buffer);
  XGetErrorDatabaseText(dpy, mtype, "MajorCode", "Request Major code %d",
      mesg, BUFSIZ);
  (void) fprintf(fp, mesg, event->request_code);
  if (event->request_code < 128) {
    sprintf(number, "%d", event->request_code);
    XGetErrorDatabaseText(dpy, "XRequest", number, "", buffer, BUFSIZ);
  }

  (void) fprintf(fp, " (%s)\n", buffer);
  if (event->request_code >= 128) {
    XGetErrorDatabaseText(dpy, mtype, "MinorCode", "Request Minor code %d",
        mesg, BUFSIZ);
    fputs("  ", fp);
    (void) fprintf(fp, mesg, event->minor_code);

    fputs("\n", fp);
  }
  if (event->error_code >= 128) {
    strcpy(buffer, "Value");
    XGetErrorDatabaseText(dpy, mtype, buffer, "", mesg, BUFSIZ);
    if (mesg[0]) {
      fputs("  ", fp);
      (void) fprintf(fp, mesg, event->resourceid);
      fputs("\n", fp);
    }

  } else if ((event->error_code == BadWindow) ||
             (event->error_code == BadPixmap) ||
             (event->error_code == BadCursor) ||
             (event->error_code == BadFont) ||
             (event->error_code == BadDrawable) ||
             (event->error_code == BadColor) ||
             (event->error_code == BadGC) ||
             (event->error_code == BadIDChoice) ||
             (event->error_code == BadValue) ||
             (event->error_code == BadAtom)) {
    if (event->error_code == BadValue)
      XGetErrorDatabaseText(dpy, mtype, "Value", "Value 0x%x",
          mesg, BUFSIZ);
    else if (event->error_code == BadAtom)
      XGetErrorDatabaseText(dpy, mtype, "AtomID", "AtomID 0x%x",
          mesg, BUFSIZ);
    else
      XGetErrorDatabaseText(dpy, mtype, "ResourceID", "ResourceID 0x%x",
          mesg, BUFSIZ);
    fputs("  ", fp);
    (void) fprintf(fp, mesg, event->resourceid);
    fputs("\n", fp);
  }
  XGetErrorDatabaseText(dpy, mtype, "ErrorSerial", "Error Serial #%d",
      mesg, BUFSIZ);
  fputs("  ", fp);
  (void) fprintf(fp, mesg, event->serial);
  XGetErrorDatabaseText(dpy, mtype, "CurrentSerial", "Current Serial #%d",
      mesg, BUFSIZ);
  fputs("\n  ", fp);
  if (event->error_code == BadImplementation) return 0;
  return 1;
}

//Mostly the same as default handler, but not exit process.
static int xerror_handler (
  Display *dpy,
  XErrorEvent *event)
{
  return x_print_error (dpy, event, stderr);
}


static gboolean
get_video_rectangle (MediaPlayerControl *self, gint *ax, gint *ay, gint *w, gint *h, gint *rx, gint *ry)
{
  XWindowAttributes video_win_attr, app_win_attr;
  gint app_x, app_y;
  Window junkwin;
  Status status;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);

  status = XGetWindowAttributes (priv->disp, priv->app_win_id, &app_win_attr);
  if (!status) {
    UMMS_DEBUG ("Get top-level window attributes failed");
    return FALSE;
  }
  status = XGetWindowAttributes (priv->disp, priv->video_win_id, &video_win_attr);
  if (!status) {
    UMMS_DEBUG ("Get top-level window attributes failed");
    return FALSE;
  }

  (void) XTranslateCoordinates (priv->disp, priv->app_win_id, app_win_attr.root,
      -app_win_attr.border_width,
      -app_win_attr.border_width,
      &app_x, &app_y, &junkwin);
  UMMS_DEBUG ("app window app_absolute_x = %d, app_absolute_y = %d", app_x, app_y);

  (void) XTranslateCoordinates (priv->disp, priv->video_win_id, video_win_attr.root,
      -video_win_attr.border_width,
      -video_win_attr.border_width,
      ax, ay, &junkwin);

  UMMS_DEBUG ("video window video_absolute_x = %d, video_absolute_y = %d", *ax, *ay);

  *rx = *ax - app_x;
  *ry = *ay - app_y;
  *w = video_win_attr.width;
  *h = video_win_attr.height;
  UMMS_DEBUG ("video window video_relative_x = %d, video_relative_y = %d", *rx, *ry);
  UMMS_DEBUG ("video w=%d,y=%d", *w, *h);
  return TRUE;
}

//FIXME: very ugly
#define IS_VIDREND_BIN(ele) \
   ((g_object_class_find_property (G_OBJECT_GET_CLASS (ele), "scale-mode") != NULL) && (g_object_class_find_property (G_OBJECT_GET_CLASS (ele), "gdl-plane") != NULL))

static gboolean setup_ismd_vbin(PlayerControlBase *self, gchar *rect, gint plane)
{
  GstElement *cur_vsink = NULL;
  GstElement *new_vsink = NULL;
  GstElement *vsink = NULL;
  gboolean   ret = TRUE;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);
  UMMS_DEBUG("virtual APIs derived Impl");

  g_object_get (priv->pipeline, "video-sink", &cur_vsink, NULL);
  if (cur_vsink)
    UMMS_DEBUG ("playbin2 aready has video-sink: %p, name: %s", cur_vsink, GST_ELEMENT_NAME(cur_vsink));

  if (!cur_vsink || !IS_VIDREND_BIN(cur_vsink)) {
    new_vsink = gst_element_factory_make ("ismd_vidrend_bin", NULL);
    if (!new_vsink) {
      UMMS_DEBUG ("Failed to make ismd_vidrend_bin");
      ret = FALSE;
      goto OUT;
    }
    UMMS_DEBUG ("new ismd_vidrend_bin: %p, name: %s", new_vsink, GST_ELEMENT_NAME(new_vsink));
  }

  vsink = new_vsink ? new_vsink : cur_vsink;

  if (rect)
    g_object_set (vsink, "rectangle", rect, NULL);

  if (plane != INVALID_PLANE_ID)
    g_object_set (vsink, "gdl-plane", plane, NULL);

  if (new_vsink) {
    g_object_set (new_vsink, "qos", FALSE, NULL);
    g_object_set (priv->pipeline, "video-sink", vsink, NULL);
    UMMS_DEBUG ("Set ismd_vidrend_bin to playbin2");
  }

OUT:
  if (cur_vsink) {
    g_object_unref (cur_vsink);
  }
  return ret;
}

static gboolean
cutout (MediaPlayerControl *self, gint x, gint y, gint w, gint h)
{
  Atom property;
  gchar data[256];
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);

  property = XInternAtom (priv->disp, "_MUTTER_HINTS", 0);
  if (!property) {
    UMMS_DEBUG ("XInternAtom failed");
    return FALSE;
  }

  g_sprintf (data, "meego-tv-cutout-x=%d:meego-tv-cutout-y=%d:meego-tv-cutout-width=%d:"
             "meego-tv-cutout-height=%d:meego-tv-half-trans=0:meego-tv-full-window=0", x, y, w, h);

  UMMS_DEBUG ("Hints to mtv-mutter = \"%s\"", data);

  XChangeProperty(priv->disp, priv->app_win_id, property, XA_STRING, 8, PropModeReplace,
                  (unsigned char *)data, strlen(data));
  return TRUE;
}

static void
handle_xevents (MediaPlayerControl *control)
{

  XEvent e;
  gint x, y, w, h, rx, ry;
  PlayerControlBasePrivate *priv = GET_PRIVATE (control);
  PlayerControlBaseClass *kclass = PLAYER_CONTROL_BASE_GET_CLASS(control);

  g_return_if_fail (control);

  /* Handle Expose */
  while (XCheckWindowEvent (priv->disp,
         priv->app_win_id, StructureNotifyMask, &e)) {
    switch (e.type) {
      case ConfigureNotify:
        get_video_rectangle (control, &x, &y, &w, &h, &rx, &ry);
        cutout (control, rx, ry, w, h);
        media_player_control_set_video_size (control, x, y, w, h);
        UMMS_DEBUG ("Got ConfigureNotify, video window abs_x=%d,abs_y=%d,w=%d,y=%d", x, y, w, h);
        break;
      default:
        break;
    }
  }

  return;
}

static gpointer
event_thread (MediaPlayerControl* control)
{

  PlayerControlBasePrivate *priv = GET_PRIVATE (control);

  UMMS_DEBUG ("Begin");
  while (priv->event_thread_running) {

    if (priv->app_win_id) {
      handle_xevents (control);
    }
    /* FIXME: do we want to align this with the framerate or anything else? */
    g_usleep (G_USEC_PER_SEC / 20);

  }

  UMMS_DEBUG ("End");
  return NULL;
}

static gboolean
create_xevent_handle_thread (MediaPlayerControl *self)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);

  UMMS_DEBUG ("Begin");
  if (!priv->event_thread) {
    /* Setup our event listening thread */
    UMMS_DEBUG ("run xevent thread");
    priv->event_thread_running = TRUE;
    priv->event_thread = g_thread_create (
          (GThreadFunc) event_thread, self, TRUE, NULL);
  }
  return TRUE;
}

/*
 * 1. Calculate top-level window according to video window.
 * 2. Cutout video window geometry according to its relative position to top-level window.
 * 3. Setup underlying ismd_vidrend_bin element.
 * 4. Create xevent handle thread.
 */

static gboolean setup_xwindow_target (PlayerControlBase *self, GHashTable *params)
{
  GValue *val;
  gint x, y, w, h, rx, ry;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);
  PlayerControlBaseClass *kclass = PLAYER_CONTROL_BASE_GET_CLASS(self);
  MediaPlayerControl *control = MEDIA_PLAYER_CONTROL(self);

  UMMS_DEBUG("virtual APIs derived Impl");
  if (priv->xwin_initialized) {
    kclass->unset_xwindow_target (self);
  }

  if (!priv->disp)
    priv->disp = XOpenDisplay (NULL);

  if (!priv->disp) {
    UMMS_DEBUG ("Could not open display");
    return FALSE;
  }

  XSetErrorHandler (xerror_handler);

  val = g_hash_table_lookup (params, "window-id");
  if (!val) {
    UMMS_DEBUG ("no window-id");
    return FALSE;
  }
  priv->video_win_id = (Window)g_value_get_int (val);
  priv->app_win_id = get_top_level_win (control, priv->video_win_id);

  if (!priv->app_win_id) {
    UMMS_DEBUG ("Get top-level window failed");
    return FALSE;
  }

  get_video_rectangle (control, &x, &y, &w, &h, &rx, &ry);
  cutout (control, rx, ry, w, h);

  //make ismd video sink and set its rectangle property
  gchar *rectangle_des = NULL;
  rectangle_des = g_strdup_printf ("%u,%u,%u,%u", x, y, w, h);

  UMMS_DEBUG ("set rectangle damension :'%s'", rectangle_des);
  setup_ismd_vbin (self, rectangle_des, INVALID_PLANE_ID);
  //kclass->setup_ismd_vbin (self, rectangle_des, INVALID_PLANE_ID);
  g_free (rectangle_des);

  //Monitor top-level window's structure change event.
  XSelectInput (priv->disp, priv->app_win_id,
                StructureNotifyMask);
  create_xevent_handle_thread (control);

  priv->target_type = XWindow;
  priv->xwin_initialized = TRUE;

  return TRUE;
}

static gboolean
destroy_xevent_handle_thread (MediaPlayerControl *self)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);

  UMMS_DEBUG ("Begin");
  if (priv->event_thread) {
    priv->event_thread_running = FALSE;
    g_thread_join (priv->event_thread);
    priv->event_thread = NULL;
  }
  UMMS_DEBUG ("End");
  return TRUE;
}



static gboolean
unset_xwindow_target (PlayerControlBase *self)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);
  UMMS_DEBUG("virtual APIs derived Impl");

  if (!priv->xwin_initialized)
    return TRUE;

  destroy_xevent_handle_thread (MEDIA_PLAYER_CONTROL(self));
  priv->xwin_initialized = FALSE;
  return TRUE;
}

static gboolean
unset_target (MediaPlayerControl *self)
{
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);
  PlayerControlBaseClass *kclass = PLAYER_CONTROL_BASE_GET_CLASS(self);

  if (!priv->target_initialized)
    return TRUE;

  /*
   *For DataCopy and ReservedType0, gstreamer will handle the video-sink element elegantly.
   *Nothing need to do here.
   */
  switch (priv->target_type) {
    case XWindow:
      kclass->unset_xwindow_target (PLAYER_CONTROL_BASE(self));
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
set_target (PlayerControlBase *self, gint type, GHashTable *params)
{
  gboolean ret = TRUE;
  PlayerControlBasePrivate *priv = GET_PRIVATE (self);
  MediaPlayerControl *self_iface = (MediaPlayerControl *)self;
  PlayerControlBaseClass *kclass = PLAYER_CONTROL_BASE_GET_CLASS(self);
  UMMS_DEBUG("virtual APIs derived Impl");

  /*
   * Set target can only happen at Null or Stopped. Two reasons:
   * 1. Gstreamer don't support switching sink on the fly.
   * 2. PlayerStateNull/PlayerStateStopped means all target related resources have been released.
   *    It is more convenience for resource management implementing.
   */
  if (priv->player_state != PlayerStateNull && priv->player_state != PlayerStateStopped) {
    UMMS_DEBUG ("Ignored, can only set target at PlayerStateNull or PlayerStateStopped");
    return FALSE;
  }

  if (!priv->pipeline) {
    UMMS_DEBUG ("PlayerControl not loaded, reason may be SetUri not invoked");
    return FALSE;
  }

  if (priv->target_initialized) {
    unset_target (self_iface);
  }

  switch (type) {
    case XWindow:
      ret = kclass->setup_xwindow_target (self, params);
      break;
    case DataCopy:
      setup_datacopy_target(self_iface, params);
      break;
    case Socket:
      UMMS_DEBUG ("unsupported target type: Socket");
      ret = FALSE;
      break;
    case ReservedType0:
      /*
       * ReservedType0 represents the gdl-plane video output in our UMMS implementation.
       */
      ret = setup_gdl_plane_target (self_iface, params);
      break;
    default:
      break;
  }

  if (ret) {
    priv->target_type = type;
    priv->target_initialized = TRUE;
  }

  return ret;

}






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
  parent_class->activate_player_control = activate_player_control;
  parent_class->release_resource = release_resource;
  parent_class->request_resource = request_resource;
  parent_class->set_target = set_target;
  parent_class->unset_xwindow_target = unset_xwindow_target;
  parent_class->setup_xwindow_target = setup_xwindow_target;
  //parent_class->setup_ismd_vbin = setup_ismd_vbin;
  parent_class->set_subtitle_uri = set_subtitle_uri;

}

static void
player_control_tv_init (PlayerControlTv *self)
{
  PlayerControlBasePrivate *priv;
  GstBus *bus;

  UMMS_DEBUG("Called");
  priv = GET_PRIVATE (self);

  /*Setup default target*/
#define FULL_SCREEN_RECT "0,0,0,0"
  if (setup_ismd_vbin (self, FULL_SCREEN_RECT, UPP_A)) {
    priv->target_type = ReservedType0;
    priv->target_initialized = TRUE;
  }
}

PlayerControlTv *
player_control_tv_new (void)
{
  UMMS_DEBUG("Called");
  return g_object_new (PLAYER_CONTROL_TYPE_TV, NULL);
}

