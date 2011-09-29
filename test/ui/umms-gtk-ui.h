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

#ifndef UI_H_
#define UI_H_

extern GtkWidget *video_window;
extern GtkWidget *window;

/***
 * @brief Create UI main framework
 *
 * @return 0 if ok, otherwise it is the error code
 */
gint ui_create(void);

/***
 * @brief Application main loop
 *
 * @return none
 */
void ui_main_loop(void);

/***
 * @brief send stop player signal, this function will change the icon, and stop the player.
 *
 * @return none
 */
void ui_send_stop_signal(void);
void ui_update_progressbar(gint64 pos, gint64 len);
void ui_update_channels(void);


#endif /* UI_H_ */
