// 빨간 벽 발사체의 생성, 이동 및 뱀과의 충돌 감지 로직 구현부
#include "RedWallProjectile.hpp"
#include "ItemSnake.hpp"
#include "Gate.hpp"
#include <ncurses.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <chrono>

// =============================================================================
// 생성 / 초기화
// =============================================================================

RedWallProjectileManager::RedWallProjectileManager(Map& map)
    : map_(map), idleSince_(std::chrono::steady_clock::now()) {
    // 게임 시작 직후에는 바로 경고가 뜨지 않도록 idleSince_를 현재 시각으로 둠
}

void RedWallProjectileManager::resetForNewMap(const Map& map) {
    (void)map;  // 실제 맵 참조는 생성자에서 받은 map_ 사용

    // 스테이지 바뀔 때 충전 중이던 칸을 원래 벽으로 되돌림 (10이 남으면 안 됨)
    if (charging_) {
        if (chargeY_ >= 0 && chargeX_ >= 0) {
            map_.setCell(chargeY_, chargeX_, savedWallValue_);
        }
        charging_ = false;
        chargeY_ = chargeX_ = -1;
    }

    destroyProjectile();
    idleSince_ = std::chrono::steady_clock::now();  // 새 스테이지에서 15초 카운트 재시작
    chargeBlinkPhase_ = 0;
}

void RedWallProjectileManager::destroyProjectile() {
    // 발사체는 맵 배열에 별도 값을 쓰지 않고 overlay로만 그리므로 좌표만 리셋
    projectileActive_ = false;
    projY_ = projX_ = -1;
    projDy_ = projDx_ = 0;
}

// =============================================================================
// 좌표·셀 판별 (static 헬퍼)
// =============================================================================

bool RedWallProjectileManager::isCornerCell(const int y, const int x, const int h, const int w) {
    // 코너 벽(2)은 후보에서 빼지만, 이 함수는 "좌표가 모서리인지"만 판별
    const bool top = (y == 0);
    const bool bottom = (y == h - 1);
    const bool left = (x == 0);
    const bool right = (x == w - 1);
    return (top || bottom) && (left || right);
}

bool RedWallProjectileManager::isBoundaryCell(const int y, const int x, const int h, const int w) {
    return y == 0 || y == h - 1 || x == 0 || x == w - 1;
}

void RedWallProjectileManager::inwardDeltaFromBoundary(const int y, const int x, const int h, const int w,
                                                       int& dy, int& dx) {
    (void)h;
    // 발사 방향 = 항상 플레이 영역 안쪽. 좌우 변이면 x 방향, 그 외는 y 방향.
    dy = 0;
    dx = 0;
    if (x == 0) {
        dx = 1;           // 왼쪽 벽 → 오른쪽으로
    } else if (x == w - 1) {
        dx = -1;          // 오른쪽 벽 → 왼쪽으로
    } else if (y == 0) {
        dy = 1;           // 위쪽 벽 → 아래로
    } else {
        dy = -1;          // 아래쪽 벽 → 위로
    }
}

bool RedWallProjectileManager::isSolidForProjectile(const int cell) {
    if (cell < 0) return true;
    // 3=뱀 머리, 4=뱀 몸은 통과 가능(별도 충돌 처리). 여기는 "벽류"만 막음.
    return cell == 1 || cell == 2 || cell == CELL_GATE || cell == 9;
}

// =============================================================================
// 충전 완료 → 발사
// =============================================================================

void RedWallProjectileManager::finishChargingAndFire(Map& map, ItemSnake& snake, bool& outGameOver,
                                                   bool& outSnakeHit) {
    outGameOver = false;
    outSnakeHit = false;
    if (!charging_) return;

    const int h = map.getHeight();
    const int w = map.getWidth();

    const int wallY = chargeY_;
    const int wallX = chargeX_;

    // 경고 타일(10) 제거 → 원래 벽(1 또는 2)으로 복구
    map.setCell(wallY, wallX, savedWallValue_);

    // 벽 바로 안쪽 한 칸이 발사체의 첫 위치·이동 방향
    int dy = 0;
    int dx = 0;
    inwardDeltaFromBoundary(wallY, wallX, h, w, dy, dx);

    charging_ = false;
    chargeY_ = chargeX_ = -1;

    const int startY = wallY + dy;
    const int startX = wallX + dx;

    // 스폰 위치가 맵 밖이면 발사체 없이 종료
    if (startY < 0 || startY >= h || startX < 0 || startX >= w) {
        destroyProjectile();
        idleSince_ = std::chrono::steady_clock::now();
        return;
    }

    // 첫 칸이 이미 다른 벽/게이트/패딩이면 발사 불가
    const int startCell = map.getCell(startY, startX);
    if (isSolidForProjectile(startCell)) {
        destroyProjectile();
        idleSince_ = std::chrono::steady_clock::now();
        return;
    }

    projDy_ = dy;
    projDx_ = dx;
    projY_ = startY;
    projX_ = startX;
    projectileActive_ = true;

    // 스폰 직후 그 칸에 뱀이 있으면 즉시 피격 (이동 전에 한 번 검사)
    const int here = map.getCell(projY_, projX_);
    if (here == 3 || here == 4) {
        snake.applyRedWallHit(map);  // 꼬리 3칸 제거 등 (ItemSnake 구현)
        if (!snake.isAlive()) {
            outGameOver = true;      // 길이가 3 이하로 떨어지면 게임 오버
        } else {
            outSnakeHit = true;      // 살아 있으면 main이 하단 알림 표시
        }
        destroyProjectile();
        idleSince_ = std::chrono::steady_clock::now();
        return;
    }
}

// =============================================================================
// 새 경고(충전) 시작
// =============================================================================

void RedWallProjectileManager::tryBeginCharge(const Map& map) {
    (void)map;

    // 충전·발사 중에는 새 이벤트를 시작하지 않음
    if (charging_ || projectileActive_) return;

    const auto now = std::chrono::steady_clock::now();
    if (now - idleSince_ < kSpawnInterval) return;  // 15초 대기

    const int h = map_.getHeight();
    const int w = map_.getWidth();

    // 후보: 테두리 + 일반/코너 벽(1,2) + 코너 좌표 제외
    std::vector<std::pair<int, int>> candidates;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (!isBoundaryCell(y, x, h, w)) continue;
            if (isCornerCell(y, x, h, w)) continue;
            const int c = map_.getCell(y, x);
            if (c == 1 || c == 2) {
                candidates.push_back({y, x});
            }
        }
    }

    if (candidates.empty()) return;

    const int idx = std::rand() % static_cast<int>(candidates.size());
    const int y = candidates[idx].first;
    const int x = candidates[idx].second;

    savedWallValue_ = map_.getCell(y, x);
    map_.setCell(y, x, CELL_RED_WALL_CHARGE);  // Map::draw에서 빨간 깜빡임으로 표시

    charging_ = true;
    chargeY_ = y;
    chargeX_ = x;
    chargeStart_ = now;
    chargeBlinkPhase_ = 0;
}

// =============================================================================
// 발사체 1칸 전진 (뱀과 동일: 3 메인 틱당 1회)
// =============================================================================

bool RedWallProjectileManager::advanceProjectile(Map& map, ItemSnake& snake, bool& outGameOver,
                                                 bool& outSnakeHit) {
    outGameOver = false;
    outSnakeHit = false;
    if (!projectileActive_) return false;

    const int h = map.getHeight();
    const int w = map.getWidth();

    const int ny = projY_ + projDy_;
    const int nx = projX_ + projDx_;

    // 맵 밖으로 나가면 소멸
    if (ny < 0 || ny >= h || nx < 0 || nx >= w) {
        destroyProjectile();
        idleSince_ = std::chrono::steady_clock::now();
        return false;
    }

    const int cell = map.getCell(ny, nx);

    // 다음 칸이 벽·게이트·패딩이면 그 앞에서 소멸 (맵 셀은 바꾸지 않음)
    if (isSolidForProjectile(cell)) {
        destroyProjectile();
        idleSince_ = std::chrono::steady_clock::now();
        return false;
    }

    // 뱀 머리(3) 또는 몸(4)과 충돌
    if (cell == 3 || cell == 4) {
        snake.applyRedWallHit(map);
        if (!snake.isAlive()) {
            outGameOver = true;
        } else {
            outSnakeHit = true;
        }
        destroyProjectile();
        idleSince_ = std::chrono::steady_clock::now();
        return false;
    }

    // 빈 칸·음식·아이템 등은 통과하며 좌표만 갱신
    projY_ = ny;
    projX_ = nx;
    return true;
}

// =============================================================================
// 메인 루프 갱신 (상태 머신)
// =============================================================================

RedWallProjectileManager::UpdateResult RedWallProjectileManager::update(Map& map, ItemSnake& snake,
                                                                        const bool snakeStepTick) {
    (void)map;  // finishChargingAndFire / advanceProjectile에 동일 참조 전달
    UpdateResult out;

    const auto now = std::chrono::steady_clock::now();

    // --- 1) 충전(경고) 단계: 5초 동안 깜빡임, 만료 시 발사 ---
    if (charging_) {
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - chargeStart_).count();
        chargeBlinkPhase_ = static_cast<int>((ms / 180) % 2);  // 180ms 주기 0/1
        out.needsRedraw = true;

        if (now - chargeStart_ >= kChargeDuration) {
            bool spawnGameOver = false;
            bool spawnHit = false;
            finishChargingAndFire(map, snake, spawnGameOver, spawnHit);
            if (spawnGameOver) {
                out.gameOver = true;
                return out;
            }
            if (spawnHit) {
                out.snakeHitRedWall = true;
            }
            out.needsRedraw = true;
        }
    }

    // --- 2) 발사체 단계: 뱀과 같은 틱에만 1칸 이동 ---
    if (projectileActive_) {
        out.needsRedraw = true;
        if (snakeStepTick) {
            bool gameOverFromHit = false;
            bool hit = false;
            advanceProjectile(map, snake, gameOverFromHit, hit);
            if (gameOverFromHit) {
                out.gameOver = true;
                return out;
            }
            if (hit) {
                out.snakeHitRedWall = true;
            }
        }
    }
    // --- 3) 아무 일도 없을 때: 15초 지나면 새 경고 위치 선정 ---
    else if (!charging_) {
        tryBeginCharge(map);
        if (charging_) {
            out.needsRedraw = true;
        }
    }

    return out;
}

// =============================================================================
// 화면 그리기 (맵 데이터와 분리)
// =============================================================================

void RedWallProjectileManager::drawOverlay() const {
    if (!projectileActive_) return;
    if (projY_ < 0 || projX_ < 0) return;

    // 맵은 셀당 화면 2칸 너비 → x를 2배한 위치에 빨간 블록 2칸
    const int drawX = projX_ * 2;
    attron(COLOR_PAIR(8) | A_BOLD);
    mvaddch(projY_, drawX, ' ');
    mvaddch(projY_, drawX + 1, ' ');
    attroff(COLOR_PAIR(8) | A_BOLD);
}
