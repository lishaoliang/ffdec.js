#ifndef __MAIN_APP_IN_H__
#define __MAIN_APP_IN_H__

#include "klb_type.h"
#include "list/klb_list.h"
#include "ffmpeg_com.h"

//#define USE_TEST_RES        1

#if defined(USE_TEST_RES)
#include "tmem_h264.h"
#include "tmem_flv.h"
#include "mnp_h264_res.h"
#endif


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct main_app_t_
{
    ffmpeg_input_t* p_input;
    ffmpeg_dec_t*   p_dec;
    ffmpeg_enc_t*   p_enc;

    klb_list_t*     p_list_frame;   // 解码之后的数据列表

#if defined(USE_TEST_RES)
    tmem_flv_t*     p_tmem_flv;
#endif
}main_app_t;


#ifdef __cplusplus
}
#endif

#endif // __MAIN_APP_IN_H__
//end
