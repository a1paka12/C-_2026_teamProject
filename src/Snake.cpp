#include "Snake.hpp"
#include "Gate.hpp"
#include <ncurses.h>
#include <vector>
#include <cmath>

Snake::Snake(Map& map) : currentDir(LEFT), alive(true) {
    initializeFromMap(map);
}

void Snake::initializeFromMap(Map& map) {
    int headY = -1, headX = -1;
    std::vector<std::pair<int, int>> tails;
    
    // 맵 전체를 순회하며 머리(3)와 몸통(4)의 위치를 찾습니다.
    for (int y = 0; y < map.getHeight(); ++y) {
        for (int x = 0; x < map.getWidth(); ++x) {
            int cell = map.getCell(y, x);
            if (cell == 3) {
                headY = y;
                headX = x;
            } else if (cell == 4) {
                tails.push_back({y, x});
            }
        }
    }
    
    // 머리를 먼저 삽입합니다.
    if (headY != -1 && headX != -1) {
        body.push_back({headY, headX});
        
        // 몸통(tails)들을 머리와 이어진 순서대로 찾아 body 덱에 뒤이어 붙입니다.
        std::pair<int, int> current = {headY, headX};
        while (!tails.empty()) {
            bool found = false;
            for (auto it = tails.begin(); it != tails.end(); ++it) {
                // 상하좌우로 1칸 거리에 있는 타일(이어져 있는 타일)인지 확인
                if (std::abs(it->first - current.first) + std::abs(it->second - current.second) == 1) {
                    body.push_back(*it);
                    current = *it;
                    tails.erase(it);
                    found = true;
                    break;
                }
            }
            if (!found) break; // 예외 처리 (끊어져 있는 맵인 경우 방지)
        }
        
        // 첫 번째 몸통을 기준으로 머리가 어디를 향하고 있는지 계산하여 초기 방향 설정
        if (body.size() >= 2) {
            int dy = body[0].first - body[1].first;
            int dx = body[0].second - body[1].second;
            if (dy == -1) currentDir = UP;
            else if (dy == 1) currentDir = DOWN;
            else if (dx == -1) currentDir = LEFT;
            else if (dx == 1) currentDir = RIGHT;
        }
    }
}

bool Snake::updateDirection(int ch) {
    if (!alive) return false;
    
    Direction newDir = currentDir;
    if (ch == KEY_UP) newDir = UP;
    else if (ch == KEY_DOWN) newDir = DOWN;
    else if (ch == KEY_LEFT) newDir = LEFT;
    else if (ch == KEY_RIGHT) newDir = RIGHT;
    
    if (newDir != currentDir) {
        // 이동 중인 방향과 반대 방향키를 눌렀는지 체크 (즉사 조건)
        if ((currentDir == UP && newDir == DOWN) ||
            (currentDir == DOWN && newDir == UP) ||
            (currentDir == LEFT && newDir == RIGHT) ||
            (currentDir == RIGHT && newDir == LEFT)) {
            alive = false;
            return false;
        }
        currentDir = newDir;
    }
    return true;
}
// (수정) Phase 3: 자식 클래스(ItemSnake)와 반환형을 맞추고, 아이템 상태값을 구별하기 위해 
// 기존 bool 반환형을 int(-1: 사망, 0: 정상 이동)로 변경.
int Snake::move(Map& map, GateManager& gateManager) {
    if (!alive) return -1;
    
    int headY = body.front().first;
    int headX = body.front().second;
    
    int nextY = headY;
    int nextX = headX;
    
    if (currentDir == UP) nextY--;
    else if (currentDir == DOWN) nextY++;
    else if (currentDir == LEFT) nextX--;
    else if (currentDir == RIGHT) nextX++;
    
    // 다음 칸이 무엇인지 확인
    int nextCell = map.getCell(nextY, nextX);

    // 맵 밖(-1)은 이동 불가(벽으로 취급)
    if (nextCell == -1) {
        alive = false;
        return -1;
    }

    // Gate(7): 반대편 Gate 밖으로 순간이동 (통과 중에는 GateManager가 재생성하지 않음)
    if (nextCell == CELL_GATE) {
        int outY = 0, outX = 0;
        Direction outDir = currentDir;
        if (!gateManager.checkGateCollision(nextY, nextX, currentDir, outY, outX, outDir)) {
            alive = false;
            return -1;
        }

        // Gate 출구가 유효 범위를 벗어나면 즉사(방향 계산/맵 상태 이상 방지)
        if (outY < 0 || outY >= map.getHeight() || outX < 0 || outX >= map.getWidth()) {
            alive = false;
            return -1;
        }

        gateManager.beginGatePassage(static_cast<int>(body.size()));

        int tailY = body.back().first;
        int tailX = body.back().second;
        map.setCell(tailY, tailX, 0);
        body.pop_back();

        map.setCell(headY, headX, 4);

        body.push_front({outY, outX});
        map.setCell(outY, outX, 3);
        currentDir = outDir;
        return 0;
    }
    
    // 벽(1, 2)이거나 꼬리를 제외한 자신의 몸통(4)에 닿으면 즉사
    // 꼬리 위치는 이번 턴에 이동하면서 사라지므로 닿아도 죽지 않도록 예외처리
    // 9는 맵 확장 패딩(비플레이); 벽처럼 막음
    if (nextCell == 1 || nextCell == 2 || nextCell == 9) {
        alive = false;
        return -1;
    }
    if (nextCell == 4 && (nextY != body.back().first || nextX != body.back().second)) {
        alive = false;
        return -1;
    }
    
    // 머리 이동 처리
    body.push_front({nextY, nextX});
    map.setCell(nextY, nextX, 3);
    
    // 이전 머리를 몸통으로 변경
    map.setCell(headY, headX, 4);
    
    // 꼬리 자르기
    int tailY = body.back().first;
    int tailX = body.back().second;
    map.setCell(tailY, tailX, 0); // 맵에서 지움
    body.pop_back(); // 덱에서 제거
    
    return 0;
}
