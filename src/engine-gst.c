#include <string.h>
#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
/* for the volume property */
#include <gst/interfaces/streamvolume.h>
#include "umms-common.h"
#include "umms-debug.h"
#include "umms-error.h"
#include "engine-gst.h"
#include "meego-media-player-control.h"
#include "param-table.h"


static void meego_media_player_control_init (MeegoMediaPlayerControl* iface);

G_DEFINE_TYPE_WITH_CODE (EngineGst, engine_gst, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (MEEGO_TYPE_MEDIA_PLAYER_CONTROL, meego_media_player_control_init))

#define PLAYER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ENGINE_TYPE_GST, EngineGstPrivate))

#define GET_PRIVATE(o) ((EngineGst *)o)->priv

static const gchar *gst_state[] = {
  "GST_STATE_VOID_PENDING",
  "GST_STATE_NULL",
  "GST_STATE_READY",
  "GST_STATE_PAUSED",
  "GST_STATE_PLAYING"
};


/* list of URIs that we consider to be live source. */
static gchar *live_src_uri[] = { "http://", "mms://", "mmsh://", "rtsp://",
    "mmsu://", "mmst://", "fd://", "myth://", "ssh://", "ftp://", "sftp://",
    NULL};

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

struct _EngineGstPrivate {
  GstElement *pipeline;

  gchar *uri;
  gint  seekable;

  GstElement *source;
  GstElement *vsink;

  //buffering stuff
  gboolean buffering;
  gint buffer_percent;

  //UMMS defined player state
  PlayerState player_state;//current state
  PlayerState target;//target state, for async state change(*==>PlayerStatePaused, PlayerStateNull/Stopped==>PlayerStatePlaying)

  gboolean is_live;
  gint64 duration;//ms
  gint64 total_bytes;
};

static void _reset_engine (MeegoMediaPlayerControl *self);
static gboolean _query_buffering_percent (GstElement *pipe, gdouble *percent);
static void _source_changed_cb (GObject *object, GParamSpec *pspec, gpointer data);
static gboolean _stop_pipe (MeegoMediaPlayerControl *control);

static gboolean
engine_gst_set_uri (MeegoMediaPlayerControl *self,
                    const gchar           *uri)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);
  GstState state, pending;
  GstStateChangeReturn ret;

  g_return_val_if_fail (uri, FALSE);

  UMMS_DEBUG ("SetUri called: old = %s new = %s",
              priv->uri,
              uri);

  _reset_engine (self);

  priv->uri = g_strdup (uri);

  g_object_set (priv->pipeline, "uri", uri, NULL);

  //preroll the pipe
  if(gst_element_set_state (priv->pipeline, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
    UMMS_DEBUG ("%s:set pipeline to paused failed", __FUNCTION__);
    return FALSE;
  }

  ret = gst_element_get_state (priv->pipeline, &state, &pending, -1);
  if (ret == GST_STATE_CHANGE_NO_PREROLL && state ==  GST_STATE_PAUSED) {
    priv->is_live = TRUE;
  } else {
    priv->is_live = FALSE;
  }
  UMMS_DEBUG ("media :'%s is %slive source'", __FUNCTION__, priv->is_live ? "" : "non-");

  return TRUE;
}

static gboolean
engine_gst_set_target (MeegoMediaPlayerControl *self, gint type, GHashTable *params)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);
  GstElement *vsink;
  GstState state, pending;
  GstStateChangeReturn ret;
  GValue *val = NULL;

  switch (type) {
    case XWINDOW:
      break;
    case DataCopy:
      break;
    case Socket:
      UMMS_DEBUG ("unsupported target type: Socket");
      ret = FALSE;
      break;
    case ReservedType0:
      /*
       * Use ReservedType0 to represent the gdl-plane video output.
       * Choose ismd_vidrend_bin as video render.
       */
      vsink = gst_element_factory_make ("ismd_vidrend_bin", NULL);
      if (!vsink) {
        UMMS_DEBUG ("Failed to make ismd_vidrend_bin");
        return FALSE;
      }

      val = g_hash_table_lookup (params, TARGET_PARAM_KEY_RECTANGLE);
      if (val) {
        g_object_set (vsink, "rectangle", g_value_get_string (val), NULL);
        UMMS_DEBUG ("rectangle = '%s'",  g_value_get_string (val));
      }

      val = g_hash_table_lookup (params, TARGET_PARAM_KEY_PlANE_ID);
      if (val) {
        g_object_set (vsink, "gdl-plane", g_value_get_int (val), NULL);
        UMMS_DEBUG ("gdl plane = '%d'",  g_value_get_int (val));
      }

      UMMS_DEBUG ("set ismd_vidrend_bin to playbin2");
      g_object_set (priv->pipeline, "video-sink", vsink, NULL);
      break;
    default:
      break;
  }

  return TRUE;
}


static gboolean
engine_gst_play (MeegoMediaPlayerControl *self)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (!priv->uri) {
    UMMS_DEBUG(" Unable to set playing state - no URI set");
    return FALSE;
  }

  if(IS_LIVE_URI(priv->uri)) {     
//    ISmdGstClock *ismd_clock = NULL;
    /* For the special case of live source.
       Becasue our hardware decoder and sink need the special clock type and if the clock type is wrong,
       the hardware can not work well.  In file case, the provide_clock will be called after all the elements
       have been made and added in pause state, so the element which represent the hardware will provide the
       right clock. But in live source case, the state change from NULL to playing is continous and the provide_clock
       function may be called before hardware element has been made. So we need to set the clock of pipeline statically here.*/
//    ismd_clock = g_object_new (ISMD_GST_TYPE_CLOCK, NULL); 
//    gst_pipeline_use_clock (GST_PIPELINE_CAST(priv->pipeline), GST_CLOCK_CAST(ismd_clock));
  }

  priv->target = PlayerStateNull; /* Set to NULL here, wait for bus's callback to report the right state. */
  gst_element_set_state (priv->pipeline, GST_STATE_PLAYING);

  return TRUE;
}

static gboolean
engine_gst_pause (MeegoMediaPlayerControl *self)
{
  GstStateChangeReturn ret;
  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (gst_element_set_state(priv->pipeline, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
    UMMS_DEBUG ("set pipeline to paused failed");
    return FALSE;
  }

  priv->target = PlayerStatePaused;
  UMMS_DEBUG ("%s: called", __FUNCTION__);
  return TRUE;
}

static void
_reset_engine (MeegoMediaPlayerControl *self)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (priv->uri) {
    _stop_pipe(self);
    g_free (priv->uri);
    priv->uri = NULL;
  }

  if (priv->source) {
    g_object_unref (priv->source);
    priv->source = NULL;
  }

  priv->total_bytes = -1;
  priv->duration = -1;
  priv->seekable = -1;
  priv->buffering = FALSE;
  priv->buffer_percent = -1;
  priv->player_state = PlayerStateStopped;
  priv->target = PlayerStateNull;

  return;
}


static gboolean
_stop_pipe (MeegoMediaPlayerControl *control)
{
  EngineGstPrivate *priv = GET_PRIVATE (control);

  if (gst_element_set_state(priv->pipeline, GST_STATE_NULL) == GST_STATE_CHANGE_FAILURE) {
    UMMS_DEBUG (" Unable to set NULL state");
    return FALSE;
  }

  UMMS_DEBUG ("gstreamer engine stopped");
  return TRUE;
}

static gboolean
engine_gst_stop (MeegoMediaPlayerControl *self)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (!_stop_pipe (self))
    return FALSE;

  priv->player_state = PlayerStateStopped;
  meego_media_player_control_emit_stopped (self);

  return TRUE;
}

static gint
_is_ismd_vidrend_bin (GstElement * element, gpointer user_data)
{
  gint ret = 1; //0: match , non-zero: dismatch
  gchar *ele_name = NULL;

  if (!GST_IS_BIN(element)) {
    goto unref_ele;
  }

  ele_name = gst_element_get_name (element);
  UMMS_DEBUG ("%s:element name='%s'\n", __FUNCTION__, ele_name);

  //ugly solution, check by element metadata will be better
  ele_name = gst_element_get_name (element);
  if(g_strrstr(ele_name, "ismdgstvidrendbin") != NULL || g_strrstr(ele_name, "ismd_vidrend_bin") != NULL)
    ret = 0;

  g_free (ele_name);
  if (ret) {//dismatch
    goto unref_ele;
  }

  return ret;

unref_ele:
  gst_object_unref (element);
  return ret;
}

/*
 * This function is used to judge whether we are using ismd_vidrend_bin
 * which allow video rectangle and plane settings through properties.
 * Deprecated.
 */
static GstElement *
_get_ismd_vidrend_bin (GstBin *bin)
{
  GstIterator *children;
  gpointer result;

  g_return_val_if_fail (GST_IS_BIN (bin), NULL);

  GST_INFO ("[%s]: looking up ismd_vidrend_bin  element",
            GST_ELEMENT_NAME (bin));

  children = gst_bin_iterate_recurse (bin);
  result = gst_iterator_find_custom (children,
           (GCompareFunc)_is_ismd_vidrend_bin, (gpointer) NULL);
  gst_iterator_free (children);

  if (result) {
    GST_INFO ("found ismd_vidrend_bin.\n" );
  }

  return GST_ELEMENT_CAST (result);
}

static gboolean
engine_gst_set_video_size (MeegoMediaPlayerControl *self,
    guint x, guint y, guint w, guint h)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);
  GstElement *pipe = priv->pipeline;
  gboolean ret;
  GstElement *vsink_bin;

  g_return_val_if_fail (pipe, FALSE);
  UMMS_DEBUG ("%s: invoked", __FUNCTION__);

  //We use ismd_vidrend_bin as video-sink, so we can set rectangle property.
  g_object_get (G_OBJECT(pipe), "video-sink", &vsink_bin, NULL);
  if (vsink_bin) {
    gchar *rectangle_des = NULL;

    rectangle_des = g_strdup_printf ("%u,%u,%u,%u", x, y, w, h);
    UMMS_DEBUG ("set rectangle damension :'%s'\n", rectangle_des);
    g_object_set (G_OBJECT(vsink_bin), "rectangle", rectangle_des, NULL);
    g_free (rectangle_des);
    gst_object_unref (vsink_bin);
    ret = TRUE;
  } else {
    UMMS_DEBUG ("Get video-sink failed");
    ret = FALSE;
  }

  return ret;
}

static gboolean
engine_gst_get_video_size (MeegoMediaPlayerControl *self,
    guint *w, guint *h)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);
  GstElement *pipe = priv->pipeline;
  guint x[1], y[1];
  gboolean ret;
  GstElement *vsink_bin;

  g_return_val_if_fail (pipe, FALSE);
  UMMS_DEBUG ("%s: invoked", __FUNCTION__);
  g_object_get (G_OBJECT(pipe), "video-sink", &vsink_bin, NULL);
  if (vsink_bin) {
    gchar *rectangle_des = NULL;
    g_object_get (G_OBJECT(vsink_bin), "rectangle", &rectangle_des, NULL);
    sscanf (rectangle_des, "%u,%u,%u,%u", x, y, w, h);
    UMMS_DEBUG ("got rectangle damension :'%u,%u,%u,%u'\n", *x, *y, *w, *h);
    gst_object_unref (vsink_bin);
    ret = TRUE;
  } else {
    UMMS_DEBUG ("Get video-sink failed");
    ret = FALSE;
  }
  return ret;
}

static gboolean
engine_gst_is_seekable (MeegoMediaPlayerControl *self, gboolean *seekable)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  gboolean res = FALSE;
  gint old_seekable;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (seekable != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

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
engine_gst_set_position (MeegoMediaPlayerControl *self, gint64 in_pos)
{
  GstElement *pipe;
  EngineGstPrivate *priv;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("Seeking to %" GST_TIME_FORMAT, GST_TIME_ARGS (in_pos * GST_MSECOND));

  gst_element_seek (pipe, 1.0,
                    GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                    GST_SEEK_TYPE_SET, in_pos * GST_MSECOND,
                    GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

  meego_media_player_control_emit_seeked (self);
  return TRUE;
}

static gboolean
engine_gst_get_position (MeegoMediaPlayerControl *self, gint64 *cur_pos)
{
  EngineGstPrivate *priv = NULL;
  GstElement *pipe = NULL;
  GstFormat fmt;
  gint64 cur = 0;
  gboolean ret = TRUE;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (cur_pos != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

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
engine_gst_set_playback_rate (MeegoMediaPlayerControl *self, gdouble in_rate)
{
  GstElement *pipe;
  EngineGstPrivate *priv;
  GstFormat fmt;
  gint64 cur;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("set playback rate to %f ", in_rate);

  fmt = GST_FORMAT_TIME;
  if (gst_element_query_position (pipe, &fmt, &cur)) {
    UMMS_DEBUG ("current position = %lld (ms)", cur / GST_MSECOND);
  } else {
    UMMS_DEBUG ("Failed to query position");
  }

  gst_element_seek (pipe, in_rate,
                    GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                    GST_SEEK_TYPE_SET, cur,
                    GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

  return TRUE;
}

static gboolean
engine_gst_get_playback_rate (MeegoMediaPlayerControl *self, gdouble *out_rate)
{
  GstElement *pipe;
  EngineGstPrivate *priv;
  GstQuery *query;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("%s:invoked", __FUNCTION__);
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
engine_gst_set_volume (MeegoMediaPlayerControl *self, gint vol)
{
  GstElement *pipe;
  EngineGstPrivate *priv;
  gdouble volume;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("%s:invoked", __FUNCTION__);

  volume = CLAMP ((((gdouble)vol) / 100), 0.0, 1.0);
  UMMS_DEBUG ("%s:set volume to = %f", __FUNCTION__, volume);
  gst_stream_volume_set_volume (GST_STREAM_VOLUME (pipe),
      GST_STREAM_VOLUME_FORMAT_CUBIC,
      volume);

  return TRUE;
}

static gboolean
engine_gst_get_volume (MeegoMediaPlayerControl *self, gint *volume)
{
  GstElement *pipe;
  EngineGstPrivate *priv;
  gdouble vol;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("%s:invoked", __FUNCTION__);

  vol = gst_stream_volume_get_volume (GST_STREAM_VOLUME (pipe),
        GST_STREAM_VOLUME_FORMAT_CUBIC);

  *volume = vol * 100;
  UMMS_DEBUG ("%s:cur volume=%f(double), %d(int)", __FUNCTION__, vol, *volume);

  return TRUE;
}

static gboolean
engine_gst_get_media_size_time (MeegoMediaPlayerControl *self, gint64 *media_size_time)
{
  GstElement *pipe;
  EngineGstPrivate *priv;
  gint64 duration;
  gboolean ret;
  GstFormat fmt = GST_FORMAT_TIME;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("%s:invoked", __FUNCTION__);

  if (gst_element_query_duration (pipe, &fmt, &duration)) {
    *media_size_time = duration / GST_MSECOND;
    UMMS_DEBUG ("%s:media size = %lld (ms)", __FUNCTION__, *media_size_time);
  } else {
    UMMS_DEBUG ("%s: query media_size_time failed", __FUNCTION__);
    *media_size_time = -1;
  }

  return ret;
}


static gboolean
engine_gst_get_media_size_bytes (MeegoMediaPlayerControl *self, gint64 *media_size_bytes)
{
  GstElement *source;
  EngineGstPrivate *priv;
  gint64 length = 0;
  gboolean ret;
  GstFormat fmt = GST_FORMAT_BYTES;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  source = priv->source;
  g_return_val_if_fail (GST_IS_ELEMENT (source), FALSE);

  UMMS_DEBUG ("%s:invoked", __FUNCTION__);

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
engine_gst_has_video (MeegoMediaPlayerControl *self, gboolean *has_video)
{
  GstElement *pipe;
  EngineGstPrivate *priv;
  gint n_video;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("%s:invoked", __FUNCTION__);

  g_object_get (G_OBJECT (pipe), "n-video", &n_video, NULL);
  UMMS_DEBUG ("%s: '%d' videos in stream", __FUNCTION__, n_video);
  *has_video = (n_video > 0) ? (TRUE) : (FALSE);

  return TRUE;
}

static gboolean
engine_gst_has_audio (MeegoMediaPlayerControl *self, gboolean *has_audio)
{
  GstElement *pipe;
  EngineGstPrivate *priv;
  gint n_audio;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("%s:invoked", __FUNCTION__);

  g_object_get (G_OBJECT (pipe), "n-audio", &n_audio, NULL);
  UMMS_DEBUG ("%s: '%d' audio tracks in stream", __FUNCTION__, n_audio);
  *has_audio = (n_audio > 0) ? (TRUE) : (FALSE);

  return TRUE;
}

static gboolean
engine_gst_support_fullscreen (MeegoMediaPlayerControl *self, gboolean *support_fullscreen)
{
  //We are using ismd_vidrend_bin, so this function always return TRUE.
  *support_fullscreen = TRUE;
  return TRUE;
}

static gboolean
engine_gst_is_streaming (MeegoMediaPlayerControl *self, gboolean *is_streaming)
{
  GstElement *pipe;
  EngineGstPrivate *priv;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);
  pipe = priv->pipeline;
  g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

  UMMS_DEBUG ("%s:invoked", __FUNCTION__);

  g_return_val_if_fail (priv->uri, FALSE);
  /*For now, we consider live source to be streaming source , hence unseekable.*/
  *is_streaming = priv->is_live;
  UMMS_DEBUG ("%s:uri:'%s' is %s streaming source", __FUNCTION__, priv->uri, (*is_streaming) ? "" : "not");
  return TRUE;
}

static gboolean
engine_gst_get_player_state (MeegoMediaPlayerControl *self,
    gint *state)
{
  EngineGstPrivate *priv;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (state != NULL, FALSE);
  g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

  priv = GET_PRIVATE (self);

  *state = priv->player_state;
  return TRUE;
}

static gboolean
engine_gst_get_buffered_bytes (MeegoMediaPlayerControl *self,
    gint64 *buffered_bytes)
{
  gint64 total_bytes;
  gdouble percent;

  g_return_val_if_fail (self != NULL, FALSE);
  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (!_query_buffering_percent(priv->pipeline, &percent)) {
    return FALSE;
  }

  if (priv->total_bytes == -1) {
    engine_gst_get_media_size_bytes (self, &total_bytes);
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
    UMMS_DEBUG ("%s: failed", __FUNCTION__);
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
engine_gst_get_buffered_time (MeegoMediaPlayerControl *self,
    gint64 *buffered_time)
{
  gint64 duration;
  gdouble percent;
  EngineGstPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (self != NULL, FALSE);

  if (!_query_buffering_percent(priv->pipeline, &percent)) {
    return FALSE;
  }

  if (priv->duration == -1) {
    engine_gst_get_media_size_time (self, &duration);
    priv->duration = duration;
  }

  *buffered_time = (priv->duration * percent) / 100;
  UMMS_DEBUG ("duration=%lld, buffered_time=%lld", priv->duration, *buffered_time);

  return TRUE;
}

static gboolean
engine_gst_set_window_id (MeegoMediaPlayerControl *self,
    gdouble window_id)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (!priv->uri || !priv->vsink || !GST_IS_X_OVERLAY(priv->vsink)) {
    UMMS_DEBUG(" Unable to set window id");
    return FALSE;
  }

  UMMS_DEBUG ("SetWindowId called, id = %d\n",
              (gint)window_id);

  gst_x_overlay_set_xwindow_id (GST_X_OVERLAY (priv->vsink), (gint) window_id );
  return TRUE;
}

static void
meego_media_player_control_init (MeegoMediaPlayerControl *iface)
{
  MeegoMediaPlayerControlClass *klass = (MeegoMediaPlayerControlClass *)iface;

  meego_media_player_control_implement_set_uri (klass,
      engine_gst_set_uri);
  meego_media_player_control_implement_set_target (klass,
      engine_gst_set_target);
  meego_media_player_control_implement_play (klass,
      engine_gst_play);
  meego_media_player_control_implement_pause (klass,
      engine_gst_pause);
  meego_media_player_control_implement_stop (klass,
      engine_gst_stop);
  meego_media_player_control_implement_set_video_size (klass,
      engine_gst_set_video_size);
  meego_media_player_control_implement_get_video_size (klass,
      engine_gst_get_video_size);
  meego_media_player_control_implement_is_seekable (klass,
      engine_gst_is_seekable);
  meego_media_player_control_implement_set_position (klass,
      engine_gst_set_position);
  meego_media_player_control_implement_get_position (klass,
      engine_gst_get_position);
  meego_media_player_control_implement_set_playback_rate (klass,
      engine_gst_set_playback_rate);
  meego_media_player_control_implement_get_playback_rate (klass,
      engine_gst_get_playback_rate);
  meego_media_player_control_implement_set_volume (klass,
      engine_gst_set_volume);
  meego_media_player_control_implement_get_volume (klass,
      engine_gst_get_volume);
  meego_media_player_control_implement_get_media_size_time (klass,
      engine_gst_get_media_size_time);
  meego_media_player_control_implement_get_media_size_bytes (klass,
      engine_gst_get_media_size_bytes);
  meego_media_player_control_implement_has_video (klass,
      engine_gst_has_video);
  meego_media_player_control_implement_has_audio (klass,
      engine_gst_has_audio);
  meego_media_player_control_implement_support_fullscreen (klass,
      engine_gst_support_fullscreen);
  meego_media_player_control_implement_is_streaming (klass,
      engine_gst_is_streaming);
  meego_media_player_control_implement_set_window_id (klass,
      engine_gst_set_window_id);
  meego_media_player_control_implement_get_player_state (klass,
      engine_gst_get_player_state);
  meego_media_player_control_implement_get_buffered_bytes(klass,
      engine_gst_get_buffered_bytes);
  meego_media_player_control_implement_get_buffered_time(klass,
      engine_gst_get_buffered_time);

}

static void
engine_gst_get_property (GObject    *object,
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
engine_gst_set_property (GObject      *object,
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
engine_gst_dispose (GObject *object)
{
  EngineGstPrivate *priv = GET_PRIVATE (object);

  _stop_pipe ((MeegoMediaPlayerControl *)object);
  if (priv->source) {
    g_object_unref (priv->source);
    priv->source = NULL;
  }

  if (priv->pipeline) {
    g_object_unref (priv->pipeline);
    priv->pipeline = NULL;
  }

  G_OBJECT_CLASS (engine_gst_parent_class)->dispose (object);
}

static void
engine_gst_finalize (GObject *object)
{
  EngineGstPrivate *priv = GET_PRIVATE (object);

  g_free (priv->uri);

  G_OBJECT_CLASS (engine_gst_parent_class)->finalize (object);
}

static void
engine_gst_class_init (EngineGstClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (EngineGstPrivate));

  object_class->get_property = engine_gst_get_property;
  object_class->set_property = engine_gst_set_property;
  object_class->dispose = engine_gst_dispose;
  object_class->finalize = engine_gst_finalize;
}

static void
bus_message_state_change_cb (GstBus     *bus,
    GstMessage *message,
    EngineGst  *self)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);

  GstState old_state, new_state;
  gpointer src;

  src = GST_MESSAGE_SRC (message);
  if (src != priv->pipeline)
    return;

  gst_message_parse_state_changed (message, &old_state, &new_state, NULL);

  UMMS_DEBUG ("state-changed: old='%s', new='%s'", gst_state[old_state], gst_state[new_state]);
  if (new_state == GST_STATE_PAUSED) {
    priv->player_state = PlayerStatePaused;
  } else if(new_state == GST_STATE_PLAYING) {
    priv->player_state = PlayerStatePlaying;
  } else {
    priv->player_state = PlayerStateStopped;
  }

  meego_media_player_control_emit_player_state_changed (self, priv->player_state);
}

static void
bus_message_eos_cb (GstBus     *bus,
                    GstMessage *message,
                    EngineGst  *self)
{
  UMMS_DEBUG ("message::eos received on bus");

  meego_media_player_control_emit_eof (self);
}


static void
bus_message_error_cb (GstBus     *bus,
    GstMessage *message,
    EngineGst  *self)
{
  GError *error = NULL;
  EngineGstPrivate *priv = GET_PRIVATE (self);

  UMMS_DEBUG ("message::error received on bus");

  gst_message_parse_error (message, &error, NULL);
  gst_element_set_state (priv->pipeline, GST_STATE_NULL);

  meego_media_player_control_emit_error (self, UMMS_ENGINE_ERROR_FAILED, error->message);

  UMMS_DEBUG ("Error emitted with message = %s", error->message);

  g_clear_error (&error);
}

static void
bus_message_buffering_cb (GstBus *bus,
    GstMessage *message,
    EngineGst *self)
{
  const GstStructure *str;
  gboolean res;
  gint buffer_percent;
  GstState state, pending_state;
  EngineGstPrivate *priv = GET_PRIVATE (self);

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
      if (priv->target == PlayerStatePlaying)
        gst_element_set_state (priv->pipeline, GST_STATE_PLAYING);
      meego_media_player_control_emit_buffered (self);

    } else if (!priv->buffering && priv->target == PlayerStatePlaying) {
      priv->buffering = TRUE;
      UMMS_DEBUG ("Set pipeline to paused for buffering data");
      gst_element_set_state (priv->pipeline, GST_STATE_PAUSED);
      meego_media_player_control_emit_buffering (self);
    }
  }
}

static GstBusSyncReply
bus_sync_handler (GstBus *bus,
                  GstMessage *message,
                  EngineGst *engine)
{
  EngineGstPrivate *priv;

  if( GST_MESSAGE_TYPE( message ) != GST_MESSAGE_ELEMENT )
    return( GST_BUS_PASS );

  if( ! gst_structure_has_name( message->structure, "prepare-xwindow-id" ) )
    return( GST_BUS_PASS );

  g_return_val_if_fail (engine, GST_BUS_DROP);
  g_return_val_if_fail (ENGINE_IS_GST (engine), GST_BUS_DROP);
  priv = GET_PRIVATE (engine);

  UMMS_DEBUG ("sync-handler received on bus: prepare-xwindow-id");
  priv->vsink =  GST_ELEMENT(GST_MESSAGE_SRC (message));

  meego_media_player_control_emit_request_window (engine);

  return( GST_BUS_DROP );
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
engine_gst_init (EngineGst *self)
{
  EngineGstPrivate *priv;
  GstBus *bus;
  GstPlayFlags flags;

  self->priv = PLAYER_PRIVATE (self);
  priv = self->priv;


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


  gst_bus_set_sync_handler(bus, (GstBusSyncHandler) bus_sync_handler,
      self);

  gst_object_unref (GST_OBJECT (bus));

  /*
   *Use GST_PLAY_FLAG_DOWNLOAD flag to enable Gstreamer Download buffer mode,
   *so that we can query the buffered time/bytes, and further the time rangs.
   */

  g_object_get (priv->pipeline, "flags", &flags, NULL);
  flags |= GST_PLAY_FLAG_DOWNLOAD;
  g_object_set (priv->pipeline, "flags", flags, NULL);

  priv->player_state = PlayerStateNull;

}

EngineGst *
engine_gst_new (void)
{
  return g_object_new (ENGINE_TYPE_GST, NULL);
}

static void
_source_changed_cb (GObject *object, GParamSpec *pspec, gpointer data)
{
  GstElement *source;
  EngineGstPrivate *priv = GET_PRIVATE (data);

  g_object_get(priv->pipeline, "source", &source, NULL);
  gst_object_replace((GstObject**) &priv->source, (GstObject*) source);
  UMMS_DEBUG ("source changed");

  return;
}
