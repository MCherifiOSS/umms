#include "audio-manager-interface.h"

struct _AudioManagerInterfaceClass {
  GTypeInterface parent_class;
  audio_manager_interface_set_volume_impl set_volume;
  audio_manager_interface_get_volume_impl get_volume;
  audio_manager_interface_set_state_impl  set_state;
  audio_manager_interface_get_state_impl  get_state;
};

static void audio_manager_interface_base_init (gpointer klass);

GType
audio_manager_interface_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0)) {
    static const GTypeInfo info = {
      sizeof (AudioManagerInterfaceClass),
      audio_manager_interface_base_init, /* base_init */
      NULL, /* base_finalize */
      NULL, /* class_init */
      NULL, /* class_finalize */
      NULL, /* class_data */
      0,
      0, /* n_preallocs */
      NULL /* instance_init */
    };

    type = g_type_register_static (G_TYPE_INTERFACE,
           "AudioManagerInterface", &info, 0);
  }

  return type;
}

gboolean
audio_manager_interface_set_volume (AudioManagerInterface *self,
    gint type, gint volume)
{
  audio_manager_interface_set_volume_impl impl = (AUDIO_MANAGER_INTERFACE_GET_CLASS (self)->set_volume);

  if (impl != NULL) {
    (impl) (self, type,
            volume);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void audio_manager_interface_implement_set_volume (AudioManagerInterfaceClass *klass,
                                                   audio_manager_interface_set_volume_impl impl)
{
  klass->set_volume = impl;
}

gboolean
audio_manager_interface_get_volume (AudioManagerInterface *self,
    gint type, gint *volume)
{
  audio_manager_interface_get_volume_impl impl = (AUDIO_MANAGER_INTERFACE_GET_CLASS (self)->get_volume);

  if (impl != NULL) {
    (impl) (self, type,
            volume);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void audio_manager_interface_implement_get_volume (AudioManagerInterfaceClass *klass,
                                                   audio_manager_interface_get_volume_impl impl)
{
  klass->get_volume = impl;
}

gboolean
audio_manager_interface_set_state (AudioManagerInterface *self,
    gint type, gint state)
{
  audio_manager_interface_set_state_impl impl = (AUDIO_MANAGER_INTERFACE_GET_CLASS (self)->set_state);

  if (impl != NULL) {
    (impl) (self, type,
            state);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void audio_manager_interface_implement_set_state (AudioManagerInterfaceClass *klass,
                                                   audio_manager_interface_set_state_impl impl)
{
  klass->set_state = impl;
}

gboolean
audio_manager_interface_get_state (AudioManagerInterface *self,
    gint type, gint *state)
{
  audio_manager_interface_get_state_impl impl = (AUDIO_MANAGER_INTERFACE_GET_CLASS (self)->get_state);

  if (impl != NULL) {
    (impl) (self, type,
            state);
  } else {
    g_warning ("Method not implemented\n");
  }
  return TRUE;
}

void audio_manager_interface_implement_get_state (AudioManagerInterfaceClass *klass,
                                                   audio_manager_interface_get_state_impl impl)
{
  klass->get_state = impl;
}

static inline void
audio_manager_interface_base_init_once (gpointer klass G_GNUC_UNUSED)
{

}

static void
audio_manager_interface_base_init (gpointer klass)
{
  static gboolean initialized = FALSE;

  if (!initialized) {
    initialized = TRUE;
    audio_manager_interface_base_init_once (klass);
  }
}