#include "ffmpeg_com.h"

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/frame.h"
#include "libavutil/error.h"

void ffmpeg_register_all()
{
    av_register_all();
    avcodec_register_all();
}

void ffmpeg_av_packet_free(AVPacket* p_pkt)
{
    av_packet_free(&p_pkt);
}


void ffmpeg_av_frame_free(AVFrame* p_frame)
{
    av_frame_free(&p_frame);
}
