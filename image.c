#include "image.h"
#define IS_WHILE(x, th) (x > th)
#define IS_BLACK(x, th) (x <= th)
unsigned char mt9v03x_image_processed[image_h][image_w] = {0};
int16_t LSP = 0;
int16_t RSP = 0;
int16_t LB[image_h] = {0};
int16_t RB[image_h] = {0};
int16_t LB_valid[image_h] = {255};
int16_t RB_valid[image_h] = {255};
int16_t ML[image_h] = {0};
int16_t LBR[image_h] = {0};
int16_t RBR[image_h] = {0};
int16_t LLWC[2];
int16_t RLWC[2];
int16_t WC[image_w];
int16_t MLW[image_h] = {0};//Ҳ�����Լ�����Ȩ��
int16_t image_threshold = 0;//ͼ��ָ���ֵ
int16_t last_image_threshold = 0;//ͼ��ָ���ֵ
float TPOWP0 = 0;
float TPOWP1 = 0;
int16_t peak = 0;
int16_t TNOL = 0;//total_number_of_laps
float mid = MID_W;
float last_mid_line = MID_W;
float min_value = 1,p0 = -0.18,p1 = 25.0,mu = image_h/2;
// float min_value = 1,p0 = -0.18,p1 = 25.0,mu = 23;//������
// float min_value = 1,p0 = -0.15,p1 = 25.0,mu = 22;//
// float min_value = 1,p0 = -0.05,p1 = 10.0,mu = 25;//
int16_t MLW_init = 0;
int16_t fps = 0;
int16_t invalid0 = 0;
int16_t invalid1 = 0;
int16_t not_monotonic0 = 0;
int16_t not_monotonic1 = 0;
int16_t lost_line0;
int16_t lost_line1;
int16_t zebra_flag         = 0;
int16_t stop_flag          = 0;
int16_t sancha_flag        = 0;
int16_t cross_flag         = 0;
int16_t left_circle_flag   = 0;
int16_t right_circle_flag  = 0;
volatile float Motor_L_Speed_Base = 0;
volatile float Motor_R_Speed_Base = 0;
volatile float gyro_1 = 0, gyro_2 = 0;
typedef enum {
    ROAD_STRAIGHT,          // ֱ��
    ROAD_CURVE_LEFT,        // �����
    ROAD_CURVE_RIGHT,       // �����
    ROAD_CROSS,             // ʮ��·��
    ROAD_TRIDENT,           // ����·��
    ROAD_ROUNDABOUT_LEFT,   // �󻷵�
    ROAD_ROUNDABOUT_RIGHT,  // �һ���
    ROAD_UNKNOWN            // δ֪��·
} RoadType;

RoadType RT;
man what_can_i_say(void) {return Mamba_Out;}
// void image_copy(void) {
//     for (uint16_t_t i = 0; i < image_h; i++) {
//         for (uint16_t_t j = 0; j < image_w; j++) {
//             image[i][j] = mt9v03x_image[i][j];
//         }
//     }
// }

float my_fabs(float value) {
    if(value>=0) return value;
    else return -value;
}
int16_t limit_a_b(int16_t x, int16_t a, int16_t b) {
    if(x<a) x = a;
    if(x>b) x = b;
    return x;
}
float limit_a_b_f(float x, float a, float b) {
    if(x<a) x = a;
    if(x>b) x = b;
    return x;
}

int my_adapt_threshold(uint8 *image, uint16 col, uint16 row)   //ע�������ֵ��һ��Ҫ��ԭͼ��
{
    #define GrayScale 256
    uint16 width = col;
    uint16 height = row;
    int pixelCount[GrayScale];
    float pixelPro[GrayScale];
    int i, j;
    int pixelSum = width * height/4;
    int threshold = 0;
    uint8* data = image;  //ָ���������ݵ�ָ��
    for (i = 0; i < GrayScale; i++)
    {
        pixelCount[i] = 0;
        pixelPro[i] = 0;
    }
    uint32 gray_sum=0;
    //ͳ�ƻҶȼ���ÿ������������ͼ���еĸ���
    for (i = 0; i < height; i+=2)
    {
        for (j = 0; j < width; j+=2)
        {
            pixelCount[(int)data[i * width + j]]++;  //����ǰ�ĵ������ֵ��Ϊ����������±�
            gray_sum+=(int)data[i * width + j];       //�Ҷ�ֵ�ܺ�
        }
    }
    //����ÿ������ֵ�ĵ�������ͼ���еı���
    for (i = 0; i < GrayScale; i++)
    {
        pixelPro[i] = (float)pixelCount[i] / pixelSum;
    }
    //�����Ҷȼ�[0,255]
    float w0, w1, u0tmp, u1tmp, u0, u1, u, deltaTmp, deltaMax = 0;
    w0 = w1 = u0tmp = u1tmp = u0 = u1 = u = deltaTmp = 0;
    for (j = 0; j < GrayScale; j++)
    {
        w0 += pixelPro[j];  //��������ÿ���Ҷ�ֵ�����ص���ռ����֮��   ���������ֵı���
        u0tmp += j * pixelPro[j];  //�������� ÿ���Ҷ�ֵ�ĵ�ı��� *�Ҷ�ֵ
        w1=1-w0;
        u1tmp=gray_sum/pixelSum-u0tmp;
        u0 = u0tmp / w0;              //����ƽ���Ҷ�
        u1 = u1tmp / w1;              //ǰ��ƽ���Ҷ�
        u = u0tmp + u1tmp;            //ȫ��ƽ���Ҷ�
        deltaTmp = w0 * pow((u0 - u), 2) + w1 * pow((u1 - u), 2);
        if (deltaTmp > deltaMax)
        {
            deltaMax = deltaTmp;
            threshold = j;
        }
        if (deltaTmp < deltaMax)
        {
            break;
        }
    }
    return threshold;
}

uint8_t ostu_fast(uint8_t *image) {//ע�������ֵ��һ��Ҫ��ԭͼ��
    #define GrayScale 256
    int Pixel_Max=0;
    int Pixel_Min=255;
    uint16_t width = image_w;
    uint16_t height = image_h;
    int pixelCount[GrayScale];
    float pixelPro[GrayScale];
    int i, j, pixelSum = width * height/4;
    int16_t threshold = 0;
    uint8_t* data = image;  //ָ���������ݵ�ָ��
    for (i = 0; i < GrayScale; i++) {
        pixelCount[i] = 0;
        pixelPro[i] = 0;
    }
    uint32 gray_sum=0;
    for (i = 0; i < height; i+=2) {//ͳ�ƻҶȼ���ÿ������������ͼ���еĸ���
        for (j = 0; j < width; j+=2){
            pixelCount[(int)data[i * width + j]]++;  //����ǰ�ĵ������ֵ��Ϊ����������±�
            gray_sum+=(int)data[i * width + j];       //�Ҷ�ֵ�ܺ�
            if(data[i * width + j]>Pixel_Max) Pixel_Max=data[i * width + j];
            if(data[i * width + j]<Pixel_Min) Pixel_Min=data[i * width + j];
        }
    }
    for (i = Pixel_Min; i < Pixel_Max; i++) {//����ÿ������ֵ�ĵ�������ͼ���еı���
        pixelPro[i] = (float)pixelCount[i] / pixelSum;
    }
    float w0, w1, u0tmp, u1tmp, u0, u1, u, deltaTmp, deltaMax = 0;
    w0 = w1 = u0tmp = u1tmp = u0 = u1 = u = deltaTmp = 0;
    for (j = Pixel_Min; j < Pixel_Max; j++) {//�����Ҷȼ�[0,255]
        w0 += pixelPro[j];  //��������ÿ���Ҷ�ֵ�����ص���ռ����֮��   ���������ֵı���
        u0tmp += j * pixelPro[j];  //�������� ÿ���Ҷ�ֵ�ĵ�ı��� *�Ҷ�ֵ
        w1=1-w0;
        u1tmp=gray_sum/pixelSum-u0tmp;
        u0 = u0tmp / w0;              //����ƽ���Ҷ�
        u1 = u1tmp / w1;              //ǰ��ƽ���Ҷ�
        u = u0tmp + u1tmp;            //ȫ��ƽ���Ҷ�
        deltaTmp = (float)(w0 *w1* (u0 - u1)* (u0 - u1));
        if (deltaTmp > deltaMax) {
            deltaMax = deltaTmp;
            threshold = j;
        }
        if (deltaTmp < deltaMax) {
            break;
        }
    }
    #ifdef limit_threshold
    if(threshold>min_threshold&&threshold<max_threshold) image_threshold = threshold;
    else threshold = image_threshold;
    #endif
    return threshold;
}
void set_image_twovalues(uint8_t value) {
    uint8_t temp_value;
    for(uint8_t i=0;i<image_h;i++) {
        for(uint8_t j=0;j<image_w;j++){
            temp_value=mt9v03x_image[i][j];
            if(temp_value<value) {mt9v03x_image_processed[i][j]=0;}
            else {mt9v03x_image_processed[i][j]=255;}
        }
    }
}
void adding_line(int16_t BR[image_h], float br_a, float br_b) {
    // ��ͼ��ײ�(image_h-1)���ϱ������ڶ���(i=1)
    for (uint8_t i = image_h - 1; i > 0; i--) {
        // ������߽�ο���ֵ�����Թ�ʽ��LBR = a * image_w + b * i��
        BR[i] = (uint8_t)(br_a + br_b * i);
    }
}
uint8_t adding_line_a_b(int16_t border_array[image_h], int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    if (x1 == x2) {
        border_array[x1] = y1;
        return 0;
    }
    float k = (float)(y2 - y1) / (float)(x2 - x1);
    if (fabs(k) < 1e-10) {
        return 0; // ���ʧ��
    }
    for (uint8_t y = SL; y>EL; y--) {
        float x = (float)(y - y1)/k+x1;
        border_array[y] = limit_a_b((uint8_t)(x + 0.5), 1, image_w - 2);
    }
    return 0;
}
typedef struct {// �ṹ�壺�洢�����Ч�������
    int16_t row;    // ��Ч���к�
    int16_t col;    // ��Ч���к�
    int16_t found;  // 1=�ҵ���Ч�㣬0=δ�ҵ�
} NearestValidPoint;
NearestValidPoint find_nearest_valid_from_bottom(
    int16_t valid[image_h], int16_t border[image_h], int16_t start_row, int16_t end_row) {
    NearestValidPoint result = {0, 0, 0};  // ��ʼ����δ�ҵ���Ч��
    for (int16_t i = start_row; i > end_row+2; i--) {
        if (valid[i] == 0&&valid[i+1] == 0&&valid[i+2] == 0) {  // �ҵ���Ч��
            result.row = i+1;
            result.col = border[i+1];
            result.found = 1;
            break;  // �ҵ���һ�������·��ģ���Ч�㼴����
        }
    }
    return result;
}
// void mark_valid_points(void) { // �����������Ч�߽�㣨���� image_deal ����ã�
//     for (uint8_t i = SL; i > EL; i--) { // ��߽���Ч������δ���ߣ�LB[i]���ڱ�Ե�����������в�ֵ������
//         if (LB[i] >= 3 && LB[i] <= image_w - 4 && my_fabs((float)LB[i] - (float)LB[i+1]) < 4&&(LB[i] >= LB[i+1])) { 
//             LB_valid[i] = 1;// �����в�ֵ<5������ͻ���
//             tft180_draw_point(limit_a_b(LB[i]*160/image_w,0,159),i+TSEL,RGB565_GREEN);
//         } else {
//             LB_valid[i] = 0;
//         }// �ұ߽���Ч������δ���ߣ�RB[i]���ڱ�Ե�����������в�ֵ������
//         if (RB[i] >= 3 && RB[i] <= image_w - 4 && my_fabs((float)RB[i] - (float)RB[i+1]) < 4&&(RB[i] <= RB[i+1])) {
//             RB_valid[i] = 1;
//             tft180_draw_point(limit_a_b(RB[i]*160/image_w,0,159),i+TSEL,RGB565_GREEN);
//         } else {
//             RB_valid[i] = 0;
//         }
//         // printf("%d,",RB_valid[i]);
//     }
//     // printf("end\n");
// }
void mark_valid_points(void) { 
    // �ӵײ���SL�����ϣ�EL�������������ж���Ч�Բ���ֵö��ֵ
    for (uint8_t i = SL; i > EL; i--) { 
        LB_valid[i]=255;
        RB_valid[i]=255;
        // -------------------------- ��߽��ֵ��� --------------------------
        // ���ж��Ƿ���Ч���߽�Ϸ� + �����ݼ� + ��ֵ<4
        if (LB[i] >= 3 && LB[i] <= image_w - 4 &&  // �߽�����Ч��Χ
            (LB[i] >= LB[i+1]) &&                  // ��߽���µ��ϵݼ���������
            my_fabs((float)LB[i] - (float)LB[i-1]) < 4&&
            my_fabs((float)LB[i] - (float)LB[i+1]) < 4) { // �����в�ֵ<4
            LB_valid[i] = 0; // ��Ч����ֵ0
            #ifdef show_line
            tft180_draw_point(limit_a_b(LB[i]*160/image_w,0,159),i+TSEL,RGB565_PURPLE); // ��Ч�㻭��ɫ
            #endif
        } else {
            // ��Ч�����ݾ���ԭ��ֵ��Ӧö��ֵ��1~4��
            if (LB[i] < 3) {
                LB_valid[i] = 1; // ��߽糬�ޣ���ֵ1
            } else if (LB[i] > image_w - 4) {
                LB_valid[i] = 2; // �ұ߽糬�ޣ���ֵ2
            } else if (!(LB[i] >= LB[i+1])) {
                LB_valid[i] = 3; // �ǵ�������ֵ3
            } else if (my_fabs((float)LB[i] - (float)LB[i+1]) >= 4) {
                LB_valid[i] = 4; // ��ֵ���󣺸�ֵ4
            }
        }

        // -------------------------- �ұ߽��ֵ��� --------------------------
        if (RB[i] >= 3 && RB[i] <= image_w - 4 &&  // �߽�Ϸ�
            (RB[i] <= RB[i+1]) &&                  // �ұ߽���µ��ϵ�����������
            my_fabs((float)RB[i] - (float)RB[i-1]) < 4&&
            my_fabs((float)RB[i] - (float)RB[i+1]) < 4) { // �����в�ֵ<4
            RB_valid[i] = 0; // ��Ч����ֵ0
            // printf("RB_valid[%d]=%d\n",i,RB_valid[i]);
            #ifdef show_line
            tft180_draw_point(limit_a_b(RB[i]*160/image_w,0,159),i+TSEL,RGB565_GREEN); // ��Ч�㻭��ɫ
            #endif
        } else {
            // ��Ч����ֵ��Ӧö��ֵ
            if (RB[i] < 3) {
                RB_valid[i] = 1; // ��߽糬�ޣ���ֵ1
            } else if (RB[i] > image_w - 4) {
                RB_valid[i] = 2; // �ұ߽糬�ޣ���ֵ2
            } else if (!(RB[i] <= RB[i+1])) {
                RB_valid[i] = 3; // �ǵ�������ֵ3
            } else if (my_fabs((float)RB[i] - (float)RB[i+1]) >= 4) {
                RB_valid[i] = 4; // ��ֵ���󣺸�ֵ4
            }
        }
    }
}
// ����߽���Ч����е����Թ��ˣ����������ݼ��ĵ㣩
void filter_left_monotonic(uint8_t valid[image_h], uint8_t border[image_h], uint8_t start, uint8_t end,uint8_t mode) {
    // �ӵײ����ϱ�����start �� end��
    for (uint8_t i = start - 1; i > end; i--) {
        // ��������ǰ��Ч����һ��Ҳ��Ч�ĵ㣨ȷ���вο���
        if (valid[i] == 0 && valid[i + 1] == 0) {
            // ��߽�Ӧ�����ݼ�����ǰ�� >= ��һ�У�������Ϊ�ǵ�����Ч��
            if(mode==0){
                if (border[i] > border[i + 1]) {
                    valid[i] = 3;  // ������ö�ٱ�Ƿǵ���
                }
            }else{
                if (border[i] < border[i + 1]) {
                    valid[i] = 3;  // ������ö�ٱ�Ƿǵ���
                }
            }
        }
    }
}
void find_LWC(void){
    int16_t i, j, start_column=1, end_column=image_w-2;
    LLWC[0] = 0; LLWC[1] = 0; RLWC[0] = 0; RLWC[1] = 0;//��ʼ��
    for(i=0;i<image_w;i++)
    {
        WC[i] = 0;
    }
    //�����ң��������ϣ�����ȫͼ��¼��Χ�ڵ�ÿһ�а׵�����
    for (j =start_column; j<=end_column; j++)
    {
        for (i = SL; i > EL; i--)
        {
            if(mt9v03x_image_processed[i][j] == 0)
                break;
            else
                WC[j]++;
        }
    }

    //����������������
    LLWC[1] =0;
    for(i=start_column;i<=end_column;i++)
    {
        if (LLWC[1] < WC[i])//�������һ��
        {
            LLWC[0] = i;
            LLWC[1] = WC[i];              
        }
    }
    //���ҵ���������������
    RLWC[1] = 0;
    for(i=end_column;i>=start_column;i--)//��������ע���������ҵ���������λ�þͿ���ͣ��
    {
        if (RLWC[1] < WC[i])//�������һ��
        {
            RLWC[0] = i;
            RLWC[1] = WC[i];
        }
    }
    #ifdef show_line
    tft180_draw_line(LLWC[0],127-LLWC[1],LLWC[0],127,RGB565_CYAN);
    tft180_draw_line(RLWC[0],127-RLWC[1],RLWC[0],127,RGB565_CYAN);
    #endif
}
float fuck_down_width(void){
    float j=0;
    for(int i=SL;i>EL;i--){
        // printf("i%d\n",(uint8_t)(3.2*i+0.5));
        if(mt9v03x_image_processed[i][(uint8_t)(3.2*i+0.5)]==255)
        {
            j++;
        }
    }
    return (float)j/64;
}
// // ��������б�ʣ�������㣩
// static float calc_slope(float x1, float y1, float x2, float y2) {
//     if (fabs(x2 - x1) < 1e-6) {
//         return 0.0f;  // ��ֱ����б����Ϊ0��ʵ�ʿɸ��ݳ���������
//     }
//     return (y2 - y1) / (x2 - x1);
// }
typedef struct {
    float k;          // ������б��
    int16_t fit_success;  // 1=��ϳɹ���0=ʧ��
} FitParams;

FitParams linear_fit(int16_t valid[image_h], int16_t border[image_h], int16_t start, int16_t end) {
    FitParams res = {0.0f, 0};
    int32_t n = 0;  // ��Ч����������int32_t�������
    
    // ����ͳ������ȫ�����������㣬���һ��ת���㣩
    int32_t sum_x = 0;    // ��� x (�к�)
    int32_t sum_y = 0;    // ��� y (�߽�ֵ)
    int32_t sum_xy = 0;   // ��� x*y
    int32_t sum_x2 = 0;   // ��� x?

    // ������Ч�㣬��������ͳ����
    for (uint8_t i = start; i > end&&i>10; i--) {
        if (valid[i] == 0) {  // ��������Ч��
            int32_t x = (int32_t)i;       // �кţ�x��
            int32_t y = (int32_t)border[i];// �߽�ֵ��y��
            // printf("valid[%d]:%d,border[%d]:%d\n",i,valid[i],i,border[i]);
            
            sum_x += x;
            sum_y += y;
            sum_xy += x * y;
            sum_x2 += x * x;
            n++;
        }
    }

    // printf("n:%d,sum_xy:%d,sum_x:%d,sum_y:%d,sum_x2:%d\n",n,sum_xy,sum_x,sum_y,sum_x2);
    // ��Ч���������㣨���ݳ���������ֵ���˴�����8����Ҫ��
    if (n < 3) {
        res.fit_success = 0;
        return res;
    }

    // ����б�� k = (n*sum_xy - sum_x*sum_y) / (n*sum_x2 - sum_x?)
    // ���ӷ�ĸ�����������㣬���ת������������ٸ������������
    int32_t numerator = n * sum_xy - sum_x * sum_y;
    int32_t denominator = n * sum_x2 - sum_x * sum_x;

    // ��ĸΪ0��ֱ�ߴ�ֱ��ˮƽ��б��Ϊ0��
    if (abs(denominator) < 1) {  // �����Ƚϣ����⸡�����
        res.k = 0.0f;
    } else {
        // �����һ���ø����������б��
        res.k = (float)numerator / (float)denominator;
    }

    res.fit_success = 1;
    return res;
}
void fill_border(int16_t valid[image_h], int16_t border[image_h], FitParams fit_param, int16_t start, int16_t end) {
    if (!fit_param.fit_success) {
        return; // ���ʧ�ܣ�������
    }
    float fitted_y;
    NearestValidPoint nearest = find_nearest_valid_from_bottom(valid, border, start, end);
    if(nearest.found) {
        for (uint8_t i = start; i > end; i--) {
            if (valid[i] != 0) { // ����ȫ��Ч�㣨���ߵ㣩
                fitted_y = fit_param.k * (float)(i - nearest.row) + (float)border[nearest.row];
                uint8_t filled_y = (uint8_t)limit_a_b_f((fitted_y+0.5), 1, image_w - 2);
                border[i] = filled_y;
                valid[i] = 0; // ���Ϊ�Ѳ�ȫ����Ϊ��Ч�㣩
            }
        }
    }
    // else{
    //     printf("�ϴ���Ч��%d",last_SP_row);
    //     for (uint8_t i = start; i > end; i--) {
    //         if (valid[i] == 0) { // ����ȫ��Ч�㣨���ߵ㣩
    //             fitted_y = fit_param.k * (float)(i - SL) + (float)border[SL];
    //             uint8_t filled_y = (uint8_t)limit_a_b_f((fitted_y+0.5), 1, image_w - 2);
    //             border[i] = filled_y;
    //             valid[i] = 1; // ���Ϊ�Ѳ�ȫ����Ϊ��Ч�㣩
    //         }
    //     }
    // }
}
void fit_and_fill_borders(void) {
    // static FitParams last_lb_fit = {0};  // ��̬���������ϴ���߽���Ͻ��
    // static FitParams last_rb_fit = {0};  // ��̬���������ϴ��ұ߽���Ͻ��
    mark_valid_points();
    FitParams lb_fit = linear_fit(LB_valid, LBR, SL, (EL+SL)/2);//��ߵ�һ�����
    if(lb_fit.fit_success) {
        lb_fit.k=limit_a_b_f(lb_fit.k,-2.5,-0.5);
        // last_lb_fit=lb_fit;
        fill_border(LB_valid, LBR, lb_fit, SL, EL);
        // printf("1:%f\n",lb_fit.k);
    } else {
        FitParams lb_fit = linear_fit(LB_valid, LBR, SL, EL);//��ߵڶ������
        if(lb_fit.fit_success) {
            lb_fit.k=limit_a_b_f(lb_fit.k,-2.5,-0.5);
            // last_lb_fit=lb_fit;
            fill_border(LB_valid, LBR, lb_fit, SL, EL);
            // printf("2:%f\n",lb_fit.k);
        } 
        // else {fill_border(LB_valid, LBR, last_lb_fit, SL, EL);
        // printf("3:%f\n",last_lb_fit.k);
        // }
    }
    FitParams rb_fit = linear_fit(RB_valid, RBR, SL, (EL+SL)/2);//�ұߵ�һ�����
    if(rb_fit.fit_success) {
        rb_fit.k=limit_a_b_f(rb_fit.k,0.5,2.5);
        // last_rb_fit=rb_fit;
        fill_border(RB_valid, RBR, rb_fit, SL, EL);
        // printf("a:%f\n",rb_fit.k);
    } else {
        FitParams rb_fit = linear_fit(RB_valid, RBR, SL, EL);//�ұߵڶ������
        if(rb_fit.fit_success) {
            rb_fit.k=limit_a_b_f(rb_fit.k,0.5,2.5);
            // last_rb_fit=rb_fit;
            fill_border(RB_valid, RBR, rb_fit, SL, EL);
            // printf("b:%f\n",rb_fit.k);
        } 
        // else {fill_border(RB_valid, RBR, last_rb_fit, SL, EL);
        // printf("c:%f\n",last_rb_fit.k);
        // }
    }
}
void find_SP(uint8_t index[image_h][image_w]) {
    if(index[SPSL][image_w/2]==255&&index[SPSL][image_w/2+1]==255&&index[SPSL][image_w/2-1]==255) {
        for(uint16_t j=image_w/2;j>0;j--) {
            if(index[SPSL][j-1]==0&&index[SPSL][j]==255&&index[SPSL][j+1]==255) {LSP=j;break;}
            if(j-1==1) {LSP=1;break;}
        }
        for(uint16_t j=image_w/2;j<image_w-2;j++) {
            if(index[SPSL][j-1]==255&&index[SPSL][j]==255&&index[SPSL][j+1]==0) {RSP=j;break;}
            if(j+1==image_w-2) {RSP=image_w-2;break;}
        }
    }
    // else if(index[SPSL][image_w/4]==255&&index[SPSL][image_w/4-1]==255&&index[SPSL][image_w/4+1]==255) {
    //     for(uint16_t j=image_w/4;j>0;j--) {
    //         if(index[SPSL][j-1]==0&&index[SPSL][j]==255&&index[SPSL][j+1]==255) {LSP=j;break;}
    //         if(j-1==1) {LSP=1;break;}
    //     }
    //     for(uint16_t j=image_w/4;j<image_w-2;j++) {
    //         if(index[SPSL][j-1]==255&&index[SPSL][j]==255&&index[SPSL][j+1]==0) {RSP=j;break;}
    //         if(j+1==image_w-2) {RSP=image_w-2;break;}
    //     }
    // }
    // else if(index[SPSL][image_w*3/4]==255&&index[SPSL][image_w*3/4-1]==255&&index[SPSL][image_w*3/4+1]==255) {
    //     for(uint16_t j=image_w*3/4;j>0;j--) {
    //         if(index[SPSL][j-1]==0&&index[SPSL][j]==255&&index[SPSL][j+1]==255) {LSP=j;break;}
    //         if(j-1==1) {LSP=1;break;}
    //     }
    //     for(uint16_t j=image_w*3/4;j<image_w-2;j++) {
    //         if(index[SPSL][j-1]==255&&index[SPSL][j]==255&&index[SPSL][j+1]==0) {RSP=j;break;}
    //         if(j+1==image_w-2) {RSP=image_w-2;break;}
    //     }
    // }
    else if(index[SPSL][image_w/8]==255&&index[SPSL][image_w/8-1]==255&&index[SPSL][image_w/8+1]==255) {
        for(uint16_t j=image_w/8;j>0;j--) {
            if(index[SPSL][j-1]==0&&index[SPSL][j]==255&&index[SPSL][j+1]==255) {LSP=j;break;}
            if(j-1==1) {LSP=1;break;}
        }
        for(uint16_t j=image_w/8;j<image_w-2;j++) {
            if(index[SPSL][j-1]==255&&index[SPSL][j]==255&&index[SPSL][j+1]==0) {RSP=j;break;}
            if(j+1==image_w-2) {RSP=image_w-2;break;}
        }
    }
    else if(index[SPSL][image_w*7/8]==255&&index[SPSL][image_w*7/8-1]==255&&index[SPSL][image_w*7/8+1]==255) {
        for(uint16_t j=image_w*7/8;j>0;j--) {
            if(index[SPSL][j-1]==0&&index[SPSL][j]==255&&index[SPSL][j+1]==255) {LSP=j;break;}
            if(j-1==1) {LSP=1;break;}
        }
        for(uint16_t j=image_w*7/8;j<image_w-2;j++) {
            if(index[SPSL][j-1]==255&&index[SPSL][j]==255&&index[SPSL][j+1]==0) {RSP=j;break;}
            if(j+1==image_w-2) {RSP=image_w-2;break;}
        }
    }
    LB[SPSL]=LSP;
    RB[SPSL]=RSP;
}
// void find_SP(uint8_t index[image_h][image_w]) {
//     // ����ע��ײ�һ�У�image_h - 1Ϊ��ײ���������
//     const uint8_t bottom_row = image_h - 1;
//     // ��¼�������ɫ���ص���Ϣ
//     uint16_t max_white_count = 0;  // ������׵�����
//     uint16_t max_start_col = image_w / 2;  // ���������ʼ��
//     uint16_t max_end_col = image_w / 2;    // ������Ľ�����

//     // ����1��ɨ����ײ�һ�У��ҵ��������ɫ��������
//     uint16_t current_white_count = 0;
//     uint16_t current_start_col = 0;
//     for (uint16_t j = 1; j < image_w - 1; j++) {  // �ܿ�ͼ���Ե��0��image_w-1��
//         if (index[bottom_row][j] == 255) {  // �����׵�
//             if (current_white_count == 0) {
//                 current_start_col = j;  // ��¼��ǰ��������ʼ��
//             }
//             current_white_count++;
//         } else {  // �����ǰ׵㣨0����������ǰ��������
//             // �����������Ϣ
//             if (current_white_count > max_white_count) {
//                 max_white_count = current_white_count;
//                 max_start_col = current_start_col;
//                 max_end_col = j - 1;  // ��ǰ����0����������ǰһ��
//             }
//             current_white_count = 0;  // ���õ�ǰ����
//         }
//     }
//     // ����ѭ������ʱ���ڰ��������
//     if (current_white_count > max_white_count) {
//         max_white_count = current_white_count;
//         max_start_col = current_start_col;
//         max_end_col = image_w - 2;  // ��������β���ܿ���Ե��
//     }

//     // �ݴ�����δ�ҵ���Ч�����������׵�̫�٣���Ĭ������
//     uint16_t center_col;
//     if (max_white_count < 3) {  // ������Ҫ3�������׵����Ϊ��Ч����
//         center_col = image_w / 2;
//     } else {
//         center_col = (max_start_col + max_end_col) / 2;  // �������������
//     }

//     // ����2����������������г�����������߽�LSP���ںڰף�0,255,255��
//     for (uint16_t j = center_col; j > 0; j--) {
//         // ȷ��j-1��j+1��Խ��
//         if (j >= 1 && j + 1 < image_w) {
//             if (index[SPSL][j-1] == 0 && index[SPSL][j] == 255 && index[SPSL][j+1] == 255) {
//                 LSP = j;
//                 break;
//             }
//         }
//         if (j == 1) {  // �������Ե��ǿ����Ϊ1
//             LSP = 1;
//             break;
//         }
//     }

//     // ����3����������������г����������ұ߽�RSP���׺ںڣ�255,255,0��
//     for (uint16_t j = center_col; j < image_w - 1; j++) {
//         // ȷ��j-1��j+1��Խ��
//         if (j - 1 >= 0 && j + 1 < image_w) {
//             if (index[SPSL][j-1] == 255 && index[SPSL][j] == 255 && index[SPSL][j+1] == 0) {
//                 RSP = j;
//                 break;
//             }
//         }
//         if (j == image_w - 2) {  // �����ұ�Ե��ǿ����Ϊimage_w-2
//             RSP = image_w - 2;
//             break;
//         }
//     }

//     // �������
//     LB[SPSL] = LSP;
//     RB[SPSL] = RSP;
// }
// #define THRESHOLD 5
// #define MAX_SEGMENTS 20

// // 1. ����ʧЧԭ��ļ�������Ӧ BorderValidity ö�٣�
// typedef struct {
//     uint8_t left_border;    // INVALID_LEFT_BORDER (1) ����
//     uint8_t right_border;   // INVALID_RIGHT_BORDER (2) ����
//     uint8_t not_monotonic;  // INVALID_NOT_MONOTONIC (3) ����
//     uint8_t diff_too_big;   // INVALID_DIFF_TOO_BIG (4) ����
// } InvalidReasonCount;

// // 2. ��չ��ķֶνṹ�����ԭ Segment��
// typedef struct {
//     uint8_t is_valid;       // 1=��Ч�Σ�ȫΪVALID����0=��Ч�Σ���ʧЧԭ��
//     uint8_t length;         // �γ��ȣ�������
//     uint8_t start_row;      // ����ʼ�У�ͼ���кţ�
//     uint8_t end_row;        // �ν����У�ͼ���кţ�
//     InvalidReasonCount reasons;  // ����Ч����Ч����ʧЧԭ�������
// } RoadSegment;
// // ȫ�ֱ������洢���ұ߽�ķֶν������ʧЧԭ��
// RoadSegment left_final_segments[MAX_SEGMENTS];
// RoadSegment right_final_segments[MAX_SEGMENTS];
// int left_final_segCount = 0;
// int right_final_segCount = 0;

// // ��ǿ�棺���ֶβ�ͳ��ʧЧԭ�򣬽������ RoadSegment ����
// void detectSequenceMemoryOptimized(
//     uint8_t* valid_array,  // ���룺��Ч�����飨VALID/INVALID_*��
//     int total_rows,               // ���룺��������image_h��
//     RoadSegment final_segments[MAX_SEGMENTS],  // ������ֶν������ʧЧԭ��
//     int *final_segCount           // �������Ч�ֶ�����
// ) {
//     RoadSegment temp_segments[MAX_SEGMENTS] = {0};  // ��ʱ�洢���зֶ�
//     int temp_segCount = 0;                          // ��ʱ�ֶμ���

//     // �������
//     *final_segCount = 0;
//     memset(final_segments, 0, sizeof(RoadSegment) * MAX_SEGMENTS);

//     if (total_rows == 0) return;  // �����봦��

//     // -------------------------- ��һ�������������У����������� --------------------------
//     // ��ʼ����һ�Σ��ӵ�0�п�ʼ
//     temp_segments[0].start_row = 0;
//     temp_segments[0].is_valid = (valid_array[0] == 0) ? 1 : 0;
//     if (!temp_segments[0].is_valid) {
//         // ��һ��Ϊ��Ч�Σ�ͳ���׸�ʧЧԭ��
//         switch(valid_array[0]) {
//             case 1:  temp_segments[0].reasons.left_border++;  break;
//             case 2: temp_segments[0].reasons.right_border++; break;
//             case 3:temp_segments[0].reasons.not_monotonic++;break;
//             case 4: temp_segments[0].reasons.diff_too_big++; break;
//             default: break;
//         }
//     }
//     temp_segCount = 1;

//     // ����ʣ���У�����������
//     for (int row = 1; row < total_rows; row++) {
//         uint8_t current_valid = (valid_array[row] == 0) ? 1 : 0;
//         RoadSegment* last_seg = &temp_segments[temp_segCount - 1];

//         if (current_valid == last_seg->is_valid) {
//             // ����һ��ͬ״̬����չ��ǰ��
//             last_seg->length = row - last_seg->start_row + 1;  // ���³���
//             last_seg->end_row = row;                           // ���½�����
//             if (!current_valid) {
//                 // ��Ч�Σ��ۼӵ�ǰ�е�ʧЧԭ��
//                 switch(valid_array[row]) {
//                     case 1:  last_seg->reasons.left_border++;  break;
//                     case 2: last_seg->reasons.right_border++; break;
//                     case 3:last_seg->reasons.not_monotonic++;break;
//                     case 4: last_seg->reasons.diff_too_big++; break;
//                     default: break;
//                 }
//             }
//         } else {
//             // ״̬�л����½��ֶ�
//             if (temp_segCount >= MAX_SEGMENTS) break;  // �������
//             temp_segments[temp_segCount].start_row = row;
//             temp_segments[temp_segCount].is_valid = current_valid;
//             temp_segments[temp_segCount].length = 1;
//             temp_segments[temp_segCount].end_row = row;
//             // �¶�Ϊ��Ч��ʱ��ͳ���׸�ԭ��
//             if (!current_valid) {
//                 switch(valid_array[row]) {
//                     case 1:  temp_segments[temp_segCount].reasons.left_border++;  break;
//                     case 2: temp_segments[temp_segCount].reasons.right_border++; break;
//                     case 3:temp_segments[temp_segCount].reasons.not_monotonic++;break;
//                     case 4: temp_segments[temp_segCount].reasons.diff_too_big++; break;
//                     default: break;
//                 }
//             }
//             temp_segCount++;
//         }
//     }

//     // -------------------------- �ڶ��������˶̷ֶΣ�����洢�����ս�� --------------------------
//     // ���˹��򣺱������ȡ�THRESHOLD�ĶΣ������һ�Σ���ײ������2*THRESHOLD
//     for (int i = temp_segCount - 1; i >= 0; i--) {
//         RoadSegment* seg = &temp_segments[i];
//         // �����������ײ������������������
//         uint8_t min_length = (i == temp_segCount - 1) ? (3 * THRESHOLD) : THRESHOLD;
//         if (seg->length >= min_length) {
//             if (*final_segCount < MAX_SEGMENTS) {
//                 final_segments[*final_segCount] = *seg;  // ���Ʒֶ���Ϣ����ʧЧԭ��
//                 (*final_segCount)++;
//             }
//         }
//     }

//     // -------------------------- �������������������ѡ�� --------------------------
//     printf("\n�ֶν������%d�Σ���\n", *final_segCount);
//     for (int i = 0; i < *final_segCount; i++) {
//         RoadSegment* seg = &final_segments[i];
//         if (seg->is_valid) {
//             printf("��%d����Ч������=%d����%d-%d��\n",
//                    i+1, seg->length, seg->start_row, seg->end_row);
//         } else {
//             printf("��%d����Ч������=%d����%d-%d����ԭ��=[����:%d, �ҳ���:%d, �ǵ���:%d, ��ֵ��:%d]\n",
//                    i+1, seg->length, seg->start_row, seg->end_row,
//                    seg->reasons.left_border,
//                    seg->reasons.right_border,
//                    seg->reasons.not_monotonic,
//                    seg->reasons.diff_too_big);
//         }
//     }
// }


// // ��·�����ж�������
// RoadType judge_road_type(void) {
//     mark_valid_points();
//     detectSequenceMemoryOptimized(LB_valid,image_h,left_final_segments,&left_final_segCount);
//     detectSequenceMemoryOptimized(RB_valid,image_h,right_final_segments,&right_final_segCount);
//     // printf("Left Final Segments (Reverse Stored):\n");
//     // for (int i = 0; i < left_final_segCount; i++) {
//     //     printf("State: %d, Length: %d\n", left_final_segments[i].state, left_final_segments[i].length);
//     // }
//     // printf("Right Final Segments (Reverse Stored):\n");
//     // for (int i = 0; i < right_final_segCount; i++) {
//     //     printf("State: %d, Length: %d\n", right_final_segments[i].state, right_final_segments[i].length);
//     // }
//     if (left_final_segments[0].is_valid == 1 && right_final_segments[0].is_valid == 1 ) {       //һ(1)(1)
//         if(left_final_segments[1].is_valid == 0&&right_final_segments[1].is_valid == 0){        //��(0)(0)
//             if(left_final_segments[1].length>20&&right_final_segments[1].length>20){            //��(0)(0)>20
//                 printf("ROAD_CROSS");
//                 return ROAD_CROSS;
//             }
//         }
//         if(left_final_segments[0].length>30&&right_final_segments[0].length>30){                //һ(1)(1)>30
//             printf("ROAD_STRAIGHT");
//             return ROAD_STRAIGHT;
//         }
//     }
//     if (left_final_segments[0].is_valid == 0 && right_final_segments[0].is_valid == 0 ) {       //һ(0)(0)
//         if(left_final_segments[1].is_valid == 1&&right_final_segments[1].is_valid == 1){        //��(1)(1)
//             if(left_final_segments[0].length>20&&right_final_segments[0].length>20){            //һ(0)(0)>20
//                 printf("ROAD_CROSS");
//                 return ROAD_CROSS;
//             }
//         }
//     }
//     if (left_final_segments[0].is_valid == 0 && right_final_segments[0].is_valid == 1 ) {       //һ(0)(1)
//         if(left_final_segments[0].length>30&&right_final_segments[0].length>20){                //һ(0)(1)>20
//             if(right_final_segments[1].reasons.not_monotonic>5){                                //�ұ߲�����
//                 printf("б��ʮ�� ROAD_CROSS");
//                 return ROAD_CROSS;
//             }else{
//                 printf("ROAD_CURVE_LEFT");
//                 return ROAD_CURVE_LEFT;
//             }
//         }
//         // if(left_final_segments[1].is_valid == 1&&right_final_segments[1].is_valid == 1){        //��(1)(1)
//         //     if(left_final_segments[0].length>20&&right_final_segments[0].length>20){            //һ(0)(0)>20
//         //         return ROAD_CROSS;
//         //     }
//         // }
//     }
//     if (left_final_segments[0].is_valid == 1 && right_final_segments[0].is_valid == 0 ) {       //һ(1)(0)
//         if(left_final_segments[0].length>20&&right_final_segments[0].length>30){                //һ(1)(0)>20
//             if(left_final_segments[1].reasons.not_monotonic>5){                                 //��߲�����
//                 printf("б��ʮ�� ROAD_CROSS");
//                 return ROAD_CROSS;
//             }else{
//                 printf("ROAD_CURVE_RIGHT");
//                 return ROAD_CURVE_RIGHT;
//             }
//         }
//         // if(left_final_segments[1].is_valid == 1&&right_final_segments[1].is_valid == 1){        //��(1)(1)
//         //     if(left_final_segments[0].length>20&&right_final_segments[0].length>20){
//         //         return ROAD_CROSS;
//         //     }
//         // }
//     }
//     printf("ROAD_UNKNOWN");
//     return ROAD_UNKNOWN;
// }

void image_deal(uint8_t index[image_h][image_w]) {
    find_SP(index);
    uint8_t LP=LSP;
    uint8_t RP=RSP;
    LB[SL]=LSP;
    RB[SL]=RSP;
    LBR[SL]=LSP;
    RBR[SL]=RSP;
    for(uint8_t i=SL;i>EL;i--) {
        uint8_t left_search_judge=0;
        uint8_t mid_start_left_search_judge=0;
        for(uint8_t j=LP;j<LP+LBRS;j++) {                                                 //����һ�е���ߵ�������
            if(LP<3){
                mid_start_left_search_judge=1;
                break;
            }
            // tft180_draw_point(limit_a_b(j *160/image_w,0,159),i+TSEL,RGB565_RED);
            if(index[i][j-1]==0&&index[i][j]==255&&index[i][j+1]==255) {LP=j;break;}    //ֱ���ѵ�
            else if(j==image_w-2) {LP=image_w-5;break;}                                 //�����ұ߽�
            else if(j==LP+LBRS-1) {left_search_judge=1;break;}                          //û���ѵ���������
        }
        if(left_search_judge==1) {
            for(uint8_t j=LP;j>LP-LBLS;j--) {                                             //����һ�е���ߵ�������
                // tft180_draw_point(limit_a_b(j *160/image_w,0,159),i+TSEL,RGB565_RED);
                if(index[i][j-1]==0&&index[i][j]==255&&index[i][j+1]==255&&j<image_w-5) {LP=j;break;}//ֱ���ѵ�
                else if(j==1) {LP =1;mid_start_left_search_judge=1;break;}//������߽�
                else if(j==LP-LBLS+1) {mid_start_left_search_judge=1;break;}//û���ѵ����м�������
            }
        }
        if(mid_start_left_search_judge==1) {
            for(uint8_t j=(LSP+RSP)/2;j>0;j--) {//���м���������
                // tft180_draw_point(limit_a_b(j *160/image_w,0,159),i+TSEL,RGB565_RED);
                if(index[i][j-1]==0&&index[i][j]==255&&index[i][j+1]==255) {LP=j;break;}
                else if(j==1) {LP=1;break;}//������߽�
            }
        }
        uint8_t right_search_judge=0;
        uint8_t mid_start_right_search_judge=0;
        for(uint8_t j=RP;j>RP-RBLS;j--) {
                        if(RP>image_w-4){
                mid_start_right_search_judge=1;
                break;
            }
            // tft180_draw_point(limit_a_b(j *160/image_w,0,159),i+TSEL,RGB565_39C5BB);
            if(index[i][j-1]==255&&index[i][j]==255&&index[i][j+1]==0) {RP=j;break;}
            else if(j==1) {RP=4;break;}
            else if(j==RP-RBLS+1) {right_search_judge=1;break;}
        }
        if(right_search_judge==1) {
            for(uint8_t j=RP;j<RP+RBRS;j++) {
                // tft180_draw_point(limit_a_b(j *160/image_w,0,159),i+TSEL,RGB565_39C5BB);
                if(index[i][j-1]==255&&index[i][j]==255&&index[i][j+1]==0&&j>4) {RP=j;break;}
                else if(j==image_w-2) {RP=image_w-2;mid_start_right_search_judge=1;break;}
                else if(j==RP+RBRS-1) {mid_start_right_search_judge=1;break;}
            }
        }
        if(mid_start_right_search_judge==1) {
            for(uint8_t j=(LSP+RSP)/2;j<image_w-1;j++) {
                // tft180_draw_point(limit_a_b(j *160/image_w,0,159),i+TSEL,RGB565_39C5BB);
                if(index[i][j-1]==255&&index[i][j]==255&&index[i][j+1]==0) {RP=j;break;}
                else if(j==image_w-2) {RP=image_w-2;break;}
            }
        }
        LB[i]=limit_a_b(LP,1,image_w-2);
        LBR[i]=limit_a_b(LP,1,image_w-2);
        RB[i]=limit_a_b(RP,1,image_w-2);
        RBR[i]=limit_a_b(RP,1,image_w-2);
    }
}

uint8_t count_lost_line(uint8_t start_line,uint8_t end_line,uint8_t mode) {
    uint8_t lost_line=0;
    if(!mode) {
        for(uint8_t i=start_line-1;i>end_line&&i>EL+1;i--) {
            if(LB[i]<3) {lost_line++;}
        }
    }
    else {
        for(uint8_t i=start_line-1;i>end_line&&i>EL+1;i--) {
            if(RB[i]>image_w-4) {lost_line++;}
        }
    }
    return lost_line;
}

uint8_t count_not_monotonic(uint8_t start_line,uint8_t end_line,uint8_t mode) {
    uint8_t not_monotonic=0;
    if(!mode) {
        for(uint8_t i=start_line-1;i>end_line&&i>EL+1;i--) {
            if(LB[i]<LB[i+1]) {not_monotonic++;}
        }
    }
    else {
        for(uint8_t i=start_line-1;i>end_line&&i>EL+1;i--) {
            if(RB[i]>RB[i+1]) {not_monotonic++;}
        }
    }
    return not_monotonic;
}

uint8_t count_invalid(uint8_t start_line,uint8_t end_line,uint8_t mode) {
    uint8_t invalid_num=0;
    if(!mode) {
        for(uint8_t i=start_line-1;i>end_line&&i>EL+1;i--) {
            if(LB_valid[i]!=0) {invalid_num++;}
        }
    }
    else {
        for(uint8_t i=start_line-1;i>end_line&&i>EL+1;i--) {
            if(RB_valid[i]!=0) {invalid_num++;}
        }
    }
    return invalid_num;
}
uint8_t find_corner(uint8_t start_line,uint8_t end_line,uint8_t mode,uint8_t strictness_leniency) {
    #define STEP 4
    if(mode==0) {
        for(uint8_t i=start_line-3;i>end_line&&i>EL+2;i--) {
            int delta1=LB[i]-LB[i-1];
            int delta2=LB[i+1]-LB[i];
            int delta3=LB[i+2]-LB[i+1];
            if(delta1>strictness_leniency
                &&my_fabs(delta2)<STEP
                &&my_fabs(delta3)<STEP
                &&my_fabs(delta2-delta3)<2) {
                #ifdef show_line
                tft180_draw_point(limit_a_b(LB[i]*160/image_w,0,159),i+TSEL,RGB565_YELLOW);
                #endif
                if(LB[i]<5) {continue;}
                else {return i;}
            }
        }
    }
    else if(mode==1) {
        for(uint8_t i=start_line-3;i>end_line&&i>EL+2;i--) {
            int delta1=RB[i]-RB[i-1];
            int delta2=RB[i+1]-RB[i];
            int delta3=RB[i+2]-RB[i+1];
            if(delta1<-strictness_leniency
                &&my_fabs(delta2)<STEP
                &&my_fabs(delta3)<STEP
                &&my_fabs(delta2-delta3)<2) {
                    #ifdef show_line
                tft180_draw_point(limit_a_b(RB[i]*160/image_w,0,159),i+TSEL,RGB565_YELLOW);
                #endif
                if(RB[i]>image_w-6) {continue;}
                else {return i;}
            }
        }
    }
    else if(mode==2) {
        for(uint8_t i=end_line+2;i<start_line&&i<SL;i++) {
            int delta1=LB[i-1]-LB[i-2];
            int delta2=LB[i]-LB[i-1];
            int delta3=LB[i+1]-LB[i];
            if(delta3<-strictness_leniency
                &&my_fabs(delta1)<STEP
                &&my_fabs(delta2)<STEP
                &&my_fabs(delta1-delta2)<2) {
                    #ifdef show_line
                tft180_draw_point(limit_a_b(LB[i]*160/image_w,0,159),i+TSEL,RGB565_YELLOW);
                #endif
                if(LB[i]<5) {continue;}
                else {return i;}
            }
        }
    }
    else if(mode==3) {
        for(uint8_t i=end_line+2;i<start_line&&i<SL;i++) {
            int delta1=RB[i-1]-RB[i-2];
            int delta2=RB[i]-RB[i-1];
            int delta3=RB[i+1]-RB[i];
            if(delta3>strictness_leniency
                &&my_fabs(delta1)<STEP
                &&my_fabs(delta2)<STEP
                &&my_fabs(delta1-delta2)<2) {
                    #ifdef show_line
                tft180_draw_point(limit_a_b(RB[i]*160/image_w,0,159),i+TSEL,RGB565_YELLOW);
                #endif
                if(RB[i]>image_w-6) {continue;}
                else {return i;}
            }
        }
    }
    return 0;
}
uint8_t find_valid_corner(uint8_t start_line,uint8_t end_line,uint8_t mode) {
    if(mode==0) {
        for(uint8_t i=start_line;i>end_line&&i>EL+3;i--) {
            if(!LB_valid[i]&&!LB_valid[i-1]&&LB_valid[i-2]&&LB_valid[i-3]) return i;
        }
    }
    else if(mode==1) {
        for(uint8_t i=start_line;i>end_line&&i>EL+3;i--) {
            if(!RB_valid[i]&&!RB_valid[i-1]&&RB_valid[i-2]&&RB_valid[i-3]) return i;
        }
    }
    else if(mode==2) {
        for(uint8_t i=end_line;i<start_line&&i<SL-2;i++) { 
            if(!LB_valid[i]&&!LB_valid[i+1]&&LB_valid[i+2]&&LB_valid[i+3]&&i>EL)return i;
        }
    }
    else if(mode==3) {
        for(uint8_t i=end_line;i<start_line&&i<SL-2;i++) { 
            if(!RB_valid[i]&&!RB_valid[i+1]&&RB_valid[i+2]&&RB_valid[i+3]&&i>EL)return i;
        }
    }
    return 0;
}
typedef struct{
    float left;
    float mid;
    float right;
} TPOWP_position;
TPOWP_position TPOWP(uint8_t mode) {//The_proportion_of_white_pixels
    TPOWP_position position={0,0,0};
    if(!mode) {
        for(uint8_t i=image_h-1;i>0;i--) {
            if(mt9v03x_image_processed[i][image_w/2-10]==255) {position.left++;}
            if(mt9v03x_image_processed[i][image_w/2]==255) {position.mid++;}
            if(mt9v03x_image_processed[i][image_w/2+10]==255) {position.right++;}
        } position.left/=(image_h-1);position.mid/=(image_h-1);position.right/=(image_h-1);
        return position;
    }
    else {
        for(uint8_t i=image_w-1;i>0;i--) {
            if(mt9v03x_image_processed[image_h/2][i]==255) {position.mid++;}
        } position.mid/=(image_w-1);
        return position;
    }
}
// uint8_t edge_points[image_w];
// void sancha(void){
//     for (uint8_t col = 5; col < image_w - 5; col++) { // �ܿ���Ե5�У������
//         for (uint8_t row = SL; row > EL+1; row--) {
//             if (image_processed[row][col] == 255 && image_processed[row-1][col] == 0) {
//                 edge_points[col] = row;
//                 break;
//             }
//             else edge_points[col] = EL;
//         }
//     }
// }
uint8_t white_num=0;
uint8_t edge_points[image_w];
void sancha(void){
    white_num=0;
    for (uint8_t col = 0; col < image_w; col++) {
        edge_points[col] = EL;
    }
    for (uint8_t col = 5; col < image_w - 5; col++) {
        uint8_t white = 0;
        for (uint8_t row = SL; row > EL+1; row--) {
            if (mt9v03x_image_processed[row][col] == 255) {
                white++;
                if (white >= 2 && mt9v03x_image_processed[row-1][col] == 0) {
                    edge_points[col] = row;
                    white_num++;
                    break;
                }
            } else {
                white = 0;
            }
        }
    }
}
uint8_t stop_judge(void){
    uint32 black_p=0;
    for (uint8_t i = SL; i > 39; i--) {
        for(uint8_t j=0;j<image_w;j++)
        {
            if(mt9v03x_image_processed[i][j]==0){
                black_p++;
            }
        }
    }
    if(black_p>10*image_w*9/10)
    {
        return 1;
    }
    else return 0;
}
uint8_t sancha_valid1=0,sancha_valid2=0;
void sancha_ass(float TPOWP0){
    sancha();
    sancha_valid1=0;
    sancha_valid2=0;
    for (uint8_t col = 1; col < image_w/2; col++) {
        // edge_points[col]<(uint8_t)(limit_a_b_f(TPOWP0,0,0.9)*SL)
        if(edge_points[col]
        &&edge_points[col-1]
        &&edge_points[col]>edge_points[col-1]
        &&my_fabs(edge_points[col] - edge_points[col-1]) < 5){
            sancha_valid1++;
        }

    }
    for (uint8_t col = image_w/2; col < image_w; col++) {
        if(edge_points[col]
        &&edge_points[col-1]
        &&edge_points[col]<edge_points[col-1]
        &&my_fabs(edge_points[col] - edge_points[col-1]) < 5){
            sancha_valid2++;
        }
    }
}
uint8_t fuck_width=0;
void fuck_sancha(void){
    fuck_width=0;
    for (uint8_t i = 0; i < image_h; i++) {
        if(mt9v03x_image_processed[i][2]==255&&mt9v03x_image_processed[i][image_w-3]==255&&mt9v03x_image_processed[i][image_w/2]==255)
        {
            fuck_width++;
        }
    }
}
// void sancha_judge(void) {
//     static float gyro_min=0,gyro_max=0;
//     tft180_show_float  (0 ,64,gyro_min,3,1);
//     tft180_show_float  (32,64,gyro_max,3,1);
//     if(invalid0>25
//     &&invalid1>25
//     &&not_monotonic0>5
//     &&not_monotonic1>5
//     &&!left_circle_flag
//     &&!right_circle_flag
//     // &&my_fabs(sancha_valid1-sancha_valid2)<3
//     // &&count_invalid(59,0,0)>40
//     // &&count_invalid(59,0,1)>40
//     // &&TPOWP(0).mid<0.6
//     // &&TPOWP(1).mid>0.9
//     ) {
//         gyro_min=gyro_min<Z_360?gyro_min:Z_360;
//         gyro_max=gyro_max>Z_360?gyro_max:Z_360;
//         if(TNOL%2==0) {
//             sancha_flag=1;
//         }
//         else {
//             sancha_flag=2;
//         }
//     }
//     if(sancha_flag==1){
//         if(fuck_width>25){
//             sancha_flag=3;
//         }
//     }
//     if(sancha_flag==3) {
//         adding_line(LBR,image_w*0.6,-1.6);
//         adding_line(RBR,image_w-1,0);
//         gyro_min=gyro_min<Z_360?gyro_min:Z_360;
//         gyro_max=gyro_max>Z_360?gyro_max:Z_360;
//         if(my_fabs(gyro_max-gyro_min)>15) {
//             sancha_flag=5;
//         gyro_min=gyro_min<Z_360?gyro_min:Z_360;
//         gyro_max=gyro_max>Z_360?gyro_max:Z_360;
//         }
//     }
//     if(sancha_flag==5){
//         if(invalid0<15&&invalid1<15){
//             sancha_flag=7;
//         }
//     }
//     if(sancha_flag==7)
//     {
//         gyro_min=gyro_min<Z_360?gyro_min:Z_360;
//         gyro_max=gyro_max>Z_360?gyro_max:Z_360;
//         if(my_fabs(gyro_max-gyro_min)>55)
//         {
//             sancha_flag=9;
//         }
//     }
//     if(sancha_flag==9)
//     {
//         adding_line(LBR,image_w*0.7,-1.6);
//         if(my_fabs(Z_360-gyro_min)<35)
//         {
//             sancha_flag=0;
//         }
//     }

//     if(sancha_flag==2&&fuck_width>25) {
//         adding_line(LBR,0,0);
//         adding_line(RBR,image_w*0.3,1.6);
//         if(count_lost_line(49,0,1)<10
//         &&TPOWP0>0.9) {
//             sancha_flag=4;
//         }
//     }
//     if(sancha_flag==4)
//     {
//         if(fuck_width>30)
//         {
//             sancha_flag=6;
//         }
//     }
//     if(sancha_flag==6)
//     {
//         adding_line(RBR,image_w*0.3,1.6);
//         if(count_lost_line(SL,EL,1)<15)
//         {
//             sancha_flag=0;
//         }
//     }
// }
void sancha_judge(void) {
    fuck_sancha();
    if(
    !left_circle_flag
    &&!right_circle_flag
    &&invalid0>50
    &&invalid1>50
    &&TPOWP0<0.75
    &&fuck_width>15) {
        if(TNOL%2==0) {
            sancha_flag=1;
        }
        else {
            sancha_flag=2;
        }
    }
    if(sancha_flag==1) {
        adding_line_a_b(LBR,0,63,80,24);
        if(lost_line0<15) {
            sancha_flag=3;
        }
    }
    if(sancha_flag==3)
    {
        if(fuck_width>10)
        {
            sancha_flag=5;
        }
    }
    if(sancha_flag==5)
    {
        adding_line_a_b(LBR,0,63,80,24);
        if(lost_line0<15)
        {
            sancha_flag=0;
        }
    }
    if(sancha_flag==2) {
        adding_line_a_b(RBR,159,63,80,24);
        if(lost_line1<15) {
            sancha_flag=4;
        }
    }
    if(sancha_flag==4)
    {
        if(fuck_width>10)
        {
            sancha_flag=6;
        }
    }
    if(sancha_flag==6)
    {
        adding_line_a_b(RBR,0,63,80,24);
        if(lost_line1<15)
        {
            sancha_flag=0;
        }
    }
}
void cross_judge(void) {
    // printf("count_not_monotonic(SL,EL,0):%d\n",not_monotonic0);
    // printf("count_not_monotonic(SL,EL,1):%d\n",not_monotonic1);
    // printf("count_invalid(SL,EL,0):%d\n",invalid0);
    // printf("count_invalid(SL,EL,1):%d\n",invalid1);
    // printf("count_lost_line(SL,EL,0):%d\n",lost_line0);
    // printf("count_lost_line(SL,EL,1):%d\n",lost_line1);
    if(!sancha_flag){
    if(invalid0>20 && invalid0<50 && invalid1>20 && invalid1<50) {cross_flag=1;}
    else if((invalid0>45 && not_monotonic1>8) || (invalid1>45 && not_monotonic0>8)) {cross_flag=2;}
    else if(invalid0>40 && invalid1>40) {cross_flag=3;}
    else {cross_flag=0;}
    if(cross_flag!=0){
        // printf("cross\n");
        fit_and_fill_borders();
    }}
}

void left_circle_judge(void) {
    static float lcx,lcy;
    if(left_circle_flag==0) {
        if(TPOWP0>0.9
        &&find_corner(SL,EL,0,10)>35
        &&!find_corner(SL,EL,1,10)
        &&lost_line1<15
        &&invalid0>20
        ) {
            left_circle_flag=1;
            gyro_1=Z_360;
        }
    }
    else if(left_circle_flag==1) {
        adding_line_a_b(LBR,LSP,SL,70,EL);
        // adding_line(LBR,70,-1.15);
        if(find_corner(49,0,2,10)) {
            left_circle_flag=2;
        }
    }
    else if(left_circle_flag==2) {
        if(find_corner(SL,EL,2,10)>8) {
            left_circle_flag=3;
        }
    }
    else if(left_circle_flag==3) {
        gyro_2=Z_360;
        lcx=LB[find_valid_corner(SL,EL,2)];
        lcy=find_valid_corner(SL,EL,2);
        adding_line_a_b(RBR,lcx,lcy,187,59);
        // rck=(float)(RSP-LB[find_valid_corner(SL,EL,2)])/(SL-find_valid_corner(SL,EL,2));
        // adding_line_a_b(RBR,187,59,rc1,rc2);
        // adding_line(LBR,image_w*0.25,1.6);
        adding_line(LBR,0,0);

        if(        
        #ifndef visual
        my_fabs(gyro_1-gyro_2)>50
        #endif 
        // !find_corner(49,0,2,10)
        // &&image_processed[30][150]==0
        ) {
            left_circle_flag=4;
        }
    }
    else if(left_circle_flag==4) {
        gyro_2=Z_360;
        if(
            #ifndef visual
            my_fabs(gyro_1-gyro_2)>230
            #endif
            #ifdef visual
            count_lost_line(49,0,1)>18&&TPOWP(1).mid>0.8
            #endif
        ) {
            left_circle_flag=5;
        }
    }
    else if(left_circle_flag==5)
    {
        gyro_2=Z_360;
        adding_line(LBR,0,0);
        adding_line(RBR,image_w*0.2,1.6);
        if(
            #ifndef visual
            my_fabs(gyro_1-gyro_2)>340
            #endif 
            #ifdef visual
            TPOWP(0).mid>0.9&&count_lost_line(49,0,1)<10
            #endif
        ) {
            left_circle_flag=6;
        }
    }
    else if(left_circle_flag==6) {
        adding_line(LBR,70,-1.2);
        if(TPOWP0>0.75&&lost_line0<25) {
            left_circle_flag=0;
            gyro_1=0,gyro_2=0;
        }
    }
}
void right_circle_judge(void) {
    // static float rcx,rcy;
    if(right_circle_flag==0) {
        if(
        TPOWP0>0.9
        &&  find_corner(SL,EL,1,10)>35
        &&!(find_corner(SL,EL,2,10)>3)
        &&!(find_corner(SL,EL,0,10)>3)
        // &&count_invalid(30,0,0)<5
        &&invalid1>20
        &&lost_line0<15
        ) {
            right_circle_flag=1;
            gyro_1=Z_360_S;
        }
    }
    else if(right_circle_flag==1) {
        adding_line(RBR,90,1.2);
        if(find_corner(49,0,3,10)>3) {
            right_circle_flag=2;
        }
    }
    else if(right_circle_flag==2) {
        if(find_corner(49,0,3,10)>12) {
            right_circle_flag=3;
        }
    }
    else if(right_circle_flag==3) {
        adding_line(LBR,image_w*0.7,-1.6);
        adding_line(RBR,image_w-1,0);
        if(!(find_corner(49,0,3,10)>3)&&mt9v03x_image_processed[30][40]==0&&count_lost_line(49,25,0)<20) {
            right_circle_flag=4;
        }
    }
    else if(right_circle_flag==4) {
        gyro_2=Z_360_S;
        if(
            #ifndef visual
            my_fabs(gyro_1-gyro_2)>230
            #endif
            #ifdef visual
            count_lost_line(49,0,0)>22&&TPOWP(1).mid>0.95
            #endif
        ) {
            right_circle_flag=5;
        }
    }
    else if(right_circle_flag==5) {
        gyro_2=Z_360_S;
        adding_line(LBR,image_w*0.75,-1.6);
        adding_line(RBR,image_w-1,0);
        if(
            #ifndef visual
            my_fabs(gyro_1-gyro_2)>330
            #endif
            #ifdef visual
            TPOWP(0).mid>0.75&&count_lost_line(49,0,0)<15
            #endif
        ) {
            right_circle_flag=6;
        }
    }
    else if(right_circle_flag==6) {
        adding_line(RBR,90,1.2);
        if(TPOWP(0).mid>0.9&&count_lost_line(49,0,1)<25) {
            right_circle_flag=0;
            gyro_1=0,gyro_2=0;
        }
    }
}
float quadratic_value(float y, float x, float p0, float p1, float mu)
{
    float t = (x - mu) * (x - mu);
    if(p0 * t + p1>1) return p0 * t + p1;
    else return y;
}
float find_mid_line_weight(void) {
    if(MLW_init==0) {
        for (int i = 0; i < image_h; i++) {
            float fitted = quadratic_value(min_value, i, p0, p1, mu);
            MLW[i]=fitted;
        }
        MLW_init=1;
    }
    float mid_line_value=MID_W;
    float mid_line=MID_W;
    uint32 weight_midline_sum=0;
    uint32 weight_sum=0;
    for(uint8_t i=SL;i>EL;i--) {
        ML[i]=(LBR[i]+RBR[i])/2;
        if(i<45){
            if(my_fabs((int16_t)(ML[i+1])-(int16_t)(ML[i]))>20)
            {
                ML[i]=ML[i+1];
            }
        }
        weight_midline_sum+=ML[i]*MLW[i];
        weight_sum+=MLW[i];
    }
    mid_line=(float)(weight_midline_sum/weight_sum);
    // mid_line_value=last_mid_line*0.1+mid_line*0.9;
    mid_line_value=mid_line;
    last_mid_line=mid_line_value;
    return mid_line_value;
}
uint8_t zebra_judge(uint8_t threshold) {
    int sum = 0;
    for(uint8_t i = 1; i < image_w; i++)
    {
        if(mt9v03x_image_processed[SPSL][i-1]==0&&mt9v03x_image_processed[SPSL][i]==255){sum++;}
    }
    if(sum > threshold)return sum;
    else return 0;
}
void stop_check(uint16_t base_speed, uint8_t variable_speed, uint8_t laps) {
    if(!zebra_flag) {
        if(!sancha_flag&&!left_circle_flag&&!right_circle_flag) {
            zebra_flag=zebra_judge(3);
            Motor_L_Speed_Base=base_speed+TPOWP0*TPOWP0*variable_speed;
            Motor_R_Speed_Base=base_speed+TPOWP0*TPOWP0*variable_speed;
        } else {
            Motor_L_Speed_Base=0.9*base_speed;
            Motor_R_Speed_Base=0.9*base_speed;
        }
    }
    else if(zebra_flag) {
        if(!zebra_judge(3)) {
            if(TNOL<255) {
                TNOL+=1;
            }
            zebra_flag=0;
        }
        if(TNOL>=laps) {
            zebra_flag=1;
            stop_flag=1;
        }
    }
    if((image_threshold!=0&&last_image_threshold!=0)
    &&(image_threshold<35||my_fabs(image_threshold-last_image_threshold)>25)) {
        stop_flag=1;
    }
    if(stop_flag) {
        fuya_value=0;
        Motor_L_Speed_Base=0;
        Motor_R_Speed_Base=0;
    }
}
RoadType judge_road_type(void){
    if(cross_flag){
        return ROAD_CROSS;
    }
    else if(sancha_flag){
        return ROAD_TRIDENT;
    }
    else if(left_circle_flag){
        return ROAD_ROUNDABOUT_LEFT;
    }
    else if(right_circle_flag){
        return ROAD_ROUNDABOUT_RIGHT;
    }
    else if(invalid0>50&&invalid1<30){
        return ROAD_CURVE_LEFT;
    }
    else if(invalid0<30&&invalid1>50){
        return ROAD_CURVE_RIGHT;
    }
    else if(invalid0<10&&invalid1<10){
        return ROAD_STRAIGHT;
    }
    else return ROAD_UNKNOWN;
}
void adjust_control_strategy(RoadType road_type) {
    switch(road_type) {
        case ROAD_STRAIGHT:
            Motor_L_Speed_Base = 300;
            Motor_R_Speed_Base = 300;
            break;
        case ROAD_CURVE_LEFT:
            Motor_L_Speed_Base = 220;
            Motor_R_Speed_Base = 260;
            break;
        case ROAD_CURVE_RIGHT:
            Motor_L_Speed_Base = 260;
            Motor_R_Speed_Base = 220;
            break;
        case ROAD_CROSS:
            Motor_L_Speed_Base = 180;
            Motor_R_Speed_Base = 180;
            break;
        case ROAD_ROUNDABOUT_LEFT:
            Motor_L_Speed_Base = 200;
            Motor_R_Speed_Base = 240;
            break;
        case ROAD_ROUNDABOUT_RIGHT:
            Motor_L_Speed_Base = 200;
            Motor_R_Speed_Base = 240;
            break;
        case ROAD_UNKNOWN:
            Motor_L_Speed_Base = 150;
            Motor_R_Speed_Base = 150;
            break;
        default:
            break;
    }
}

// // ��ʾ��·����
// void display_road_type(RoadType road_type) {
//     #ifdef TFT180
//     // tft180_fill_rect(120, 0, 160, 16, RGB565_BLACK);
    
//     switch(road_type) {
//         case ROAD_STRAIGHT:
//             tft180_show_string(120, 0, "STR");
//             printf("STR");
//             break;
            
//         case ROAD_CURVE_LEFT:
//             tft180_show_string(120, 0, "LFT");
//             printf("LFT");
//             break;
            
//         case ROAD_CURVE_RIGHT:
//             tft180_show_string(120, 0, "RGT");
//             printf("RGT");
//             break;
            
//         case ROAD_CROSS:
//             tft180_show_string(120, 0, "CRO");
//             printf("CRO");
//             break;
            
//         case ROAD_TRIDENT:
//             tft180_show_string(120, 0, "TRI");
//             printf("TRI");
//             break;
            
//         case ROAD_ROUNDABOUT_LEFT:
//             tft180_show_string(120, 0, "LC");
//             printf("LC");
//             break;
            
//         case ROAD_ROUNDABOUT_RIGHT:
//             tft180_show_string(120, 0, "RC");
//             printf("RC");
//             break;

//         default:
//             tft180_show_string(120, 0, "UNK");
//             printf("UNK");
//             break;
//     }
//     #endif
// }


void draw_line(void) {
    tft180_draw_line(0,TSEL-1,159,TSEL-1,RGB565_BLACK);//�Ϸⶥ
    tft180_draw_line(limit_a_b(image_w/2*160/image_w,0,159),TSEL,limit_a_b(image_w/2*160/image_w,0,159),TSSL,RGB565_BLACK);//������
    for(uint8_t i=SL;i>EL;i--) {
        tft180_draw_point(limit_a_b(LB[i] *160/image_w,0,159),i+TSEL,RGB565_RED);
        tft180_draw_point(limit_a_b(ML[i] *160/image_w,0,159),i+TSEL,RGB565_PINK);
        tft180_draw_point(limit_a_b(RB[i] *160/image_w,0,159),i+TSEL,RGB565_BLUE);
        tft180_draw_point(limit_a_b(LBR[i]*160/image_w,0,159),i+TSEL,RGB565_BROWN);
        tft180_draw_point(limit_a_b(RBR[i]*160/image_w,0,159),i+TSEL,RGB565_39C5BB);
        // tft180_draw_point(limit_a_b(LBU[i]/image_w*160,0,159),i+TSEL,RGB565_66CCFF);
        // tft180_draw_point(limit_a_b(RBU[i]/image_w*160,0,159),i+TSEL,RGB565_66CCFF);
        // tft180_draw_point(MLW[i],i+TSEL,RGB565_PURPLE);
    }
    sancha_ass(TPOWP0);
    for(uint8_t i=0;i<image_w;i++) {
        tft180_draw_point(limit_a_b(i *160/image_w,0,159),edge_points[i]+TSEL,RGB565_66CCFF);
    }
}
void start_calc(void){
    timer_clear(TIM_8);
    timer_get(TIM_8);
}
void end_calc(void){
    tft180_show_int(48,96,timer_get(TIM_8),6);
    timer_clear(TIM_8);
}
void calc_fps(void){
    if(timer_get(TIM_8)>1000)
    {
        timer_clear(TIM_8);
        tft180_show_int(48,96,fps,3);
        fps=0;
    }
}
void show_all(void) {
    #ifdef WIFI_SPI
    seekfree_assistant_camera_send();
    #endif
    #ifdef show_image
    tft180_show_gray_image(0, TSEL, (const uint8_t *)mt9v03x_image,image_w, image_h,160,image_h,0);
    #endif
    #ifdef show_image_processed
    tft180_show_gray_image(0, TSEL, (const uint8_t *)mt9v03x_image_processed,image_w, image_h,160,image_h,0);
    #endif
    #ifdef show_line
    draw_line();
    // tft180_draw_line(10,49+TSEL,60,0+TSEL,RGB565_BLUE);
    // tft180_draw_line(150,49+TSEL,100,0+TSEL,RGB565_BLUE);
    #endif
    #ifdef show_1 /*//////��һ��//////*/
    // tft180_show_int(0 ,0,count_invalid(SL,EL,0),3);
    // tft180_show_int(32,0,count_invalid(SL,EL,1),3);
    tft180_show_int(0 ,0,invalid0,3);
    tft180_show_int(32,0,invalid1,3);
    tft180_show_int(64,0,sancha_valid1,3);
    tft180_show_int(96,0,sancha_valid2,3);
    tft180_show_int(128,0,fuck_width,3);
    #endif
    #ifdef show_2 /*//////�ڶ���//////*/
    tft180_show_int(0 ,16,find_corner(SL,EL,0,0),3);
    tft180_show_int(32,16,find_corner(SL,EL,1,0),3);
    tft180_show_int(64,16,find_corner(SL,EL,2,0),3);
    tft180_show_int(96,16,find_corner(SL,EL,3,0),3);
    #endif
    #ifdef show_3 /*//////������//////*/
    tft180_show_char (0 ,32,'L');
    tft180_show_int  (8 ,32,count_lost_line(50,0,0),2);
    tft180_show_char (32,32,'R');
    tft180_show_int  (40,32,count_lost_line(50,0,1),2);
    tft180_show_float(64,32,TPOWP0,1,1);
    tft180_show_float(96,32,TPOWP1,1,1);
    #endif
    #ifdef show_4 /*//////������//////*/
    tft180_show_char(0  ,48,'M');
    tft180_show_int (8  ,48,mid              ,3);/*��Ȩ����ֵ*/
    tft180_show_char(32 ,48,'T');
    tft180_show_int (40 ,48,image_threshold  ,3);
    tft180_show_int (64 ,48,cross_flag       ,1);
    tft180_show_int (80 ,48,left_circle_flag ,1);
    tft180_show_int (96 ,48,right_circle_flag,1);
    tft180_show_int (112,48,sancha_flag      ,1);
    #endif
    #ifdef show_5 /*//////������//////*/
    // gyro_1=Z_360;

    // tft180_show_int  (64 ,32,find_valid_corner(SL,EL,2)             ,3);
    // tft180_show_int  (96 ,32,find_valid_corner(SL,EL,3)             ,3);
    // tft180_show_int  (48,32,fuck_width   ,3);
    // tft180_show_int  (0,32,cross_judge_assist()             ,3);
    // tft180_show_int  (64,32,sancha_valid1             ,3);
    // tft180_show_int  (24,32,TNOL             ,3);
    // tft180_show_int  (48,32,zebra_judge(0)   ,3);
    // tft180_show_int  (48,32,RSP-LSP   ,3);
    // tft180_show_float(80,64,fuck_down_width(),5,3);
    // tft180_show_float(95,32,Motor_L.encoder_speed,5,3);
    // tft180_show_float(80,32,imu660ra_gyro_z_c,5,3);
    // tft180_show_float(40,32,Z_360_S          ,3,1);
    #endif
}
#ifdef WIFI_SPI
void connect_wifi(void){
    // while(wifi_spi_init("ESaTI-Lab", "dianzikeji"))
    while(wifi_spi_init("test", "12345678"))
    {
        printf("\r\n connect wifi failed. \r\n");
        system_delay_ms(100);                                                   // ��ʼ��ʧ�� �ȴ� 100ms
    }
    printf("\r\n module version:%s",wifi_spi_version);                          // ģ��̼��汾
    printf("\r\n module mac    :%s",wifi_spi_mac_addr);                         // ģ�� MAC ��Ϣ
    printf("\r\n module ip     :%s",wifi_spi_ip_addr_port);                     // ģ�� IP ��ַ
    // zf_device_wifi_spi.h �ļ��ڵĺ궨����Ը���ģ������(����) WIFI ֮���Ƿ��Զ����� TCP ������������ UDP ���ӡ����� TCP �������Ȳ���
    if(1 != WIFI_SPI_AUTO_CONNECT)                                              // ���û�п����Զ����� ����Ҫ�ֶ�����Ŀ�� IP
    {
        while(wifi_spi_socket_connect(                                          // ��ָ��Ŀ�� IP �Ķ˿ڽ��� TCP ����
            "TCP",                                                              // ָ��ʹ��TCP��ʽͨѶ
            WIFI_SPI_TARGET_IP,                                                 // ָ��Զ�˵�IP��ַ����д��λ����IP��ַ
            WIFI_SPI_TARGET_PORT,                                               // ָ��Զ�˵Ķ˿ںţ���д��λ���Ķ˿ںţ�ͨ����λ��Ĭ����8080
            WIFI_SPI_LOCAL_PORT))                                               // ָ�������Ķ˿ں�
        {
            // ���һֱ����ʧ�� ����һ���ǲ���û�н�Ӳ����λ
            printf("\r\n Connect TCP Servers error, try again.");
            system_delay_ms(100);                                               // ��������ʧ�� �ȴ� 100ms
        }
    }
    seekfree_assistant_interface_init(SEEKFREE_ASSISTANT_WIFI_SPI);
    seekfree_assistant_camera_information_config(SEEKFREE_ASSISTANT_MT9V03X, mt9v03x_image, image_w, image_h);
}
#endif
void image_main(void) {

    #ifndef simulator
    #ifdef TFT180
    tft180_set_font(TFT180_6X8_FONT);
    tft180_set_dir(TFT180_CROSSWISE);
    tft180_init();
    #endif
    #endif
    #ifdef WIFI_SPI
    connect_wifi();
    #endif
    mt9v03x_init();//����ͷ��ʼ��
    #ifndef simulator
    // fuya_value = 850;
    // timer_init(TIM_8,TIMER_MS);
    while(1){
    #endif
    if(mt9v03x_finish_flag) {fps++;
        // start_calc();
        last_image_threshold=image_threshold;//��¼��һ�ζ�ֵ����ֵ
        fps%3==0?(image_threshold=ostu_fast(mt9v03x_image[0]),fps=0)
        :(image_threshold=last_image_threshold);//���㱾�ζ�ֵ����ֵ
        set_image_twovalues(image_threshold);//��ֵ��
        find_SP(mt9v03x_image_processed);//Ѱ�����
        image_deal(mt9v03x_image_processed);//����������
        mark_valid_points();//�����Ч��
        lost_line0=count_lost_line(SL,EL,0);
        lost_line1=count_lost_line(SL,EL,1);
        not_monotonic0=count_not_monotonic(SL,EL,0);
        not_monotonic1=count_not_monotonic(SL,EL,1);
        invalid0=count_invalid(SL,EL+10,0);
        invalid1=count_invalid(SL,EL+10,1);
        TPOWP0=TPOWP(0).mid;//���߳���
        TPOWP1=TPOWP(1).mid;//���߿���
        cross_judge();
        left_circle_judge();
        right_circle_judge();
        sancha_judge();
        // RT=judge_road_type();
        // adjust_control_strategy(RT);
        stop_check(360,0,10);
        mid=find_mid_line_weight();
        show_all();
        mt9v03x_finish_flag = 0;
        // end_calc();
    }
    #ifndef simulator
    }
    #endif
}
