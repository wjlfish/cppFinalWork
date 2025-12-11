#ifndef GAME_H
#define GAME_H

#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <thread>
#include <chrono>
#include <fstream> 
#include <cstdio> // 用于 remove 删除存档文件
#include <sstream>

#include "Map.h"
#include "Player.h"
#include "Enemy.h"
#include "Item.h"
#include "MessageLog.h"
#include "Input.h"

// 定义游戏模式常量
const int MODE_STORY = 0;   // 剧情模式 (5关结束)
const int MODE_INFINITE = 1; // 无尽模式

class Game {
private:
    std::unique_ptr<Map> map;
    std::shared_ptr<Player> player;
    std::vector<std::shared_ptr<Creature>> enemies;
    std::vector<std::shared_ptr<Item>> items;
    
    int currentLevel;
    int difficulty; 
    int gameMode;

    // --- 辅助功能：XOR 加密/解密算法 ---

    std::string xorCipher(std::string data) {
        const std::string key = "bukeyixiugaizhegekeyyinweizhegekeyshibaozhengwanjiabunengzuobideguanjianyinsu39fh319cb93bcv92buev";
        std::string result = data;
        
        for (size_t i = 0; i < data.size(); i++) {
            // 利用取模运算循环使用密钥的每一个字符进行异或
            result[i] = data[i] ^ key[i % key.size()];
        }
        return result;
    }

    int currentSlot; // 【新增】记录当前存档槽位 (1, 2, 3...)

    // 【新增】根据槽位生成文件名
    std::string getSaveFileName(int slot) const {
        return "saves/savegame_" + std::to_string(slot) + ".dat";
    }

    // 【新增】辅助UI：让玩家选择槽位
    int askForSaveSlot() {
        std::cout << "\n请选择存档槽位 (1 - 3):\n";
        // 简单的检查文件是否存在，给玩家一点提示（可选优化）
        for (int i = 1; i <= 3; ++i) {
            std::ifstream f(getSaveFileName(i));
            std::cout << i << ". 槽位 " << i;
            if (f.good()) std::cout << " [已有存档]";
            else std::cout << " [空]";
            std::cout << std::endl;
        }
        std::cout << "> " << std::flush;
        
        char c = Input::get();
        if (c >= '1' && c <= '3') return c - '0';
        return 1; // 默认返回槽位 1
    }

public:
    Game() : currentLevel(1), difficulty(2), gameMode(MODE_STORY), currentSlot(1) {
        srand(time(0));
    }

    void run() {
        while (true) { 
            // 1. 主菜单
            int choice = showMainMenu();
            
            if (choice == 3) {
                std::cout << "再见，勇士！" << std::endl;
                break; 
            }

            // 2. 初始化数据
            bool loadSuccess = false;
            if (choice == 2) {
                loadSuccess = loadGame();
                if (!loadSuccess) {
                    currentLevel = 1;
                    // difficulty 和 gameMode 都在 showMainMenu 里被重置了默认值，
                    // 但如果是读档失败转新游戏，最好再次确认，这里简化处理，直接开始新游戏
                    initPlayer(); 
                }
            } else {
                // 新游戏：参数已经在 showMainMenu 中设置好了
                currentLevel = 1;
                initPlayer(); 
            }

            // 3. 游戏局循环
            bool sessionRunning = true;
            while (sessionRunning) {
                showStory();    
                initLevel();    
                gameLoop();     
                
                if (player->isDead()) {
                    handleGameOver(); 
                    sessionRunning = false; 
                } else {
                    // --- 通关判断逻辑 ---
                    
                    // 如果是剧情模式，且打通了第 5 关 (currentLevel == 5)
                    if (gameMode == MODE_STORY && currentLevel >= 5) {
                        handleVictory(); // 播放胜利结局
                        sessionRunning = false; // 结束会话，回到主菜单
                        // 通关后删除存档，防止玩家读档继续打第六关
                        std::remove(getSaveFileName(currentSlot).c_str());
                    } else {
                        // 普通过关
                        handleLevelComplete();
                        saveGame(); 
                    }
                }
            }
        }
    }

private:
    void clearScreen() {
        #ifdef _WIN32
            system("cls");
        #else
            std::cout << "\033[2J\033[1;1H";
        #endif
    }

    void typewriterPrint(const std::string& text, int delayMs = 30) {
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

    // --- 菜单逻辑 (包含模式选择) ---
    int showMainMenu() {
        while (true) {
            clearScreen();
            std::cout << "========================================" << std::endl;
            std::cout << "       地牢传说 (Dungeon Legend)        " << std::endl;
            std::cout << "========================================" << std::endl;
            std::cout << "1. 新的游戏 (New Game)" << std::endl;
            std::cout << "2. 继续征程 (Load Game)" << std::endl;
            std::cout << "3. 退出游戏 (Exit)" << std::endl;
            std::cout << "> " << std::flush;
            
            char choice = Input::get();
            
            if (choice == '1') {
                // 1. 先选槽位 (为了确定往哪里存)
                currentSlot = askForSaveSlot();

                // 2. 选难度
                std::cout << "\n请选择难度 (1:萌新 2:普通 3:受苦): " << std::flush;
                char diff = Input::get();
                difficulty = (diff >= '1' && diff <= '3') ? (diff - '0') : 2;

                // 3. 选模式
                std::cout << "\n请选择模式 (1:剧情 2:无尽): " << std::flush;
                char mode = Input::get();
                gameMode = (mode == '2') ? MODE_INFINITE : MODE_STORY;

                return 1;
            } else if (choice == '2') {
                // 读取时，直接问要读哪个槽位
                currentSlot = askForSaveSlot();
                return 2;
            } else if (choice == '3') {
                return 3;
            }
        }
    }

    void initPlayer() {
        player = std::make_shared<Player>(1, 1);
        if (difficulty == 1) player->heal(50); 
    }

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

    void initLevel() {
        // 限制地图大小
        int rawW = 20 + currentLevel * 2;
        int rawH = 10 + currentLevel;
        int mapW = (rawW > 60) ? 60 : rawW; 
        int mapH = (rawH > 25) ? 25 : rawH;

        map = std::make_unique<Map>(mapW, mapH);
        map->generateObstacles(currentLevel);

        player->setPosition(1, 1);
        enemies.clear();
        enemies.push_back(player); 

        // 怪物生成
        int calculatedSlimeCount = currentLevel * difficulty + 2;
        int maxSlimes = (mapW * mapH) / 10;
        int slimeCount = (calculatedSlimeCount > maxSlimes) ? maxSlimes : calculatedSlimeCount;

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

    void showStory() {
        clearScreen();
        std::cout << Color::CYAN << "----------------------------------------" << std::endl;
        std::string modeStr = (gameMode == MODE_STORY) ? " (剧情模式 5层)" : " (无尽模式)";
        std::cout << "           第 " << currentLevel << " 层" << modeStr << std::endl;
        std::cout << "----------------------------------------" << Color::RESET << std::endl;
        
        if (currentLevel == 1) {
            typewriterPrint("你踏入了阴暗的地下城，传说地牢深处藏着'虚空之心'。");
            if (gameMode == MODE_STORY) typewriterPrint("只有打通第5层，才能彻底封印这里的邪恶。");
            else typewriterPrint("这是一条不归路，看你能坚持多久...");
        } else if (currentLevel == 2) {
            typewriterPrint("空气变得更加潮湿，墙壁上渗出绿色的粘液。");
        } else if (currentLevel == 3) {
            typewriterPrint("这里热得让人窒息。岩浆在地板缝隙中流动。");
            typewriterPrint("巨龙的巢穴就在前方！");
        } else if (currentLevel == 5 && gameMode == MODE_STORY) {
            typewriterPrint("【最终层】");
            typewriterPrint("你感觉到了前所未有的压迫感。");
            typewriterPrint("这是最后的试炼，虚空之心就在前方！");
        } else {
            typewriterPrint("你向着无尽的深渊继续进发...");
        }
        
        std::cout << Color::GREY << "\n(按任意键开始战斗...)" << Color::RESET << std::endl;
        Input::get(); 
    }

    void gameLoop() {
        bool levelRunning = true;
        while (levelRunning && !player->isDead()) {
            std::vector<GameObject*> renderList;
            for (const auto& i : items) renderList.push_back(i.get());
            for (const auto& c : enemies) renderList.push_back(c.get());
            map->draw(renderList);

            std::cout << "LV: " << currentLevel << " | DIFF: " << difficulty;
            if (gameMode == MODE_STORY) std::cout << " | GOAL: Level 5";
            std::cout << std::endl;
            
            std::cout << player->getStatsString() << std::endl;
            int logSize = MessageLog::getLogs().size();
            int start = (logSize > 5) ? (logSize - 5) : 0;
            for (int i = start; i < logSize; ++i) std::cout << MessageLog::getLogs()[i] << std::endl;

            Point pPos = player->getPosition();
            if (pPos.x == map->getWidth() - 2 && pPos.y == map->getHeight() - 2) {
                levelRunning = false; 
                return;
            }

            std::vector<Creature*> activeCreatures;
            for(const auto& c : enemies) activeCreatures.push_back(c.get());
            player->onTurn(*map, activeCreatures);

             for (auto it = items.begin(); it != items.end(); ) {
                if ((*it)->getPosition() == player->getPosition()) {
                    if ((*it)->onPickUp(player.get())) {
                        it = items.erase(it); 
                        continue; 
                    }
                }
                ++it;
            }

            for (auto& c : enemies) {
                if (c.get() != player.get() && !c->isDead()) {
                    c->onTurn(*map, activeCreatures);
                }
            }

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
        clearScreen();
        std::cout << Color::RED << "\n\n胜败乃兵家常事。但你的冒险到此为止了。" << Color::RESET << std::endl;
        std::cout << "按任意键返回主菜单...";
        Input::get(); 
        // 游戏结束，删除存档
        std::remove(getSaveFileName(currentSlot).c_str());
    }

    // 【新增】胜利结局处理
    void handleVictory() {
        clearScreen();
        std::cout << Color::YELLOW << "\n\n################################################" << std::endl;
        std::cout << "#               VICTORY!                       #" << std::endl;
        std::cout << "#                                              #" << std::endl;
        std::cout << "#      你成功击穿了第 5 层地牢！               #" << std::endl;
        std::cout << "#      虚空之心被你摧毁，世界恢复了和平。      #" << std::endl;
        std::cout << "#                                              #" << std::endl;
        std::cout << "#           感谢游玩 地牢传说                  #" << std::endl;
        std::cout << "################################################\n" << Color::RESET << std::endl;
        
        std::cout << "按任意键返回主菜单...";
        Input::get(); 
        std::remove(getSaveFileName(currentSlot).c_str());
    }

    void handleLevelComplete() {
        currentLevel++;
        clearScreen();
        std::cout << Color::YELLOW << "\n\n>>> 恭喜通过第 " << (currentLevel-1) << " 层！ <<<" << Color::RESET << std::endl;
        std::cout << "稍微休息一下，准备进入下一层..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // --- 7. 存档功能 (加密版) ---
    // --- 存档功能 (自动创建文件夹版) ---
    // --- 存档功能 (系统命令版) ---
    void saveGame() {
        // 1. 序列化与加密 (不变)
        std::stringstream ss;
        ss << currentLevel << " " << difficulty << " " 
           << player->getHp() << " " << player->getMaxHp() << " " 
           << player->getAttack() << " " << gameMode;
        
        std::string rawData = ss.str();
        std::string encryptedData = xorCipher(rawData);
        
        // 2. 【核心修改】使用系统命令创建文件夹
        // 这行代码会让系统去建一个叫 saves 的文件夹
        // mkdir -p (Mac/Linux) 和 if not exist (Windows) 互不干扰
        #ifdef _WIN32
            system("if not exist saves mkdir saves");
        #else
            system("mkdir -p saves");
        #endif

        // 3. 打开文件写入 (不变)
        std::ofstream outFile(getSaveFileName(currentSlot), std::ios::binary); 
        
        if (outFile.is_open()) {
            outFile.write(encryptedData.c_str(), encryptedData.size());
            outFile.close();
            MessageLog::add(Color::YELLOW + ">>> 进度已保存至槽位 " + std::to_string(currentSlot) + " <<<" + Color::RESET);
        } else {
            MessageLog::add(Color::RED + "错误：无法写入存档 (权限被拒绝)！" + Color::RESET);
        }
    }
    // --- 8. 读档功能 (解密版) ---
    bool loadGame() {
        // 1. 以二进制模式打开
        std::ifstream inFile(getSaveFileName(currentSlot), std::ios::binary);
        
        if (inFile.is_open()) {
            // 2. 读取整个文件内容到 string
            std::stringstream buffer;
            buffer << inFile.rdbuf();
            std::string encryptedData = buffer.str();
            inFile.close();
            
            std::string decryptedData = xorCipher(encryptedData);
            std::stringstream ss(decryptedData);
            int hp, maxHp, atk;
            
            if (ss >> currentLevel >> difficulty >> hp >> maxHp >> atk >> gameMode) {
                if (!player) player = std::make_shared<Player>(1, 1);
                player->setStats(hp, maxHp, atk); 
                
                std::cout << ">>> 载入槽位 " << currentSlot << " 成功！ <<<" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                return true;
            } else {
                // 如果解密后数据格式不对（说明文件被篡改或损坏）
                std::cout << Color::RED << "存档文件损坏或被篡改！" << Color::RESET << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                return false;
            }
        } else {
            std::cout << Color::RED << "没有找到存档文件！" << Color::RESET << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            return false;
        }
    }
};

#endif