/*********************************************************************************************************************
 * TFT180 模拟器主程序 (main.c)
 * 纯原生 Win32 GDI 模拟器，调用核心图像处理算法 image_main()
 ********************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <windows.h>

#include "zf_device_tft180.h"
#include "image.h"
#include "avilib.h"

#define MAX_FRAME_SIZE (MT9V03X_W * MT9V03X_H * 3)

// 全局图像缓冲区与状态变量定义 (160x64)
uint8_t mt9v03x_image[MT9V03X_H][MT9V03X_W] = {0};
uint8_t mt9v03x_finish_flag = 0;
float Z_360 = 0.0f;
float Z_360_S = 0.0f;
float fuya_value = 0.0f;

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    printf("==================================================\n");
    printf("         TFT180 Simulator (Native GUI)            \n");
    printf("==================================================\n");

    // 1. 初始化 TFT180 原生窗口
    tft180_init();
    tft180_clear();
    tft180_set_color(RGB565_WHITE, RGB565_BLACK);

    // 2. 打开赛道录像 AVI 视频文件 (优先尝试 160x64 视频)
    const char *avi_path = "image_video\\2025_11_01_00_33_46_Video.avi";
    avi_t *avi_file = AVI_open_input_file(avi_path, 1);
    if (avi_file == NULL) {
        avi_path = "image_video\\2025_10_12_17_03_53_Video.avi";
        avi_file = AVI_open_input_file(avi_path, 1);
    }

    if (avi_file == NULL) {
        printf("[TFT180] Cannot open AVI video in image_video folder.\n");
        tft180_performance_test();
        while (!tft180_is_closed()) {
            tft180_delay(50);
        }
        return 0;
    }

    long total_frames = AVI_video_frames(avi_file);
    int avi_w = AVI_video_width(avi_file);
    int avi_h = AVI_video_height(avi_file);
    printf("[TFT180] Video opened: %s (Total: %ld frames, Resolution: %dx%d)\n", avi_path, total_frames, avi_w, avi_h);

    size_t frame_buf_size = (size_t)avi_w * avi_h * 3;
    if (frame_buf_size < MAX_FRAME_SIZE) frame_buf_size = MAX_FRAME_SIZE;
    char *frame_buffer = (char *)malloc(frame_buf_size);
    if (!frame_buffer) {
        fprintf(stderr, "[TFT180] Error: Failed to allocate frame buffer\n");
        AVI_close(avi_file);
        return -1;
    }

    // 3. 逐帧处理循环
    printf("[TFT180] Running simulation... (Press [Space] to Pause/Resume, [ESC] to Exit)\n");
    long current_frame = 0;

    while (!tft180_is_closed() && current_frame < total_frames) {
        int keyframe = 0;

        if (AVI_set_video_position(avi_file, current_frame) != 0) {
            current_frame++;
            continue;
        }

        long frame_size = AVI_read_frame(avi_file, frame_buffer, &keyframe);
        if (frame_size <= 0) {
            break;
        }

        // AVI 图像转单通道灰度矩阵 mt9v03x_image
        for (int y = 0; y < MT9V03X_H; y++) {
            int src_y = (avi_h == MT9V03X_H) ? y : (y * avi_h / MT9V03X_H);
            int flipped_y = avi_h - 1 - src_y;
            for (int x = 0; x < MT9V03X_W; x++) {
                int src_x = (avi_w == MT9V03X_W) ? x : (x * avi_w / MT9V03X_W);
                int idx = (flipped_y * avi_w + src_x) * 3;
                uint8_t r = (uint8_t)frame_buffer[idx];
                uint8_t g = (uint8_t)frame_buffer[idx + 1];
                uint8_t b = (uint8_t)frame_buffer[idx + 2];
                mt9v03x_image[y][x] = (uint8_t)(0.299f * r + 0.587f * g + 0.114f * b);
            }
        }

        // 触发算法处理并刷新窗口
        mt9v03x_finish_flag = 1;
        image_main();
        tft180_flush();

        if (current_frame % 30 == 0) {
            printf("[TFT180] Frame: %ld / %ld\n", current_frame, total_frames);
        }

        tft180_delay(3);
        current_frame++;
    }

    printf("[TFT180] Video playback finished.\n");
    free(frame_buffer);
    AVI_close(avi_file);

    while (!tft180_is_closed()) {
        tft180_delay(50);
    }

    return 0;
}
