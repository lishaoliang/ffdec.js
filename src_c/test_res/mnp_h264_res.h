#ifndef __MNP_H264_RES_H__
#define __MNP_H264_RES_H__
#include <stdio.h>
#include <string.h>

#define MNP_MAP_BUF   68
typedef struct mnp_map_t_
{
    char    name[MNP_MAP_BUF];
    char*   p_data;
    int     data_len;
}mnp_map_t;

extern mnp_map_t g_res_key_map[];

#endif
