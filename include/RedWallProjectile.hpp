#ifndef RED_WALL_PROJECTILE_HPP
#define RED_WALL_PROJECTILE_HPP

#include "Map.hpp"
#include <chrono>

class ItemSnake;

// ---------------------------------------------------------------------------
// 맵 셀 값 (RedWall 전용)
// ---------------------------------------------------------------------------
// 경계 벽(1=일반벽, 2=코너벽) 위치에 5초간 표시되는 "충전/경고" 타일.
// Map::draw()에서 이 값을 읽어 빨간색으로 깜빡이게 그린다.
constexpr int CELL_RED_WALL_CHARGE = 10;

// ---------------------------------------------------------------------------
// RedWallProjectileManager
// ---------------------------------------------------------------------------
// 맵 가장자리(코너 제외)의 벽 한 칸을 골라 경고(충전) → 5초 후 안쪽으로 발사체가
// 직진한다. 발사체는 뱀과 같은 속도(메인 루프 3틱당 1칸)로만 이동한다.
//
// 상태 흐름:
//   [대기 idle] --(15초 경과)--> [충전 charging, 5초] --(시간 만료)--> [발사체 이동]
//        ^                              |                                    |
//        |                              +-- 맵 셀을 CELL_RED_WALL_CHARGE로 변경
//        +-- 발사체 소멸/충돌 후 idleSince_ 갱신, 다시 15초 후 다음 경고
//
// main.cpp 연동:
//   - 매 프레임 update() 호출 (약 100ms 간격)
//   - snakeStepTick == (tick % 3 == 0) 일 때만 발사체 1칸 전진
//   - map.draw() 후 drawOverlay()로 발사체(빨간 블록)를 맵 위에 덮어 그림
//   - mapDrawBlinkPhase()로 충전 타일 깜빡임 위상 전달
// ---------------------------------------------------------------------------
class RedWallProjectileManager {
public:
    explicit RedWallProjectileManager(Map& map);

    // 스테이지 전환·맵 재로드 시 호출. 충전 중이던 벽은 원래 값(1/2)으로 복구하고
    // 발사체·타이머를 모두 초기화한다.
    void resetForNewMap(const Map& map);

    // update() 반환값 — main이 화면 갱신·게임오버·하단 알림에 사용
    struct UpdateResult {
        bool needsRedraw = false;      // 이번 틱에 화면을 다시 그려야 함
        bool gameOver = false;         // 빨간 벽에 맞아 뱀 길이가 3 이하가 되어 종료
        bool snakeHitRedWall = false;  // 맞았지만 생존(꼬리 -3 등) → 하단 상태 메시지용
    };

    // 게임 메인 루프에서 호출하는 핵심 갱신 함수.
    //   map, snake : 충돌·발사 시 맵/뱀 상태 변경에 사용
    //   snakeStepTick : true일 때만 발사체를 한 칸 이동 (뱀 이동 주기와 동기)
    UpdateResult update(Map& map, ItemSnake& snake, const bool snakeStepTick);

    // 충전(경고) 단계에서 Map::draw가 사용. 180ms마다 0↔1 토글 → 깜빡임.
    int mapDrawBlinkPhase() const { return chargeBlinkPhase_; }

    // 발사체가 활성일 때, 맵 배경을 그린 뒤 ncurses로 빨간 2칸 너비 블록을 덮어씀.
    // (맵 데이터의 음식·아이템 셀은 건드리지 않음)
    void drawOverlay() const;

    // 충전 중이거나 발사체가 날아가는 중이면 true (다른 시스템과 겹침 방지용)
    bool isBusy() const { return charging_ || projectileActive_; }

private:
    Map& map_;

    // ----- 충전(경고) 단계 -----
    bool charging_ = false;           // true면 가장자리 한 칸이 경고 타일(10)로 표시 중
    int chargeY_ = -1;
    int chargeX_ = -1;
    int savedWallValue_ = 1;         // 충전 전 벽 종류(1 또는 2), 해제 시 복원
    std::chrono::steady_clock::time_point chargeStart_;
    int chargeBlinkPhase_ = 0;       // Map::draw 깜빡임용 0/1

    // ----- 발사체 단계 -----
    bool projectileActive_ = false;
    int projY_ = -1;                 // 발사체 현재 위치 (맵 좌표)
    int projX_ = -1;
    int projDy_ = 0;                 // 매 이동 시 더해지는 방향 (벽 안쪽 = 맵 중심 쪽)
    int projDx_ = 0;

    // ----- 타이머 -----
    std::chrono::steady_clock::time_point idleSince_;  // 마지막 이벤트 종료 시각
    static constexpr std::chrono::seconds kChargeDuration{5};   // 경고 표시 시간
    static constexpr std::chrono::seconds kSpawnInterval{15};   // 다음 경고까지 대기

    // 충전 5초 완료 시: 벽 셀 복구 → 안쪽 첫 칸에 발사체 스폰 → 즉시 그 칸 검사
    void finishChargingAndFire(Map& map, ItemSnake& snake, bool& outGameOver, bool& outSnakeHit);

    // 발사체 비활성화 (맵 셀은 변경하지 않음 — overlay로만 그림)
    void destroyProjectile();

    // idle이고 15초 지났으면 가장자리 벽 후보 중 랜덤 한 칸을 충전 시작
    void tryBeginCharge(const Map& map);

    // snakeStepTick일 때 1칸 전진. 벽/맵 밖/뱀 충돌 시 소멸.
    // 반환: true = 아직 살아 있음, false = 이번에 소멸됨
    bool advanceProjectile(Map& map, ItemSnake& snake, bool& outGameOver, bool& outSnakeHit);

    // (y,x)가 맵 네 모서리 코너인지 — 코너는 발사 위치 후보에서 제외
    static bool isCornerCell(const int y, const int x, const int h, const int w);

    // (y,x)가 맵 테두리(첫/끝 행·열)인지
    static bool isBoundaryCell(const int y, const int x, const int h, const int w);

    // 테두리 벽 칸에서 맵 안쪽으로 한 칸 가는 (dy,dx) 계산 (좌/우/상/하 우선)
    static void inwardDeltaFromBoundary(const int y, const int x, const int h, const int w, int& dy, int& dx);

    // 발사체가 통과 불가로 보는 셀: 벽(1,2), Gate(7), 패딩(9), 음수(맵 밖 표현)
    static bool isSolidForProjectile(const int cell);
};

#endif
