#ifndef GATE_HPP
#define GATE_HPP

#include "Map.hpp"
#include <vector>
#include <utility>  // std::pair
#include <chrono>

// 맵 셀 값
// 0: 빈 공간
// 1: 벽(게이트 생성 가능)
// 2: 코너 벽(게이트 생성 불가)
// 3: 뱀 머리
// 4: 뱀 몸통
// 7: Gate.
constexpr int CELL_GATE = 7;

// 방향 상수 (뱀 이동 방향과 동일하게 맞출 것)
enum Direction { UP = 0, DOWN, LEFT, RIGHT };

struct GatePos {
    int y, x;
    bool operator==(const GatePos& o) const { return y == o.y && x == o.x; }
};

class GateManager {
public:
    GateManager(Map& map);

    // 현재 맵에서 벽(1)을 수집해 랜덤하게 두 위치를 선택, Gate 쌍을 생성
    // 이전 Gate가 있으면 먼저 제거한 뒤 재생성
    void spawnGates();

    // 매 루프 호출용: 20초마다 Gate 위치를 무작위로 재생성
    // 반환값 true면 실제로 재생성 되었음을 의미
    bool update();

    // 맵에서 Gate를 제거하고 원래 벽(1)으로 복원
    void removeGates();

    // Gate가 활성 상태인지 확인
    bool isActive() const { return active; }

    // 뱀 머리가 Gate 위치에 있는지 확인하고, 있다면 출구 좌표와 진출 방향을 반환
    // 반환값: true = Gate 통과, outY/outX = 나오는 위치, outDir = 나오는 방향
    bool checkGateCollision(const int headY, const int headX, const Direction enterDir,
                            int& outY, int& outX, Direction& outDir) const;

    // Gate 순간이동 직전에 호출 (snake 길이만큼 이동이 끝날 때까지 통과 구간으로 본다)
    void beginGatePassage(const int snakeSegmentCount);

    // 뱀이 실제로 1칸 이동을 마칠 때마다 호출 (통과 구간 종료 시 20초 타이머 보정)
    void afterSnakeMove();

    bool isPassageActive() const { return passageActive; }

    // Gate 재생성(20초) 직전 카운트다운 표시용.
    // 반환값: 3/2/1 (남은 초), 그 외(표시 안 함)는 0.
    int respawnCountdown() const;

    GatePos getGate1() const { return gate1; }
    GatePos getGate2() const { return gate2; }

private:
    Map& map;
    GatePos gate1;
    GatePos gate2;
    bool active;

    std::chrono::steady_clock::time_point lastSpawn;
    static constexpr std::chrono::seconds RESPAWN_INTERVAL{20};

    // Gate 통과 중: 재생성 금지, 경과 시간은 타이머에서 제외
    bool passageActive;
    std::chrono::steady_clock::time_point passagePauseStart;
    int passageMovesLeft;

    // pos가 벽의 어느 면에 붙어 있는지 판단해 진출 방향을 결정
    Direction exitDirection(const GatePos& gatePos, const Direction enterDir) const;

    // 출구 Gate 바깥쪽 한 칸(빈 공간)의 좌표를 계산
    // 만약 바깥이 벽이면 같은 Gate 위치를 반환(방향 우선순위로 재시도)
    std::pair<int,int> outsideCell(const GatePos& gatePos, const Direction dir) const;
};

#endif
