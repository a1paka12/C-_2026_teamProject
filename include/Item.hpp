#ifndef ITEM_HPP
#define ITEM_HPP

#include "Map.hpp"
#include <vector>
#include <utility>

struct Item {
    int y;
    int x;
    int type; // 5: Growth, 6: Poison, 11: Invincible(무적), 12: Mystery(미스터리)
    int lifetimeTicks; // 사라질 때까지 남은 틱(Tick) 수
};

class ItemManager {
public:
    ItemManager(Map& map);

    // 매 틱(또는 시간 단위)마다 호출되어 수명을 감소시키고 아이템을 생성/재생성합니다.
    bool update(Map& map);

    // 뱀이 아이템을 먹었을 때 호출됩니다.
    void consumeItem(Map& map, int y, int x);

    // (5단계 ScoreBoard에서 추가) 스테이지/맵 교체 후 아이템 목록을 비우고 맵에 다시 스폰
    void resetForNewMap(Map& map);

private:
    std::vector<Item> items;
    int maxItems = 3;     // 맵에 존재할 수 있는 최대 아이템 수
    int maxLifetime = 50; // 50 틱 ~ 약 5초 (1 틱이 100ms일 경우)

    void spawnItem(Map& map);
};

#endif
    