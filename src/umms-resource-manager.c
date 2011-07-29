#include "umms-debug.h"
#include "umms-common.h"
#include "umms-resource-manager.h"

G_DEFINE_TYPE (UmmsResourceManager, umms_resource_manager, G_TYPE_OBJECT)
#define MANAGER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), UMMS_TYPE_RESOURCE_MANAGER, UmmsResourceManagerPrivate))

#define GET_PRIVATE(o) ((UmmsResourceManager *)o)->priv

//UPP_A and UPP_B
#define MAX_PLANE 2
typedef struct _ResourceMngr{
}ResourceMngr;

struct _Plane{
  gint     id;
  gboolean used;
};

struct _Plane planes[MAX_PLANE] = {{UPP_A, FALSE},{UPP_B, FALSE}};

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

static Resource *
_request_plane_resource_unlock (UmmsResourceManager *self, gint prefer_plane)
{
  Resource *res = NULL;
  gint i;

  //Respect the preference given by client.
  if (prefer_plane != INVALID_RES_HANDLE) {
    for (i=0; i<MAX_PLANE; i++) {
      if ((planes[i].id == prefer_plane) && (planes[i].used == FALSE)) {
        planes[i].used = TRUE;
        goto done;
      }
    }
    UMMS_DEBUG ("Prefer plane(%d) is not available", prefer_plane);
  }

  //Find the first available plane.
  for (i=0; i<MAX_PLANE; i++) {
    if (!planes[i].used) {
      planes[i].used = TRUE;
      goto done;
    }
  }
  UMMS_DEBUG ("No available plane");
  return NULL;

done:
  res = (Resource *)g_malloc (sizeof(Resource));
  if (!res) {
    UMMS_DEBUG ("malloc failed");
    return NULL;
  }
  res->type = ResourceTypePlane;
  res->handle = planes[i].id;
  UMMS_DEBUG ("Return the avaliable plane(%d)", res->handle);
  return res;
}

static void
_release_plane_resource_unlock (UmmsResourceManager *self, Resource *res)
{
  gint i;

  for (i=0; i<MAX_PLANE; i++) {
    if (planes[i].id == res->handle) {
      planes[i].used = FALSE;
      g_free (res);
      UMMS_DEBUG ("plane '%d' released", planes[i].id);
      return;
    }
  }

  UMMS_DEBUG ("Invalid plane id '%d'", res->handle);
  return;
}

Resource *
umms_resource_manager_request_resource (UmmsResourceManager *self, ResourceRequest *req)
{
  UmmsResourceManagerPrivate *priv;
  Resource *result = NULL;

  g_return_val_if_fail (self, NULL);
  g_return_val_if_fail (req, NULL);

  priv = GET_PRIVATE (self);
  g_mutex_lock (priv->lock);
  switch (req->type) {
    case ResourceTypePlane:
      result = _request_plane_resource_unlock (self, req->preference);
      break;
    default:
      break;
  }
  g_mutex_unlock (priv->lock);
  return result;
}

void 
umms_resource_manager_release_resource (UmmsResourceManager *self, Resource *res)
{
  UmmsResourceManagerPrivate *priv;

  g_return_if_fail (self && res);

  priv = GET_PRIVATE (self);

  g_mutex_lock (priv->lock);
  switch (res->type) {
    case ResourceTypePlane:
      _release_plane_resource_unlock (self, res);
      break;
    default:
      break;
  }
  g_mutex_unlock (priv->lock);
}


