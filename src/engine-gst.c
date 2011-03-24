#include <string.h>
#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
/* for the volume property */
#include <gst/interfaces/streamvolume.h>
#include "umms-common.h"
#include "engine-gst.h"
#include "meego-media-player-control.h"


static void meego_media_player_control_init (MeegoMediaPlayerControl* iface);

G_DEFINE_TYPE_WITH_CODE (EngineGst, engine_gst, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEEGO_TYPE_MEDIA_PLAYER_CONTROL, meego_media_player_control_init))

#define PLAYER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ENGINE_TYPE_GST, EngineGstPrivate))

#define GET_PRIVATE(o) ((EngineGst *)o)->priv

/* list of URIs that we consider to be streams and that need buffering.
 * We have no mechanism yet to figure this out with a query. 
 */
static const gchar *stream_uris[] = { "http://", "mms://", "mmsh://",
    "mmsu://", "mmst://", "fd://", "myth://", "ssh://", "ftp://", "sftp://",
    NULL
};

#define IS_STREAM_URI(uri)          (_array_has_uri_value (stream_uris, uri))

struct _EngineGstPrivate
{
  GstElement *pipeline;

  gchar *uri;
  gint  seekable;
  gdouble stacked_progress;

  gboolean video_available;

  gint tick_timeout_id;

  GstElement *vsink;
  GstState stacked_state;

  //buffering stuff
  gboolean buffering;
  gint buffer_percent;

  //UMMS defined player state
  PlayerState player_state;
};


static gboolean
_array_has_uri_value (const gchar * values[], const gchar * value)
{
    gint i;

    for (i = 0; values[i]; i++) {
        if (!g_ascii_strncasecmp (value, values[i], strlen (values[i])))
            return TRUE;
    }
    return FALSE;
}

static gboolean
engine_gst_set_uri (MeegoMediaPlayerControl *self,
                    const gchar           *uri)
{
    EngineGstPrivate *priv = GET_PRIVATE (self);
    GstState state, pending;

    g_return_val_if_fail (uri, FALSE);

    UMMS_DEBUG ("SetUri called: old = %s new = %s",
            priv->uri,
            uri);

    if (priv->uri) {
        g_free (priv->uri);
        priv->uri = NULL;
    }

    priv->uri = g_strdup (uri);

    gst_element_get_state (priv->pipeline, &state, &pending, 0);

    if (pending)
        state = pending;

    gst_element_set_state (priv->pipeline, GST_STATE_NULL);

    g_object_set (priv->pipeline, "uri", uri, NULL);
    priv->seekable = -1;

    /*
     * Restore state.
     */
    gst_element_set_state (priv->pipeline, state);
    return TRUE;
}

static gboolean
engine_gst_play (MeegoMediaPlayerControl *self)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (!priv->uri) {
    g_warning (G_STRLOC ": Unable to set playing state - no URI set");
    return FALSE;
  }

  gst_element_set_state (priv->pipeline, GST_STATE_PLAYING);

  return TRUE;
}

static gboolean
engine_gst_pause (MeegoMediaPlayerControl *self)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (gst_element_set_state(priv->pipeline, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
      g_warning (G_STRLOC ": Unable to set pause state");
      return FALSE;
  }
  return TRUE;
}

static gboolean
engine_gst_stop (MeegoMediaPlayerControl *self)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (gst_element_set_state(priv->pipeline, GST_STATE_NULL) == GST_STATE_CHANGE_FAILURE) {
      g_warning (G_STRLOC ": Unable to set NULL state");
  } 

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
  g_debug ("%s:element name='%s'\n",__FUNCTION__, ele_name);
  
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

    g_return_val_if_fail (pipe, FALSE);
    g_debug ("%s: invoked", __FUNCTION__);

    GstElement *vsink_bin = _get_ismd_vidrend_bin(GST_BIN(pipe));
    if (vsink_bin) {
        gchar *rectangle_des = NULL;

        rectangle_des = g_strdup_printf ("%u,%u,%u,%u", x, y, w, h);
        g_debug ("set rectangle damension :'%s'\n", rectangle_des);
        g_object_set (G_OBJECT(vsink_bin), "rectangle", rectangle_des, NULL);
        g_free (rectangle_des);
        gst_object_unref (vsink_bin);
        ret = TRUE;
    } else {
        //TODO:implement set_video_size for other video render types.
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

    g_return_val_if_fail (pipe, FALSE);
    g_debug ("%s: invoked", __FUNCTION__);
    GstElement *vsink_bin = _get_ismd_vidrend_bin(GST_BIN(pipe));
    if (vsink_bin) {
        gchar *rectangle_des = NULL;
        g_object_get (G_OBJECT(vsink_bin), "rectangle", &rectangle_des, NULL);
        sscanf (rectangle_des, "%u,%u,%u,%u", x, y, w, h);
        g_debug ("got rectangle damension :'%u,%u,%u,%u'\n", *x,*y,*w,*h);
        gst_object_unref (vsink_bin);
        ret = TRUE;
    } else {
        //TODO:implement get_video_size for other video render types.
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
      GST_DEBUG ("seeking query says the stream is%s seekable", (res) ? "" : " not");
      priv->seekable = (res) ? 1 : 0;
    } else {
      GST_DEBUG ("seeking query failed");
    }
    gst_query_unref (query);
  }

  if (priv->seekable != -1) {
    res = (priv->seekable != 0);
  }

  /*
  if (old_seekable != priv->seekable)
    g_object_notify (G_OBJECT (bvw), "seekable");
  */
  GST_DEBUG ("stream is%s seekable", (res) ? "" : " not");
  return res;
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

    GST_LOG ("Seeking to %" GST_TIME_FORMAT, GST_TIME_ARGS (in_pos* GST_MSECOND));

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
        UMMS_DEBUG ("current position = %lld (ms)", *cur_pos = cur/GST_MSECOND);
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
        UMMS_DEBUG ("current position = %lld (ms)", cur/GST_MSECOND);
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

    volume = CLAMP ((((gdouble)vol)/100), 0.0, 1.0);
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

    *volume = vol*100;
    UMMS_DEBUG ("%s:cur volume=%f(double), %d(int)", __FUNCTION__, vol, *volume);

    return TRUE;
}

static gboolean
_query_media_size (GstElement *player, gint64 *media_size, GstFormat fmt)
{
    gboolean ret; 

    if (gst_element_query_duration (player, &fmt, media_size)) {
        ret = TRUE;
    } else {
        ret = FALSE;
    }
    return ret;
}

static gboolean 
engine_gst_get_media_size_time (MeegoMediaPlayerControl *self, gint64 *media_size_time)
{
    GstElement *pipe;
    EngineGstPrivate *priv;
    gint64 duration;
    gboolean ret;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

    priv = GET_PRIVATE (self);
    pipe = priv->pipeline;
    g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

    UMMS_DEBUG ("%s:invoked", __FUNCTION__);
    if ((ret = _query_media_size (pipe, &duration, GST_FORMAT_TIME))) {
        *media_size_time = duration/GST_MSECOND;
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
    GstElement *pipe;
    EngineGstPrivate *priv;
    gint64 duration;
    gboolean ret;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

    priv = GET_PRIVATE (self);
    pipe = priv->pipeline;
    g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

    UMMS_DEBUG ("%s:invoked", __FUNCTION__);
    if ((ret = _query_media_size (pipe, &duration, GST_FORMAT_BYTES))) {
        UMMS_DEBUG ("%s:media size = %lld (bytes)", __FUNCTION__, *media_size_bytes);
        *media_size_bytes = duration;
    } else {
        UMMS_DEBUG ("%s: query media_size_bytes failed", __FUNCTION__);
        *media_size_bytes = -1;
    }

    return ret;
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
    *has_video = (n_video>0)?(TRUE):(FALSE);

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
    *has_audio = (n_audio>0)?(TRUE):(FALSE);

    return TRUE;
}

static gboolean 
engine_gst_support_fullscreen (MeegoMediaPlayerControl *self, gboolean *support_fullscreen)
{
    GstElement *pipe, *ismd_vbin;
    EngineGstPrivate *priv;
    gboolean ret;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (MEEGO_IS_MEDIA_PLAYER_CONTROL(self), FALSE);

    priv = GET_PRIVATE (self);
    pipe = priv->pipeline;
    g_return_val_if_fail (GST_IS_ELEMENT (pipe), FALSE);

    UMMS_DEBUG ("%s:invoked", __FUNCTION__);
    if ((ismd_vbin = _get_ismd_vidrend_bin (GST_BIN(pipe)))) {
        UMMS_DEBUG ("%s: support fullscreen", __FUNCTION__);
        *support_fullscreen = TRUE;
        ret = TRUE;
    } else {
        //TODO: verify fullscreen capability by other ways.
        ret = FALSE;
    } 

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
    *is_streaming = IS_STREAM_URI (priv->uri);
    UMMS_DEBUG ("%s:uri:'%s' is %s streaming source", __FUNCTION__, priv->uri, (*is_streaming)?"":"not");
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
_get_buffer_depth (GstElement *player, GstFormat format, gint64 *depth)
{
    g_return_val_if_fail (player!= NULL, FALSE);
    g_return_val_if_fail (depth!= NULL, FALSE);

    UMMS_DEBUG ("%s:invoked", __FUNCTION__);

    if (format == GST_FORMAT_TIME){
        g_object_get (player, "buffer-time", depth, NULL);
    } else {
        gint bytes;
        g_object_get (player, "buffer-size", &bytes, NULL);
        *depth = (gint64)bytes;
    }
    return TRUE;
}


    static gboolean
engine_gst_get_buffered_bytes (MeegoMediaPlayerControl *self,
        gint64 *buffered_bytes)
{
    g_return_val_if_fail (self != NULL, FALSE);
    EngineGstPrivate *priv = GET_PRIVATE (self);
    _get_buffer_depth (priv->pipeline, GST_FORMAT_BYTES, buffered_bytes);
    return TRUE;
}
    static gboolean
engine_gst_get_buffered_time (MeegoMediaPlayerControl *self,
        gint64 *buffered_time)
{
    g_return_val_if_fail (self != NULL, FALSE);
    EngineGstPrivate *priv = GET_PRIVATE (self);
    _get_buffer_depth (priv->pipeline, GST_FORMAT_TIME, buffered_time);
    return TRUE;
}

static gboolean
engine_gst_set_window_id (MeegoMediaPlayerControl *self,
                          gdouble window_id)
{
  EngineGstPrivate *priv = GET_PRIVATE (self);

  if (!priv->uri || !priv->vsink || !GST_IS_X_OVERLAY(priv->vsink)) {
    g_warning (G_STRLOC ": Unable to set window id");
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
  switch (property_id)
    {
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
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
engine_gst_dispose (GObject *object)
{
    EngineGstPrivate *priv = GET_PRIVATE (object);

    if (priv->pipeline)
    {
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

    UMMS_DEBUG ("message::state-changed received on bus");

    gst_message_parse_state_changed (message, &old_state, &new_state, NULL);

    if (new_state == GST_STATE_PAUSED){
        priv->player_state = PlayerStatePaused;
    }else if(new_state == GST_STATE_PLAYING){
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

  meego_media_player_control_emit_error (self, ErrorTypeEngine, error->message);

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
    EngineGstPrivate *priv = GET_PRIVATE (self);

    UMMS_DEBUG ("message::buffer progress received on bus");
    str = gst_message_get_structure (message);
    if (!str)
        return;

    res = gst_structure_get_int (str, "buffer-percent", &buffer_percent);
    if (res)
    {
        priv->buffer_percent = buffer_percent;
        //priv->buffer_percent = CLAMP ((gdouble) buffer_percent / 100.0,
        //                      0.0,
        //                      1.0);

        if (priv->buffer_percent == 100) {
            meego_media_player_control_emit_buffered (self);
        } else if (!priv->buffering) {
            priv->buffering = TRUE;
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
                   
static void
engine_gst_init (EngineGst *self)
{
  EngineGstPrivate *priv;
  GstBus *bus;

  self->priv = PLAYER_PRIVATE (self);
  priv = self->priv;

  
  priv->pipeline = gst_element_factory_make ("playbin2", "pipeline");

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

  priv->player_state = PlayerStateNull;

}

EngineGst *
engine_gst_new (void)
{
  return g_object_new (ENGINE_TYPE_GST, NULL);
}
