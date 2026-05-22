#include "Gate.hpp"
#include <vector>
#include <cstdlib>   // rand
#include <ctime>     // time
#include <utility>
#include <chrono>

static inline Direction Opposite(Direction d) {
    switch (d) {
        case UP: return DOWN;
        case DOWN: return UP;
        case LEFT: return RIGHT;
        case RIGHT: return LEFT;
        default: return d;
    }
}

// Clockwise: UP->RIGHT->DOWN->LEFT
static inline Direction Clockwise(Direction d) {
    switch (d) {
        case UP: return RIGHT;
        case RIGHT: return DOWN;
        case DOWN: return LEFT;
        case LEFT: return UP;
        default: return d;
    }
}

// Counterclockwise: UP->LEFT->DOWN->RIGHT
static inline Direction CounterClockwise(Direction d) {
    switch (d) {
        case UP: return LEFT;
        case LEFT: return DOWN;
        case DOWN: return RIGHT;
        case RIGHT: return UP;
        default: return d;
    }
}

static inline int DY(Direction d) {
    switch (d) {
        case UP: return -1;
        case DOWN: return 1;
        case LEFT: return 0;
        case RIGHT: return 0;
        default: return 0;
    }
}

static inline int DX(Direction d) {
    switch (d) {
        case UP: return 0;
        case DOWN: return 0;
        case LEFT: return -1;
        case RIGHT: return 1;
        default: return 0;
    }
}

// 맵 패딩(예: 확장 후 값 9)은 통과 불가. Gate 진출 목적지 또한 이동 불가 블록이면 선택하지 않음.
static inline bool isExitDestinationCell(int cell) {
    if (cell == 1 || cell == 2 || cell == CELL_GATE || cell == 9)
        return false;
    return true;
}

// ---------------------------------------------------------------
// 생성자
// ---------------------------------------------------------------
GateManager::GateManager(Map& map)
    : map(map), gate1{-1, -1}, gate2{-1, -1}, active(false), lastSpawn(std::chrono::steady_clock::now()),
      passageActive(false), passagePauseStart(std::chrono::steady_clock::now()), passageMovesLeft(0)
{
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
    // 뱀이 Gate 통과 중이면 Gate 유지, 다른 위치에 새 Gate도 나오지 않음, 20초 타이머도 멈춤
    if (passageActive) {
        return false;
    }

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

int GateManager::respawnCountdown() const {
    if (!active) return 0;
    if (passageActive) return 0; // 통과 중엔 카운트도 멈춤

    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - lastSpawn;
    auto remaining = RESPAWN_INTERVAL - std::chrono::duration_cast<std::chrono::seconds>(elapsed);

    // remaining.count()는 0 이하가 될 수 있음(이 때 update()가 곧 spawn)
    long long sec = remaining.count();
    if (sec <= 0) return 0;
    if (sec > 3) return 0;
    return static_cast<int>(sec);
}

void GateManager::beginGatePassage(int snakeSegmentCount) {
    if (!passageActive) {
        passageActive = true;
        passagePauseStart = std::chrono::steady_clock::now();
        passageMovesLeft = (snakeSegmentCount > 0) ? snakeSegmentCount : 1;
    }
}

void GateManager::afterSnakeMove() {
    if (!passageActive) return;

    passageMovesLeft--;
    if (passageMovesLeft <= 0) {
        auto now = std::chrono::steady_clock::now();
        lastSpawn += now - passagePauseStart;
        passageActive = false;
    }
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
//   ※ 논리 맵 확장(rows/cols 패딩) 때문에 getHeight()-1 과 "실제 테두리 벽" 행이 다를 수 있음.
//   Edge/Middle 분기는 진출 후보 칸 탐색으로 통일함.
//   규칙:
//   1) 진출 가능한 인접 칸이 오직 하나이면 (= 외벽 Gate) 무조건 그 방향(맵 플레이 안쪽 고정과 동등)
//   2) 둘 이상 후보면 Middle Gate 로 간주, Entry 우선순위(직진→시계→반시계→후진)로 탐험 가능한 방향 선택
// ---------------------------------------------------------------
Direction GateManager::exitDirection(const GatePos& gatePos, Direction enterDir) const {
    int h = map.getHeight();
    int w = map.getWidth();

    Direction allDir[4] = {UP, DOWN, LEFT, RIGHT};
    Direction firstOk = enterDir;
    int okCount = 0;

    for (Direction d : allDir) {
        int ny = gatePos.y + DY(d);
        int nx = gatePos.x + DX(d);
        if (ny < 0 || ny >= h || nx < 0 || nx >= w) continue;
        if (!isExitDestinationCell(map.getCell(ny, nx))) continue;
        if (okCount == 0) firstOk = d;
        okCount++;
    }

    // 1) 단일 진입 방향 가능 = 외곽 게이트 안쪽 진출 고정과 동등
    if (okCount == 1)
        return firstOk;

    // 2) Middle (또는 모호한 교차 구간): 우선순위로 선택
    Direction priority[4] = {
        enterDir,
        Clockwise(enterDir),
        CounterClockwise(enterDir),
        Opposite(enterDir)
    };
    for (Direction d : priority) {
        int ny = gatePos.y + DY(d);
        int nx = gatePos.x + DX(d);
        if (ny < 0 || ny >= h || nx < 0 || nx >= w) continue;
        if (isExitDestinationCell(map.getCell(ny, nx)))
            return d;
    }

    // 3) fallback: 우선 가능한 어떤 방향이든(맵 안으로만)
    for (Direction d : allDir) {
        int ny = gatePos.y + DY(d);
        int nx = gatePos.x + DX(d);
        if (ny < 0 || ny >= h || nx < 0 || nx >= w) continue;
        if (isExitDestinationCell(map.getCell(ny, nx)))
            return d;
    }

    return enterDir;
}

// ---------------------------------------------------------------
// outsideCell
//   Gate 위치에서 dir 방향으로 한 칸 나간 좌표를 계산.
//   exitDirection에서 이미 통과 가능한 dir을 선정하므로 여기서는 1칸만 계산.
// ---------------------------------------------------------------
std::pair<int,int> GateManager::outsideCell(const GatePos& gatePos, Direction dir) const {
    int ny = gatePos.y + DY(dir);
    int nx = gatePos.x + DX(dir);
    if (ny < 0 || ny >= map.getHeight() || nx < 0 || nx >= map.getWidth()) {
        return {gatePos.y, gatePos.x};
    }
    int cell = map.getCell(ny, nx);
    if (!isExitDestinationCell(cell)) {
        return {gatePos.y, gatePos.x};
    }
    return {ny, nx};
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

    // 출구 방향 결정 (플레이 영역 안으로 나가는 인접 칸 기준; 확장 패딩 9 제외)
    outDir = exitDirection(exitGate, enterDir);

    // 출구 Gate 안쪽 한 칸 (머리가 놓일 위치)
    auto [ey, ex] = outsideCell(exitGate, outDir);
    outY = ey;
    outX = ex;

    // 비정상(패딩/벽 선택 실패 등) 방지 — 반드시 맵 안쪽 유효 셀로
    int h = map.getHeight();
    int w = map.getWidth();
    if (outY < 0 || outY >= h || outX < 0 || outX >= w ||
        !isExitDestinationCell(map.getCell(outY, outX)) ||
        (outY == exitGate.y && outX == exitGate.x)) {
        Direction allDir[4] = {UP, DOWN, LEFT, RIGHT};
        bool fixed = false;
        for (Direction d : allDir) {
            int ny = exitGate.y + DY(d);
            int nx = exitGate.x + DX(d);
            if (ny < 0 || ny >= h || nx < 0 || nx >= w) continue;
            if (!isExitDestinationCell(map.getCell(ny, nx))) continue;
            outDir = d;
            outY = ny;
            outX = nx;
            fixed = true;
            break;
        }
        if (!fixed)
            return false;
    }

    return true;
}
