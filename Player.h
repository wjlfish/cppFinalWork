#ifndef PLAYER_H
#define PLAYER_H

#include "Creature.h"
#include "Input.h"

class Player : public Creature {
private:
    int level;
    int exp;

public:
    Player(int x, int y) 
        : Creature(x, y, "@", "Hero", 100, 10, 2, Color::GREEN), level(1), exp(0) {}

    // 实现多态方法 onTurn
    // 修改 onTurn 方法：

    void onTurn(Map& map, std::vector<Creature*>& others) override {
        char input = Input::get();
        int dx = 0, dy = 0;

        switch (std::toupper(input)) {
            case 'W': dy = -1; break;
            case 'S': dy = 1; break;
            case 'A': dx = -1; break;
            case 'D': dx = 1; break;
            case 'Q': exit(0); break;
            default: return; // 无效按键，回合不消耗（或者消耗，看设计）
        }

        int targetX = pos.x + dx;
        int targetY = pos.y + dy;

        // 1. 碰撞检测：是否有怪物？
        bool attacked = false;
        for (auto* c : others) {
            // 注意：others 列表里包含玩家自己（在 main 中传入时包含），要排除
            if (c == this) continue; 
            
            // 如果该生物还活着，并且在目标位置
            if (!c->isDead() && c->getPosition().x == targetX && c->getPosition().y == targetY) {
                // 执行攻击！
                attack(c);
                attacked = true;
                break;
            }
        }

        // 2. 如果没有发生战斗，尝试移动
        if (!attacked) {
            tryMove(dx, dy, map);
        }
    }

    // 在 Player 类 public 区域添加：

    void heal(int amount) {
        hp += amount;
        if (hp > maxHp) hp = maxHp;
    }

    void buffAttack(int amount) {
        attackPower += amount;
    }

    int getAttack() const { return attackPower; }

    // 用于读档时强制覆盖属性
    void setStats(int h, int mh, int atk) {
        hp = h;
        maxHp = mh;
        attackPower = atk;
    }

    // 玩家特有的UI信息获取
    std::string getStatsString() const {
        return "HP: " + std::to_string(hp) + "/" + std::to_string(maxHp) + 
               " | ATK: " + std::to_string(attackPower) + 
               " | LV: " + std::to_string(level);
    }
};

#endif // PLAYER_H