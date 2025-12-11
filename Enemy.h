#ifndef ENEMY_H
#define ENEMY_H

#include "Creature.h"
#include <cstdlib> // for rand()

// 敌人基类
class Enemy : public Creature {
public:
    Enemy(int x, int y, std::string sym, std::string n, int hp, int atk, int def, std::string c)
        : Creature(x, y, sym, n, hp, atk, def, c) {}
        
    // 后面会由具体子类覆盖，这里提供默认行为
    void onTurn(Map& map, std::vector<Creature*>& others) override {
        // 默认空AI，不动
    }
};

// 具体怪物：史莱姆 (特点：血少，随机移动)
class Slime : public Enemy {
public:
    Slime(int x, int y) 
        // 这里的参数：符号 's', 名字 "Slime", HP 20, 攻 5, 防 0, 颜色 青色
        : Enemy(x, y, "s", "Slime", 20, 5, 0, Color::CYAN) {}

    void onTurn(Map& map, std::vector<Creature*>& others) override {
        // 简单的随机 AI
        int dir = rand() % 4;
        int dx = 0, dy = 0;
        switch(dir) {
            case 0: dy = -1; break; // 上
            case 1: dy = 1;  break; // 下
            case 2: dx = -1; break; // 左
            case 3: dx = 1;  break; // 右
        }

        // 检查目标位置
        int targetX = pos.x + dx;
        int targetY = pos.y + dy;

        // 1. 检查是否撞墙
        if (!map.isWalkable(targetX, targetY)) return;

        // 2. 检查是否撞到玩家或其他怪物
        // 哪怕是简单的史莱姆，我们也要遍历 others 列表看看目标点有没有人
        for (auto* other : others) {
            if (other == this) continue; // 跳过自己
            if (other->getPosition().x == targetX && other->getPosition().y == targetY) {
                // 如果撞到的是 Hero (玩家)，则攻击！
                // 这里用简单的名称判断，严谨点应该用 dynamic_cast 或 type tag
                if (other->getName() == "Hero") {
                    attack(other);
                }
                return; // 撞到人就停下，不移动
            }
        }

        // 3. 没人没墙，移动
        pos.x = targetX;
        pos.y = targetY;
    }
};

// 在 Enemy.h 末尾添加：

class Dragon : public Enemy {
public:
    Dragon(int x, int y) 
        : Enemy(x, y, "D", "Dragon", 50, 15, 5, Color::RED) {}

    void onTurn(Map& map, std::vector<Creature*>& others) override {
        // 1. 寻找玩家位置
        Creature* player = nullptr;
        for (auto* c : others) {
            if (c->getName() == "Hero") {
                player = c;
                break;
            }
        }
        if (!player) return; // 没找到玩家

        // 2. 计算理想方向
        int dx = 0, dy = 0;
        if (pos.x < player->getPosition().x) dx = 1;
        else if (pos.x > player->getPosition().x) dx = -1;
        
        if (pos.y < player->getPosition().y) dy = 1;
        else if (pos.y > player->getPosition().y) dy = -1;

        // --- 3. 智能移动尝试逻辑 ---
        
        // 定义一个 lambda 函数来尝试移动，成功返回 true
        auto tryMove = [&](int tryDx, int tryDy) -> bool {
            if (tryDx == 0 && tryDy == 0) return false;
            
            int targetX = pos.x + tryDx;
            int targetY = pos.y + tryDy;

            // 检查地形
            if (!map.isWalkable(targetX, targetY)) return false;

            // 检查生物碰撞
            for (auto* other : others) {
                if (other == this) continue;
                if (other->getPosition().x == targetX && other->getPosition().y == targetY) {
                    // 如果挡路的是玩家，攻击！
                    if (other->getName() == "Hero") {
                        attack(other);
                        MessageLog::add(Color::RED + "巨龙吐息！造成巨大伤害！" + Color::RESET);
                        return true; // 攻击也算作一次行动成功
                    }
                    return false; // 被其他怪物（比如史莱姆）挡住了，不能走
                }
            }

            // 没人没墙，移动成功
            pos.x = targetX;
            pos.y = targetY;
            return true;
        };

        // A计划：尝试理想路径（比如直接走对角线）
        if (tryMove(dx, dy)) return;

        // B计划：如果对角线走不通，尝试只走X轴（滑墙）
        if (dx != 0 && tryMove(dx, 0)) return;

        // C计划：如果X轴也走不通，尝试只走Y轴
        if (dy != 0 && tryMove(0, dy)) return;
        
        // 如果都走不通，那只能休息一回合了
    }
};

#endif // ENEMY_H