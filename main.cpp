#include "Game.h"

// 静态成员定义
std::vector<std::string> MessageLog::logs;

int main() {
    // 1. 初始化输入系统 (开启无回显模式)
    Input::init();

    // 2. 启动游戏
    Game game;
    game.run();

    // 3. 恢复终端设置 (非常重要！否则退出后终端会乱)
    Input::restore();

    return 0;
}