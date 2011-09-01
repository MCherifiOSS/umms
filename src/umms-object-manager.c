


#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib.h>

#include "umms-debug.h"
#include "umms-object-manager.h"
#include "meego-media-player-gstreamer.h"
#include "./glue/umms-media-player-glue.h"


G_DEFINE_TYPE (UmmsObjectManager, umms_object_manager, G_TYPE_OBJECT)
#define MANAGER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), UMMS_TYPE_OBJECT_MANAGER, UmmsObjectManagerPrivate))

#define GET_PRIVATE(o) ((UmmsObjectManager *)o)->priv


#define OBJ_NAME_PREFIX "/com/meego/UMMS/MediaPlayer"

static void _player_list_free (GList *player_list);
static gint _find_player_by_name (gconstpointer player_in, gconstpointer  name);
static gboolean _stop_execution(gpointer data);
static void _dump_player (gpointer a, gpointer b);
static void _dump_player_list (GList *players);
static MeegoMediaPlayer *_gen_media_player (UmmsObjectManager *mngr, gboolean attended);
static gboolean _remove_media_player (MeegoMediaPlayer *player);

enum {
  SIGNAL_PLAYER_ADDED,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = {0};

struct _UmmsObjectManagerPrivate {
  GList *player_list;
  gint  cur_player_id;
};

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
  switch (property_id) {
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
  UmmsObjectManagerClass *p_class = UMMS_OBJECT_MANAGER_CLASS (klass);

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
                  1, MEEGO_TYPE_MEDIA_PLAYER);
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
umms_object_manager_new (void)
{
  if (!mngr_global) {
    mngr_global = g_object_new (UMMS_TYPE_OBJECT_MANAGER, NULL);
  }
  return mngr_global;
}

static void client_no_reply_cb (MeegoMediaPlayer *player, gpointer user_data)
{
  UMMS_DEBUG ("called");
  _remove_media_player (player);
}


gboolean
umms_object_manager_request_media_player(UmmsObjectManager *self, gchar **object_path, GError **error)
{
  MeegoMediaPlayer *player;

  UMMS_DEBUG("request attended media player");

  player = _gen_media_player (self, TRUE);

  g_signal_connect_object (player, "client-no-reply", G_CALLBACK(client_no_reply_cb), NULL, 0);

  g_object_get(G_OBJECT(player), "name", object_path, NULL, 0);

  UMMS_DEBUG("object_path returned to client = '%s'", *object_path);

  _dump_player_list (self->priv->player_list);

  //FIXME:do something to munipulate MeegoMediaPlayer

  return TRUE;
}



gboolean
umms_object_manager_request_media_player_unattended(UmmsObjectManager *self,
    gdouble time_to_execution,
    gchar **token,
    gchar **object_path,
    GError **error)
{
  MeegoMediaPlayer *player;
  GError *err = NULL;

  UMMS_DEBUG("request unattened media player, time_to_execution = '%lf' seconds", time_to_execution);

  player = _gen_media_player (self, FALSE);
  g_object_get(G_OBJECT(player), "name", object_path, NULL);

  //FIXME: return a unique ID token for this execution
  //Ref: Unified Multi Media Service, Section 7, Transparency, Attended and Non Attended execution
  *token = g_strdup ("Dummy ID token");

  UMMS_DEBUG("object_path returned to client = '%s', token = '%s'", *object_path, *token);

  g_timeout_add ((time_to_execution * 1000), _stop_execution, player);

  return TRUE;
}



gboolean
umms_object_manager_remove_media_player(UmmsObjectManager *self, gchar *object_path, GError **error)
{
  UmmsObjectManagerPrivate *priv;
  MeegoMediaPlayer *player;
  GList *ele;
  GError *err = NULL;

  UMMS_DEBUG("removing '%s'", object_path);

  priv = self->priv;
  ele = g_list_find_custom (priv->player_list, object_path, _find_player_by_name);
  g_return_val_if_fail (ele, FALSE);
  _remove_media_player ((MeegoMediaPlayer *)(ele->data));

  return TRUE;
}

static void _player_list_free (GList *player_list)
{
  GList *g;

  for (g = player_list; g; g = g->next) {
    MeegoMediaPlayer *player = (MeegoMediaPlayer *) (g->data);
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
  MeegoMediaPlayer *player;

  g_return_val_if_fail (player_in, 1);
  g_return_val_if_fail (name, 1);

  dest = (gchar *)name;
  player = (MeegoMediaPlayer *)player_in;

  g_object_get (G_OBJECT(player), "name", &src, NULL);
  res = g_strcmp0 (src, dest);
  g_free (src);

  return res;
}

static gboolean _stop_execution(gpointer data)
{
  MeegoMediaPlayer *player = (MeegoMediaPlayer *)data;

  UMMS_DEBUG ("Stop unattended execution!");

  _remove_media_player (player);

  return FALSE;
}

static void _dump_player (gpointer a, gpointer b)
{
  MeegoMediaPlayer *player = (MeegoMediaPlayer *)a;
  gchar *name;
  gboolean attended;

  g_object_get (G_OBJECT (player), "name", &name, "attended", &attended, NULL);

  UMMS_DEBUG ("player=%p, name=%s, attended=%d", player, name, attended);
  g_free (name);
  return;
}

static void
_dump_player_list (GList *players)
{

  g_return_if_fail(players);

  g_list_foreach (players, _dump_player, NULL);
}

static MeegoMediaPlayer *
_gen_media_player (UmmsObjectManager *mngr, gboolean attended)
{

  DBusGConnection    *connection;
  MeegoMediaPlayer   *player;
  gchar              *object_path;
  gint                id;
  UmmsObjectManagerPrivate *priv;
  GError *err = NULL;

  priv = mngr->priv;

  id = priv->cur_player_id++;
  object_path = g_strdup_printf (OBJ_NAME_PREFIX"%d", id);

  UMMS_DEBUG("object_path='%s' ", object_path, object_path);

  dbus_g_object_type_install_info (MEEGO_TYPE_MEDIA_PLAYER, &dbus_glib_meego_media_player_object_info);

  player = (MeegoMediaPlayer *)g_object_new (MEEGO_TYPE_MEDIA_PLAYER_GSTREAMER,
           "name", object_path,
           "attended", attended,
           NULL);
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
_remove_media_player (MeegoMediaPlayer *player)
{
  DBusGConnection *connection;
  UmmsObjectManager *mngr;
  UmmsObjectManagerPrivate *priv;
  GError *err = NULL;

  UMMS_DEBUG ("Removing player(%p)", player);
  g_return_if_fail (mngr_global);

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

  //distory player
  g_object_unref (player);

  return TRUE;
}


GList *umms_object_manager_get_player_list (UmmsObjectManager *self)
{
  UmmsObjectManagerPrivate *priv = GET_PRIVATE (self);
  return priv->player_list;
}
