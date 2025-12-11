#ifndef GAME_H
#define GAME_H

#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <thread>
#include <chrono>
#include <fstream> 

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
    int difficulty; 

    void clearScreen() {
        #ifdef _WIN32
            system("cls");
        #else
            // Mac/Linux 使用 ANSI 转义序列清屏
            // \033[2J 清除屏幕，\033[1;1H 将光标移回左上角
            std::cout << "\033[2J\033[1;1H";
        #endif
    }

public:
    Game() : currentLevel(1), difficulty(2) {
        srand(time(0));
    }

    // --- 1. 游戏主流程控制 (重构版) ---
    void run() {
        while (true) { // 【外层循环】：主程序循环
            // 1. 显示主菜单
            int choice = showMainMenu();
            
            if (choice == 3) {
                // 退出游戏
                std::cout << "再见，勇士！" << std::endl;
                break; // 跳出外层循环，程序结束
            }

            // 2. 根据选择初始化游戏数据
            bool loadSuccess = false;
            if (choice == 2) {
                // 尝试读档
                loadSuccess = loadGame();
                if (!loadSuccess) {
                    // 读档失败，自动转为新游戏，但要重置等级
                    currentLevel = 1;
                    initPlayer(); 
                }
            } else {
                // 新游戏
                currentLevel = 1;
                initPlayer(); 
            }

            // 3. 【内层循环】：一局游戏的循环 (Level by Level)
            bool sessionRunning = true;
            while (sessionRunning) {
                showStory();    // 播放剧情
                initLevel();    // 生成关卡
                gameLoop();     // 玩这一关 (直到过关或死亡)
                
                if (player->isDead()) {
                    handleGameOver(); 
                    sessionRunning = false; // 结束这局游戏，跳出内层循环 -> 回到主菜单
                } else {
                    // 过关了
                    handleLevelComplete();
                    saveGame(); // 自动存档
                    // sessionRunning 依然为 true，继续下一关循环
                }
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
                    Input::clearBuffer(); 
                    skip = true;
                }
            }
        }
        std::cout << std::endl; 
    }

    // --- 2. 菜单逻辑 ---
    int showMainMenu() {
        while (true) {
            clearScreen();
            std::cout << "========================================" << std::endl;
            std::cout << "       地牢传说 (Dungeon Legend)        " << std::endl;
            std::cout << "========================================" << std::endl;
            std::cout << "1. 新的游戏 (New Game)" << std::endl;
            std::cout << "2. 继续征程 (Load Game)" << std::endl;
            std::cout << "3. 退出游戏 (Exit)" << std::endl;
            
            // 【关键修复 1】加上 << std::flush 确保 "> " 能立刻显示
            std::cout << "> " << std::flush;
            
            char choice = Input::get();
            if (choice == '1') {
                // 【关键修复 2】加上 << std::flush 确保这行字立刻显示，不让玩家盲选
                std::cout << "\n请选择难度 (1:萌新 2:普通 3:受苦): " << std::flush;
                
                char diff = Input::get();
                difficulty = (diff >= '1' && diff <= '3') ? (diff - '0') : 2;
                return 1;
            } else if (choice == '2') {
                return 2;
            } else if (choice == '3') {
                return 3;
            }
        }
    }

    // --- 3. 初始化玩家 ---
    void initPlayer() {
        // 每次重新开始都创建一个全新的满血玩家对象
        player = std::make_shared<Player>(1, 1);
        if (difficulty == 1) player->heal(50); 
    }

    // 辅助函数：寻找一个合法的空坐标
    Point getValidSpawnPosition() {
        int x, y;
        int attempts = 0;
        do {
            x = rand() % (map->getWidth() - 2) + 1;
            y = rand() % (map->getHeight() - 2) + 1;
            attempts++;
            if (attempts > 1000) break; 
        } while (!map->isWalkable(x, y) || 
                 (x == 1 && y == 1) || 
                 (x == map->getWidth()-2 && y == map->getHeight()-2));
        return {x, y};
    }

    // --- 4. 关卡生成 ---
    void initLevel() {
        int mapW = 20 + currentLevel * 2; 
        int mapH = 10 + currentLevel;
        map = std::make_unique<Map>(mapW, mapH);
        map->generateObstacles(currentLevel);

        player->setPosition(1, 1);

        enemies.clear();
        enemies.push_back(player); 

        int slimeCount = currentLevel * difficulty + 2; 
        for(int i=0; i<slimeCount; ++i) {
            Point p = getValidSpawnPosition();
            enemies.push_back(std::make_shared<Slime>(p.x, p.y));
        }

        if (difficulty == 3 || currentLevel >= 3) {
             Point p = getValidSpawnPosition();
             enemies.push_back(std::make_shared<Dragon>(p.x, p.y));
        }

        items.clear();
        Point potionPos = getValidSpawnPosition();
        items.push_back(std::make_shared<Potion>(potionPos.x, potionPos.y));
        
        if (currentLevel % 2 == 0) { 
            Point swordPos = getValidSpawnPosition();
            items.push_back(std::make_shared<Sword>(swordPos.x, swordPos.y));
        }
    }

    // --- 5. 剧情系统 ---
    void showStory() {
        clearScreen();
        std::cout << Color::CYAN << "----------------------------------------" << std::endl;
        std::cout << "           第 " << currentLevel << " 层" << std::endl;
        std::cout << "----------------------------------------" << Color::RESET << std::endl;
        
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
        Input::get(); 
    }

    // --- 6. 游戏内循环 ---
    void gameLoop() {
        bool levelRunning = true;
        // 只要本关没结束，且玩家没死，就一直循环
        while (levelRunning && !player->isDead()) {
            // 1. 渲染
            std::vector<GameObject*> renderList;
            for (const auto& i : items) renderList.push_back(i.get());
            for (const auto& c : enemies) renderList.push_back(c.get());
            map->draw(renderList);

            // 2. UI
            std::cout << "LV: " << currentLevel << " | DIFF: " << difficulty << std::endl;
            std::cout << player->getStatsString() << std::endl;
            int logSize = MessageLog::getLogs().size();
            int start = (logSize > 5) ? (logSize - 5) : 0;
            for (int i = start; i < logSize; ++i) std::cout << MessageLog::getLogs()[i] << std::endl;

            // 3. 检查过关
            Point pPos = player->getPosition();
            if (pPos.x == map->getWidth() - 2 && pPos.y == map->getHeight() - 2) {
                levelRunning = false; 
                return;
            }

            // 4. 玩家回合
            std::vector<Creature*> activeCreatures;
            for(const auto& c : enemies) activeCreatures.push_back(c.get());
            player->onTurn(*map, activeCreatures);

            // 5. 物品拾取
             for (auto it = items.begin(); it != items.end(); ) {
                if ((*it)->getPosition() == player->getPosition()) {
                    if ((*it)->onPickUp(player.get())) {
                        it = items.erase(it); 
                        continue; 
                    }
                }
                ++it;
            }

            // 6. 敌人回合
            for (auto& c : enemies) {
                if (c.get() != player.get() && !c->isDead()) {
                    c->onTurn(*map, activeCreatures);
                }
            }

            // 7. 清理尸体
             enemies.erase(
                std::remove_if(enemies.begin(), enemies.end(), 
                    [this](const std::shared_ptr<Creature>& c) {
                        return c->isDead() && c != player; 
                    }),
                enemies.end()
            );
        }
    }

    void handleGameOver() {
        std::cout << Color::RED << "\n胜败乃兵家常事。大侠请重新来过。" << Color::RESET << std::endl;
        std::cout << "按任意键返回主菜单...";
        Input::get(); 
        // 这里不需要做任何事，函数结束后会回到 run() 的内层循环判断，
        // 因为 player->isDead() 为真，sessionRunning 会被设为 false，从而跳回主菜单
    }

    void handleLevelComplete() {
        currentLevel++;
        std::cout << Color::YELLOW << "恭喜通过第 " << (currentLevel-1) << " 层！" << Color::RESET << std::endl;
    }

    // --- 7. 存档功能 ---
    void saveGame() {
        std::ofstream outFile("savegame.txt"); 
        if (outFile.is_open()) {
            outFile << currentLevel << " "
                    << difficulty << " "
                    << player->getHp() << " "
                    << player->getMaxHp() << " "
                    << player->getAttack(); 
            outFile.close();
            MessageLog::add(Color::YELLOW + ">>> 游戏进度已保存！ <<<" + Color::RESET);
        }
    }

    // --- 8. 读档功能 ---
    bool loadGame() {
        std::ifstream inFile("savegame.txt");
        if (inFile.is_open()) {
            int hp, maxHp, atk;
            inFile >> currentLevel >> difficulty >> hp >> maxHp >> atk;
            
            if (!player) player = std::make_shared<Player>(1, 1);
            player->setStats(hp, maxHp, atk); 
            
            inFile.close();
            std::cout << ">>> 存档读取成功！ <<<" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            return true;
        } else {
            std::cout << Color::RED << "没有找到存档文件！" << Color::RESET << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            return false;
        }
    }
};

#endif