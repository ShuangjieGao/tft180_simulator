/*********************************************************************************************************************
 * 逐飞头文件集合 (zf_common_headfile.h) - PC 模拟器适配版
 ********************************************************************************************************************/

#ifndef _ZF_COMMON_HEADFILE_H_
#define _ZF_COMMON_HEADFILE_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "zf_common_typedef.h"
#include "zf_common_font.h"
#include "zf_common_function.h"
#include "zf_device_tft180.h"

// 摄像头 MT9V03X 默认分辨率 (160x64)
#ifndef MT9V03X_W
#define MT9V03X_W (160)
#endif

#ifndef MT9V03X_H
#define MT9V03X_H (64)
#endif

// 全局图像缓冲区与采集完成标志
extern uint8_t mt9v03x_image[MT9V03X_H][MT9V03X_W];
extern uint8_t mt9v03x_finish_flag;

// 陀螺仪与风扇等单片机全局变量模拟
extern float Z_360;
extern float Z_360_S;
extern float fuya_value;

// 定时器桩函数定义
#define TIM_8 8
#define TIMER_MS 1
static inline void timer_init(int tim, int mode) { (void)tim; (void)mode; }
static inline void timer_clear(int tim) { (void)tim; }
static inline uint32_t timer_get(int tim) { (void)tim; return 0; }
static inline void mt9v03x_init(void) {}

// 逐飞助手兼容桩定义
#define SEEKFREE_ASSISTANT_MT9V03X 0
#define SEEKFREE_ASSISTANT_WIFI_SPI 1
static inline void seekfree_assistant_interface_init(int type) { (void)type; }
static inline void seekfree_assistant_camera_information_config(int type, void *img, int w, int h) {
    (void)type; (void)img; (void)w; (void)h;
}
static inline void system_delay_ms(uint32_t ms) { (void)ms; }

#endif /* _ZF_COMMON_HEADFILE_H_ */

