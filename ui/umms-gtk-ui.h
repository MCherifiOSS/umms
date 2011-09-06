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

#endif /* UI_H_ */
