#ifndef RED_WALL_PROJECTILE_HPP
#define RED_WALL_PROJECTILE_HPP

#include "Map.hpp"
#include <chrono>

class ItemSnake;

// 경계 빨간 벽 충전 표시용 맵 셀 값 (일반 벽 1/2 자리를 잠시 대체)
constexpr int CELL_RED_WALL_CHARGE = 10;

// 가장자리 빨간 벽 경고 → 5초 후 발사체 이동(뱀과 동일: 3메인틱당 1칸) 관리
class RedWallProjectileManager {
public:
    explicit RedWallProjectileManager(Map& map);

    // 스테이지/맵 교체 시 상태 초기화 (새 맵 로드 전후 호출 가능)
    void resetForNewMap(Map& map);

    // 매 메인 루프(약 100ms)마다 호출. snakeStepTick은 tick%3==0일 때 true.
    // 반환: 화면 갱신 필요, 게임 오버(뱀 길이 3 이하 충돌)
    struct UpdateResult {
        bool needsRedraw = false;
        bool gameOver = false;
    };
    UpdateResult update(Map& map, ItemSnake& snake, bool snakeStepTick);

    // Map::draw 경고 타일 깜빡임용 (0/1 토글)
    int mapDrawBlinkPhase() const { return chargeBlinkPhase_; }

    // 맵 그린 뒤 발사체를 위에 덮어씀(음식 셀은 맵에 유지)
    void drawOverlay() const;

    bool isBusy() const { return charging_ || projectileActive_; }

private:
    Map& map_;

    bool charging_ = false;
    int chargeY_ = -1;
    int chargeX_ = -1;
    int savedWallValue_ = 1; // 1 또는 2
    std::chrono::steady_clock::time_point chargeStart_;
    int chargeBlinkPhase_ = 0;

    bool projectileActive_ = false;
    int projY_ = -1;
    int projX_ = -1;
    int projDy_ = 0;
    int projDx_ = 0;

    std::chrono::steady_clock::time_point idleSince_;
    static constexpr std::chrono::seconds kChargeDuration{5};
    static constexpr std::chrono::seconds kSpawnInterval{15};

    void finishChargingAndFire(Map& map, ItemSnake& snake, bool& outGameOver);
    void destroyProjectile();
    void tryBeginCharge(Map& map);
    // 반환: true면 발사체가 아직 존재(이동 후), false면 소멸/종료. outGameOver는 뱀 규칙 위반 시 true.
    bool advanceProjectile(Map& map, ItemSnake& snake, bool& outGameOver);

    static bool isCornerCell(int y, int x, int h, int w);
    static bool isBoundaryCell(int y, int x, int h, int w);
    static void inwardDeltaFromBoundary(int y, int x, int h, int w, int& dy, int& dx);
    static bool isSolidForProjectile(int cell);
};

#endif
