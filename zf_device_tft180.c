#include "zf_device_tft180.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>

#define TFT180_W 160
#define TFT180_H 128
#define TFT180_SCALE 3

static struct {
    int dir;
    uint16_t pen;
    uint16_t bg;
    int initialized;
} s_tft = {
    .dir = 0,
    .pen = RGB565_WHITE,
    .bg = RGB565_BLACK,
    .initialized = 0
};

// 32-bit RGB DIB Framebuffer (0x00RRGGBB)
static uint32_t g_fb[TFT180_H][TFT180_W];

static HWND s_hwnd = NULL;
static HANDLE s_hThread = NULL;
static CRITICAL_SECTION s_cs;
static volatile int s_window_ready = 0;
static volatile int s_window_closed = 0;
static volatile int s_is_paused = 0;

int tft180_is_closed(void) {
    return s_window_closed;
}

int tft180_is_paused(void) {
    return s_is_paused;
}

void tft180_set_pause(int pause) {
    s_is_paused = pause;
    if (s_hwnd && IsWindow(s_hwnd)) {
        SetWindowTextA(s_hwnd, s_is_paused ? "TFT180 [PAUSED]" : "TFT180");
    }
}

static inline uint32_t rgb565_to_rgb888(uint16_t color) {
    uint32_t r = ((color >> 11) & 0x1F) * 255 / 31;
    uint32_t g = ((color >> 5) & 0x3F) * 255 / 63;
    uint32_t b = (color & 0x1F) * 255 / 31;
    return (r << 16) | (g << 8) | b;
}

static LRESULT CALLBACK TFT180_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            BITMAPINFO bmi;
            ZeroMemory(&bmi, sizeof(BITMAPINFO));
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = TFT180_W;
            bmi.bmiHeader.biHeight = -TFT180_H; // Top-Down DIB
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            SetStretchBltMode(hdc, COLORONCOLOR);

            EnterCriticalSection(&s_cs);
            StretchDIBits(
                hdc,
                0, 0, TFT180_W * TFT180_SCALE, TFT180_H * TFT180_SCALE,
                0, 0, TFT180_W, TFT180_H,
                g_fb,
                &bmi,
                DIB_RGB_COLORS,
                SRCCOPY
            );
            LeaveCriticalSection(&s_cs);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                s_window_closed = 1;
                PostQuitMessage(0);
            } else if (wParam == VK_SPACE) {
                s_is_paused = !s_is_paused;
                SetWindowTextA(hwnd, s_is_paused ? "TFT180 [PAUSED]" : "TFT180");
            }
            return 0;

        case WM_DESTROY:
            s_window_closed = 1;
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

static DWORD WINAPI TFT180_WindowThread(LPVOID lpParam) {
    (void)lpParam;
    HINSTANCE hInstance = GetModuleHandle(NULL);
    const char *CLASS_NAME = "TFT180_Simulator_Class";

    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = TFT180_WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = CLASS_NAME;

    RegisterClassEx(&wc);

    RECT rc = {0, 0, TFT180_W * TFT180_SCALE, TFT180_H * TFT180_SCALE};
    AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);
    int winWidth = rc.right - rc.left;
    int winHeight = rc.bottom - rc.top;

    // Window title pure ASCII to prevent any garbled text
    s_hwnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        CLASS_NAME,
        "TFT180",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        winWidth, winHeight,
        NULL, NULL, hInstance, NULL
    );

    if (s_hwnd == NULL) {
        s_window_closed = 1;
        return 0;
    }

    ShowWindow(s_hwnd, SW_SHOWNORMAL);
    UpdateWindow(s_hwnd);
    s_window_ready = 1;

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    s_window_closed = 1;
    return 0;
}

void tft180_init(void) {
    if (s_tft.initialized) return;

    InitializeCriticalSection(&s_cs);
    memset(g_fb, 0, sizeof(g_fb));

    s_hThread = CreateThread(NULL, 0, TFT180_WindowThread, NULL, 0, NULL);
    if (!s_hThread) {
        fprintf(stderr, "[TFT180] Error: Failed to create UI thread\n");
        return;
    }

    int timeout_ms = 3000;
    while (!s_window_ready && timeout_ms > 0) {
        Sleep(10);
        timeout_ms -= 10;
    }

    s_tft.initialized = 1;
}

void tft180_clear(void) {
    EnterCriticalSection(&s_cs);
    memset(g_fb, 0, sizeof(g_fb));
    LeaveCriticalSection(&s_cs);
    tft180_flush();
}

void tft180_full(const uint16_t color) {
    uint32_t c = rgb565_to_rgb888(color);
    EnterCriticalSection(&s_cs);
    for (int y = 0; y < TFT180_H; y++) {
        for (int x = 0; x < TFT180_W; x++) {
            g_fb[y][x] = c;
        }
    }
    LeaveCriticalSection(&s_cs);
    tft180_flush();
}

void tft180_set_dir(int dir) { s_tft.dir = dir; }
void tft180_set_font(int font) { (void)font; }
void tft180_set_color(const uint16_t pen, const uint16_t bgcolor) {
    s_tft.pen = pen;
    s_tft.bg = bgcolor;
}

void tft180_draw_point(uint16_t x, uint16_t y, const uint16_t color) {
    if (x < TFT180_W && y < TFT180_H) {
        g_fb[y][x] = rgb565_to_rgb888(color);
    }
}

void tft180_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, const uint16_t color) {
    int dx = abs((int)x2 - (int)x1);
    int dy = -abs((int)y2 - (int)y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx + dy;

    int cur_x = x1;
    int cur_y = y1;

    while (1) {
        tft180_draw_point(cur_x, cur_y, color);
        if (cur_x == x2 && cur_y == y2) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; cur_x += sx; }
        if (e2 <= dx) { err += dx; cur_y += sy; }
    }
}

void tft180_draw_rectangle(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, const uint16_t color) {
    tft180_draw_line(x, y, x2, y, color);
    tft180_draw_line(x2, y, x2, y2, color);
    tft180_draw_line(x2, y2, x, y2, color);
    tft180_draw_line(x, y2, x, y, color);
}

void tft180_fill_rectangle(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, const uint16_t color) {
    uint16_t min_x = (x < x2) ? x : x2;
    uint16_t max_x = (x > x2) ? x : x2;
    uint16_t min_y = (y < y2) ? y : y2;
    uint16_t max_y = (y > y2) ? y : y2;

    uint32_t c = rgb565_to_rgb888(color);
    for (uint16_t cy = min_y; cy <= max_y && cy < TFT180_H; cy++) {
        for (uint16_t cx = min_x; cx <= max_x && cx < TFT180_W; cx++) {
            g_fb[cy][cx] = c;
        }
    }
}

void tft180_show_grayscale_image(uint16_t x, uint16_t y, const uint8_t *image, 
                                uint16_t width, uint16_t height, 
                                uint16_t dis_width, uint16_t dis_height) {
    if (!image || width == 0 || height == 0 || dis_width == 0 || dis_height == 0) return;

    for (uint16_t dy = 0; dy < dis_height; dy++) {
        int py = y + dy;
        if (py >= TFT180_H) break;
        uint16_t sy = (uint32_t)dy * height / dis_height;

        for (uint16_t dx = 0; dx < dis_width; dx++) {
            int px = x + dx;
            if (px >= TFT180_W) break;
            uint16_t sx = (uint32_t)dx * width / dis_width;

            uint8_t gray = image[sy * width + sx];
            g_fb[py][px] = ((uint32_t)gray << 16) | ((uint32_t)gray << 8) | gray;
        }
    }
}

void tft180_show_binary_image(uint16_t x, uint16_t y, const uint8_t *image, 
                             uint16_t width, uint16_t height, 
                             uint16_t dis_width, uint16_t dis_height) {
    if (!image || width == 0 || height == 0 || dis_width == 0 || dis_height == 0) return;

    uint32_t pen_rgb = rgb565_to_rgb888(s_tft.pen);
    uint32_t bg_rgb = rgb565_to_rgb888(s_tft.bg);

    for (uint16_t dy = 0; dy < dis_height; dy++) {
        int py = y + dy;
        if (py >= TFT180_H) break;
        uint16_t sy = (uint32_t)dy * height / dis_height;

        for (uint16_t dx = 0; dx < dis_width; dx++) {
            int px = x + dx;
            if (px >= TFT180_W) break;
            uint16_t sx = (uint32_t)dx * width / dis_width;

            uint8_t pixel = image[sy * width + sx];
            g_fb[py][px] = (pixel >= 128) ? pen_rgb : bg_rgb;
        }
    }
}

void tft180_show_gray_image(uint16_t x, uint16_t y, const uint8_t *image, 
                           uint16_t width, uint16_t height, 
                           uint16_t dis_width, uint16_t dis_height, 
                           uint8_t threshold) {
    if (threshold == 0) {
        tft180_show_grayscale_image(x, y, image, width, height, dis_width, dis_height);
    } else {
        if (!image || width == 0 || height == 0 || dis_width == 0 || dis_height == 0) return;

        uint32_t pen_rgb = rgb565_to_rgb888(s_tft.pen);
        uint32_t bg_rgb = rgb565_to_rgb888(s_tft.bg);

        for (uint16_t dy = 0; dy < dis_height; dy++) {
            int py = y + dy;
            if (py >= TFT180_H) break;
            uint16_t sy = (uint32_t)dy * height / dis_height;

            for (uint16_t dx = 0; dx < dis_width; dx++) {
                int px = x + dx;
                if (px >= TFT180_W) break;
                uint16_t sx = (uint32_t)dx * width / dis_width;

                uint8_t val = image[sy * width + sx];
                g_fb[py][px] = (val >= threshold) ? pen_rgb : bg_rgb;
            }
        }
    }
}

void tft180_show_rgb565_image(uint16_t x, uint16_t y, const uint16_t *image, 
                             uint16_t width, uint16_t height, 
                             uint16_t dis_width, uint16_t dis_height, 
                             uint8_t color_mode) {
    if (!image || width == 0 || height == 0 || dis_width == 0 || dis_height == 0) return;

    for (uint16_t dy = 0; dy < dis_height; dy++) {
        int py = y + dy;
        if (py >= TFT180_H) break;
        uint16_t sy = (uint32_t)dy * height / dis_height;

        for (uint16_t dx = 0; dx < dis_width; dx++) {
            int px = x + dx;
            if (px >= TFT180_W) break;
            uint16_t sx = (uint32_t)dx * width / dis_width;

            uint16_t color = image[sy * width + sx];
            if (color_mode == TFT180_COLOR_MODE_BGR) {
                uint16_t r = (color >> 11) & 0x1F;
                uint16_t g = (color >> 5) & 0x3F;
                uint16_t b = color & 0x1F;
                color = (b << 11) | (g << 5) | r;
            }
            g_fb[py][px] = rgb565_to_rgb888(color);
        }
    }
}

void tft180_flush(void) {
    if (s_hwnd && IsWindow(s_hwnd)) {
        InvalidateRect(s_hwnd, NULL, FALSE);
    }
}

void tft180_delay(uint32_t ms) {
    Sleep(ms);
    while (s_is_paused && !s_window_closed) {
        Sleep(20);
    }
}

void tft180_show_char(uint16_t x, uint16_t y, const char dat) {
    uint8_t ch = (uint8_t)dat;
    if (ch < 32 || ch > 126) ch = ' ';
    uint16_t index = ch - 32;

    for (uint8_t col = 0; col < 8; col++) {
        uint8_t byte_up = ascii_font_8x16[index][col];
        uint8_t byte_down = ascii_font_8x16[index][col + 8];

        for (uint8_t row = 0; row < 8; row++) {
            uint16_t color = ((byte_up >> row) & 0x01) ? s_tft.pen : s_tft.bg;
            tft180_draw_point(x + col, y + row, color);
        }

        for (uint8_t row = 0; row < 8; row++) {
            uint16_t color = ((byte_down >> row) & 0x01) ? s_tft.pen : s_tft.bg;
            tft180_draw_point(x + col, y + 8 + row, color);
        }
    }
}

void tft180_show_string(uint16_t x, uint16_t y, const char dat[]) {
    if (!dat) return;
    uint16_t cur_x = x;
    uint16_t cur_y = y;
    while (*dat) {
        if (*dat == '\n') {
            cur_y += 16;
            cur_x = x;
        } else {
            tft180_show_char(cur_x, cur_y, *dat);
            cur_x += 8;
        }
        dat++;
    }
}

void tft180_show_int(uint16_t x, uint16_t y, const int32_t dat, uint8_t num) {
    char buf[32];
    char fmt[16];
    snprintf(fmt, sizeof(fmt), "%%%dd", num);
    snprintf(buf, sizeof(buf), fmt, dat);
    tft180_show_string(x, y, buf);
}

void tft180_show_uint(uint16_t x, uint16_t y, const uint32_t dat, uint8_t num) {
    char buf[32];
    char fmt[16];
    snprintf(fmt, sizeof(fmt), "%%%du", num);
    snprintf(buf, sizeof(buf), fmt, dat);
    tft180_show_string(x, y, buf);
}

void tft180_show_float(uint16_t x, uint16_t y, const double dat, uint8_t num, uint8_t pointnum) {
    char buf[32];
    char fmt[16];
    snprintf(fmt, sizeof(fmt), "%%%d.%df", num, pointnum);
    snprintf(buf, sizeof(buf), fmt, dat);
    tft180_show_string(x, y, buf);
}

void tft180_performance_test(void) {
    tft180_clear();
    tft180_draw_line(0, 0, TFT180_W - 1, TFT180_H - 1, RGB565_GREEN);
    tft180_draw_line(0, TFT180_H - 1, TFT180_W - 1, 0, RGB565_RED);
    tft180_flush();
}
