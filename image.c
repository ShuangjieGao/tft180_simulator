#include "image.h"

void image_main(void) {
    // 默认渲染摄像头原始图像至屏幕下半部 (起始行: 128 - MT9V03X_H)
    tft180_show_gray_image(0, 128 - MT9V03X_H, (const uint8_t *)mt9v03x_image, MT9V03X_W, MT9V03X_H, MT9V03X_W, MT9V03X_H, 0);
    tft180_show_string(0, 0, "hello");
}
