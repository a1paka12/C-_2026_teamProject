#ifndef MAP_H
#define MAP_H

#include <vector>
#include <string>
#include <ncurses.h>

// ── 속도 존 (게임 셀 data와 별도 zoneData 레이어) ──
// ZoneManager가 런타임에 setZone으로만 채움. 뱀이 지나가도 바닥(0)은 유지됨.
constexpr int ZONE_NONE = 0;
constexpr int ZONE_SLOW = 1;  // 슬로우 존: 이동 느림
constexpr int ZONE_FAST = 2;  // 패스트 존: 이동 빠름

class Map {
public:
    Map(const std::string& filePath);

    // (5단계 ScoreBoard에서 추가) 스테이지 전환 시 같은 인스턴스에 새 맵 파일을 다시 로드
    void resetFromFile(const std::string& filePath);
    
    // boundaryRedBlinkPhase: 경계 빨간 벽(CELL_RED_WALL_CHARGE) 깜빡임 패턴용 0/1
    void draw(int boundaryRedBlinkPhase = 0) const;
    void setCell(int y, int x, int value);
    int getCell(int y, int x) const;

    int getZone(int y, int x) const;
    void setZone(int y, int x, int zoneType);  // ZoneManager 전용

    // main 루프 timeout 조절용: 슬로우 +35ms, 패스트 -35ms
    int getZoneTimeoutDelta(int y, int x) const;
    
    int getHeight() const { return height; }
    int getWidth() const { return width; }

private:
    int height;
    int width;
    std::vector<std::vector<int>> data;      // 맵 타일 (벽·뱀·아이템 등)
    std::vector<std::vector<int>> zoneData;  // 속도 존 오버레이 (0/1/2)
    
    void loadFromFile(const std::string& filePath);
    void validateAndResize();
};

#endif
