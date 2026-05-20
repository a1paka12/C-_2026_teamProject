#include "Zone.hpp"
#include <algorithm>
#include <cstdlib>
#include <ctime>

// 게임 시작·스테이지 전환 시 맵에 초기 존 배치
ZoneManager::ZoneManager(Map& map) {
    std::srand(static_cast<unsigned>(std::time(nullptr)) + 12345u);
    for (int i = 0; i < maxZones; ++i) {
        spawnZone(map);
    }
}

// 빈 칸(0)이면서 다른 존과 겹치지 않는 NxN 영역인지 검사
bool ZoneManager::canPlacePatch(Map& map, int y, int x, int size) const {
    if (y < 0 || x < 0 || y + size > map.getHeight() || x + size > map.getWidth())
        return false;
    for (int dy = 0; dy < size; ++dy) {
        for (int dx = 0; dx < size; ++dx) {
            const int cy = y + dy;
            const int cx = x + dx;
            if (map.getCell(cy, cx) != 0)
                return false;
            if (map.getZone(cy, cx) != ZONE_NONE)
                return false;
        }
    }
    return true;
}

// Map의 zoneData 레이어에 슬로우/패스트 표시 (바닥 셀 값 0은 유지)
void ZoneManager::applyPatch(Map& map, const ZonePatch& patch) {
    for (int dy = 0; dy < patch.size; ++dy) {
        for (int dx = 0; dx < patch.size; ++dx) {
            map.setZone(patch.y + dy, patch.x + dx, patch.type);
        }
    }
}

void ZoneManager::removePatch(Map& map, const ZonePatch& patch) {
    for (int dy = 0; dy < patch.size; ++dy) {
        for (int dx = 0; dx < patch.size; ++dx) {
            map.setZone(patch.y + dy, patch.x + dx, ZONE_NONE);
        }
    }
}

bool ZoneManager::update(Map& map) {
    bool changed = false;

    // 수명이 0이 된 패치는 맵에서 지우고 목록에서 제거
    for (auto it = zones.begin(); it != zones.end(); ) {
        it->lifetimeTicks--;
        if (it->lifetimeTicks <= 0) {
            removePatch(map, *it);
            it = zones.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }

    // 항상 maxZones개를 유지 (아이템 매니저와 같은 방식)
    while (static_cast<int>(zones.size()) < maxZones) {
        spawnZone(map);
        changed = true;
    }

    return changed;
}

// 빈 칸에 2x2 패치를 랜덤 위치에 생성 (슬로우/패스트 50%)
void ZoneManager::spawnZone(Map& map) {
    const int h = map.getHeight();
    const int w = map.getWidth();

    for (int attempt = 0; attempt < 120; ++attempt) {
        const int y = std::rand() % std::max(1, h - patchSize + 1);
        const int x = std::rand() % std::max(1, w - patchSize + 1);
        if (!canPlacePatch(map, y, x, patchSize))
            continue;

        ZonePatch patch;
        patch.y = y;
        patch.x = x;
        patch.size = patchSize;
        patch.type = (std::rand() % 2 == 0) ? ZONE_SLOW : ZONE_FAST;
        patch.lifetimeTicks = maxLifetime;

        applyPatch(map, patch);
        zones.push_back(patch);
        return;
    }
}

void ZoneManager::resetForNewMap(Map& map) {
    for (const ZonePatch& z : zones) {
        removePatch(map, z);
    }
    zones.clear();
    for (int i = 0; i < maxZones; ++i) {
        spawnZone(map);
    }
}
