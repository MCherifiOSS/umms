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

#ifndef _UMMS_ERROR_H
#define _UMMS_ERROR_H
#include <glib.h>

typedef enum {
  UMMS_BACKEND_ERROR_NOT_LOADED = 0,
  UMMS_BACKEND_ERROR_METHOD_NOT_IMPLEMENTED,
  UMMS_BACKEND_ERROR_INVALID_STATE,
  UMMS_BACKEND_ERROR_FAILED,
  UMMS_RESOURCE_ERROR_NO_RESOURCE = 20,
  UMMS_RESOURCE_ERROR_FAILED,
  UMMS_GENERIC_ERROR_CREATING_OBJ_FAILED = 30,
  UMMS_GENERIC_ERROR_INVALID_PARAM,
  UMMS_AUDIO_ERROR_FAILED = 40
} UmmsErrorCode;

GQuark umms_backend_error_quark (void);
GQuark umms_resource_error_quark (void);
GQuark umms_generic_error_quark (void);
GQuark umms_audio_error_quark (void);

#define UMMS_BACKEND_ERROR umms_backend_error_quark()
#define UMMS_RESOURCE_ERROR umms_resource_error_quark()
#define UMMS_GENERIC_ERROR umms_generic_error_quark()
#define UMMS_AUDIO_ERROR umms_audio_error_quark()

#endif /* _UMMS_ERROR_H */

