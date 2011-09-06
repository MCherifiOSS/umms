
#ifndef AV_DECODER_H_
#define AV_DECODER_H_

gint avdec_init(void);
gint avdec_set_source(gchar *filename);
gint avdec_start(void);
gint avdec_stop(void);
gint avdec_pause(void);
gint avdec_resume(void);
gint avdec_seek_from_beginning(gint64 nanosecond);

#endif /* AV_DECODER_H_ */
