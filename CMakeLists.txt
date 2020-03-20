cmake_minimum_required(VERSION 3.8.2)

project(pose_body)

set(SRC
    ui.c
    shadow_display.c
    rkisp_control.c
    rga_control.c
    video_common.c
    rockx_control.cpp
    usb_camera.c
    vpu_decode.c
    main.c
)

include_directories(${DRM_HEADER_DIR})

add_executable(pose_body ${SRC})

target_link_libraries(pose_body rkisp rkisp_api rockx rknn_api drm rga pthread ts minigui_ths png12 jpeg freetype rockchip_mpp)

install(TARGETS pose_body DESTINATION bin)

install(FILES minigui/MiniGUI-1280x720.cfg DESTINATION ../etc RENAME MiniGUI.cfg)
