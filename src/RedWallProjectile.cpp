#include "RedWallProjectile.hpp"
#include "ItemSnake.hpp"
#include "Gate.hpp"
#include <ncurses.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <chrono>

RedWallProjectileManager::RedWallProjectileManager(Map& map)
    : map_(map), idleSince_(std::chrono::steady_clock::now()) {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
}

void RedWallProjectileManager::resetForNewMap(Map& map) {
    (void)map;
    if (charging_) {
        if (chargeY_ >= 0 && chargeX_ >= 0) {
            map_.setCell(chargeY_, chargeX_, savedWallValue_);
        }
        charging_ = false;
        chargeY_ = chargeX_ = -1;
    }
    destroyProjectile();
    idleSince_ = std::chrono::steady_clock::now();
    chargeBlinkPhase_ = 0;
}

void RedWallProjectileManager::destroyProjectile() {
    projectileActive_ = false;
    projY_ = projX_ = -1;
    projDy_ = projDx_ = 0;
}

bool RedWallProjectileManager::isCornerCell(int y, int x, int h, int w) {
    const bool top = (y == 0);
    const bool bottom = (y == h - 1);
    const bool left = (x == 0);
    const bool right = (x == w - 1);
    return (top || bottom) && (left || right);
}

bool RedWallProjectileManager::isBoundaryCell(int y, int x, int h, int w) {
    return y == 0 || y == h - 1 || x == 0 || x == w - 1;
}

void RedWallProjectileManager::inwardDeltaFromBoundary(int y, int x, int h, int w, int& dy, int& dx) {
    (void)h;
    dy = 0;
    dx = 0;
    if (x == 0) {
        dx = 1;
    } else if (x == w - 1) {
        dx = -1;
    } else if (y == 0) {
        dy = 1;
    } else {
        dy = -1;
    }
}

bool RedWallProjectileManager::isSolidForProjectile(int cell) {
    if (cell < 0) return true;
    return cell == 1 || cell == 2 || cell == CELL_GATE || cell == 9;
}

void RedWallProjectileManager::finishChargingAndFire(Map& map, ItemSnake& snake, bool& outGameOver,
                                                   bool& outSnakeHit) {
    outGameOver = false;
    outSnakeHit = false;
    if (!charging_) return;

    const int h = map.getHeight();
    const int w = map.getWidth();

    const int wallY = chargeY_;
    const int wallX = chargeX_;

    map.setCell(wallY, wallX, savedWallValue_);

    int dy = 0;
    int dx = 0;
    inwardDeltaFromBoundary(wallY, wallX, h, w, dy, dx);

    charging_ = false;
    chargeY_ = chargeX_ = -1;

    const int startY = wallY + dy;
    const int startX = wallX + dx;

    if (startY < 0 || startY >= h || startX < 0 || startX >= w) {
        destroyProjectile();
        idleSince_ = std::chrono::steady_clock::now();
        return;
    }

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

    const int here = map.getCell(projY_, projX_);
    if (here == 3 || here == 4) {
        snake.applyRedWallHit(map);
        if (!snake.isAlive()) {
            outGameOver = true;
        } else {
            outSnakeHit = true;
        }
        destroyProjectile();
        idleSince_ = std::chrono::steady_clock::now();
        return;
    }
}

void RedWallProjectileManager::tryBeginCharge(Map& map) {
    (void)map;
    if (charging_ || projectileActive_) return;

    const auto now = std::chrono::steady_clock::now();
    if (now - idleSince_ < kSpawnInterval) return;

    const int h = map_.getHeight();
    const int w = map_.getWidth();

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
    map_.setCell(y, x, CELL_RED_WALL_CHARGE);

    charging_ = true;
    chargeY_ = y;
    chargeX_ = x;
    chargeStart_ = now;
    chargeBlinkPhase_ = 0;
}

bool RedWallProjectileManager::advanceProjectile(Map& map, ItemSnake& snake, bool& outGameOver,
                                                 bool& outSnakeHit) {
    outGameOver = false;
    outSnakeHit = false;
    if (!projectileActive_) return false;

    const int h = map.getHeight();
    const int w = map.getWidth();

    const int ny = projY_ + projDy_;
    const int nx = projX_ + projDx_;

    if (ny < 0 || ny >= h || nx < 0 || nx >= w) {
        destroyProjectile();
        idleSince_ = std::chrono::steady_clock::now();
        return false;
    }

    const int cell = map.getCell(ny, nx);

    if (isSolidForProjectile(cell)) {
        destroyProjectile();
        idleSince_ = std::chrono::steady_clock::now();
        return false;
    }

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

    projY_ = ny;
    projX_ = nx;
    return true;
}

RedWallProjectileManager::UpdateResult RedWallProjectileManager::update(Map& map, ItemSnake& snake,
                                                                        bool snakeStepTick) {
    (void)map;
    UpdateResult out;

    const auto now = std::chrono::steady_clock::now();

    if (charging_) {
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - chargeStart_).count();
        chargeBlinkPhase_ = static_cast<int>((ms / 180) % 2);
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
    } else if (!charging_) {
        tryBeginCharge(map);
        if (charging_) {
            out.needsRedraw = true;
        }
    }

    return out;
}

void RedWallProjectileManager::drawOverlay() const {
    if (!projectileActive_) return;
    if (projY_ < 0 || projX_ < 0) return;

    const int drawX = projX_ * 2;
    attron(COLOR_PAIR(8) | A_BOLD);
    mvaddch(projY_, drawX, ' ');
    mvaddch(projY_, drawX + 1, ' ');
    attroff(COLOR_PAIR(8) | A_BOLD);
}
