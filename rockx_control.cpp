/*
 * Copyright (C) 2019 Rockchip Electronics Co., Ltd.
 * author: Zhihua Wang, hogan.wang@rock-chips.com
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL), available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <stdio.h>
#include <memory.h>
#include <sys/time.h>

#include <vector>

#include <rockx/rockx.h>
#include "rockx_control.h"

static rockx_handle_t g_pose_body;

int rockx_control_init(void)
{
    rockx_ret_t ret;

    ret = rockx_create(&g_pose_body, ROCKX_MODULE_POSE_BODY, nullptr, 0);
    if (ret != ROCKX_RET_SUCCESS) {
        printf("init rockx module ROCKX_MODULE_POSE_BODY error %d\n", ret);
        return -1;
    }

    return 0;
}

void rockx_control_exit(void)
{
    rockx_destroy(g_pose_body);
}

int rockx_control_pose_body(void *ptr, int width, int height, RgaSURF_FORMAT rga_fmt)
{
    rockx_ret_t ret;
    rockx_pixel_format fmt;
    rockx_image_t image;
    rockx_keypoints_array_t body_array;
    static const std::vector<std::pair<int,int>> posePairs = {
        {1,2}, {1,5}, {2,3}, {3,4}, {5,6}, {6,7},
        {1,8}, {8,9}, {9,10}, {1,11}, {11,12}, {12,13},
        {1,0}, {0,14}, {14,16}, {0,15}, {15,17}
    };

    if (rga_fmt == RK_FORMAT_YCbCr_420_SP)
        fmt = ROCKX_PIXEL_FORMAT_YUV420SP_NV12;
    else if (rga_fmt == RK_FORMAT_RGB_888)
        fmt = ROCKX_PIXEL_FORMAT_RGB888;
    else {
        printf("%s: unsupport rga fmt\n");
        return -1;
    }

    memset(&image, 0, sizeof(rockx_image_t));
    image.width = width;
    image.height = height;
    image.data = (uint8_t *)ptr;
    image.pixel_format = fmt;

    ret = rockx_pose_body(g_pose_body, &image, &body_array, nullptr);
    if (ret != ROCKX_RET_SUCCESS)
        return -1;

    for (int i = 0; i < body_array.count; i++) {
        //printf("person %d:\n", i);

        for(int j = 0; j < body_array.keypoints[i].count; j++) {
            int x = body_array.keypoints[i].points[j].x;
            int y = body_array.keypoints[i].points[j].y;
            float score = body_array.keypoints[i].score[j];
            //printf("  %s [%d, %d] %f\n", ROCKX_POSE_BODY_KEYPOINTS_NAME[j], x, y, score);
            rockx_image_draw_circle(&image, {x, y}, 3, {255, 0, 0}, -1);
        }

        for(int j = 0; j < posePairs.size(); j ++) {
            const std::pair<int,int>& posePair = posePairs[j];
            int x0 = body_array.keypoints[i].points[posePair.first].x;
            int y0 = body_array.keypoints[i].points[posePair.first].y;
            int x1 = body_array.keypoints[i].points[posePair.second].x;
            int y1 = body_array.keypoints[i].points[posePair.second].y;

            if( x0 > 0 && y0 > 0 && x1 > 0 && y1 > 0) {
                rockx_image_draw_line(&image, {x0, y0}, {x1, y1}, {0, 255, 0}, 1);
            }
        }
    }

    return 0;
}
