#include <dbus/dbus-glib.h>
#include <glib-object.h>
#include <stdio.h>

#include "../src/umms-common.h"
#include "../src/umms-debug.h"
#include "umms-client-object.h"


G_DEFINE_TYPE (UmmsClientObject, umms_client_object, G_TYPE_OBJECT)
#define PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), UMMS_TYPE_CLIENT_OBJECT, UmmsClientObjectPrivate))

#define GET_PRIVATE(o) ((UmmsClientObject *)o)->priv



struct _UmmsClientObjectPrivate {
  DBusGProxy *obj_mngr;

};

static void need_reply_cb(DBusGProxy *player);
static DBusGProxy *get_iface(const gchar *obj_path, const gchar *iface_name);

static void
umms_client_object_get_property (GObject    *object,
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
umms_client_object_set_property (GObject      *object,
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
umms_client_object_dispose (GObject *object)
{
  UmmsClientObjectPrivate *priv = GET_PRIVATE (object);

  if (priv->obj_mngr)
    g_object_unref (priv->obj_mngr);
  priv->obj_mngr = NULL;

  G_OBJECT_CLASS (umms_client_object_parent_class)->dispose (object);
}

static void
umms_client_object_finalize (GObject *object)
{
  G_OBJECT_CLASS (umms_client_object_parent_class)->finalize (object);
}

static void
umms_client_object_class_init (UmmsClientObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (UmmsClientObjectPrivate));

  object_class->get_property = umms_client_object_get_property;
  object_class->set_property = umms_client_object_set_property;
  object_class->dispose = umms_client_object_dispose;
  object_class->finalize = umms_client_object_finalize;

}

static void
umms_client_object_init (UmmsClientObject *self)
{
  UmmsClientObjectPrivate *priv;
  DBusGConnection *bus;
  GError *error = NULL;

  self->priv = PRIVATE (self);
  priv = self->priv;

  bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
  if (!bus)
    UMMS_GERROR ("Couldn't connect to session bus", error);

  priv->obj_mngr = dbus_g_proxy_new_for_name (bus,
                   UMMS_SERVICE_NAME,
                   UMMS_OBJECT_MANAGER_OBJECT_PATH,
                   UMMS_OBJECT_MANAGER_INTERFACE_NAME);
}

UmmsClientObject *
umms_client_object_new (void)
{
  return g_object_new (UMMS_TYPE_CLIENT_OBJECT, NULL);
}


DBusGProxy *umms_client_object_request_player (UmmsClientObject *self, gboolean attended, gdouble time_to_execution, gchar **name)
{
  UmmsClientObjectPrivate *priv = GET_PRIVATE (self);
  GError *error = NULL;
  gchar  *obj_path;
  gchar  *token;
  DBusGProxy  *player = NULL;

  if (attended) {
    if (!dbus_g_proxy_call (priv->obj_mngr, "RequestMediaPlayer", &error,
         G_TYPE_INVALID,
         G_TYPE_STRING, &obj_path, G_TYPE_INVALID)) {
      UMMS_GERROR ("RequestMediaPlayer failed", error);
    }
    UMMS_DEBUG ("obj_path='%s'", obj_path);

  } else {
    if (!dbus_g_proxy_call (priv->obj_mngr, "RequestMediaPlayerUnattended", &error,
         G_TYPE_DOUBLE, time_to_execution, G_TYPE_INVALID,
         G_TYPE_STRING, &token,
         G_TYPE_STRING, &obj_path, G_TYPE_INVALID)) {
      UMMS_GERROR ("RequestMediaPlayerUnattended failed", error);
    }
    UMMS_DEBUG ("unattended execution, timeout=%f, obj_path='%s'", time_to_execution, obj_path);
  }

  player = get_iface (obj_path, MEDIA_PLAYER_INTERFACE_NAME);

  if (attended) {
    dbus_g_proxy_add_signal (player, "NeedReply", G_TYPE_INVALID);
    dbus_g_proxy_connect_signal (player,
        "NeedReply",
        G_CALLBACK(need_reply_cb),
        NULL, NULL);
  }

  *name = g_strdup(obj_path);
  g_free(obj_path);
  g_free(token);

  return player;
}

static DBusGProxy *get_iface(const gchar *obj_path, const gchar *iface_name)
{
  DBusGConnection *bus;
  DBusGProxy *proxy = NULL;
  GError *error = NULL;

  bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
  if (!bus) {
    UMMS_GERROR ("Couldn't connect to session bus", error);
  }

  proxy = dbus_g_proxy_new_for_name (bus,
          UMMS_SERVICE_NAME,
          obj_path,
          iface_name);

  return proxy;
}

static void
need_reply_cb(DBusGProxy *player)
{
  GError *error = NULL;

  if (!dbus_g_proxy_call (player, "Reply", &error,
       G_TYPE_INVALID, G_TYPE_INVALID)) {
    UMMS_GERROR ("Reply failed", error);
  }
  return;
}

void umms_client_object_remove_player (UmmsClientObject *self, DBusGProxy *player)
{
  const gchar *obj_path = NULL;
  UmmsClientObjectPrivate *priv = GET_PRIVATE (self);
  GError *error = NULL;

  UMMS_DEBUG ("called");

  obj_path = dbus_g_proxy_get_path (player);

  if (!dbus_g_proxy_call (priv->obj_mngr, "RemoveMediaPlayer", &error,
       G_TYPE_STRING, obj_path, G_TYPE_INVALID,
       G_TYPE_INVALID)) {
    UMMS_GERROR ("RemoveMediaPlayer failed", error);
  }
  return;
}
