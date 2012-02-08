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

#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib.h>

#include "umms-debug.h"
#include "umms-common.h"
#include "umms-object-manager.h"
#include "media-player.h"
#include "./glue/umms-media-player-glue.h"


G_DEFINE_TYPE (UmmsObjectManager, umms_object_manager, G_TYPE_OBJECT)
#define MANAGER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), UMMS_TYPE_OBJECT_MANAGER, UmmsObjectManagerPrivate))

#define GET_PRIVATE(o) ((UmmsObjectManager *)o)->priv


#define OBJ_NAME_PREFIX "/com/UMMS/MediaPlayer"

static void _player_list_free (GList *player_list);
static gint _find_player_by_name (gconstpointer player_in, gconstpointer  name);
static gboolean _stop_execution(gpointer data);
static void _dump_player (gpointer a, gpointer b);
static void _dump_player_list (GList *players);
static MediaPlayer *_gen_media_player (UmmsObjectManager *mngr, gboolean attended);
static gboolean _remove_media_player (MediaPlayer *player);
static gboolean _start_record (gpointer data);
static gboolean _stop_record (gpointer data);

enum {
  SIGNAL_PLAYER_ADDED,
  N_SIGNALS
};

/*property for object manager*/
enum PROPTYPE{
  PROP_0,
  PROP_PLATFORM,
  PROP_LAST
};

static guint signals[N_SIGNALS] = {0};

struct _UmmsObjectManagerPrivate {
  GList *player_list;
  gint  cur_player_id;
  PlatformType platform_type;
};

typedef struct _RecordItem{
  MediaPlayer *recorder;
  gchar       *location;
  gdouble     duration;
  guint       delay_timer_id;
  guint       duration_timer_id;
}RecordItem;

typedef struct _PlayerCtx{
  
  void (*free_func) (void *);
  void * data;
}PlayerCtx;

static void
umms_object_manager_get_property (GObject    *object,
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
umms_object_manager_set_property (GObject      *object,
    guint         property_id,
    const GValue *value,
    GParamSpec   *pspec)
{
  UmmsObjectManagerPrivate *priv = GET_PRIVATE (object);
  gint tmp;

  switch (property_id) {
    case PROP_PLATFORM:
      tmp = g_value_get_int(value);
      priv->platform_type = tmp;
      UMMS_DEBUG("platform type: %d", tmp);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}



static void
umms_object_manager_dispose (GObject *object)
{
  UmmsObjectManagerPrivate *priv = GET_PRIVATE (object);
  _player_list_free (priv->player_list);

  G_OBJECT_CLASS (umms_object_manager_parent_class)->dispose (object);
}

static void
umms_object_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (umms_object_manager_parent_class)->finalize (object);
}

static void
umms_object_manager_class_init (UmmsObjectManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (UmmsObjectManagerPrivate));

  object_class->get_property = umms_object_manager_get_property;
  object_class->set_property = umms_object_manager_set_property;
  object_class->dispose = umms_object_manager_dispose;
  object_class->finalize = umms_object_manager_finalize;

  signals[SIGNAL_PLAYER_ADDED] =
    g_signal_new ("player-added",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE,
                  1, TYPE_MEDIA_PLAYER);


  g_object_class_install_property (object_class, PROP_PLATFORM,
      g_param_spec_int ("platform", "platform type", "indication for platform type: Tv, netbook, etc",0,PLATFORM_INVALID,
          CETV, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

}

static void
umms_object_manager_init (UmmsObjectManager *self)
{
  UmmsObjectManagerPrivate *priv;

  self->priv = MANAGER_PRIVATE (self);
  priv = self->priv;
  priv->player_list = NULL;

}

static UmmsObjectManager *mngr_global = NULL;

UmmsObjectManager *
umms_object_manager_new (gint platform)
{
  if (!mngr_global) {
    mngr_global = g_object_new (UMMS_TYPE_OBJECT_MANAGER, "platform", platform,NULL);
  }
  return mngr_global;
}

static void client_no_reply_cb (MediaPlayer *player, gpointer user_data)
{
  UMMS_DEBUG ("called");
  _remove_media_player (player);
}


gboolean
umms_object_manager_request_media_player(UmmsObjectManager *self, gchar **object_path, GError **error)
{
  MediaPlayer *player;

  UMMS_DEBUG("request attended media player");

  player = _gen_media_player (self, TRUE);

  g_signal_connect_object (player, "client-no-reply", G_CALLBACK(client_no_reply_cb), NULL, 0);

  g_object_get(G_OBJECT(player), "name", object_path, NULL);

  UMMS_DEBUG("object_path returned to client = '%s'", *object_path);

  _dump_player_list (self->priv->player_list);

  //FIXME:do something to munipulate MediaPlayer

  return TRUE;
}

static void
timer_source_free (void *data)
{
  guint source_id = (guint) data;

  if (source_id)
    g_source_remove(source_id);

  return;
}

gboolean
umms_object_manager_request_media_player_unattended(UmmsObjectManager *self,
    gdouble time_to_execution,
    gchar **token,
    gchar **object_path,
    GError **error)
{
  MediaPlayer *player;

  UMMS_DEBUG("request unattened media player, time_to_execution = '%lf' seconds", time_to_execution);

  player = _gen_media_player (self, FALSE);
  g_object_get(G_OBJECT(player), "name", object_path, NULL);

  //FIXME: return a unique ID token for this execution
  //Ref: Unified Multi Media Service, Section 7, Transparency, Attended and Non Attended execution
  *token = g_strdup ("Dummy ID token");

  UMMS_DEBUG("object_path returned to client = '%s', token = '%s'", *object_path, *token);

  PlayerCtx *ctx;
  ctx = (PlayerCtx *)g_malloc0(sizeof (PlayerCtx));
  
  ctx->free_func = timer_source_free;
  ctx->data = g_timeout_add ((time_to_execution * 1000), _stop_execution, player);
  g_object_set_data (G_OBJECT (player), "ctx", ctx);

  return TRUE;
}

static void
record_item_free (void *data)
{
  RecordItem *item = (RecordItem *)data;

  if (item->delay_timer_id)
    g_source_remove(item->delay_timer_id);
  if (item->duration_timer_id)
    g_source_remove(item->duration_timer_id);

  g_free (item->location);
  g_free (item);

  return;
}

gboolean 
umms_object_manager_request_scheduled_recorder(UmmsObjectManager *self, 
    gdouble start_time, 
    gdouble duration,
    gchar *uri,
    gchar *location, 
    gchar **token, 
    gchar **object_path, 
    GError **error)
{
  MediaPlayer *player;
  RecordItem *record_item;
  PlayerCtx *ctx;

  player = _gen_media_player (self, FALSE);
  g_object_get(G_OBJECT(player), "name", object_path, NULL);

  *token = g_strdup ("Dummy ID token");

  media_player_set_uri (player, uri, NULL);

  record_item = g_malloc0 (sizeof (RecordItem));
  record_item->recorder = player;
  record_item->location = g_strdup (location);
  record_item->duration = duration;

  ctx = g_malloc0 (sizeof (PlayerCtx));
  ctx->data = record_item;
  ctx->free_func = record_item_free;
  g_object_set_data (G_OBJECT(player), "ctx", ctx);

  //FIXME: start_time is relative to current time, is a absolute time more appropriate?
  record_item->delay_timer_id = g_timeout_add ((start_time* 1000), _start_record, record_item);
  
  return TRUE;
}



gboolean
umms_object_manager_remove_media_player(UmmsObjectManager *self, gchar *object_path, GError **error)
{
  UmmsObjectManagerPrivate *priv;
  GList *ele;

  UMMS_DEBUG("removing '%s'", object_path);

  priv = self->priv;
  ele = g_list_find_custom (priv->player_list, object_path, _find_player_by_name);
  g_return_val_if_fail (ele, FALSE);
  _remove_media_player ((MediaPlayer *)(ele->data));

  return TRUE;
}

static void _player_list_free (GList *player_list)
{
  GList *g;

  for (g = player_list; g; g = g->next) {
    MediaPlayer *player = (MediaPlayer *) (g->data);
    g_object_unref (player);
  }
  g_list_free (player_list);

  return;
}

static gint
_find_player_by_name (gconstpointer player_in, gconstpointer  name)
{
  gchar *src, *dest;
  gint  res;
  MediaPlayer *player;

  g_return_val_if_fail (player_in, 1);
  g_return_val_if_fail (name, 1);

  dest = (gchar *)name;
  player = (MediaPlayer *)player_in;

  g_object_get (G_OBJECT(player), "name", &src, NULL);
  res = g_strcmp0 (src, dest);
  g_free (src);

  return res;
}

static gboolean _stop_execution(gpointer data)
{
  MediaPlayer *player = (MediaPlayer *)data;

  UMMS_DEBUG ("Stop unattended execution!");

  _remove_media_player (player);

  return FALSE;
}

static gboolean _stop_record (gpointer data)
{
  RecordItem *record_item = (RecordItem *)data;
  MediaPlayer *player = record_item->recorder;

  UMMS_DEBUG ("Stop record!");
  record_item->duration_timer_id = 0;
  media_player_record (player, FALSE, NULL, NULL);
  _remove_media_player (player);

  return FALSE;
}

static gboolean _start_record (gpointer data)
{
  GError *err = NULL;
  RecordItem *record_item = (RecordItem *)data;
  MediaPlayer *player = record_item->recorder;
  gboolean ret;

  UMMS_DEBUG ("Start record!");
  /*
   * Before invoking media_player_record(), we should load internal player engine.
   * Setting the target state to PlayerStateNull means we just load the engine and do nothing to construct the pipeline.
   */
  media_player_activate (player, PlayerStateNull);
  media_player_record (player, TRUE, record_item->location, NULL);
  
  record_item->delay_timer_id = 0;
  record_item->duration_timer_id = g_timeout_add ((record_item->duration * 1000), _stop_record, record_item);

  return FALSE;
}

static void _dump_player (gpointer a, gpointer b)
{
  MediaPlayer *player = (MediaPlayer *)a;
  gchar *name;
  gboolean attended;
  gint type;

  g_object_get (G_OBJECT (player), "name", &name, "attended", &attended, "platform", &type, NULL);

  UMMS_DEBUG ("player=%p, name=%s, attended=%d, platform_type=%d", player, name, attended, type);
  g_free (name);
  return;
}

static void
_dump_player_list (GList *players)
{

  g_return_if_fail(players);

  g_list_foreach (players, _dump_player, NULL);
}

static MediaPlayer *
_gen_media_player (UmmsObjectManager *mngr, gboolean attended)
{

  DBusGConnection    *connection;
  MediaPlayer   *player;
  gchar              *object_path;
  gint                id;
  UmmsObjectManagerPrivate *priv;
  GError *err = NULL;

  priv = mngr->priv;

  id = priv->cur_player_id++;
  object_path = g_strdup_printf (OBJ_NAME_PREFIX"%d", id);

  UMMS_DEBUG("object_path='%s' ", object_path);

  dbus_g_object_type_install_info (TYPE_MEDIA_PLAYER, &dbus_glib_media_player_object_info);

#if 0
  player = (MediaPlayer *)g_object_new (TYPE_MEDIA_PLAYER_FACTORY,
           "name", object_path,
           "attended", attended,
           "platform", priv->platform_type,
           NULL);
#else
  /*create a media player instance*/
  player = (MediaPlayer *)g_object_new (TYPE_MEDIA_PLAYER,
           "name", object_path,
           "attended", attended,
           "platform", priv->platform_type,
           NULL);
#endif

  priv->player_list = g_list_append (priv->player_list, player);

  //register object with connection
  connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &err);
  if (connection == NULL) {
    g_printerr ("Failed to open connection to DBus: %s\n", err->message);
    g_error_free (err);
    goto get_connection_failed;
  }
  dbus_g_connection_register_g_object (connection,
      object_path,
      G_OBJECT (player));

  g_free (object_path);

  g_signal_emit (mngr, signals[SIGNAL_PLAYER_ADDED], 0, player);
  return player;

get_connection_failed: {
    g_free (object_path);
    g_object_unref (player);
    return NULL;
  }
}

static gboolean
_remove_media_player (MediaPlayer *player)
{
  DBusGConnection *connection;
  UmmsObjectManagerPrivate *priv;
  PlayerCtx *ctx;
  GError *err = NULL;

  UMMS_DEBUG ("Removing player(%p)", player);
  g_return_val_if_fail (mngr_global, FALSE);

  priv = mngr_global->priv;

  //unregister object with connection
  connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &err);
  if (connection == NULL) {
    g_printerr ("Failed to open connection to DBus: %s\n", err->message);
    g_error_free (err);
    return FALSE;
  }
  dbus_g_connection_unregister_g_object (connection,
      G_OBJECT (player));

  //remove from player list
  priv->player_list = g_list_remove (priv->player_list, player);

  /*distory its factory*/
  if(player->factory){
    g_object_unref (player->factory);
    player->factory = NULL;
  }

  //destory extra ctx
  ctx = g_object_get_data (G_OBJECT(player), "ctx");

  if (ctx){
    ctx->free_func (ctx->data);
    g_free (ctx);
  }

  //distory player
  g_object_unref (player);

  return TRUE;
}


GList *umms_object_manager_get_player_list (UmmsObjectManager *self)
{
  UmmsObjectManagerPrivate *priv = GET_PRIVATE (self);
  return priv->player_list;
}
