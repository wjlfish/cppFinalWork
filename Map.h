#ifndef MAP_H
#define MAP_H

#include <vector>
#include <string>
#include <iostream>
#include <memory> // 引入智能指针
#include "GameObject.h"
#include "utils.h"

class Map {
private:
    int width;
    int height;
    std::vector<std::string> grid; // 存储地形：墙、地板

public:
    // 构造函数：初始化地图大小
    Map(int w, int h) : width(w), height(h) {
        // 初始化一个空地图（全墙壁或者空地）
        // 这里我们先生成一个简单的围墙房间
        generateDefaultMap();
    }

    // 生成默认地图（暂时硬编码，后续改为读取文件）
    void generateDefaultMap() {
        grid.clear();
        for (int y = 0; y < height; ++y) {
            std::string row = "";
            for (int x = 0; x < width; ++x) {
                if (y == 0 || y == height - 1 || x == 0 || x == width - 1) {
                    row += "#"; // 墙壁
                } else {
                    row += "."; // 地面
                }
            }
            grid.push_back(row);
        }
        
        // 放置出口
        grid[height-2][width-2] = '>';
    }

    // 判断坐标是否可通行（用于碰撞检测）
    bool isWalkable(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) return false;
        char tile = grid[y][x];
        return tile != '#'; // 不是墙壁就可以走
    }

    // 绘制地图
    // 这里我们传入一个 GameObject 指针列表，以便在地图上叠加绘制对象
    // const std::vector<std::unique_ptr<GameObject>>& objects 
    // 这个参数暂时保留，下一阶段会用到，目前先画地形
    // 修改 draw 方法签名，接收需要绘制的游戏对象列表
    // 使用 const std::vector<GameObject*>& 避免拷贝，且利用多态
    void draw(const std::vector<GameObject*>& objects) const {
        #ifdef _WIN32
            system("cls");
        #else
            std::cout << "\033[2J\033[1;1H"; // ANSI 清屏并回原点，比 system("clear") 更快更少闪烁
        #endif

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                // 1. 检查当前坐标 (x, y) 是否有对象
                bool objectDrawn = false;
                for (const auto* obj : objects) {
                    if (obj->getPosition().x == x && obj->getPosition().y == y) {
                        obj->draw(); // 多态调用！如果是玩家画@，是怪物画M
                        objectDrawn = true;
                        break; // 假设同一位置只显示一个顶层对象
                    }
                }

                // 2. 如果没有对象，绘制地图本身
                if (!objectDrawn) {
                    char tile = grid[y][x];
                    if (tile == '#') {
                        std::cout << Color::GREY << tile << Color::RESET;
                    } else if (tile == '>') {
                        std::cout << Color::YELLOW << tile << Color::RESET;
                    } else {
                        std::cout << tile;
                    }
                }
            }
            std::cout << std::endl;
        }
    }

    // 生成随机障碍物，难度越高，墙壁越密集
    void generateObstacles(int level) {
        // 先清空中间区域（保留四周围墙）
        generateDefaultMap(); 
        
        // 简单的随机算法：level 越高，生成的墙越多
        int obstacleCount = (width * height) / 10 + (level * 5);
        
        for (int i = 0; i < obstacleCount; ++i) {
            int x = rand() % (width - 2) + 1; // 1 ~ width-2
            int y = rand() % (height - 2) + 1; // 1 ~ height-2
            
            // 不要在出口(width-2, height-2)和起点(1,1)放墙
            if ((x == 1 && y == 1) || (x == width-2 && y == height-2)) continue;
            
            grid[y][x] = '#';
        }
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }
};

#endif // MAP_H