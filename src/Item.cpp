#include "Item.hpp"
#include <cstdlib>
#include <ctime>

ItemManager::ItemManager(Map& map) {
    std::srand(std::time(nullptr));
    // 초기 아이템들을 생성합니다.
    for (int i = 0; i < maxItems; ++i) {
        spawnItem(map);
    }
}

bool ItemManager::update(Map& map) {
    bool mapChanged = false;
    
    // 수명이 다한 아이템을 확인합니다.
    for (auto it = items.begin(); it != items.end(); ) {
        it->lifetimeTicks--;
        if (it->lifetimeTicks <= 0) {
            // 뱀이 아직 먹지 않아서 맵에 남아있는 경우 맵에서 제거(0으로 변경)
            if (map.getCell(it->y, it->x) == it->type) {
                map.setCell(it->y, it->x, 0);
            }
            it = items.erase(it);
            mapChanged = true;
        } else {
            ++it;
        }
    }

    // 최대 아이템 수(maxItems)를 유지하도록 새로운 아이템을 생성.
    while ((int)items.size() < maxItems) {
        spawnItem(map);
        mapChanged = true;
    }

    return mapChanged;
}

void ItemManager::spawnItem(Map& map) {
    int y, x;
    int attempts = 0;
    while (attempts < 100) {
        y = std::rand() % map.getHeight();
        x = std::rand() % map.getWidth();
        
        // 아이템은 빈 공간(0)에만 생성될 수 있음.
        if (map.getCell(y, x) == 0) {
            int randVal = std::rand() % 4;
            int type;
            if (randVal == 0) type = 5;       // Growth (성장)
            else if (randVal == 1) type = 6;  // Poison (독)
            else if (randVal == 2) type = 11; // Invincible (무적)
            else type = 12;                  // Mystery (미스터리)
            
            Item item = {y, x, type, maxLifetime};
            items.push_back(item);
            map.setCell(y, x, type);
            break;
        }
        attempts++;
    }
}

void ItemManager::consumeItem(Map& map, int y, int x) {
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (it->y == y && it->x == x) {
            items.erase(it);
            break;
        }
    }
}

// (5단계 ScoreBoard에서 추가) 스테이지 전환 시 기존 아이템을 모두 제거하고 새 맵에 다시 생성한다.
void ItemManager::resetForNewMap(Map& map) {
    items.clear();
    for (int i = 0; i < maxItems; ++i) {
        spawnItem(map);
    }
}
