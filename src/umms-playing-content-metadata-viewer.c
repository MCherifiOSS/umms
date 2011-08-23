#include "umms-debug.h"
#include "umms-common.h"
#include "umms-error.h"
#include "umms-playing-content-metadata-viewer.h"
#include "umms-object-manager.h"
#include "meego-media-player.h"


G_DEFINE_TYPE (UmmsPlayingContentMetadataViewer, umms_playing_content_metadata_viewer, G_TYPE_OBJECT)
#define PLAYER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), UMMS_TYPE_PLAYING_CONTENT_METADATA_VIEWER, UmmsPlayingContentMetadataViewerPrivate))

#define GET_PRIVATE(o) ((UmmsPlayingContentMetadataViewer *)o)->priv

struct _UmmsPlayingContentMetadataViewerPrivate {
  UmmsObjectManager *obj_mngr; 
};

/* props */
enum
{
  PROP_OBJECT_MANAGER = 1
};

enum
{
  SIGNAL_METADATA_UPDATED,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = {0};

static void
umms_playing_content_metadata_viewer_get_property (GObject    *object,
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
player_added_cb(UmmsObjectManager *obj_mngr, MeegoMediaPlayer *player, UmmsPlayingContentMetadataViewer *viewer)
{
  UMMS_DEBUG ("Begin");
//TODO: connect the player's metadata updated singal.
  UMMS_DEBUG ("End");
}

static void
umms_playing_content_metadata_viewer_set_property (GObject      *object,
    guint         property_id,
    const GValue *value,
    GParamSpec   *pspec)
{
  UmmsPlayingContentMetadataViewerPrivate *priv = GET_PRIVATE(object);

  switch (property_id) {
    case PROP_OBJECT_MANAGER:
      {
        GList *players;
        priv->obj_mngr = g_value_get_object (value);
        g_object_ref (priv->obj_mngr);
        g_signal_connect (priv->obj_mngr, "player-added", G_CALLBACK(player_added_cb), object);
        break;
      }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
umms_playing_content_metadata_viewer_dispose (GObject *object)
{
  UmmsPlayingContentMetadataViewerPrivate *priv = GET_PRIVATE (object);

  if (priv->obj_mngr)
    g_object_unref (priv->obj_mngr);

  G_OBJECT_CLASS (umms_playing_content_metadata_viewer_parent_class)->dispose (object);
}

static void
umms_playing_content_metadata_viewer_finalize (GObject *object)
{
  G_OBJECT_CLASS (umms_playing_content_metadata_viewer_parent_class)->finalize (object);
}

static GType 
get_metadata_type ()
{
  return dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE);
}

static void
umms_playing_content_metadata_viewer_class_init (UmmsPlayingContentMetadataViewerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (UmmsPlayingContentMetadataViewerPrivate));

  object_class->get_property = umms_playing_content_metadata_viewer_get_property;
  object_class->set_property = umms_playing_content_metadata_viewer_set_property;
  object_class->dispose = umms_playing_content_metadata_viewer_dispose;
  object_class->finalize = umms_playing_content_metadata_viewer_finalize;

  g_object_class_install_property (object_class, PROP_OBJECT_MANAGER,
      g_param_spec_object ("umms-object-manager", "UMMS object manager",
        "UMMS object manager provides access to all MediaPlayer object",
        UMMS_TYPE_OBJECT_MANAGER, G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  signals[SIGNAL_METADATA_UPDATED] =
    g_signal_new ("metadata-updated",
        G_OBJECT_CLASS_TYPE (klass),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL,
        g_cclosure_marshal_VOID__BOXED,
        G_TYPE_NONE,
        1, dbus_g_type_get_collection("GPtrArray", get_metadata_type()));
}

static void
umms_playing_content_metadata_viewer_init (UmmsPlayingContentMetadataViewer *self)
{
  UmmsPlayingContentMetadataViewerPrivate *priv;

  self->priv = PLAYER_PRIVATE (self);
  priv = self->priv;
}

UmmsPlayingContentMetadataViewer *
umms_playing_content_metadata_viewer_new (UmmsObjectManager *obj_mngr)
{
  return g_object_new (UMMS_TYPE_PLAYING_CONTENT_METADATA_VIEWER, "umms-object-manager", obj_mngr, NULL);
}

//TODO: connect this callback to MediaPlayer signal.
static void
_metadata_updated_cb (MeegoMediaPlayer *player, UmmsPlayingContentMetadataViewer *self)
{
  GPtrArray *metadata;
  if (umms_playing_content_metadata_viewer_get_playing_content_metadata (self, &metadata, NULL))
    g_signal_emit (self, signals[SIGNAL_METADATA_UPDATED], 0, metadata);
  else
    UMMS_DEBUG ("getting content metadata failed");
}

#if 1
//just for testing, remove it when the actual function accomplished
gboolean 
umms_playing_content_metadata_viewer_get_playing_content_metadata (UmmsPlayingContentMetadataViewer *self, GPtrArray **metadata, GError **err)
{
#define NUM_TABLE 0
  GHashTable *ht;
  GValue *val;
  gint i;

  UMMS_DEBUG ("Begin");
  *metadata = g_ptr_array_new ();
  if (!*metadata) {
    g_assert (err == NULL || *err == NULL);
    if (err != NULL)
      g_set_error (err, UMMS_GENERIC_ERROR, UMMS_GENERIC_ERROR_CREATING_OBJ_FAILED, "Creating GPtrArray metadata failed");
    return FALSE;
  }

  for (i=0; i<NUM_TABLE; i++) {
      /*TODO:
        1. get tags of URI, Title, Artist.
        2. Fill the metadata.
        */
      ht = g_hash_table_new (NULL, NULL);

      val = g_new0(GValue, 1);
      g_value_init (val, G_TYPE_STRING);
      g_value_set_string (val, "uri");//set URI
      g_hash_table_insert (ht, "URI", val);

      val = g_new0(GValue, 1);
      g_value_init (val, G_TYPE_STRING);
      g_value_set_string (val, "title");//set Title
      g_hash_table_insert (ht, "Title", val);

      val = g_new0(GValue, 1);
      g_value_init (val, G_TYPE_STRING);
      g_value_set_string (val, "artist");//set Artist
      g_hash_table_insert (ht, "Artist", val);

      g_ptr_array_add (*metadata, ht);
  }
  UMMS_DEBUG ("End");
  return TRUE;
}
#else
gboolean 
umms_playing_content_metadata_viewer_get_playing_content_metadata (UmmsPlayingContentMetadataViewer *self, GPtrArray **metadata, GError **err)
{
  GHashTable *ht;
  GValue *val;
  gint i, state;
  GList *players, *item;
  MeegoMediaPlayer *player;
  UmmsPlayingContentMetadataViewerPrivate *priv = GET_PRIVATE (self);

  UMMS_DEBUG ("Begin");
  *metadata = g_ptr_array_new ();
  if (!*metadata) {
    g_assert (err == NULL || *err == NULL);
    if (err != NULL)
      g_set_error (err, UMMS_GENERIC_ERROR, UMMS_GENERIC_ERROR_CREATING_OBJ_FAILED, "Creating GPtrArray metadata failed");
    return FALSE;
  }

  players = umms_object_manager_get_player_list (priv->obj_mngr);
  for (i=0; i<g_list_length(players); i++) {

    item = g_list_nth (players, i);
    player = MEEGO_MEDIA_PLAYER (item->data);
    meego_media_player_get_player_state (player,  &state, NULL);

    if (state == PlayerStatePlaying || state == PlayerStatePaused) {
      /*TODO:
        1. get tags of URI, Title, Artist.
        2. Fill the metadata.
        */
      ht = g_hash_table_new (NULL, NULL);

      val = g_new0(GValue, 1);
      g_value_init (val, G_TYPE_STRING);
      g_value_set_string (val, "");//set URI
      g_hash_table_insert (ht, "URI", val);

      val = g_new0(GValue, 1);
      g_value_init (val, G_TYPE_STRING);
      g_value_set_string (val, "");//set Title
      g_hash_table_insert (ht, "Title", val);

      val = g_new0(GValue, 1);
      g_value_init (val, G_TYPE_STRING);
      g_value_set_string (val, "");//set Artist
      g_hash_table_insert (ht, "Artist", val);

      g_ptr_array_add (*metadata, ht);
    }
  }
  UMMS_DEBUG ("End");

  return TRUE;
}
#endif


