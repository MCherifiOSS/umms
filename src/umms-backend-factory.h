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

#ifndef _UMMS_BACKEND_FACTORY_H
#define _UMMS_BACKEND_FACTORY_H

#include <glib-object.h>
#include "umms-player-backend.h"
#include "umms-audio-manager-backend.h"

G_BEGIN_DECLS

/*
 * uri:             uri to play
 *
 * Returns:         A UmmsPlayerBackend
 *                  NULL if failed
 *
 * Load player backend according to uri prefix.
 */
UmmsPlayerBackend *umms_player_backend_make_from_uri (const gchar *uri);

G_END_DECLS

#endif /* _UMMS_BACKEND_FACTORY_H */
