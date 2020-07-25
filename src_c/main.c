#include "emscripten.h"
#include <SDL.h>
#include "em_util/em_log.h"
#include "em_util/em_timer.h"
#include "main_app/main_app.h"
#include "emscripten/threading.h"
#include "mem/klb_mem.h"
#include "ffmpeg_com.h"
#include <stdio.h>
#include <assert.h>

#include "mnp_h264_res.h"

/*extern*/ main_app_t* g_main_app = NULL;       ///< 主APP
/*extern*/ bool        g_main_hidden = false;   ///< 页面是否隐藏


static void cb_main_loop(void);


int main() 
{
    // 打印
    LOG("C https://github.com/lishaoliang/ffdec.js");
    LOG("C https://gitee.com/lishaoliang/ffdec.js");

    // 打印是否支持
    LOG("C emscripten_has_threading_support: %d", emscripten_has_threading_support());

    // 初始化 ffmpeg
    ffmpeg_register_all();

    // 初始化 main_app_t
    g_main_app = main_app_create();
    assert(NULL != g_main_app);

    main_app_start(g_main_app);

    emscripten_set_main_loop(cb_main_loop, 60, 0);

    LOG("C main loop start.");
    return 0;
}

static void cb_main_loop(void)
{
    //uint32_t now = em_get_ticks();
    int64_t now = emscripten_get_now();
    now *= 1000;

    main_app_run(g_main_app, now);
}


//////////////////////////////////////////////////////////////////////////
// 导出函数

/// @brief 测试打印,是否加载成功
/// @param [in] n  数值
/// @return int 返回n
EMSCRIPTEN_KEEPALIVE int ffdecjs_hello(int n)
{
    // Test
    LOG("C ffdecjs_hello:[%d]", n);

    return n;
}

/// @brief 设置是否隐藏
/// 1.true,隐藏
/// 0.false,不隐藏
EMSCRIPTEN_KEEPALIVE int ffdecjs_set_hidden(int hidden)
{
    g_main_hidden = (0 == hidden) ? false : true;
    //LOG("C ffdecjs_set_hidden:[%d]", g_main_hidden);

    return 0;
}

/// @brief 模块退出, 清理资源
EMSCRIPTEN_KEEPALIVE int ffdecjs_quit(int status)
{
    // Test
    LOG("C ffdecjs_quit.[%d]", status);

    // stop
    if (NULL != g_main_app)
    {
        main_app_stop(g_main_app);
    }

    // destroy
    KLB_FREE_BY(g_main_app, main_app_destroy);

    LOG("C ffdecjs_quit over.");

    return 0;
}

/// @brief 控制命令: 设置参数等
EMSCRIPTEN_KEEPALIVE char* ffdecjs_control(const char* p_cmd, const char* p_lparam, const char* p_wparam)
{
    return NULL;
}


/// @brief 打开待解码的文件
/// @param [in] *p_path_name  文件路径
/// @param [in] *p_param json 格式的参数信息
/// @return int 0.成功; 非0.错误码
EMSCRIPTEN_KEEPALIVE int ffdecjs_open(const char* p_path_name, const char* p_param)
{
    //LOG("C ffdecjs_open:[%s]:[%s]", p_name, p_param);

    return main_app_open(g_main_app, p_path_name);
}


/// @brief 关闭文件
/// @return int 0.成功
/// @note 关闭仅表示不再有数据被解码, 并不影响已经解码之后的数据
EMSCRIPTEN_KEEPALIVE int ffdecjs_close()
{
    //LOG("C ffdecjs_close");
    main_app_close(g_main_app);
    return 0;
}

/// @brief 获取处于队列头部帧信息
/// @return ffdecjs_media_t* 帧信息指针
///  仅用于测试
EMSCRIPTEN_KEEPALIVE ffdecjs_media_t* ffdecjs_get_head()
{
    return main_app_get_head(g_main_app);
}

/// @brief 弹出(释放)掉处于队列头部帧
/// @return int 0.成功; 非0.错误码
EMSCRIPTEN_KEEPALIVE int ffdecjs_pop_head()
{
    return main_app_pop_head(g_main_app);
}

/// @brief 写测试文件
/// @param [in] *p_path_name  文件路径名称: eg."/xxx.flv"
/// @return int 0.成功; 非0.错误码
///  把内置的测试flv数据写成文件
///  unix文件系统风格
EMSCRIPTEN_KEEPALIVE int ffdecjs_write_test_file(const char* p_path_name)
{
    // "/xxx.flv"
    if (NULL == p_path_name)
    {
        LOG("C ffdecjs_write_test_file fopen error!path:[NULL]");
        return 1;
    }

    FILE* pf = fopen(p_path_name, "wb");
    if (NULL != pf)
    {
        fwrite(g_res_key_map[1].p_data, 1, g_res_key_map[1].data_len, pf);
        fclose(pf);
        return 0;
    }

    LOG("C ffdecjs_write_test_file fopen error!path:[%s]", p_path_name);
    return 1;
}

/// @brief 打印文件
/// @param [in] *p_path_name  文件路径名称: eg."/xxx.flv"
/// @return int 0.成功; 非0.错误码
///  unix文件系统风格
EMSCRIPTEN_KEEPALIVE int ffdecjs_print_file(const char* p_path_name)
{
    if (NULL == p_path_name)
    {
        LOG("C ffdecjs_print_file fopen error!path:[NULL]");
        return 1;
    }

    FILE* pf = fopen(p_path_name, "rb");
    if (NULL != pf)
    {
        fseek(pf, 0, SEEK_END);
        int filelen = ftell(pf);
        fseek(pf, 0, SEEK_SET);

        LOG("C ffdecjs_print_file fopen:[%s],filelen:[%d]", p_path_name, filelen);

        int len = filelen;
        uint8_t buf[1024] = { 0 };
        while (0 < len)
        {
            memset(buf, 0, 1024);

            int read_len = MIN(len, 1024);
            fread(buf, 1, read_len, pf);

            for (int i = 0; i < read_len; i += 16)
            {
                LOG("0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x",
                    buf[i + 0], buf[i + 1], buf[i + 2], buf[i + 3],
                    buf[i + 4], buf[i + 5], buf[i + 6], buf[i + 7],
                    buf[i + 8], buf[i + 9], buf[i + 10], buf[i + 11],
                    buf[i + 12], buf[i + 13], buf[i + 14], buf[i + 15]);
            }

            len -= read_len;
        }

        fclose(pf);
        return 0;
    }

    LOG("C ffdecjs_print_file fopen error!path:[%s]", p_path_name);
    return 1;
}
