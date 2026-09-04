#ifndef CODE_IMAGE_H_
#define CODE_IMAGE_H_

#include<zf_common_headfile.h>
// #define limit_threshold //限制阈值范围,注释关闭
#ifdef limit_threshold
#define max_threshold 150//最大阈值
#define min_threshold 55//最小阈值
#endif
// #define visual //环岛纯视觉,注释关闭
// #define WIFI_SPI
#define TFT180
#define show_image
// #define show_image_processed
#ifdef TFT180
#define show_line
// #define show_1
// #define show_2
// #define show_3
// #define show_4
// #define show_5
#endif

#define image_h 64
#define image_w MT9V03X_W
#define TSSL 127
#define TSEL 127-image_h+1
#define SPSL image_h-1
#define SL image_h-1
#define EL 0
#define LBRS 10
#define LBLS 5
#define RBLS 10
#define RBRS 5
#define MID_W image_w/2
#define man uint8
#define Mamba_Out 24

extern int16_t LSP;
extern int16_t RSP;
extern int16_t LB[image_h];
extern int16_t RB[image_h];
extern int16_t ML[image_h];
extern unsigned char mt9v03x_image_processed[image_h][image_w];

extern int16_t zebra_flag        ;
extern int16_t sancha_flag       ;
extern int16_t cross_flag        ;
extern int16_t left_circle_flag  ;
extern int16_t right_circle_flag ;
extern volatile float Motor_L_Speed_Base;
extern volatile float Motor_R_Speed_Base;
extern float mid;
void image_main(void);

/*
本摄像头思路为以(0,49)为起点建系
（0，0）
^                       （187，0）
|
|
|
|
|______________________>（187，49）
（0，49）
*/
#endif /* CODE_IMAGE_H_ */
