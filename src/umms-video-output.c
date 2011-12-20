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

#include <dlfcn.h>
#include "umms-debug.h"
#include "umms-common.h"
#include "umms-video-output.h"

G_DEFINE_TYPE (UmmsVideoOutput, umms_video_output, G_TYPE_OBJECT)

#define VIDEO_OUTPUT_PRIVATE(o) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((o), UMMS_TYPE_VIDEO_OUTPUT, UmmsVideoOutputPrivate))
#define GET_PRIVATE(o) ((UmmsVideoOutput *)o)->priv

struct _UmmsVideoOutputPrivate {
  int dummy;
};

static void
umms_video_output_get_property (GObject    *object,
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
umms_video_output_set_property (GObject      *object,
    guint         property_id,
    const GValue *value,
    GParamSpec   *pspec)
{
  UmmsVideoOutputPrivate *priv = GET_PRIVATE (object);
  gint tmp;
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
umms_video_output_dispose (GObject *object)
{
  UmmsVideoOutputPrivate *priv = GET_PRIVATE (object);

  G_OBJECT_CLASS (umms_video_output_parent_class)->dispose (object);
}

static void
umms_video_output_finalize (GObject *object)
{
  G_OBJECT_CLASS (umms_video_output_parent_class)->finalize (object);
}

static UmmsVideoOutput *the_only = NULL;

static void
umms_video_output_class_init (UmmsVideoOutputClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (UmmsVideoOutputPrivate));

  object_class->get_property = umms_video_output_get_property;
  object_class->set_property = umms_video_output_set_property;
  object_class->dispose = umms_video_output_dispose;
  object_class->finalize = umms_video_output_finalize;
}

static void
umms_video_output_init (UmmsVideoOutput *self)
{
  void *module = NULL;
  gchar *plugin_dir = NULL;
  gchar *path[128];

  memset (&self->video_output_if, 0, sizeof (UmmsVideoOutputInterface));
  plugin_dir = getenv ("UMMS_PLUGINS_PATH");

  if (plugin_dir)
    snprintf(path, sizeof path, "%s/%s", plugin_dir, "libummsvideooutput.so");
  else 
    snprintf(path, sizeof path, "%s", "libummsvideooutput.so");

  module = dlopen(path, RTLD_LAZY);
  if (!module) {
    UMMS_DEBUG ("failed to load module: %s\n", dlerror());
    return;
  }

  self->video_output_if.init = dlsym(module, "init");
  if (!self->video_output_if.init) {
    UMMS_DEBUG ("failed to lookup init function: %s\n", dlerror());
    return;
  }

  if (self->video_output_if.init (self) == FALSE) {
    UMMS_DEBUG ("Video output init failed");
  }

  return;
}

UmmsVideoOutput *
umms_video_output_new (gint platform)
{
  return g_object_new (UMMS_TYPE_VIDEO_OUTPUT, NULL);
}

gboolean umms_video_output_get_valid_video_output(UmmsVideoOutput *self, gchar ***outputs, GError **err)
{
  gboolean ret = TRUE;
  
  if (!self->video_output_if.get_valid_video_output) {
    g_warning ("Method not implemented\n");
  } else {
    ret = self->video_output_if.get_valid_video_output (self, outputs);
  }

  return ret;
}

gboolean umms_video_output_get_valid_mode (UmmsVideoOutput *self, const char* output_name, gchar ***modes, GError **err)
{
  gboolean ret = TRUE;
  
  if (!self->video_output_if.get_valid_mode) {
    g_warning ("Method not implemented\n");
  } else {
    ret = self->video_output_if.get_valid_mode (self, output_name, modes);
  }

  return ret;
}

gboolean umms_video_output_set_mode(UmmsVideoOutput *self, const char *output_name, const char *mode, GError **err)
{
  gboolean ret = TRUE;
  
  if (!self->video_output_if.set_mode) {
    g_warning ("Method not implemented\n");
  } else {
    ret = self->video_output_if.set_mode (self, output_name, mode);
  }

  return ret;
}

gboolean umms_video_output_get_mode(UmmsVideoOutput *self, const char *output_name, char **mode, GError **err)
{
  gboolean ret = TRUE;

  if (!self->video_output_if.get_mode) {
    g_warning ("Method not implemented\n");
  } else {
    ret = self->video_output_if.get_mode (self, output_name, mode);
  }

  return ret;
}
