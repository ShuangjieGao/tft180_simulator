#ifndef CODE_READ_BMP_H_
#define CODE_READ_BMP_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MT9V03X_W 160  // 图像宽度
#define MT9V03X_H 50  // 图像高度

#pragma pack(push, 1) // 确保结构体紧凑对齐，防止编译器插入填充字节

typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BmpFileHeader;

typedef struct {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BmpInfoHeader;
#pragma pack(pop)

int read_bmp_to_array(const char* filename, uint8_t image_array[MT9V03X_H][MT9V03X_W], int expected_width, int expected_height);
#endif /* CODE_READ_BMP_H_ */
