#include "ffmpeg_enc.h"
#include "mem/klb_mem.h"
#include "em_util/em_log.h"
#include <assert.h>

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/frame.h"
#include "libavutil/error.h"
#include <libavutil/opt.h>


typedef struct ffmpeg_enc_t_
{
    AVFormatContext*    p_fmt_context;
    AVStream*           p_stream;

    AVCodec*            p_codec;
    AVCodecContext*     p_context;

    AVPacket*           p_packet;

    AVFrame*            p_avframe;
    struct SwsContext*  p_sws_ctx;

    enum AVCodecID      id;
}ffmpeg_enc_t;


static void jpg_quality(AVCodecContext* p_codec_ctx, int quality_level)
{
    //图片质量的选择
    if (quality_level == 0)
    {
        p_codec_ctx->global_quality = 40;
        p_codec_ctx->qmin = 10;
        p_codec_ctx->qmax = 65;
    }
    else if (quality_level == 1)
    {
        p_codec_ctx->global_quality = 50;
        p_codec_ctx->qmin = 10;
        p_codec_ctx->qmax = 60;
    }
    else if (quality_level == 2)
    {
        p_codec_ctx->global_quality = 60;
        p_codec_ctx->qmin = 10;
        p_codec_ctx->qmax = 55;
    }
    else if (quality_level == 3)
    {
        p_codec_ctx->global_quality = 70;
        p_codec_ctx->qmin = 10;
        p_codec_ctx->qmax = 50;
    }
    else if (quality_level == 4)
    {
        p_codec_ctx->global_quality = 80;
        p_codec_ctx->qmin = 10;
        p_codec_ctx->qmax = 45;
    }
    else if (quality_level == 5)
    {
        p_codec_ctx->global_quality = 90;
        p_codec_ctx->qmin = 10;
        p_codec_ctx->qmax = 40;
    }
    else if (quality_level == 6)
    {
        p_codec_ctx->global_quality = 100;
        p_codec_ctx->qmin = 10;
        p_codec_ctx->qmax = 35;
    }
    else
    {
        p_codec_ctx->global_quality = 80;
        p_codec_ctx->qmin = 10;
        p_codec_ctx->qmax = 50;
    }
}

ffmpeg_enc_t* ffmpeg_enc_create(enum AVCodecID id /*=AV_CODEC_ID_H264*/)
{
    id = AV_CODEC_ID_H264;

    ffmpeg_enc_t* p_dec = KLB_MALLOC(ffmpeg_enc_t, 1, 0);
    KLB_MEMSET(p_dec, 0, sizeof(ffmpeg_enc_t));

    p_dec->p_fmt_context = avformat_alloc_context();
    assert(NULL != p_dec->p_fmt_context);
    p_dec->p_fmt_context->oformat = av_guess_format("mjpeg", NULL, NULL);
    p_dec->p_stream = avformat_new_stream(p_dec->p_fmt_context, 0);
    assert(NULL != p_dec->p_stream);

    enum AVCodecID codec_id = id;// AV_CODEC_ID_MJPEG, AV_CODEC_ID_H264, AV_CODEC_ID_H265

    p_dec->p_codec = avcodec_find_decoder(codec_id);
    assert(NULL != p_dec->p_codec);
    p_dec->p_context = avcodec_alloc_context3(p_dec->p_codec);
    assert(NULL != p_dec->p_context);

#if 0
    p_dec->p_context->pix_fmt = AV_PIX_FMT_YUVJ420P;
    p_dec->p_context->width = 432;
    p_dec->p_context->height = 240;
    p_dec->p_context->time_base.num = 1;
    p_dec->p_context->time_base.den = 25;

    jpg_quality(p_dec->p_context, 90);

    p_dec->p_context->qcompress = (float)p_dec->p_context->global_quality / 100.f; // 0~1.0, def
#else
    /* put sample parameters */
    p_dec->p_context->bit_rate = 400000;
    /* resolution must be a multiple of two */
    p_dec->p_context->width = 432;
    p_dec->p_context->height = 240;
    /* frames per second */
    p_dec->p_context->time_base = (AVRational) { 1, 25 };
    p_dec->p_context->framerate = (AVRational) { 25, 1 };

    /* emit one intra frame every ten frames
    * check frame pict_type before passing frame
    * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
    * then gop_size is ignored and the output of encoder
    * will always be I frame irrespective to gop_size
    */
    p_dec->p_context->gop_size = 10;
    p_dec->p_context->max_b_frames = 1;
    p_dec->p_context->pix_fmt = AV_PIX_FMT_YUV420P;

    if (p_dec->p_context->codec_id == AV_CODEC_ID_H264)
    {
        av_opt_set(p_dec->p_context->priv_data, "preset", "slow", 0);
    }
#endif

    if (avcodec_open2(p_dec->p_context, p_dec->p_codec, NULL) < 0)
    {
        assert(false);
    }

    p_dec->id = id;

    return p_dec;
}

void ffmpeg_enc_destroy(ffmpeg_enc_t* p_enc)
{
    assert(NULL != p_enc);

    if (NULL != p_enc->p_avframe)
    {
        av_frame_free(&p_enc->p_avframe);
    }

    if (NULL != p_enc->p_sws_ctx)
    {
        sws_freeContext(p_enc->p_sws_ctx);
    }

    if (NULL != p_enc->p_context)
    {
        avcodec_close(p_enc->p_context);
        avcodec_free_context(&(p_enc->p_context));
    }

    KLB_FREE(p_enc);
}

enum AVCodecID ffmpeg_enc_id(ffmpeg_enc_t* p_enc)
{
    return p_enc->id;
}

AVPacket* ffmpeg_enc_encode(ffmpeg_enc_t* p_enc, AVFrame* p_frame)
{
    assert(NULL != p_enc);
    AVCodecContext* p_context = p_enc->p_context;

#if 0
    avpicture_fill((AVPicture *)p_frame, p_frame->data[0], p_context->pix_fmt, p_context->width, p_context->height);

    //Write Header
    avformat_write_header(p_enc->p_fmt_context, NULL);

    AVPacket* p_pkt = av_packet_alloc();
    av_new_packet(p_pkt, p_context->width * p_context->height * 3);

    //Encode
    int got_picture = 0;
    int ret = avcodec_encode_video2(p_context, p_pkt, p_frame, &got_picture);
    if (ret < 0)
    {
        LOG("avcodec_encode_video2 error.\n");
    }

    if (got_picture == 1)
    {
        p_pkt->stream_index = p_enc->p_stream->index;

        //  p_pkt->data, p_pkt->size
        return p_pkt;
    }

    av_free_packet(p_pkt);
#else
    //avpicture_fill((AVPicture *)p_frame, p_frame->data[0], p_context->pix_fmt, p_context->width, p_context->height);
    //avformat_write_header(p_enc->p_fmt_context, NULL);

    int ret = avcodec_send_frame(p_context, p_frame);
    if (ret < 0)
    {
        LOG("avcodec_send_frame error!code:[%d]", ret);
    }
    else
    {
        if (NULL == p_enc->p_packet)
        {
            p_enc->p_packet = av_packet_alloc();
        }

        ret = avcodec_receive_packet(p_context, p_enc->p_packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            return NULL;
        }
        else if (ret < 0) 
        {
            LOG("avcodec_receive_packet error!");
            return NULL;
        }

        AVPacket* p_pkt = p_enc->p_packet;
        p_enc->p_packet = NULL;

        p_pkt->stream_index = p_enc->p_stream->index;
        return p_pkt;
    }
#endif

    return NULL; //未得到帧, 还需要继续送数据
}

void ffmpeg_enc_av_packet_free(AVPacket* p_av_packet)
{
    if (NULL != p_av_packet)
    {
        av_free_packet(p_av_packet);
    }
}
