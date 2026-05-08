#include "Gate.hpp"
#include <vector>
#include <cstdlib>   // rand
#include <ctime>     // time
#include <utility>
#include <chrono>

// ---------------------------------------------------------------
// 생성자
// ---------------------------------------------------------------
GateManager::GateManager(Map& map)
    : map(map), gate1{-1, -1}, gate2{-1, -1}, active(false), lastSpawn(std::chrono::steady_clock::now())
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
}

// ---------------------------------------------------------------
// spawnGates
//   1) 기존 Gate 제거
//   2) 맵에서 값이 1인 셀(일반 벽) 목록 수집
//   3) 충분한 벽이 없으면 아무것도 하지 않음
//   4) 서로 다른 두 위치를 무작위 선택 → 맵 값을 7로 변경
// ---------------------------------------------------------------
void GateManager::spawnGates() {
    if (active) removeGates();

    // 값 1인 벽 위치 수집 (코너 벽 2는 제외)
    std::vector<GatePos> wallCells;
    for (int y = 0; y < map.getHeight(); ++y) {
        for (int x = 0; x < map.getWidth(); ++x) {
            if (map.getCell(y, x) == 1) {
                wallCells.push_back({y, x});
            }
        }
    }

    if (wallCells.size() < 2) return;  // 벽이 2개 미만이면 생성 불가

    // 첫 번째 Gate 선택
    int idx1 = std::rand() % static_cast<int>(wallCells.size());
    gate1 = wallCells[idx1];

    // 두 번째 Gate: 첫 번째와 다른 위치가 나올 때까지 재시도
    int idx2;
    do {
        idx2 = std::rand() % static_cast<int>(wallCells.size());
    } while (idx2 == idx1);
    gate2 = wallCells[idx2];

    // 맵 데이터에 Gate 값(7) 반영
    map.setCell(gate1.y, gate1.x, CELL_GATE);
    map.setCell(gate2.y, gate2.x, CELL_GATE);

    active = true;
    lastSpawn = std::chrono::steady_clock::now();
}

// ---------------------------------------------------------------
// update
//   Gate가 없거나(초기) 20초 경과 시 Gate를 다시 생성
// ---------------------------------------------------------------
bool GateManager::update() {
    if (!active) {
        spawnGates();
        return active; // 생성 성공이면 true
    }

    auto now = std::chrono::steady_clock::now();
    if (now - lastSpawn >= RESPAWN_INTERVAL) {
        spawnGates();
        return active; // 생성 성공이면 true
    }
    return false;
}

// ---------------------------------------------------------------
// removeGates
//   Gate 셀을 다시 일반 벽(1)으로 복원
// ---------------------------------------------------------------
void GateManager::removeGates() {
    if (!active) return;

    map.setCell(gate1.y, gate1.x, 1);
    map.setCell(gate2.y, gate2.x, 1);

    gate1 = {-1, -1};
    gate2 = {-1, -1};
    active = false;
}

// ---------------------------------------------------------------
// exitDirection
//   Gate가 맵 경계 어느 쪽에 붙어 있는지 판단해 '바깥 방향'을 반환.
//   벽은 보통 맵 테두리이므로 아래 우선순위로 판별:
//     top row    → 진출 방향: UP
//     bottom row → DOWN
//     left col   → LEFT
//     right col  → RIGHT
//   판별 불가(내부 벽) 시 진입 방향 그대로 유지
// ---------------------------------------------------------------
Direction GateManager::exitDirection(const GatePos& gatePos, Direction enterDir) const {
    int h = map.getHeight();
    int w = map.getWidth();

    if (gatePos.y == 0)     return UP;
    if (gatePos.y == h - 1) return DOWN;
    if (gatePos.x == 0)     return LEFT;
    if (gatePos.x == w - 1) return RIGHT;

    // 내부 벽인 경우 진입 방향을 그대로 사용
    return enterDir;
}

// ---------------------------------------------------------------
// outsideCell
//   Gate 위치에서 dir 방향으로 한 칸 나간 좌표를 계산.
//   그 칸이 맵 밖이거나 벽(1, 2, 7)이면 다음 우선순위 방향으로 재시도.
//   우선순위: enterDir → RIGHT → DOWN → LEFT → UP
// ---------------------------------------------------------------
std::pair<int,int> GateManager::outsideCell(const GatePos& gatePos, Direction dir) const {
    const int dy[] = {-1, 1, 0, 0};  // UP, DOWN, LEFT, RIGHT
    const int dx[] = { 0, 0,-1, 1};

    // 시도 순서: 입력 방향 우선, 이후 나머지 방향
    Direction order[4] = {dir,
        static_cast<Direction>((dir + 1) % 4),
        static_cast<Direction>((dir + 2) % 4),
        static_cast<Direction>((dir + 3) % 4)
    };

    for (Direction d : order) {
        int ny = gatePos.y + dy[d];
        int nx = gatePos.x + dx[d];
        if (ny < 0 || ny >= map.getHeight() || nx < 0 || nx >= map.getWidth()) continue;
        int cell = map.getCell(ny, nx);
        if (cell != 1 && cell != 2 && cell != CELL_GATE) {
            return {ny, nx};
        }
    }

    // 모든 방향이 막혀 있는 경우 Gate 위치 자체를 반환 (예외 처리용)
    return {gatePos.y, gatePos.x};
}

// ---------------------------------------------------------------
// checkGateCollision
//   뱀 머리(headY, headX)가 Gate 위치와 일치하면:
//     - 반대쪽 Gate 출구 좌표와 진출 방향을 계산해 outY/outX/outDir에 저장
//     - true 반환
//   일치하지 않으면 false 반환
// ---------------------------------------------------------------
bool GateManager::checkGateCollision(int headY, int headX, Direction enterDir,
                                     int& outY, int& outX, Direction& outDir) const {
    if (!active) return false;

    bool hitGate1 = (headY == gate1.y && headX == gate1.x);
    bool hitGate2 = (headY == gate2.y && headX == gate2.x);

    if (!hitGate1 && !hitGate2) return false;

    // 출구 Gate 결정
    const GatePos& exitGate = hitGate1 ? gate2 : gate1;

    // 출구 방향 결정: 출구 Gate의 '바깥 면' 방향
    outDir = exitDirection(exitGate, enterDir);

    // 출구 Gate 바깥 한 칸 (뱀이 실제로 나오는 위치)
    auto [ey, ex] = outsideCell(exitGate, outDir);
    outY = ey;
    outX = ex;

    return true;
}
