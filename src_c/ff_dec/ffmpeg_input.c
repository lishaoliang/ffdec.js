#include "ffmpeg_input.h"
#include "ffmpeg_com.h"
#include "mem/klb_mem.h"
#include "em_util/em_log.h"
#include <assert.h>

#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/frame.h"
#include "libavutil/error.h"
#include "libavformat/avformat.h"


typedef struct ffmpeg_input_t_
{
    AVFormatContext*        p_fmt_ctx;
    AVCodecContext*         p_video_ctx;

    int                     video_stream_idx;
    AVStream*               p_video_stream;
    AVRational              video_time_base;
    AVRational              video_frame_rate;

    int                     audio_stream_idx;

    AVPacket*               p_pkt;
    AVFrame*                p_avframe;
}ffmpeg_input_t;

static int refcount = 0;

static int open_codec_context(ffmpeg_input_t* p_input, int *stream_idx, AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret, stream_index;
    AVStream *st;
    AVCodec *dec = NULL;
    AVDictionary *opts = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        LOG("C Could not find stream in input file");
        //fprintf(stderr, "Could not find %s stream in input file '%s'\n",
        //    av_get_media_type_string(type), src_filename);
        return ret;
    }
    else 
    {
        stream_index = ret;
        st = fmt_ctx->streams[stream_index];

        /* find decoder for the stream */
        dec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!dec) {
            //fprintf(stderr, "Failed to find %s codec\n",
            //    av_get_media_type_string(type));

            LOG("C Failed to find %s codec\n", av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }

        /* Allocate a codec context for the decoder */
        *dec_ctx = avcodec_alloc_context3(dec);
        if (!*dec_ctx) {
            //fprintf(stderr, "Failed to allocate the %s codec context\n",
            //    av_get_media_type_string(type));

            LOG("C Failed to allocate the %s codec context\n", av_get_media_type_string(type));
            return AVERROR(ENOMEM);
        }

        /* Copy codec parameters from input stream to output codec context */
        if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
            //fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
            //    av_get_media_type_string(type));

            LOG("C Failed to copy %s codec parameters to decoder context\n", av_get_media_type_string(type));
            return ret;
        }

        /* Init the decoders, with or without reference counting */
        av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);
        if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
            //fprintf(stderr, "Failed to open %s codec\n",
            //    av_get_media_type_string(type));

            LOG("C Failed to open %s codec\n", av_get_media_type_string(type));
            return ret;
        }
        *stream_idx = stream_index;
    }

    return 0;
}

ffmpeg_input_t* ffmpeg_input_create(const char* p_filename)
{
    AVFormatContext* p_fmt_cxt = NULL;
    if (avformat_open_input(&p_fmt_cxt, p_filename, NULL, NULL) < 0) {
        LOG("C avformat_open_input error!path:[%s]", p_filename);
        return NULL;
    }

    if (avformat_find_stream_info(p_fmt_cxt, NULL) < 0) {
        LOG("C avformat_find_stream_info error!");
        return NULL;
    }

    ffmpeg_input_t* p_input = KLB_MALLOC(ffmpeg_input_t, 1, 0);
    KLB_MEMSET(p_input, 0, sizeof(ffmpeg_input_t));

    p_input->p_fmt_ctx = p_fmt_cxt;
    p_input->video_stream_idx = -1;

    p_input->video_time_base.num = 1;
    p_input->video_time_base.den = 25;
    p_input->video_frame_rate.num = 25;
    p_input->video_frame_rate.den = 1;

    p_input->audio_stream_idx = -1;

    if (open_codec_context(p_input, &p_input->video_stream_idx, &p_input->p_video_ctx, p_input->p_fmt_ctx, AVMEDIA_TYPE_VIDEO) < 0)
    {
        LOG("C open_codec_context error!");
    }
    else
    {
        p_input->p_video_stream = p_input->p_fmt_ctx->streams[p_input->video_stream_idx];

        p_input->video_time_base = p_input->p_video_stream->time_base;
        p_input->video_frame_rate = p_input->p_video_stream->avg_frame_rate;

        LOG("C time_base:[%d,%d]", p_input->video_time_base.num, p_input->video_time_base.den);
        LOG("C avg_frame_rate:[%d,%d]", p_input->video_frame_rate.num, p_input->video_frame_rate.den);
    }

    //av_dump_format(p_input->p_fmt_ctx, 0, p_filename, 0);
    return p_input;
}

void ffmpeg_input_destroy(ffmpeg_input_t* p_input)
{
    KLB_FREE_BY(p_input->p_avframe, ffmpeg_av_frame_free);
    KLB_FREE_BY(p_input->p_pkt, ffmpeg_av_packet_free);

    if (NULL != p_input->p_video_ctx)
    {
        avcodec_free_context(&p_input->p_video_ctx);
    }

    if (NULL != p_input->p_fmt_ctx)
    {
        avformat_close_input(&p_input->p_fmt_ctx);
    }

    KLB_FREE(p_input);
}

enum AVCodecID ffmpeg_input_video_id(ffmpeg_input_t* p_input)
{
    if (NULL != p_input && NULL != p_input->p_video_ctx)
    {
        return p_input->p_video_ctx->codec_id;
    }

    return AV_CODEC_ID_NONE;
}

static void ffmpeg_input_print(AVPacket* p_pkt)
{
    LOG("C ffmpeg_input_print pkt:[%p,%d]", p_pkt->data, p_pkt->size);
    uint8_t* p_data = p_pkt->data;
    int data_len = MIN(p_pkt->size, 128);

    for (int i = 0; i < data_len; i += 16)
    {
        LOG("0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x",
            p_data[i + 0], p_data[i + 1], p_data[i + 2], p_data[i + 3],
            p_data[i + 4], p_data[i + 5], p_data[i + 6], p_data[i + 7],
            p_data[i + 8], p_data[i + 9], p_data[i + 10], p_data[i + 11],
            p_data[i + 12], p_data[i + 13], p_data[i + 14], p_data[i + 15]);
    }
}

AVPacket* ffmpeg_input_read(ffmpeg_input_t* p_input, int* p_out_type)
{
    if (NULL == p_input->p_pkt)
    {
        p_input->p_pkt = av_packet_alloc();
    }

    if (0 <= av_read_frame(p_input->p_fmt_ctx, p_input->p_pkt))
    {
        AVPacket* p_pkt = p_input->p_pkt;
        p_input->p_pkt = NULL;

        //LOG("packet size:[%d],[%d]", p_pkt->stream_index, p_pkt->size);
        if (p_pkt->stream_index == p_input->video_stream_idx)
        {
            *p_out_type = FFMPEG_AVPACKET_VIDEO;
        }
        else if (p_pkt->stream_index == p_input->audio_stream_idx)
        {
            *p_out_type = FFMPEG_AVPACKET_AUDIO;
        }
        else
        {
            *p_out_type = 0;
        }

#if 0
        if (FFMPEG_AVPACKET_VIDEO == *p_out_type)
        {
            ffmpeg_input_print(p_pkt);
        }
#endif

        return p_pkt;
    }

    return NULL;
}


AVFrame* ffmpeg_input_decode_video(ffmpeg_input_t* p_input, AVPacket* p_pkt)
{
    assert(NULL != p_input);

    if (NULL == p_input->p_avframe)
    {
        p_input->p_avframe = av_frame_alloc();
    }

    // 解码数据
    int got_picture = 0;

    avcodec_decode_video2(p_input->p_video_ctx, p_input->p_avframe, &got_picture, p_pkt);

    if (0 != got_picture)
    {
        if (NULL != p_input->p_avframe)
        {
            AVFrame* p_frame = p_input->p_avframe;
            p_input->p_avframe = NULL;

            return p_frame;
        }
    }

    return NULL; //未得到帧, 还需要继续送数据
}

static em_frame_yuv_wav_t* ffmpeg_avframe_yuv420p(AVFrame* p_avframe, int64_t pts)
{
    int w = p_avframe->width;
    int h = p_avframe->height;

    int uv_w = w / 2;
    int uv_h = h / 2;

    // Opengl纹理, 需要4的倍数
    int pitch_y = (w >> 2) << 2;    //(w + 3) / 4 * 4;
    int pitch_u = (uv_w >> 2) << 2; //(uv_w + 3) / 4 * 4;
    int pitch_v = (uv_w >> 2) << 2; //(uv_w + 3) / 4 * 4;

                                    // 防止越界
    pitch_y = MIN(pitch_y, p_avframe->linesize[0]);
    pitch_u = MIN(pitch_u, p_avframe->linesize[1]);
    pitch_v = MIN(pitch_v, p_avframe->linesize[2]);

    int y_len = pitch_y * h;
    int u_len = pitch_u * uv_h;
    int v_len = pitch_v * uv_h;

    em_frame_yuv_wav_t* p_yuv = KLB_MALLOC(em_frame_yuv_wav_t, 1, 0);
    KLB_MEMSET(p_yuv, 0, sizeof(em_frame_yuv_wav_t));

    p_yuv->type = EM_FRAME_TYPE_YUV;
    p_yuv->w = w;
    p_yuv->h = h;

    p_yuv->buf_len = y_len + u_len + v_len;
    p_yuv->end = y_len + u_len + v_len;
    p_yuv->p_buf = KLB_MALLOC(uint8_t, p_yuv->buf_len, 0);

    p_yuv->p_y = p_yuv->p_buf;
    p_yuv->p_u = p_yuv->p_buf + y_len;
    p_yuv->p_v = p_yuv->p_buf + y_len + u_len;

    p_yuv->pitch_y = pitch_y;
    p_yuv->h_y = h;
    p_yuv->pitch_u = pitch_u;
    p_yuv->h_u = uv_h;
    p_yuv->pitch_v = pitch_v;
    p_yuv->h_v = uv_h;

    // Y
    uint8_t* p_src = p_avframe->data[0];
    uint8_t* p_dst = (uint8_t*)(p_yuv->p_y);
    for (int i = 0; i < h; i++)
    {
        memcpy(p_dst, p_src, pitch_y/*p_avframe->width*/);
        p_src += p_avframe->linesize[0];
        p_dst += pitch_y;
    }

    //U
    p_src = p_avframe->data[1];
    p_dst = (uint8_t*)(p_yuv->p_u);
    for (int i = 0; i < uv_h; i++)
    {
        memcpy(p_dst, p_src, pitch_u/*uv_w*/);
        p_src += p_avframe->linesize[1];
        p_dst += pitch_u;
    }

    //V
    p_src = p_avframe->data[2];
    p_dst = (uint8_t*)(p_yuv->p_v);
    for (int i = 0; i < uv_h; i++)
    {
        memcpy(p_dst, p_src, pitch_v/*uv_w*/);
        p_src += p_avframe->linesize[2];
        p_dst += pitch_v;
    }

    p_yuv->pts = pts;//p_avframe->pts;

    return p_yuv;
}

em_frame_yuv_wav_t* ffmpeg_input_read_frame(ffmpeg_input_t* p_input)
{
    int data_type = 0;
    AVPacket* p_pkt = ffmpeg_input_read(p_input, &data_type);
    if (NULL != p_pkt)
    {
        if (FFMPEG_AVPACKET_VIDEO == data_type)
        {
            AVFrame* p_frame = ffmpeg_input_decode_video(p_input, p_pkt);
            if (NULL != p_frame)
            {
                int64_t pts = p_frame->pts;
                if (p_frame->pts != AV_NOPTS_VALUE)
                {
                    pts = av_q2d(p_input->video_time_base) * 1000 * p_frame->pts;
                }

                em_frame_yuv_wav_t* p_yuv_vav = ffmpeg_avframe_yuv420p(p_frame, pts);
                ffmpeg_av_frame_free(p_frame);

                //LOG("C pts:[%lld]", pts);
                return p_yuv_vav;
            }

            ffmpeg_av_packet_free(p_pkt);
        }
    }

    return NULL;
}
