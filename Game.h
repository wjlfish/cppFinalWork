#ifndef GAME_H
#define GAME_H

#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <thread> // 用于剧情文字延时
#include <chrono>
#include <fstream> // 用于文件读写

#include "Map.h"
#include "Player.h"
#include "Enemy.h"
#include "Item.h"
#include "MessageLog.h"
#include "Input.h"

class Game {
private:
    std::unique_ptr<Map> map;
    std::shared_ptr<Player> player;
    std::vector<std::shared_ptr<Creature>> enemies;
    std::vector<std::shared_ptr<Item>> items;
    
    int currentLevel;
    int difficulty; // 1: 简单, 2: 普通, 3: 困难
    bool isRunning;

public:
    Game() : currentLevel(1), difficulty(2), isRunning(true) {
        // 初始化随机数种子
        srand(time(0));
    }

    // --- 1. 游戏主流程控制 ---
    void run() {
        // 1. 显示菜单并获取选择
        int choice = showMainMenu(); // 修改 showMainMenu 让它返回 int
        
        // 2. 根据选择初始化
        if (choice == 2) {
            // --- 读档模式 ---
            initPlayer(); // 先创建一个默认玩家
            if (!loadGame()) { 
                // 如果读档失败（文件不存在），转为新游戏
                currentLevel = 1; 
                // difficulty 已经在 showMainMenu 设了默认值
            }
        } else {
            // --- 新游戏模式 ---
            currentLevel = 1;
            initPlayer();
        }
        
        // 3. 开始循环
        while (isRunning) {
            showStory(); 
            initLevel(); 
            gameLoop();
            
            if (player->isDead()) {
                handleGameOver();
                // 游戏结束时不自动保存，否则玩家就永远死在存档里了
                // 可以选择删除存档 remove("savegame.txt");
            } else {
                handleLevelComplete();
                // 自动存档：每过一关存一次
                saveGame();
            }
        }
    }

private:
    // --- 辅助功能：打字机输出 ---
    void typewriterPrint(const std::string& text, int delayMs = 50) {
        bool skip = false;
        
        for (char c : text) {
            std::cout << c << std::flush;
            
            if (!skip) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
                
                if (Input::hasPending()) {
                    Input::clearBuffer(); // 消耗掉按下的键（不会再显示在屏幕上了）
                    skip = true;
                }
            }
        }
        std::cout << std::endl; 
    }

    // --- 5. 剧情系统 (升级版) ---
    void showStory() {
        system("cls"); // Mac/Linux use system("clear")
        std::cout << Color::CYAN << "----------------------------------------" << std::endl;
        std::cout << "           第 " << currentLevel << " 层" << std::endl;
        std::cout << "----------------------------------------" << Color::RESET << std::endl;
        
        // 使用打字机效果输出剧情
        // 确保每行文字都作为单独的字符串传入，这样每行都有机会暂停
        
        if (currentLevel == 1) {
            typewriterPrint("你踏入了阴暗的地下城，手中紧握着生锈的铁剑。");
            typewriterPrint("传说地牢的最深处藏着能够改写命运的 '虚空之心'。");
            typewriterPrint("为了家乡，你必须前进...");
        } else if (currentLevel == 2) {
            typewriterPrint("空气变得更加潮湿，墙壁上渗出绿色的粘液。");
            typewriterPrint("你隐约听到了低沉的咆哮声，似乎有什么东西在窥视你。");
        } else if (currentLevel == 3) {
            typewriterPrint("这里热得让人窒息。岩浆在地板缝隙中流动。");
            typewriterPrint("前方的热浪中，巨大的黑影正在缓缓升起...");
            typewriterPrint("巨龙的巢穴就在前方！");
        } else {
            typewriterPrint("你向着无尽的深渊继续进发...");
            typewriterPrint("不知道前方还有多少挑战在等待。");
        }
        
        std::cout << Color::GREY << "\n(按任意键开始战斗...)" << Color::RESET << std::endl;
        Input::get(); // 等待玩家准备好
    }
    // 辅助函数：寻找一个合法的空坐标
    Point getValidSpawnPosition() {
        int x, y;
        int attempts = 0;
        do {
            x = rand() % (map->getWidth() - 2) + 1;
            y = rand() % (map->getHeight() - 2) + 1;
            attempts++;
            
            // 防止死循环（虽然极小概率，但为了健壮性）
            if (attempts > 1000) break; 
            
            // 规则：
            // 1. 必须是地板 (isWalkable)
            // 2. 不能是玩家起点 (1,1)
            // 3. 不能是出口 (Width-2, Height-2)
            // 4. (可选) 最好不要和已有的物品重叠，暂略
        } while (!map->isWalkable(x, y) || 
                 (x == 1 && y == 1) || 
                 (x == map->getWidth()-2 && y == map->getHeight()-2));
        
        return {x, y};
    }
    // --- 2. 菜单与自定义选项 ---
    int showMainMenu() {
        while (true) {
            system("cls");
            std::cout << "========================================" << std::endl;
            std::cout << "       地牢传说 (Dungeon Legend)        " << std::endl;
            std::cout << "========================================" << std::endl;
            std::cout << "1. 新的游戏 (New Game)" << std::endl;
            std::cout << "2. 继续征程 (Load Game)" << std::endl;
            std::cout << "3. 退出游戏 (Exit)" << std::endl;
            std::cout << "> ";
            
            char choice = Input::get();
            if (choice == '1') {
                // 如果是新游戏，才问难度
                std::cout << "\n请选择难度 (1:萌新 2:普通 3:受苦): ";
                char diff = Input::get();
                difficulty = (diff >= '1' && diff <= '3') ? (diff - '0') : 2;
                return 1;
            } else if (choice == '2') {
                return 2;
            } else if (choice == '3') {
                exit(0);
            }
        }
    }

    // --- 3. 初始化玩家 ---
    void initPlayer() {
        player = std::make_shared<Player>(1, 1);
        // 根据难度调整初始状态，这里简单演示
        if (difficulty == 1) player->heal(50); // 简单模式加血
    }

    // --- 4. 关卡生成 (核心算法) ---
    // --- 4. 关卡生成 (修复后) ---
    void initLevel() {
        // 1. 生成地图
        int mapW = 20 + currentLevel * 2; 
        int mapH = 10 + currentLevel;
        map = std::make_unique<Map>(mapW, mapH);
        
        // 加入随机障碍物
        map->generateObstacles(currentLevel);

        // 2. 重置玩家位置
        player->setPosition(1, 1);

        // 3. 生成敌人 (使用安全坐标)
        enemies.clear();
        enemies.push_back(player); 

        int slimeCount = currentLevel * difficulty + 2; 
        for(int i=0; i<slimeCount; ++i) {
            Point p = getValidSpawnPosition();
            enemies.push_back(std::make_shared<Slime>(p.x, p.y));
        }

        // 生成巨龙
        if (difficulty == 3 || currentLevel >= 3) {
             Point p = getValidSpawnPosition();
             enemies.push_back(std::make_shared<Dragon>(p.x, p.y));
        }

        // 4. 生成物品 (关键修复点！)
        items.clear();
        
        // 生成药水
        Point potionPos = getValidSpawnPosition();
        items.push_back(std::make_shared<Potion>(potionPos.x, potionPos.y));
        
        // 生成武器
        if (currentLevel % 2 == 0) { 
            Point swordPos = getValidSpawnPosition();
            items.push_back(std::make_shared<Sword>(swordPos.x, swordPos.y));
        }
    }

    // --- 6. 游戏内循环 (原 main 函数的逻辑) ---
    void gameLoop() {
        bool levelRunning = true;
        while (levelRunning && !player->isDead()) {
            // ... (这里复制原 main.cpp 中的 渲染 -> 输入 -> 逻辑 代码) ...
            // 为了节省篇幅，这里简写，你需要把之前的逻辑搬进来
            
            // 渲染
            std::vector<GameObject*> renderList;
            for (const auto& i : items) renderList.push_back(i.get());
            for (const auto& c : enemies) renderList.push_back(c.get());
            map->draw(renderList);

            // UI
            std::cout << "LV: " << currentLevel << " | DIFF: " << difficulty << std::endl;
            std::cout << player->getStatsString() << std::endl;
             // 显示日志...
             int logSize = MessageLog::getLogs().size();
             int start = (logSize > 5) ? (logSize - 5) : 0;
             for (int i = start; i < logSize; ++i) std::cout << MessageLog::getLogs()[i] << std::endl;

            // 检查过关
            Point pPos = player->getPosition();
            if (pPos.x == map->getWidth() - 2 && pPos.y == map->getHeight() - 2) {
                levelRunning = false; // 跳出当前层循环，进入下一层
                return;
            }

            // 玩家回合
            std::vector<Creature*> activeCreatures;
            for(const auto& c : enemies) activeCreatures.push_back(c.get());
            player->onTurn(*map, activeCreatures);

            // 物品拾取
             for (auto it = items.begin(); it != items.end(); ) {
                if ((*it)->getPosition() == player->getPosition()) {
                    if ((*it)->onPickUp(player.get())) {
                        it = items.erase(it); 
                        continue; 
                    }
                }
                ++it;
            }

            // 敌人回合
            for (auto& c : enemies) {
                if (c.get() != player.get() && !c->isDead()) {
                    c->onTurn(*map, activeCreatures);
                }
            }

            // 清理尸体
             enemies.erase(
                std::remove_if(enemies.begin(), enemies.end(), 
                    [this](const std::shared_ptr<Creature>& c) {
                        return c->isDead() && c != player; 
                    }),
                enemies.end()
            );
        }
    }

    // --- 7. 存档功能 ---
    void saveGame() {
        std::ofstream outFile("savegame.txt"); // 创建/覆盖文件
        if (outFile.is_open()) {
            // 序列化数据：用空格分隔
            outFile << currentLevel << " "
                    << difficulty << " "
                    << player->getHp() << " "
                    << player->getMaxHp() << " "
                    // 注意：我们需要在 Player 类加一个 getAttack()，或者直接把 attackPower 设为 public
                    // 为了 OOP 封装性，建议去 Player.h 加一个 getAttack()。
                    // 这里假设你已经加了，或者暂时用 player->attackPower (如果改了权限)
                    << player->getAttack(); 
            
            outFile.close();
            MessageLog::add(Color::YELLOW + ">>> 游戏进度已保存！ <<<" + Color::RESET);
        } else {
            MessageLog::add(Color::RED + "错误：无法写入存档文件！" + Color::RESET);
        }
    }

    // --- 8. 读档功能 ---
    // 返回 true 表示读取成功
    bool loadGame() {
        std::ifstream inFile("savegame.txt");
        if (inFile.is_open()) {
            int hp, maxHp, atk;
            
            // 按照写入的顺序读取
            inFile >> currentLevel >> difficulty >> hp >> maxHp >> atk;
            
            // 恢复玩家状态
            // 注意：这里需要确保 player 对象已经创建。
            // 我们会在调用 loadGame 前先 initPlayer，然后用读取的数据覆盖它。
            if (!player) player = std::make_shared<Player>(1, 1);
            
            player->setStats(hp, maxHp, atk); // 需要在 Player.h 实现这个 setter
            
            inFile.close();
            std::cout << ">>> 存档读取成功！欢迎回来，勇士。 <<<" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            return true;
        } else {
            std::cout << Color::RED << "没有找到存档文件！" << Color::RESET << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            return false;
        }
    }

    void handleGameOver() {
        std::cout << Color::RED << "胜败乃兵家常事。大侠请重新来过。" << Color::RESET << std::endl;
        isRunning = false;
        Input::get();
    }

    void handleLevelComplete() {
        currentLevel++;
        std::cout << Color::YELLOW << "恭喜通过第 " << (currentLevel-1) << " 层！" << Color::RESET << std::endl;
        // 这里可以增加存档逻辑：saveGame();
    }
};

#endif