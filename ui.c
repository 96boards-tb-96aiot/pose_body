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
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/ctrl/static.h>
#include <minigui/control.h>

#include "shadow_display.h"

#define TEXT_SIZE 36

static HWND g_main_hwnd = HWND_INVALID;
static PLOGFONT g_font = NULL;
DWORD g_bkcolor;

static unsigned int g_left, g_top, g_right, g_bottom;
static bool g_update = false;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static int draw_text(HDC hdc, const char *buf, int n, int left, int top,
                     int right, int bottom, UINT format)
{
    RECT rc;

    rc.left = left;
    rc.top = top;
    rc.right = right;
    rc.bottom = bottom;
    return DrawText(hdc, buf, n, &rc, format);
}

static void ui_paint(HWND hwnd)
{
    HDC hdc;
    hdc = BeginPaint(hwnd);
    EndPaint(hwnd, hdc);
}

static LRESULT ui_win_proc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param)
{
    HDC hdc;

    switch (message) {
    case MSG_CREATE:
        break;
    case MSG_TIMER:
        break;
    case MSG_PAINT:
        ui_paint(hwnd);
        break;
    case MSG_KEYDOWN :
        switch (w_param) {
        case SCANCODE_ENTER:
            break;
        case SCANCODE_CURSORBLOCKLEFT:
            break;
        }
        break;
    case MSG_COMMAND:
        break;
    }
    return DefaultMainWinProc(hwnd, message, w_param, l_param);
}

void ui_run(void)
{
    MSG msg;
    HDC sndHdc;
    MAINWINCREATE create_info;

    memset(&create_info, 0, sizeof(MAINWINCREATE));
    create_info.dwStyle = WS_VISIBLE;
    create_info.dwExStyle = WS_EX_NONE | WS_EX_AUTOSECONDARYDC;
    create_info.spCaption = "ui";
    //create_info.hCursor = GetSystemCursor(0);
    create_info.hIcon = 0;
    create_info.MainWindowProc = ui_win_proc;
    create_info.lx = 0;
    create_info.ty = 0;
    create_info.rx = g_rcScr.right;
    create_info.by = g_rcScr.bottom;
    create_info.dwAddData = 0;
    create_info.hHosting = HWND_DESKTOP;
    //  create_info.language = 0; //en

    g_main_hwnd = CreateMainWindow(&create_info);
    if (g_main_hwnd == HWND_INVALID) {
        printf("%s failed\n", __func__);
        return;
    }

    g_bkcolor = GetWindowElementPixel(g_main_hwnd, WE_BGC_DESKTOP);
    SetWindowBkColor(g_main_hwnd, g_bkcolor);

    ShowWindow(g_main_hwnd, SW_SHOWNORMAL);
    sndHdc = GetSecondaryDC((HWND)g_main_hwnd);
    SetMemDCAlpha(sndHdc, MEMDC_FLAG_SWSURFACE, 0);

    g_font = CreateLogFont(FONT_TYPE_NAME_SCALE_TTF, "ubuntuMono", "ISO8859-1",
                           FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_FLIP_NIL,
                           FONT_OTHER_NIL, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE,
                           TEXT_SIZE, 0);
    if (!g_font) {
        printf("%s create font failed\n", __func__);
        return;
    }
    SetWindowFont(g_main_hwnd, g_font);

    RegisterMainWindow(g_main_hwnd);

    while (GetMessage(&msg, g_main_hwnd)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DestroyLogFont(g_font);
    DestroyMainWindow(g_main_hwnd);
    MainWindowThreadCleanup(g_main_hwnd);
}

void ui_paint_refresh(void)
{
    int ui_width, ui_height;
    shadow_get_crop_screen(&ui_width, &ui_height);
    RECT rect = {0, 0, ui_width, ui_height};
    if (g_main_hwnd == HWND_INVALID)
        return;
    pthread_mutex_lock(&mutex);
    if (g_update)
        InvalidateRect(g_main_hwnd, &rect, TRUE);
    else
        InvalidateRect(g_main_hwnd, &rect, FALSE);
    pthread_mutex_unlock(&mutex);
}
