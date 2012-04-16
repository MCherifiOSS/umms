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

#ifndef _UMMS_DEBUG_H
#define _UMMS_DEBUG_H

#define UMMS_ENABLE_DEBUG

#ifdef UMMS_ENABLE_DEBUG
#define UMMS_DEBUG(format, ...) \
    g_debug (G_STRLOC " :%s(): " format, G_STRFUNC, ##__VA_ARGS__)
#define UMMS_WARNING(format, ...) \
    g_debug (G_STRLOC " Warning :%s(): " format, G_STRFUNC, ##__VA_ARGS__)

#else
#define UMMS_DEBUG(format, ...)
#define UMMS_WARNING(format, ...)
#endif

#include <stdlib.h>
#define UMMS_GERROR(prefix, gerror) \
          do {\
                g_print (prefix ":%s", error->message); \
                g_error_free(gerror); \
          }while(0)

#endif /* _UMMS_DEBUG_H */

