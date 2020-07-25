#include "main_app.h"
#include "main_app_in.h"
#include "em_buf_mnp.h"
#include "em_log.h"
#include "mem/klb_mem.h"
#include "emscripten/em_asm.h"
#include "list/klb_list.h"
#include <assert.h>


extern bool g_main_hidden;


main_app_t* main_app_create()
{
    main_app_t* p_app = KLB_MALLOC(main_app_t, 1, 0);
    KLB_MEMSET(p_app, 0, sizeof(main_app_t));

    p_app->p_list_frame = klb_list_create();

#if defined(USE_TEST_RES)
    p_app->p_tmem_flv = tmem_flv_open(g_res_key_map[1].p_data, g_res_key_map[1].data_len);
#endif

    return p_app;
}

void main_app_destroy(main_app_t* p_app)
{
    assert(NULL != p_app);

    while (0 < klb_list_size(p_app->p_list_frame))
    {
        main_app_pop_head(p_app);
    }

    KLB_FREE_BY(p_app->p_dec, ffmpeg_dec_destroy);

#if defined(USE_TEST_RES)
    KLB_FREE_BY(p_app->p_tmem_flv, tmem_flv_close);
#endif

    KLB_FREE(p_app);
}

int main_app_start(main_app_t* p_app)
{
    assert(NULL != p_app);
    // 开启工作线程

    return 0;
}

void main_app_stop(main_app_t* p_app)
{
    assert(NULL != p_app);

}

static ffmpeg_dec_t* main_app_get_dec(main_app_t* p_app, enum AVCodecID id)
{
    if (NULL != p_app->p_dec)
    {
        if (id != ffmpeg_dec_id(p_app->p_dec))
        {
            KLB_FREE_BY(p_app->p_dec, ffmpeg_dec_destroy);
        }
    }

    if (NULL == p_app->p_dec)
    {
        p_app->p_dec = ffmpeg_dec_create(id);
    }

    return p_app->p_dec;
}

static ffmpeg_enc_t* main_app_get_enc(main_app_t* p_app, enum AVCodecID id)
{
    if (NULL != p_app->p_enc)
    {
        if (id != ffmpeg_enc_id(p_app->p_enc))
        {
            KLB_FREE_BY(p_app->p_enc, ffmpeg_enc_destroy);
        }
    }

    if (NULL == p_app->p_enc)
    {
        p_app->p_enc = ffmpeg_enc_create(id);
    }

    return p_app->p_enc;
}

ffdecjs_media_t* main_app_get_head(main_app_t* p_app)
{
    ffdecjs_media_t* p_media = (ffdecjs_media_t*)klb_list_head(p_app->p_list_frame);

    //LOG("main_app_get_head,wh:[%d,%d],otype:[%d]", p_media->w, p_media->h, p_media->otype);

    return p_media;
}

int main_app_pop_head(main_app_t* p_app)
{
    ffdecjs_media_t* p_media = (ffdecjs_media_t*)klb_list_pop_head(p_app->p_list_frame);

    if (NULL != p_media)
    {
        KLB_FREE_BY(p_media->p_yuv_wav, em_frame_yuv_wav_free);
        KLB_FREE(p_media);
    }

    return 0;
}

int main_app_open(main_app_t* p_app, const char* p_filename)
{
    assert(NULL == p_app->p_input);
    p_app->p_input = ffmpeg_input_create(p_filename);
    //assert(NULL != p_app->p_input);

    return (NULL != p_app->p_input) ? 0 : 1;
}

void main_app_close(main_app_t* p_app)
{
    KLB_FREE_BY(p_app->p_input, ffmpeg_input_destroy);
}

static int main_app_push_yuv_wav(main_app_t* p_app, em_frame_yuv_wav_t* p_yuv_wav)
{
    ffdecjs_media_t* p_media = KLB_MALLOC(ffdecjs_media_t, 1, 0);
    KLB_MEMSET(p_media, 0, sizeof(ffdecjs_media_t));

    p_media->w = p_yuv_wav->w;
    p_media->h = p_yuv_wav->h;
    p_media->time = p_yuv_wav->pts;

    p_media->otype = FFDECJS_MEDIA_YUV420P;
    p_media->p_y = p_yuv_wav->p_y;          ///<  Y分量
    p_media->p_u = p_yuv_wav->p_u;          ///<  U分量
    p_media->p_v = p_yuv_wav->p_v;          ///<  V分量

    p_media->pitch_y = p_yuv_wav->pitch_y;  ///<  Y分量, 行夸距
    p_media->pitch_u = p_yuv_wav->pitch_u;  ///<  U分量, 行夸距
    p_media->pitch_v = p_yuv_wav->pitch_v;  ///<  V分量, 行夸距

    p_media->p_yuv_wav = p_yuv_wav;

    klb_list_push_tail(p_app->p_list_frame, p_media);

    return 0;
}

int main_app_run(main_app_t* p_app, int64_t now_ticks)
{
    assert(NULL != p_app);
    // 直接使用主线程干活: 必须非柱塞方式, 且耗时不能太长, 保证主线程流畅

    if (4 < klb_list_size(p_app->p_list_frame))
    {
        return 0;
    }

#if defined(USE_TEST_RES)
    em_buf_t* p_buf = tmem_flv_get_next(p_app->p_tmem_flv);

    if (NULL != p_buf)
    {
        klb_mnp_media_t* p_media = (klb_mnp_media_t*)p_buf->p_buf;
        uint8_t* p_data = p_buf->p_buf + sizeof(klb_mnp_media_t);
        int data_len = p_media->size - sizeof(klb_mnp_media_t);

        if (KLB_MNP_DTYPE_H264 == p_media->dtype)
        {
            ffmpeg_dec_t* p_dec = main_app_get_dec(p_app, AV_CODEC_ID_H264);
            //ffmpeg_enc_t* p_enc = main_app_get_enc(p_app, AV_CODEC_ID_MJPEG);
            if (NULL != p_dec)
            {
                em_frame_yuv_wav_t* p_yuv_wav = ffmpeg_dec_decode2(p_dec, p_data, data_len, p_media->time, p_media->time);
                if (NULL != p_yuv_wav)
                {
                    //LOG("ffmpeg_dec_decode2 yuv wav:[%d,%d]", p_yuv_wav->w, p_yuv_wav->h);
                    main_app_push_yuv_wav(p_app, p_yuv_wav);
                }
            }
        }

        em_buf_unref_next(p_buf);
    }
#else
    if (NULL == p_app->p_input)
    {
        return 0;
    }

    //int data_type = 0;
    //AVPacket* p_pkt = ffmpeg_input_read(p_app->p_input, &data_type);
    //if (NULL != p_pkt)
    //{
    //    if (FFMPEG_AVPACKET_VIDEO == data_type)
    //    {
    //        ffmpeg_dec_t* p_dec = main_app_get_dec(p_app, ffmpeg_input_video_id(p_app->p_input));
    //        if (NULL != p_dec)
    //        {
    //            em_frame_yuv_wav_t* p_yuv_wav = ffmpeg_dec_decode2(p_dec, p_pkt->data, p_pkt->size, p_pkt->pts, p_pkt->dts);
    //            if (NULL != p_yuv_wav)
    //            {
    //                //LOG("ffmpeg_dec_decode2 yuv wav:[%d,%d]", p_yuv_wav->w, p_yuv_wav->h);
    //                main_app_push_yuv_wav(p_app, p_yuv_wav);
    //            }
    //        }
    //    }

    //    ffmpeg_av_packet_free(p_pkt);
    //}

    em_frame_yuv_wav_t* p_yuv_wav = ffmpeg_input_read_frame(p_app->p_input);
    if (NULL != p_yuv_wav)
    {
        main_app_push_yuv_wav(p_app, p_yuv_wav);
    }

#endif

    return 0;
}
