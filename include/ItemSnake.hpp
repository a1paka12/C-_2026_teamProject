#ifndef ITEM_SNAKE_HPP
#define ITEM_SNAKE_HPP

#include "Snake.hpp"

class ItemSnake : public Snake {
public:
    // 생성자는 부모의 생성자를 호출.
    ItemSnake(Map& map) : Snake(map) {}

    // 부모의 이동 로직을 오버라이딩하여 아이템 처리 로직을 추가.
    int move(Map& map) override;
};

#endif
