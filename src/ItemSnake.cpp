#include "ItemSnake.hpp"
#include "Gate.hpp"

int ItemSnake::move(Map& map, GateManager& gateManager) {
    if (!alive) return -1;
    
    // 다음 이동할 좌표를 미리 계산하여 아이템 여부만 먼저 확인.
    int nextY = body.front().first;
    int nextX = body.front().second;
    
    if (currentDir == UP) nextY--;
    else if (currentDir == DOWN) nextY++;
    else if (currentDir == LEFT) nextX--;
    else if (currentDir == RIGHT) nextX++;
    
    int nextCell = map.getCell(nextY, nextX);

    // Gate(7): 부모의 순간이동 처리 (아이템보다 우선)
    if (nextCell == CELL_GATE) {
        return Snake::move(map, gateManager);
    }
    
    // 1. Growth Item (5)인 경우
    if (nextCell == 5) {
        // 부모의 move()를 호출하면 기본적으로 꼬리가 하나 잘리므로, 미리 꼬리 좌표를 저장.
        std::pair<int, int> oldTail = body.back();
        
        // 부모의 기본 이동 로직 수행 (머리 전진, 꼬리 1개 제거됨)
        int result = Snake::move(map, gateManager);
        if (result == -1) return -1;
        
        // 잘렸던 꼬리를 다시 복구하여 길이를 늘림.
        body.push_back(oldTail);
        map.setCell(oldTail.first, oldTail.second, 4);
        
        return 5; // Growth 아이템 획득 알림
    }
    // 2. Poison Item (6)인 경우
    else if (nextCell == 6) {
        // 부모의 기본 이동 로직 수행 (머리 전진, 꼬리 1개 제거됨)
        int result = Snake::move(map, gateManager);
        if (result == -1) return -1;
        
        // 독을 먹었으므로 꼬리를 한 개 더 자릅니다 (총 2개 감소)
        std::pair<int, int> extraTail = body.back();
        map.setCell(extraTail.first, extraTail.second, 0);
        body.pop_back();

        // 길이가 3 미만이면 게임 오버
        if (body.size() < 3) {
            alive = false;
            return -1;
        }
        
        return 6; // Poison 아이템 획득 알림
    }
    
    // 3. 아이템이 없는 빈칸 이동의 경우 부모 로직 그대로 사용
    return Snake::move(map, gateManager);
}
