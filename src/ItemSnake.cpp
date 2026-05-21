#include "ItemSnake.hpp"
#include "Gate.hpp"
#include "Snake.hpp"  // SNAKE_MOVE_* 상수 사용을 위해 포함
#include <cstdlib>

bool ItemSnake::applyRedWallHit(Map& map) {
    if (!alive) return false;
    if (invincibleTicks > 0) return true; // 무적 상태면 통과 (데미지 없음)

    if (static_cast<int>(body.size()) <= 3) {
        alive = false;
        return false;
    }

    for (int i = 0; i < 3; ++i) {
        if (body.size() <= 1) break; // 머리는 남겨둠
        const auto tail = body.back();
        map.setCell(tail.first, tail.second, 0);
        body.pop_back();
    }
    return true;
}

bool ItemSnake::updateDirection(int ch) {
    const Direction oldDir = currentDir;
    bool result = Snake::updateDirection(ch);
    if (result && isStopped && currentDir != oldDir) {
        // 무적 상태에서 벽에 막혔을 때, 진행 방향과 다른 방향으로 꺾으면 다시 움직임
        isStopped = false;
    }
    return result;
}

int ItemSnake::move(Map& map, GateManager& gateManager) {
    if (!alive) return -1;
    
    // 다음 이동할 좌표를 미리 계산하여 아이템 여부만 먼저 확인.
    const auto& head = body.front();
    int nextY = head.first;
    int nextX = head.second;
    
    if (currentDir == UP) nextY--;
    else if (currentDir == DOWN) nextY++;
    else if (currentDir == LEFT) nextX--;
    else if (currentDir == RIGHT) nextX++;
    
    const int nextCell = map.getCell(nextY, nextX);

    // 무적 상태일 때 벽(1, 2, 9)을 만나면 멈춤
    if (invincibleTicks > 0 && (nextCell == 1 || nextCell == 2 || nextCell == 9)) {
        isStopped = true;
        return SNAKE_MOVE_OK;
    } else {
        // 무적이 끝났거나 앞에 벽이 없으면 멈춤 상태 해제
        isStopped = false;
    }

    // 멈춤 상태면 이동하지 않음. 무적 상태인 경우
    if (isStopped) return SNAKE_MOVE_OK;

    // 부모의 순간이동 처리 (아이템보다 우선)
    if (nextCell == CELL_GATE) {
        return Snake::move(map, gateManager);
    }
    
    // Growth Item (5)인 경우
    if (nextCell == 5) {
        const std::pair<int, int> oldTail = body.back();
        const int result = Snake::move(map, gateManager);
        if (result == -1) return -1;
        body.push_back(oldTail);
        map.setCell(oldTail.first, oldTail.second, 4);
        return 5;
    }
    // Poison Item (6)인 경우
    else if (nextCell == 6) {
        const int result = Snake::move(map, gateManager);
        if (result == -1) return -1;
        // 무적이면 독 피해(길이 감소)는 무시하되, 미션 Poison 획득으로는 인정 (길이 3 이하면 즉사)
        if (invincibleTicks > 0 && body.size() > 3)
            return 6;
        const std::pair<int, int> extraTail = body.back();
        map.setCell(extraTail.first, extraTail.second, 0);
        body.pop_back();
        if (body.size() < 3) {
            alive = false;
            return -1;
        }
        return 6;
    }
    // Invincible Item (11)인 경우
    else if (nextCell == 11) {
        const int result = Snake::move(map, gateManager);
        if (result == -1) return -1;
        invincibleTicks = 50; // 5초 (50 ticks)
        return 11;
    }
    // Mystery Box (12)인 경우
    else if (nextCell == 12) {
        const int result = Snake::move(map, gateManager);
        if (result == -1) return -1;
        
        const int randEffect = std::rand() % 3;
        if (randEffect == 0) { // 성장 (Growth)
            const std::pair<int, int> oldTail = body.back();
            body.push_back(oldTail);
            map.setCell(oldTail.first, oldTail.second, 4);
            return SNAKE_MOVE_MYSTERY_GROWTH;
        } else if (randEffect == 1) { // 독 (Poison)
            // 무적이면 길이는 유지하고 미션 Poison만 반영 (길이 3 이하면 즉사)
            if (invincibleTicks > 0 && body.size() > 3)
                return SNAKE_MOVE_MYSTERY_POISON;
            const std::pair<int, int> extraTail = body.back();
            map.setCell(extraTail.first, extraTail.second, 0);
            body.pop_back();
            if (body.size() < 3) {
                alive = false;
                return -1;
            }
            return SNAKE_MOVE_MYSTERY_POISON;
        } else { // 무적 (Invincible)
            invincibleTicks = 50;
            return SNAKE_MOVE_MYSTERY_INVINCIBLE;
        }
    }
    
    // 아이템이 없는 빈칸 이동의 경우 부모 로직 그대로 사용
    return Snake::move(map, gateManager);
}
