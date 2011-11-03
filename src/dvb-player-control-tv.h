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

#ifndef _DVB_PLAYER_CONTROL_TV_H
#define _DVB_PLAYER_CONTROL_TV_H

#include <glib-object.h>
#include "dvb-player-control-base.h" 

G_BEGIN_DECLS

#define DVB_PLAYER_CONTROL_TYPE_TV dvb_player_control_tv_get_type()

#define DVB_PLAYER_CONTROL_TV(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  DVB_PLAYER_CONTROL_TYPE_TV, DvbPlayerControlTv))

#define DVB_PLAYER_CONTROL_TV_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  DVB_PLAYER_CONTROL_TYPE_TV, DvbPlayerControlTvClass))

#define DVB_PLAYER_CONTROL_IS_TV(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  DVB_PLAYER_CONTROL_TYPE_TV))

#define DVB_PLAYER_CONTROL_IS_TV_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  DVB_PLAYER_CONTROL_TYPE_TV))

#define DVB_PLAYER_CONTROL_TV_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  DVB_PLAYER_CONTROL_TYPE_TV, DvbPlayerControlTvClass))

typedef struct _DvbPlayerControlTv DvbPlayerControlTv;
typedef struct _DvbPlayerControlTvClass DvbPlayerControlTvClass;

struct _DvbPlayerControlTv
{
  //  GObject parent;
  DvbPlayerControlBase parent;

};

struct _DvbPlayerControlTvClass
{
  DvbPlayerControlBaseClass parent_class;
};

GType dvb_player_control_tv_get_type (void) G_GNUC_CONST;

DvbPlayerControlTv *dvb_player_control_tv_new (void);

G_END_DECLS

#endif /* _DVB_PLAYER_CONTROL_TV_H */
