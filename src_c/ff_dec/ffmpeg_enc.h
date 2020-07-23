#ifndef __FFMPEG_ENC_H__
#define __FFMPEG_ENC_H__

#include "klb_type.h"
#include "em_frame_yuv_wav.h"
#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct ffmpeg_enc_t_ ffmpeg_enc_t;

ffmpeg_enc_t* ffmpeg_enc_create(enum AVCodecID id /*=AV_CODEC_ID_H264*/);
void ffmpeg_enc_destroy(ffmpeg_enc_t* p_enc);

enum AVCodecID ffmpeg_enc_id(ffmpeg_enc_t* p_enc);

AVPacket* ffmpeg_enc_encode(ffmpeg_enc_t* p_enc, AVFrame* p_frame);

void ffmpeg_enc_av_packet_free(AVPacket* p_av_packet);


#ifdef __cplusplus
}
#endif


#endif // __FFMPEG_ENC_H__
//end
