#ifndef __FFMPEG_COM_H__
#define __FFMPEG_COM_H__

#include "klb_type.h"
#include "ffmpeg_dec.h"
#include "ffmpeg_enc.h"
#include "ffmpeg_input.h"


void ffmpeg_register_all();


void ffmpeg_av_packet_free(AVPacket* p_pkt);


void ffmpeg_av_frame_free(AVFrame* p_frame);


#endif // __FFMPEG_COM_H__
//end
