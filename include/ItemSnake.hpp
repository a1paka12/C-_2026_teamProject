#ifndef ITEM_SNAKE_HPP
#define ITEM_SNAKE_HPP

#include "Snake.hpp"

class ItemSnake : public Snake {
public:
    // 생성자는 부모의 생성자를 호출.
    ItemSnake(const Map& map) : Snake(map), invincibleTicks(0), isStopped(false) {}

    // 부모의 이동 로직을 오버라이딩하여 아이템 처리 로직을 추가
    int move(Map& map, GateManager& gateManager) override;

    // 빨간 벽 발사체 충돌: 길이 3 감소. 현재 길이가 3 이하면 즉사
    bool applyRedWallHit(Map& map);

    int getInvincibleTicks() const { return invincibleTicks; }
    void tickInvincible() { if (invincibleTicks > 0) invincibleTicks--; }
    bool updateDirection(int ch); // 오버라이딩하여 멈춤 상태 해제 로직 추가

private:
    int invincibleTicks; // 무적 지속 시간
    bool isStopped;      // 벽 충돌로 인해 멈춘 상태인지 여부
};

#endif
