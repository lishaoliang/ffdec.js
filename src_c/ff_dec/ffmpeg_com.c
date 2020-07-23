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
