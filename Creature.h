#ifndef CREATURE_H
#define CREATURE_H

#include "GameObject.h"
#include "Map.h" // 生物移动需要知道地图信息
#include "MessageLog.h" // 日志输出

class Creature : public GameObject {
protected:
    int hp;
    int maxHp;
    int attackPower;
    int defense;

public:
    Creature(int x, int y, std::string sym, std::string n, int maxH, int atk, int def, std::string c)
        : GameObject(x, y, sym, n, c), hp(maxH), maxHp(maxH), attackPower(atk), defense(def) {}

    // 纯虚函数：每个生物的回合行为不同
    // 玩家是等待输入，怪物是AI计算
    virtual void onTurn(Map& map, std::vector<Creature*>& others) = 0;

    // 尝试移动逻辑：检查地图是否阻挡
    bool tryMove(int dx, int dy, const Map& map) {
        int newX = pos.x + dx;
        int newY = pos.y + dy;

        if (map.isWalkable(newX, newY)) {
            // 这里未来还需要检测是否撞到其他怪物
            pos.x = newX;
            pos.y = newY;
            return true;
        }
        return false;
    }

    // 基础绘制实现（如果不需要特殊绘制，直接复用父类的）
    void draw() const override {
        GameObject::printSymbol();
    }

    // Getters
    int getHp() const { return hp; }
    int getMaxHp() const { return maxHp; }

    // 在类内部添加以下方法：

    // 接收伤害
    void takeDamage(int amount) {
        int actualDamage = amount - defense;
        if (actualDamage < 1) actualDamage = 1; // 破防机制：最少扣1血
        
        hp -= actualDamage;
        if (hp < 0) hp = 0;
        
        // 记录日志
        // 注意：这里简单拼接字符串，并未处理很复杂的格式
        // MessageLog::add(name + " 受到 " + std::to_string(actualDamage) + " 点伤害！");
    }

    // 攻击目标
    void attack(Creature* target) {
        // 这里可以加入命中率计算，目前必中
        MessageLog::add(name + " 攻击了 " + target->getName() + " !");
        target->takeDamage(attackPower);
        
        if (target->isDead()) {
            MessageLog::add(target->getName() + " 被击败了！");
            // 这里可以处理经验值获取，稍后在 Player 类完善
        }
    }

    bool isDead() const {
        return hp <= 0;
    }
};

#endif // CREATURE_H