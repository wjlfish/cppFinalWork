#ifndef INPUT_H
#define INPUT_H

#include <iostream>

#ifdef _WIN32
    #include <conio.h>
    namespace Input {
        void init() { 
            // Windows 的 _getch 本身就是无回显的，通常不需要特殊初始化
            // 但为了兼容性，可以设置控制台代码页等，这里暂时留空
            system("chcp 65001"); 
        }
        void restore() { } // 留空
        
        char get() { return _getch(); }
        bool hasPending() { return _kbhit() != 0; }
        void clearBuffer() {
            while (_kbhit()) _getch();
        }
    }
#else
    // === Mac / Linux 专用实现 ===
    #include <termios.h>
    #include <unistd.h>
    #include <stdio.h>
    #include <sys/ioctl.h>

    namespace Input {
        struct termios originalTermios; // 保存原始设置用于恢复
        bool isInitialized = false;

        void restore() {
            if (isInitialized) {
                tcsetattr(STDIN_FILENO, TCSANOW, &originalTermios);
                isInitialized = false;
            }
        }

        void init() {
            if (isInitialized) return;
            
            // 1. 获取当前终端设置
            tcgetattr(STDIN_FILENO, &originalTermios);
            
            // 2. 修改设置：关闭 规范模式(ICANON) 和 回显(ECHO)
            struct termios newTermios = originalTermios;
            newTermios.c_lflag &= ~(ICANON | ECHO);
            
            // 3. 设置读取行为：VMIN=1 表示 read 至少读取 1 个字符才返回（阻塞模式）
            newTermios.c_cc[VMIN] = 1;
            newTermios.c_cc[VTIME] = 0;
            
            // 4. 应用新设置
            tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);
            isInitialized = true;
        }

        // 检查是否有输入（非阻塞）
        bool hasPending() {
            int bytesWaiting;
            ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting);
            return bytesWaiting > 0;
        }

        // 读取一个字符（如果缓冲区为空则阻塞等待）
        char get() {
            char buf = 0;
            // 因为 init() 已经设置了 VMIN=1，这里 read 会自动阻塞直到有输入
            if (read(STDIN_FILENO, &buf, 1) < 0) {
                return 0; 
            }
            return buf;
        }

        void clearBuffer() {
            char temp;
            // 当还有等待的字符时，持续读取
            while (hasPending()) {
                read(STDIN_FILENO, &temp, 1);
            }
        }
    }
#endif

#endif // INPUT_H