#ifndef __MAIN_APP_H__
#define __MAIN_APP_H__

#include "klb_type.h"
#include "em_frame_yuv_wav.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct main_app_t_ main_app_t;


typedef struct ffdecjs_media_t_
{
    int32_t     w;
    int32_t     h;
    int64_t     time;

    int32_t     dtype;
#define FFDECJS_MEDIA_JPEG      101

    // file: JPEG
    struct
    {
        uint8_t*            p_data;
        int32_t             data_len;
    };

    int32_t     otype;
#define FFDECJS_MEDIA_YUV420P   1
#define FFDECJS_MEDIA_ARGB      2
#define FFDECJS_MEDIA_RGBA      3

    union
    {
        // YUV420
        struct
        {
            uint8_t*        p_y;        ///<  Y分量
            uint8_t*        p_u;        ///<  U分量
            uint8_t*        p_v;        ///<  V分量

            int32_t         pitch_y;    ///<  Y分量, 行夸距
            int32_t         pitch_u;    ///<  U分量, 行夸距
            int32_t         pitch_v;    ///<  V分量, 行夸距
        };

        // RGBA8888
        struct
        {
            union
            {
                uint8_t*    p_argb;
                uint8_t*    p_rgba;
            };
            int32_t         pitch_8888;
        };
    };

    // 以下项目, 内部使用
    em_frame_yuv_wav_t*     p_yuv_wav;
}ffdecjs_media_t;


main_app_t* main_app_create();
void main_app_destroy(main_app_t* p_app);


int main_app_start(main_app_t* p_app);
void main_app_stop(main_app_t* p_app);

ffdecjs_media_t* main_app_get_head(main_app_t* p_app);
int main_app_pop_head(main_app_t* p_app);


int main_app_open(main_app_t* p_app, const char* p_filename);
void main_app_close(main_app_t* p_app);


int main_app_run(main_app_t* p_app, int64_t now_ticks);


#ifdef __cplusplus
}
#endif

#endif // __MAIN_APP_H__
//end
