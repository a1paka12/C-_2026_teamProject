#ifndef SNAKE_HPP
#define SNAKE_HPP

#include "Map.hpp"
#include "Gate.hpp" // Direction enum 사용을 위해 포함
#include <deque>
#include <utility>

// (5단계 ScoreBoard에서 추가) move() / ItemSnake::move() 반환 코드 상수
// 기존 반환값(-1, 0, 5, 6)에 이름을 붙여 main.cpp에서 가독성 있게 사용한다.
constexpr int SNAKE_MOVE_DEAD = -1;
constexpr int SNAKE_MOVE_OK = 0;
constexpr int SNAKE_MOVE_GROWTH = 5;
constexpr int SNAKE_MOVE_POISON = 6;

class Snake {
public:
    // (5단계 ScoreBoard에서 추가) unique_ptr로 관리하기 위해 가상 소멸자 추가
    virtual ~Snake() = default;

    // 생성자: 맵을 분석하여 초기 뱀의 위치와 방향을 설정합니다.
    Snake(Map& map);

    // 사용자의 입력을 받아 방향을 업데이트합니다.
    // 반대 방향키를 눌러서 죽은 경우 false를 반환합니다.
    bool updateDirection(int ch);

    // 현재 방향으로 뱀을 1칸 이동시키고 맵 데이터를 갱신합니다.
    // 벽이나 자신의 몸에 부딪혀 죽은 경우 false를 반환합니다.
    // (수정) Phase 3: ItemSnake 클래스에서 아이템 획득 로직을 오버라이딩하기 위해 virtual 및 반환형(int) 추가
    virtual int move(Map& map, GateManager& gateManager);

    bool isAlive() const { return alive; }
    // (5단계 ScoreBoard에서 추가) Score Board에 현재 뱀 길이를 전달하기 위한 함수
    int getLength() const { return static_cast<int>(body.size()); }

// (수정) Phase 3: 상속받은 ItemSnake 클래스에서 변수에 접근할 수 있도록 private에서 protected로 변경
protected:
    std::deque<std::pair<int, int>> body; // 뱀의 좌표들 (front가 머리, back이 꼬리)
    Direction currentDir;
    bool alive;

    // 맵 데이터에서 3(머리), 4(몸통)를 찾아 body에 넣고 초기 방향을 계산하는 함수
    void initializeFromMap(Map& map);
};

#endif
