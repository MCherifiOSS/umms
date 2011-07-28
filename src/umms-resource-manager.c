#include "umms-debug.h"
#include "umms-resource-manager.h"

G_DEFINE_TYPE (UmmsResourceManager, umms_resource_manager, G_TYPE_OBJECT)
#define MANAGER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), UMMS_TYPE_RESOURCE_MANAGER, UmmsResourceManagerPrivate))

#define GET_PRIVATE(o) ((UmmsResourceManager *)o)->priv

//UPP_A and UPP_B
#define MAX_ACTIVE_PLANE 2
typedef struct _ResourceMngr{
}ResourceMngr;


struct _UmmsResourceManagerPrivate {
  gint active_plane_cnt;
  GMutex *lock;
};

static void
umms_resource_manager_get_property (GObject    *object,
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
umms_resource_manager_set_property (GObject      *object,
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
umms_resource_manager_dispose (GObject *object)
{
  UmmsResourceManagerPrivate *priv = GET_PRIVATE (object);

  g_mutex_free(priv->lock);

  G_OBJECT_CLASS (umms_resource_manager_parent_class)->dispose (object);
}

static void
umms_resource_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (umms_resource_manager_parent_class)->finalize (object);
}

static void
umms_resource_manager_class_init (UmmsResourceManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  UmmsResourceManagerClass *p_class = UMMS_RESOURCE_MANAGER_CLASS (klass);

  g_type_class_add_private (klass, sizeof (UmmsResourceManagerPrivate));

  object_class->get_property = umms_resource_manager_get_property;
  object_class->set_property = umms_resource_manager_set_property;
  object_class->dispose = umms_resource_manager_dispose;
  object_class->finalize = umms_resource_manager_finalize;

}

static void
umms_resource_manager_init (UmmsResourceManager *self)
{
  UmmsResourceManagerPrivate *priv;

  self->priv = MANAGER_PRIVATE (self);
  priv = self->priv;

  priv->lock = g_mutex_new ();
  priv->active_plane_cnt = 0;
}

static UmmsResourceManager *mngr_global = NULL;

UmmsResourceManager *
umms_resource_manager_new (void)
{
  if (!mngr_global) {
    mngr_global = g_object_new (UMMS_TYPE_RESOURCE_MANAGER, NULL);
  }
  return mngr_global;
}

gboolean 
_request_plane_resource_unlock (UmmsResourceManager *self)
{
  gboolean ret;
  UmmsResourceManagerPrivate *priv = GET_PRIVATE (self);
  if (priv->active_plane_cnt < MAX_ACTIVE_PLANE) {
    priv->active_plane_cnt++;
    ret = TRUE;
  } else {
    ret = FALSE;
  }
  UMMS_DEBUG ("active_plane_cnt = %d, ret = %d", priv->active_plane_cnt, ret);
  return ret;
}

gboolean 
_release_plane_resource_unlock (UmmsResourceManager *self)
{
  UmmsResourceManagerPrivate *priv = GET_PRIVATE (self);

  if (priv->active_plane_cnt > 0) {
    priv->active_plane_cnt--;
  }

  UMMS_DEBUG ("active_plane_cnt = %d", priv->active_plane_cnt);
  return TRUE;
}

gboolean 
umms_resource_manager_request_resource (UmmsResourceManager *self, ResourceType type)
{
  gboolean result = FALSE;
  UmmsResourceManagerPrivate *priv = GET_PRIVATE (self);

  g_mutex_lock (priv->lock);
  switch (type) {
    case ResourceTypePlane:
      result = _request_plane_resource_unlock (self);
      break;
    default:
      break;
  }
  g_mutex_unlock (priv->lock);
  return result;
}

gboolean umms_resource_manager_release_resource (UmmsResourceManager *self, ResourceType type) 
{
  UmmsResourceManagerPrivate *priv = GET_PRIVATE (self);

  g_mutex_lock (priv->lock);
  switch (type) {
    case ResourceTypePlane:
      _release_plane_resource_unlock (self);
      break;
    default:
      break;
  }
  g_mutex_unlock (priv->lock);
}


