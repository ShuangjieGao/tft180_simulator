# TFT180 智能车视觉算法本地原生模拟器

这是一个专为全国大学生智能汽车竞赛（以及基于逐飞科技开源库的嵌入式视觉开发）打造的 **本地原生 Win32 GUI 屏幕与图像模拟器**。

无需连接单片机和摄像头，无需搭建任何复杂的 Web/Node.js/Python 依赖环境，直接在 Windows PC 上读取赛道录像 AVI 文件，点对点运行并调试 `image.c` 中的视觉算法！

---

## 🌟 核心特性

- **纯原生零依赖**：基于 Windows 原生 Win32 GDI API 实现，单文件 `main.exe` 即可直接双击运行。
- **逐飞 API 100% 兼容**：完整实现 `tft180_draw_point`、`tft180_draw_line`、`tft180_show_gray_image` 等常用图元与图像绘制接口，算法代码可直接在 PC 与单片机之间无缝移植。
- **高画质点阵缩放**：采用 3x 最近邻清晰点阵放大（物理 160×128 像素放大为 480×384 窗口），像素边缘锐利清晰，极大方便排查噪点与像素级边界提取。
- **杜绝乱码与极简轻量**：精简底层实现，移除了多余的字库依赖，标题栏与控制台全方位防止乱码。

---

## 🚀 快速上手

### 1. 运行模拟器
直接在 VS Code 终端中运行编译好的程序：
```powershell
.\main.exe
```
或者在文件资源管理器中直接**双击 `main.exe`** 运行。

### 2. 交互快捷键
- **按键盘 `ESC` 键**：立即退出模拟器。
- **点击窗口右上角 `[X]` 关闭按钮**：立即退出模拟器。

---

## 🛠️ 编译与构建指南

本项目支持两种编译方式：

### 方式一：VS Code 一键编译（推荐）
1. 使用 VS Code 打开工程根目录 `tft180_simulator`。
2. 按快捷键 **`Ctrl + Shift + B`**（或点击顶部菜单栏：**终端 -> 运行生成任务**）。
3. 终端将自动调用 MinGW GCC 编译生成最新的 `main.exe`。

### 方式二：PowerShell 终端命令行编译
如果系统已配置 MinGW-w64 GCC 环境，直接在工程根目录下执行：
```powershell
gcc -fdiagnostics-color=always -g -O2 -Wno-misleading-indentation -I. -Dsimulator main.c image.c zf_device_tft180.c zf_common_function.c avilib.c read_bmp.c -o main.exe -lgdi32 -luser32
```

> **新手提示**：若终端提示 `gcc 不是内部或外部命令`，说明尚未将 MinGW 的 `bin` 目录加入环境变量。
> 如果你安装了 OpenMV IDE，可以直接使用其自带的 MinGW：
> ```powershell
> $env:PATH = "C:\Program Files\OpenMV IDE\share\qtcreator\stedgeai\Utilities\windows\mingw64\bin;" + $env:PATH
> ```

---

## 📁 工程目录结构

```text
tft180_simulator/
├── image_video/                # 赛道 AVI 录像文件目录
│   ├── 2025_11_01_00_33_46_Video.avi  # 160x64 原生录像 (推荐，1131帧)
│   └── 2025_10_12_17_03_53_Video.avi  # 160x50 录像 (345帧)
├── main.c                      # 模拟器主程序入口 (视频解码、驱动循环、算法触发)
├── image.c / image.h           # 智能车核心视觉算法 (寻线、边线提取、拐点与元素识别)
├── zf_device_tft180.c / .h     # 逐飞 TFT180 屏幕驱动的 PC 本地 GUI 仿真实现
├── zf_common_headfile.h        # 逐飞库头文件集合与模拟桩定义
├── zf_common_function.c / .h   # 逐飞常用算法数学函数 (开方、绝对值、限幅等)
├── zf_common_typedef.h         # 基础数据类型重命名 (uint8, int16, float 等)
├── avilib.c / avilib.h         # 轻量级 AVI 视频解码底层库
├── read_bmp.c / read_bmp.h     # 静态 BMP 图像读取工具
├── .vscode/tasks.json          # VS Code 自动化构建任务配置文件
└── README.md                   # 新手使用手册 (本文件)
```

---

## ⚙️ 分辨率与核心参数配置

| 参数宏定义 | 文件位置 | 当前默认值 | 说明 |
| :--- | :--- | :--- | :--- |
| `MT9V03X_W` | `zf_common_headfile.h` | **160** | 摄像头输入宽度 |
| `MT9V03X_H` | `zf_common_headfile.h` | **64** | 摄像头输入高度 |
| `image_w` | `image.h` | `MT9V03X_W` (160) | 算法图像处理宽度 |
| `image_h` | `image.h` | **64** | 算法图像处理高度 |
| `TSEL` | `image.h` | `127 - image_h + 1` (64) | 图像在 TFT180 屏幕上的起始行 |

### 屏幕显示区域说明
- **屏幕总规格**：160 × 128 像素。
- **上部区域 (Y: 0 ~ 63)**：保留给状态调试区域。
- **下部区域 (Y: 64 ~ 127)**：图像与边线识别显示区（`image_h = 64`）。

---

## 💡 如何调试我自己的算法？

1. 打开 **`image.c`**，像平时在单片机上写代码一样编写或修改图像处理函数（例如 `image_deal`、`cross_judge`、`left_circle_judge` 等）。
2. 在 `show_all()` 函数或算法内部，使用标准逐飞绘图函数绘制调试标记：
   ```c
   // 示例：在屏幕上画出识别到的左边线（红点）
   tft180_draw_point(limit_a_b(LB[i] * 160 / image_w, 0, 159), i + TSEL, RGB565_RED);
   
   // 示例：在两点之间画辅助线
   tft180_draw_line(x1, y1 + TSEL, x2, y2 + TSEL, RGB565_BLUE);
   ```
3. 按 **`Ctrl + Shift + B`** 重新编译，运行 `.\main.exe` 即可实时查看算法修改后的视觉轨迹效果！

---

## ❓ 常见问题排查 (FAQ)

#### Q1: 如何更换播放的赛道录像？
在 `main.c` 中修改 `avi_path` 路径：
```c
const char *avi_path = "image_video\\你的视频名称.avi";
```
重新编译运行即可。

#### Q2: 视频播放太快或太慢如何调整？
在 `main.c` 主循环底部调整延迟时间：
```c
tft180_delay(3); // 默认约 3ms，数值调大可慢速单帧观察，调小可全速快放
```

#### Q3: 为什么窗口只有下半部分在动？
逐飞 TFT180 屏幕物理分辨率为 160×128，而摄像头高度为 64 行。根据智能车常规做法，摄像头画面显示在屏幕下半部（第 64~127 行），上半部默认留黑以避免冗余文字刷新造成的帧率下降。
