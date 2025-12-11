#ifndef MESSAGELOG_H
#define MESSAGELOG_H

#include <vector>
#include <string>

// 简单的静态日志系统，也可以设计成单例，这里为了简单直接做成静态工具类
class MessageLog {
public:
    static std::vector<std::string> logs;
    
    // 添加一条日志
    static void add(const std::string& msg) {
        logs.push_back(msg);
        // 保持只显示最近的 5 条，防止屏幕溢出
        if (logs.size() > 5) {
            logs.erase(logs.begin());
        }
    }

    // 获取所有日志用于渲染
    static const std::vector<std::string>& getLogs() {
        return logs;
    }
    
    // 清空（新游戏时用）
    static void clear() {
        logs.clear();
    }
};

// 在 .cpp 文件中定义静态成员，但在头文件模式下，
// 我们可以用 C++17 的 inline 变量，或者简单点在 main.cpp 里定义一次
// 这里为了防报错，我们暂时不做外部定义，而是每次包含时注意链接问题。
// *规范做法*：应该在 main.cpp 开头写 std::vector<std::string> MessageLog::logs;
#endif