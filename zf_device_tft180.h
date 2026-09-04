#ifndef _ZF_DEVICE_TFT180_H_
#define _ZF_DEVICE_TFT180_H_

#include <stdint.h>
#include "zf_common_typedef.h"
#include "zf_common_font.h"

// 常用别名定义
#define TFT180_6X8_FONT     0
#define TFT180_FONT_8X16    0
#define TFT180_CROSSWISE    0

typedef int tft180_dir_enum;
typedef int tft180_font_size_enum;
typedef int tft180_color_mode_enum;

#define TFT180_COLOR_MODE_RGB 0
#define TFT180_COLOR_MODE_BGR 1

// 核心模拟器控制与图元绘制 API
void tft180_init(void);
void tft180_clear(void);
void tft180_full(const uint16_t color);
void tft180_set_dir(int dir);
void tft180_set_font(int font);
void tft180_set_color(const uint16_t pen, const uint16_t bgcolor);

void tft180_draw_point(uint16_t x, uint16_t y, const uint16_t color);
void tft180_draw_line(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end, const uint16_t color);
void tft180_draw_rectangle(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end, const uint16_t color);
void tft180_fill_rectangle(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end, const uint16_t color);

// 图像渲染 API
void tft180_show_binary_image(uint16_t x, uint16_t y, const uint8_t *image, 
                             uint16_t width, uint16_t height, 
                             uint16_t dis_width, uint16_t dis_height);
void tft180_show_grayscale_image(uint16_t x, uint16_t y, const uint8_t *image, 
                                uint16_t width, uint16_t height, 
                                uint16_t dis_width, uint16_t dis_height);
void tft180_show_gray_image(uint16_t x, uint16_t y, const uint8_t *image, 
                           uint16_t width, uint16_t height, 
                           uint16_t dis_width, uint16_t dis_height, 
                           uint8_t threshold);
void tft180_show_rgb565_image(uint16_t x, uint16_t y, const uint16_t *image, 
                             uint16_t width, uint16_t height, 
                             uint16_t dis_width, uint16_t dis_height, 
                             uint8_t color_mode);

// 顶部文本显示桩函数（最简空实现，彻底避免乱码和字库依赖）
static inline void tft180_show_char(uint16_t x, uint16_t y, const char dat) { (void)x; (void)y; (void)dat; }
static inline void tft180_show_string(uint16_t x, uint16_t y, const char dat[]) { (void)x; (void)y; (void)dat; }
static inline void tft180_show_int(uint16_t x, uint16_t y, const int32_t dat, uint8_t num) { (void)x; (void)y; (void)dat; (void)num; }
static inline void tft180_show_uint(uint16_t x, uint16_t y, const uint32_t dat, uint8_t num) { (void)x; (void)y; (void)dat; (void)num; }
static inline void tft180_show_float(uint16_t x, uint16_t y, const double dat, uint8_t num, uint8_t pointnum) { (void)x; (void)y; (void)dat; (void)num; (void)pointnum; }
static inline void tft180_show_chinese(uint16_t x, uint16_t y, uint8_t size, const uint8_t *buf, uint8_t num, const uint16_t color) { (void)x; (void)y; (void)size; (void)buf; (void)num; (void)color; }

// 性能与窗口工具函数
void tft180_flush(void);
void tft180_delay(uint32_t ms);
int  tft180_is_closed(void);
int  tft180_is_paused(void);
void tft180_set_pause(int pause);
void tft180_performance_test(void);

#endif
