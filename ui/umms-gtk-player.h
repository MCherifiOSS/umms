
#ifndef PLAYER_CORE_H_
#define PLAYER_CORE_H_

typedef enum _PlyMainState {
    PLY_MAIN_STATE_IDLE,
    PLY_MAIN_STATE_READY,
    PLY_MAIN_STATE_RUN,
    PLY_MAIN_STATE_PAUSE,
}PlyMainState;

typedef struct _PlyMainData {
    PlyMainState state;
    GString filename;
    gint64 duration_nanosecond;
}PlyMainData;

/***
 * @brief Play Core module init
 *
 * @return 0 if ok, otherwise it is the error code
 */
gint ply_init(void);

PlyMainData *ply_get_maindata(void);

/***
 * @brief Get player main state
 *
 * @return PlyMainState
 */
PlyMainState ply_get_state(void);

/***
 * @brief Reload file and play it, this function returns without block.
 *
 * @return 0 if ok, otherwise it is the error code
 */
gint ply_reload_file(gchar *file_name);

/***
 * @brief Play the stream from the very beginning, this function just needs to send the message to AV decoder thread, and returns without block.
 *
 * @return 0 if ok, otherwise it is the error code
 */
gint ply_play_stream(void);

/***
 * @brief Stop the stream, this function just needs to send the message to AV decoder thread, and returns without block.
 *
 * @return 0 if ok, otherwise it is the error code
 */
gint ply_stop_stream(void);

/***
 * @brief Pause the stream, this function just needs to send the message to AV decoder thread, and returns without block.
 *
 * @return 0 if ok, otherwise it is the error code
 */
gint ply_pause_stream(void);

/***
 * @brief Resume the paused stream, this function just needs to send the message to AV decoder thread, and returns without block.
 *
 * @return 0 if ok, otherwise it is the error code
 */
gint ply_resume_stream(void);

/***
 * @brief Seek the stream, this function just needs to send the message to AV decoder thread, and returns without block.
 *
 * @param ts if negative means rewind, if positive means forward, the unit is second
 * @return 0 if ok, otherwise it is the error code
 */
gint ply_seek_stream_from_beginging(gint64 nanosecond);

#endif /* PLAYER_CORE_H_ */
