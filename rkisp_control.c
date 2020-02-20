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
#include <pthread.h>
#include <stdbool.h>
#include "shadow_display.h"
#include "video_common.h"

#include <stdint.h>
#include <camera_engine_rkisp/interface/rkisp_api.h>
#include "rga_control.h"
#include "rockx_control.h"

static bool g_def_expo_weights = false;
bool g_expo_weights_en = false;

static bo_t g_rotate_bo;
static int g_rotate_fd = -1;

static const struct rkisp_api_ctx *ctx;
static const struct rkisp_api_buf *buf;
static bool g_run;
static pthread_t g_tid;

static void *process(void *arg)
{
    rga_info_t src, dst;

    do {
        buf = rkisp_get_frame(ctx, 0);

        memset(&src, 0, sizeof(rga_info_t));
        src.fd = buf->fd;
        src.mmuFlag = 1;
        src.rotation = HAL_TRANSFORM_ROT_90;
        rga_set_rect(&src.rect, 0, 0, ctx->width, ctx->height, ctx->width, ctx->height,
                     RK_FORMAT_YCbCr_420_SP);
        memset(&dst, 0, sizeof(rga_info_t));
        dst.fd = g_rotate_fd;
        dst.mmuFlag = 1;
        rga_set_rect(&dst.rect, 0, 0, ctx->height, ctx->width, ctx->height, ctx->width,
                     RK_FORMAT_RGB_888);
        if (c_RkRgaBlit(&src, &dst, NULL)) {
            printf("%s: rga fail\n", __func__);
            continue;
        }

        rockx_control_pose_body(g_rotate_bo.ptr, ctx->height, ctx->width, RK_FORMAT_RGB_888);

        if (shadow_display_vertical_cb)
            shadow_display_vertical_cb(g_rotate_bo.ptr, g_rotate_fd, RK_FORMAT_RGB_888,
                                       ctx->height, ctx->width);

        rkisp_put_frame(ctx, buf);
    } while (g_run);

    pthread_exit(NULL);
}

int rkisp_control_init(void)
{
    char name[32];

    int id = get_video_id("rkisp1_mainpath");
    if (id < 0) {
        printf("%s: get video id fail!\n", __func__);
        return -1;
    }

    snprintf(name, sizeof(name), "/dev/video%d", id);
    printf("%s: %s\n", __func__, name);
    ctx = rkisp_open_device(name, 1);
    if (ctx == NULL) {
        printf("%s: ctx is NULL\n", __func__);
        return -1;
    }

    rkisp_set_fmt(ctx, 1280, 720, ctx->fcc);

    if (rga_control_buffer_init(&g_rotate_bo, &g_rotate_fd, ctx->width, ctx->height, 24))
        return -1;

    if (rkisp_start_capture(ctx))
        return -1;

    g_run = true;
    if (pthread_create(&g_tid, NULL, process, NULL)) {
        printf("pthread_create fail\n");
        return -1;
    }

    return 0;
}

void rkisp_control_exit(void)
{
    g_run = false;
    if (g_tid) {
        pthread_join(g_tid, NULL);
        g_tid = 0;
    }

    rkisp_stop_capture(ctx);
    rkisp_close_device(ctx);

    rga_control_buffer_deinit(&g_rotate_bo, g_rotate_fd);
}

void rkisp_control_expo_weights_270(int left, int top, int right, int bottom)
{
    if (g_expo_weights_en) {
        unsigned char weights[81];
        int x = ctx->width - bottom;
        int y = left;
        int w = bottom - top;
        int h = right - left;
        x = x * 9 / ctx->width;
        y = y * 9 / ctx->height;
        w = w * 9 / ctx->width + 1;
        h = h * 9 / ctx->height + 1;
        memset(weights, 1, sizeof(weights));
        for (int j = 0; j < 9; j++)
            for (int i = 0; i < 9; i++)
                if (i >=x && i < x + w && j >= y && j < y + h)
                    weights[j * 9 + i] = 31;
        rkisp_set_expo_weights(ctx, weights, sizeof(weights));
        g_def_expo_weights = false;
    }
}

void rkisp_control_expo_weights_90(int left, int top, int right, int bottom)
{
    if (g_expo_weights_en) {
        unsigned char weights[81];
        int x = ctx->width - top;
        int y = ctx->height - right;
        int w = bottom - top;
        int h = right - left;
        x = x * 9 / ctx->width;
        y = y * 9 / ctx->height;
        w = w * 9 / ctx->width + 1;
        h = h * 9 / ctx->height + 1;
        memset(weights, 1, sizeof(weights));
        for (int j = 0; j < 9; j++)
            for (int i = 0; i < 9; i++)
                if (i >=x && i < x + w && j >= y && j < y + h)
                    weights[j * 9 + i] = 31;
        rkisp_set_expo_weights(ctx, weights, sizeof(weights));
        g_def_expo_weights = false;
    }
}

void rkisp_control_expo_weights_default(void)
{
    if (g_expo_weights_en) {
        unsigned char weights[81] = {
            1,  5,  9,  15, 31, 15, 9,  5,  1,
            1,  5,  9,  15, 31, 15, 9,  5,  1,
            1,  5,  9,  15, 31, 15, 9,  5,  1,
            1,  5,  9,  15, 31, 15, 9,  5,  1,
            1,  5,  9,  15, 31, 15, 9,  5,  1,
            1,  5,  9,  15, 31, 15, 9,  5,  1,
            1,  5,  9,  15, 31, 15, 9,  5,  1,
            1,  5,  9,  15, 31, 15, 9,  5,  1,
            1,  5,  9,  15, 31, 15, 9,  5,  1,
        };
        if (!g_def_expo_weights) {
            g_def_expo_weights = true;
            rkisp_set_expo_weights(ctx, weights, sizeof(weights));
        }
    }
}
