#ifndef __FFMPEG_INPUT_H__
#define __FFMPEG_INPUT_H__

#include "klb_type.h"

#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
#include "em_frame_yuv_wav.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define FFMPEG_AVPACKET_VIDEO       1
#define FFMPEG_AVPACKET_AUDIO       2


typedef struct ffmpeg_input_t_ ffmpeg_input_t;

ffmpeg_input_t* ffmpeg_input_create(const char* p_filename);
void ffmpeg_input_destroy(ffmpeg_input_t* p_input);

enum AVCodecID ffmpeg_input_video_id(ffmpeg_input_t* p_input);


AVPacket* ffmpeg_input_read(ffmpeg_input_t* p_input, int* p_out_type/*FFMPEG_AVPACKET_VIDEO,FFMPEG_AVPACKET_AUDIO*/);


AVFrame* ffmpeg_input_decode_video(ffmpeg_input_t* p_input, AVPacket* p_pkt);


em_frame_yuv_wav_t* ffmpeg_input_read_frame(ffmpeg_input_t* p_input);

#ifdef __cplusplus
}
#endif

#endif // __FFMPEG_INPUT_H__
//end
