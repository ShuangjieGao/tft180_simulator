#include "read_bmp.h"

/**
 * @brief 读取BMP文件并将其灰度数据存入预分配的二维数组
 * @param filename  BMP文件路径
 * @param image_array  目标二维数组指针
 * @param expected_width  期望的图像宽度（应等于MT9V03X_W）
 * @param expected_height 期望的图像高度（应等于MT9V03X_H）
 * @return int 成功返回0，失败返回-1
 */

int read_bmp_to_array(const char* filename, uint8_t image_array[MT9V03X_H][MT9V03X_W], int expected_width, int expected_height) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening BMP file");
        return -1;
    }

    BmpFileHeader file_header;
    BmpInfoHeader info_header;

    // 读取文件头和信息头
    if (fread(&file_header, sizeof(file_header), 1, file) != 1) {
        perror("Error reading BMP file header");
        fclose(file);
        return -1;
    }
    if (fread(&info_header, sizeof(info_header), 1, file) != 1) {
        perror("Error reading BMP info header");
        fclose(file);
        return -1;
    }

    // 检查是否是BMP文件
    if (file_header.bfType != 0x4D42) { // "BM"
        fprintf(stderr, "Error: Not a valid BMP file.\n");
        fclose(file);
        return -1;
    }

    // 检查图像尺寸是否匹配预期
    int bmp_width = info_header.biWidth;
    int bmp_height = abs(info_header.biHeight); // 高度可能为负，取绝对值
    if (bmp_width != expected_width || bmp_height != expected_height) {
        fprintf(stderr, "Error: BMP dimensions (%dx%d) do not match array dimensions (%dx%d).\n",
                bmp_width, bmp_height, expected_width, expected_height);
        fclose(file);
        return -1;
    }

    // 检查位深度，支持8位灰度和24位真彩色
    if (info_header.biBitCount != 8 && info_header.biBitCount != 24) {
        fprintf(stderr, "Error: Only 8-bit grayscale or 24-bit BMP files are supported. This file has %d bits per pixel.\n", info_header.biBitCount);
        fclose(file);
        return -1;
    }

    // 计算每行实际的字节数（包括可能的填充字节）
    int bytes_per_pixel = info_header.biBitCount / 8;
    int unpadded_row_size = bmp_width * bytes_per_pixel;
    int actual_row_size = ((unpadded_row_size + 3) / 4) * 4; // 每行字节数向上对齐到4的倍数

    int padding = actual_row_size - unpadded_row_size; // 计算每行的填充字节数

    // 定位到像素数据开始处
    fseek(file, file_header.bfOffBits, SEEK_SET);

    // 临时缓冲区，用于读取一行像素数据
    uint8_t* row_buffer = (uint8_t*)malloc(unpadded_row_size);
    if (!row_buffer) {
        perror("Error allocating memory for row buffer");
        fclose(file);
        return -1;
    }

    // BMP数据通常从下往上存储。如果高度值为正，则第一行是最后一行。
    // 我们希望 image_array[0][0] 对应左上角，所以可能需要逆序读行。
    int start_row, end_row, step;
    if (info_header.biHeight > 0) {
        // 高度为正，数据从下到上存储
                start_row = 0;
        end_row = bmp_height;
        step = 1;

    } else {
        // 高度为负，数据从上到下存储（罕见）
        start_row = bmp_height - 1;
        end_row = -1;
        step = -1;
    }

    // 逐行读取并处理
    for (int y = start_row; y != end_row; y += step) {
        if (fread(row_buffer, unpadded_row_size, 1, file) != 1) {
            perror("Error reading BMP pixel data");
            free(row_buffer);
            fclose(file);
            return -1;
        }

        // 跳过行末的填充字节
        fseek(file, padding, SEEK_CUR);

        // 处理该行的每个像素
        for (int x = 0; x < bmp_width; x++) {
            if (info_header.biBitCount == 8) {
                // 8位灰度：直接读取灰度值
                image_array[(info_header.biHeight > 0) ? (bmp_height - 1 - y) : y][x] = row_buffer[x]; 
            } else if (info_header.biBitCount == 24) {
                // 24位色：将BGR转换为灰度值（常用公式：Gray = 0.299*R + 0.587*G + 0.114*B）
                int blue = row_buffer[x * 3 + 0];
                int green = row_buffer[x * 3 + 1];
                int red = row_buffer[x * 3 + 2];
                uint8_t gray_value = (uint8_t)((299 * red + 587 * green + 114 * blue) / 1000);
                image_array[(info_header.biHeight > 0) ? (bmp_height - 1 - y) : y][x] = gray_value;
            }
        }
    }

    free(row_buffer);
    fclose(file);
    printf("BMP image successfully loaded into array.\n");
    return 0;
}
