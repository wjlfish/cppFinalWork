#ifndef MAP_H
#define MAP_H

#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <queue> // 【新增】用于 BFS 寻路算法
#include "GameObject.h"
#include "utils.h"

class Map {
private:
    int width;
    int height;
    std::vector<std::string> grid; 

public:
    Map(int w, int h) : width(w), height(h) {
        generateDefaultMap();
    }

    void generateDefaultMap() {
        grid.clear();
        for (int y = 0; y < height; ++y) {
            std::string row = "";
            for (int x = 0; x < width; ++x) {
                if (y == 0 || y == height - 1 || x == 0 || x == width - 1) {
                    row += "#"; 
                } else {
                    row += "."; 
                }
            }
            grid.push_back(row);
        }
        grid[height-2][width-2] = '>';
    }

    bool isWalkable(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) return false;
        char tile = grid[y][x];
        return tile != '#'; 
    }

    // --- 【新增】BFS 路径检查算法 ---
    // 检查从 (startX, startY) 是否能走到 (endX, endY)
    bool hasPath(int startX, int startY, int endX, int endY) {
        // 1. 如果起点或终点本身就是墙，直接死局
        if (!isWalkable(startX, startY) || !isWalkable(endX, endY)) return false;

        // 2. 准备访问记录表 (visited)，防止兜圈子
        std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
        
        // 3. BFS 队列
        std::queue<Point> q;
        q.push({startX, startY});
        visited[startY][startX] = true;

        // 4. 方向数组：上下左右
        int dirs[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};

        while (!q.empty()) {
            Point curr = q.front();
            q.pop();

            // 如果到达终点，说明通路存在！
            if (curr.x == endX && curr.y == endY) return true;

            // 探索四周
            for (auto& dir : dirs) {
                int nx = curr.x + dir[0];
                int ny = curr.y + dir[1];

                // 检查边界、是否是墙、是否访问过
                if (nx >= 0 && nx < width && ny >= 0 && ny < height && 
                    isWalkable(nx, ny) && !visited[ny][nx]) {
                    
                    visited[ny][nx] = true;
                    q.push({nx, ny});
                }
            }
        }

        // 5. 队列空了也没找到终点，说明路断了
        return false;
    }

    // --- 【修改】生成障碍物 ---
    // 使用 while 循环，直到生成出一张能通关的地图为止
    void generateObstacles(int level) {
        bool pathFound = false;
        int attempts = 0;

        do {
            // 1. 重置为空房间
            generateDefaultMap();

            // 2. 随机撒墙
            // 随着等级提升，墙壁密度增加，但设置上限防止死循环
            int obstacleCount = (width * height) / 10 + (level * 5);
            // 简单的防止密度过大
            if (obstacleCount > (width * height) * 0.6) obstacleCount = (width * height) * 0.6;

            for (int i = 0; i < obstacleCount; ++i) {
                int x = rand() % (width - 2) + 1;
                int y = rand() % (height - 2) + 1;

                // 保护起点和终点不被直接覆盖
                // 同时保护起点周围一圈，防止出门就被堵死
                if ((std::abs(x - 1) <= 1 && std::abs(y - 1) <= 1) || 
                    (x == width - 2 && y == height - 2)) {
                    continue;
                }
                
                grid[y][x] = '#';
            }

            // 3. 检查死活：从 (1,1) 到 (width-2, height-2) 有路吗？
            pathFound = hasPath(1, 1, width - 2, height - 2);
            
            attempts++;
            // 防止极其罕见的无限循环（虽然 BFS 保证了只要有解就能找到）
            if (attempts > 1000) {
                // 如果实在随机不出来，就生成一个空地图保底
                generateDefaultMap();
                pathFound = true; 
            }

        } while (!pathFound); // 如果没路，就回滚重来
        
        // 可选：在这里打印一下 "Generated map in X attempts" 方便调试
        // std::cout << "Map generated in " << attempts << " attempts." << std::endl;
    }

    void draw(const std::vector<GameObject*>& objects) const {
        #ifdef _WIN32
            system("cls");
        #else
            std::cout << "\033[2J\033[1;1H"; 
        #endif

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                bool objectDrawn = false;
                for (const auto* obj : objects) {
                    if (obj->getPosition().x == x && obj->getPosition().y == y) {
                        obj->draw();
                        objectDrawn = true;
                        break;
                    }
                }

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

    int getWidth() const { return width; }
    int getHeight() const { return height; }
};

#endif // MAP_H