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
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <getopt.h>

#include "ui.h"
#include "rkisp_control.h"
#include "shadow_display.h"
#include "video_common.h"
#include "rockx_control.h"
#include "usb_camera.h"

void usage(const char *name)
{
    printf("Usage: %s options\n", name);
    printf("-h --help  Display this usage information.\n"
           "-i --isp   Use isp camera.\n"
           "-u --usb   Use usb camera.\n");
    printf("e.g. %s -i\n", name);
    printf("e.g. %s -u\n", name);
    exit(0);
}

int MiniGUIMain(int argc, const char *argv[])
{
    int next_option;
    bool isp_en = false;
    bool usb_en = false;

    const char* const short_options = "hiu";
    const struct option long_options[] = {
        {"help", 0, NULL, 'h'},
        {"isp", 0, NULL, 'i'},
        {"usb", 0, NULL, 'u'},
    };

    do {
        next_option = getopt_long(argc, argv, short_options, long_options, NULL);
        switch (next_option) {
        case 'i':
            isp_en = true;
            break;
        case 'u':
            usb_en = true;
            break;
        case -1:
            break;
        default:
            usage(argv[0]);
            break;
        }
    } while (next_option != -1);

    if (!isp_en && !usb_en)
        usage(argv[0]);

    register_shadow_display_vertical(shadow_display_vertical);

    if (rockx_control_init())
        return -1;

    if (isp_en) {
        if (rkisp_control_init())
            return -1;
    } else if (usb_en) {
        if (usb_camera_init(1280, 720))
            return -1;
    }

    ui_run();

    if (isp_en)
        rkisp_control_exit();
    else if (usb_en)
        usb_camera_exit();

    rockx_control_exit();

    return 0;
}
