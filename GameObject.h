#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <string>
#include <iostream>
#include "utils.h"

// 抽象基类
class GameObject {
protected:
    Point pos;          // 坐标
    std::string symbol; // 显示字符 (使用 string 兼容多字节字符/Emoji)
    std::string name;   // 名称
    std::string color;  // 颜色代码

public:
    // 构造函数
    GameObject(int x, int y, std::string sym, std::string n, std::string c = Color::WHITE)
        : pos{x, y}, symbol(sym), name(n), color(c) {}

    // 虚析构函数：确保派生类能正确释放资源
    virtual ~GameObject() = default;

    // 纯虚函数：强制子类必须实现自己的绘制逻辑
    virtual void draw() const = 0;

    // Getters
    Point getPosition() const { return pos; }
    std::string getName() const { return name; }

    // Setters
    void setPosition(int x, int y) {
        pos.x = x;
        pos.y = y;
    }
    
    // 简单的通用绘制实现（子类可以复用）
    void printSymbol() const {
        std::cout << color << symbol << Color::RESET;
    }
};

#endif // GAMEOBJECT_H