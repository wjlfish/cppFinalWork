#ifndef ITEM_H
#define ITEM_H

#include "GameObject.h"
#include "Player.h"
#include "MessageLog.h"

class Item : public GameObject {
public:
    Item(int x, int y, std::string sym, std::string n, std::string c)
        : GameObject(x, y, sym, n, c) {}

    // 【关键修复】实现父类的纯虚函数 draw
    // 所有的物品（剑、药水）都通用这个绘制逻辑
    void draw() const override {
        printSymbol();
    }

    // 纯虚函数：物品被玩家触碰时发生什么
    virtual bool onPickUp(Player* p) = 0;
};

// 治疗药水
class Potion : public Item {
    int healAmount;
public:
    Potion(int x, int y) : Item(x, y, "!", "Potion", Color::MAGENTA), healAmount(30) {}
    
    bool onPickUp(Player* p) override {
        if (p->getHp() < p->getMaxHp()) {
            p->heal(healAmount); 
            MessageLog::add("你喝下了药水，恢复了 " + std::to_string(healAmount) + " 点 HP!");
            return true;
        } else {
            MessageLog::add("你生命值是满的，现在不需要药水。");
            return false;
        }
    }
};

// 武器：圣剑
class Sword : public Item {
    int atkBonus;
public:
    Sword(int x, int y) : Item(x, y, "/", "Excalibur", Color::YELLOW), atkBonus(5) {}

    bool onPickUp(Player* p) override {
        p->buffAttack(atkBonus); 
        MessageLog::add("你拔出了石中剑！攻击力增加了 " + std::to_string(atkBonus) + " 点!");
        return true;
    }
};

#endif