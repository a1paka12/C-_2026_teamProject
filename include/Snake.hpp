#ifndef SNAKE_HPP
#define SNAKE_HPP

#include "Map.hpp"
#include "Gate.hpp" // Direction enum 사용을 위해 포함
#include <deque>
#include <utility>

class Snake {
public:
    // 생성자: 맵을 분석하여 초기 뱀의 위치와 방향을 설정합니다.
    Snake(Map& map);

    // 사용자의 입력을 받아 방향을 업데이트합니다.
    // 반대 방향키를 눌러서 죽은 경우 false를 반환합니다.
    bool updateDirection(int ch);

    // 현재 방향으로 뱀을 1칸 이동시키고 맵 데이터를 갱신합니다.
    // 벽이나 자신의 몸에 부딪혀 죽은 경우 false를 반환합니다.
    bool move(Map& map);

    bool isAlive() const { return alive; }

private:
    std::deque<std::pair<int, int>> body; // 뱀의 좌표들 (front가 머리, back이 꼬리)
    Direction currentDir;
    bool alive;

    // 맵 데이터에서 3(머리), 4(몸통)를 찾아 body에 넣고 초기 방향을 계산하는 함수
    void initializeFromMap(Map& map);
};

#endif
