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
// 7: Gate
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
    bool checkGateCollision(int headY, int headX, Direction enterDir,
                            int& outY, int& outX, Direction& outDir) const;

    GatePos getGate1() const { return gate1; }
    GatePos getGate2() const { return gate2; }

private:
    Map& map;
    GatePos gate1;
    GatePos gate2;
    bool active;

    std::chrono::steady_clock::time_point lastSpawn;
    static constexpr std::chrono::seconds RESPAWN_INTERVAL{20};

    // pos가 벽의 어느 면에 붙어 있는지 판단해 진출 방향을 결정
    Direction exitDirection(const GatePos& gatePos, Direction enterDir) const;

    // 출구 Gate 바깥쪽 한 칸(빈 공간)의 좌표를 계산
    // 만약 바깥이 벽이면 같은 Gate 위치를 반환(방향 우선순위로 재시도)
    std::pair<int,int> outsideCell(const GatePos& gatePos, Direction dir) const;
};

#endif
