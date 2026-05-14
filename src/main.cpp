//test
#include <ncurses.h>
#include <chrono>
#include <memory>
#include <algorithm>

#include "Map.hpp"
#include "Gate.hpp"
#include "Item.hpp"
#include "ItemSnake.hpp"
#include "Snake.hpp"
#include "ScoreBoard.hpp"
#include "RedWallProjectile.hpp"

// 스테이지별 맵 파일 경로 (Stage 1~4)
namespace {

const char* kStageMaps[] = {
    nullptr,          // 인덱스 0은 사용하지 않음
    "data/map1.txt",  // Stage 1
    "data/map2.txt",  // Stage 2
    "data/map3.txt",  // Stage 3
    "data/map4.txt",  // Stage 4
};

} // namespace

int main(int argc, char* argv[]) {
    // ncurses 초기화
    initscr();
    start_color();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    // 디버그 스테이지 설정
    int startStage = 1;
    if (argc >= 2) {
        startStage = std::atoi(argv[1]);
        if (startStage < 1) startStage = 1;
        if (startStage > 4) startStage = 4;
    }

    // 게임 시작 시각을 기록 (경과 시간 계산용)
    using clock = std::chrono::steady_clock;
    const auto gameStart = clock::now();

    // 시작 스테이지 맵 로드 및 게임 객체 생성
    Map gameMap(kStageMaps[startStage]);
    auto snake = std::make_unique<ItemSnake>(gameMap);

    GateManager gateManager(gameMap);
    gateManager.spawnGates();

    ItemManager itemManager(gameMap);
    RedWallProjectileManager redWall(gameMap);

    // Score Board 초기화
    ScoreBoard scoreBoard;
    scoreBoard.setStage(startStage);
    scoreBoard.resetForNewStage(snake->getLength());

    // Score Board 시작 열 위치: 맵 가로 크기(셀 * 2칸) + 여백 2칸
    const int scoreCol = gameMap.getWidth() * 2 + 2;

    // 미스터리 박스 획득 메시지 관리
    std::string mysteryMsg = "";
    int mysteryMsgTicks = 0;

    // 화면 전체를 다시 그리는 람다 함수 (맵 + Score Board + Gate 교체 카운트다운)
    auto paintUi = [&]() {
        clear();
        gameMap.draw(redWall.mapDrawBlinkPhase());
        redWall.drawOverlay();
        const int gateCd = gateManager.respawnCountdown();
        scoreBoard.draw(scoreCol, 0, gateCd);
        
        // 미스터리 박스 메시지 표시 (Score Board 하단)
        if (!mysteryMsg.empty()) {
            attron(COLOR_PAIR(11) | A_BOLD);
            mvprintw(gameMap.getHeight() + 1, 0, "Mystery Box: %s", mysteryMsg.c_str());
            attroff(COLOR_PAIR(11) | A_BOLD);
        }

        // 무적 시간 표시 (머리 위)
        int invTicks = snake->getInvincibleTicks();
        if (invTicks > 0) {
            std::pair<int, int> head = snake->getHeadPos();
            if (head.first > 0) { // 머리 위에 공간이 있는 경우 (맵 상단 경계 제외)
                attron(COLOR_PAIR(11) | A_BOLD);
                // 10틱을 1초로 계산하여 남은 초 표시
                mvprintw(head.first - 1, head.second * 2, "%d", (invTicks + 9) / 10);
                attroff(COLOR_PAIR(11) | A_BOLD);
            }
        }

        // Gate 타일 2칸 폭에 노란 배경+굵은 숫자 (자색/자색 페어는 숫자가 안 보임)
        if (gateCd > 0) {
            GatePos g1 = gateManager.getGate1();
            GatePos g2 = gateManager.getGate2();
            const int drawX1 = g1.x * 2;
            const int drawX2 = g2.x * 2;
            const chtype num = static_cast<chtype>('0' + gateCd);
            if (has_colors()) {
                attron(COLOR_PAIR(12) | A_BOLD);
            } else {
                attron(A_STANDOUT | A_BOLD);
            }
            mvaddch(g1.y, drawX1, num);
            mvaddch(g1.y, drawX1 + 1, num);
            mvaddch(g2.y, drawX2, num);
            mvaddch(g2.y, drawX2 + 1, num);
            if (has_colors()) {
                attroff(COLOR_PAIR(12) | A_BOLD);
            } else {
                attroff(A_STANDOUT | A_BOLD);
            }
        }
        refresh();
    };

    // 초기 화면 출력
    paintUi();

    // 100ms마다 getch()가 반환하도록 비차단 모드 설정
    timeout(100);

    int ch = 0;
    int tick = 0;
    bool gameOver = false;
    int lastGateCd = -1;
    int lastPaintedSecond = -1;

    // ── 메인 게임 루프 ──
    while (true) {
        ch = getch();
        if (ch == 'q' || ch == 'Q') break;  // Q키로 게임 종료

        // 미스터리 메시지 지속 시간 감소
        if (mysteryMsgTicks > 0) mysteryMsgTicks--;
        else mysteryMsg = "";

        // 게임 경과 시간을 Score Board에 반영
        const int elapsedSec = static_cast<int>(
            std::chrono::duration_cast<std::chrono::seconds>(clock::now() - gameStart).count());
        scoreBoard.setElapsedSeconds(elapsedSec);

        // 무적 시간 감소
        snake->tickInvincible();

        bool redraw = false;

        // 키 입력이 있으면 뱀 방향 전환 (반대 방향 입력 시 즉사)
        if (ch != ERR) {
            if (!snake->updateDirection(ch)) {
                gameOver = true;
                break;
            }
        }

        // 아이템 매니저 업데이트 (수명 체크, 소멸/재생성)
        if (itemManager.update(gameMap)) {
            redraw = true;
        }

        // 게이트 매니저 업데이트 (20초마다 위치 재생성)
        if (gateManager.update()) {
            redraw = true;
        }

        // 3틱(300ms)마다 뱀을 1칸 이동시킨다.
        bool snakeStep = false;
        tick++;
        if (tick % 3 == 0) {
            snakeStep = true;
            // Gate 통과 여부를 감지하기 위해 이동 전 상태를 저장
            bool wasPassage = gateManager.isPassageActive();
            int moveResult = snake->move(gameMap, gateManager);

            // 이동 결과가 사망이면 게임 오버
            if (moveResult == SNAKE_MOVE_DEAD) {
                gameOver = true;
                break;
            }
            gateManager.afterSnakeMove();

            // 이동 결과에 따라 Score Board에 이벤트 반영
            if (moveResult == SNAKE_MOVE_GROWTH || moveResult == SNAKE_MOVE_MYSTERY_GROWTH) {
                scoreBoard.addGrowth();   // Growth Item 획득
                if (moveResult == SNAKE_MOVE_MYSTERY_GROWTH) {
                    mysteryMsg = "GROWTH! (+1)";
                    mysteryMsgTicks = 20;
                }
            } else if (moveResult == SNAKE_MOVE_POISON || moveResult == SNAKE_MOVE_MYSTERY_POISON) {
                scoreBoard.addPoison();   // Poison Item 획득
                if (moveResult == SNAKE_MOVE_MYSTERY_POISON) {
                    mysteryMsg = "POISON! (-1)";
                    mysteryMsgTicks = 20;
                }
            } else if (moveResult == SNAKE_MOVE_MYSTERY_INVINCIBLE) {
                mysteryMsg = "INVINCIBLE! (5s)";
                mysteryMsgTicks = 20;
            }
            // 이동 전에는 Gate 통과 중이 아니었는데, 이동 후 통과 중이면 Gate 사용으로 판단
            if (!wasPassage && gateManager.isPassageActive()) {
                scoreBoard.addGate();     // Gate 사용 횟수 증가
            }

            // 뱀 길이를 Score Board에 반영 (최대 길이도 자동 갱신)
            scoreBoard.updateLength(snake->getLength());
            redraw = true;

            // ── 미션 달성 체크 ──
            if (scoreBoard.isMissionClear()) {
                bool isFinal = (scoreBoard.getStage() >= 4);

                // 화면에 맵 + Score Board + 클리어 팝업을 출력
                clear();
                gameMap.draw(redWall.mapDrawBlinkPhase());
                redWall.drawOverlay();
                scoreBoard.draw(scoreCol, 0);
                scoreBoard.drawClearPopup(gameMap.getWidth(), gameMap.getHeight(), isFinal);
                refresh();

                // Enter 키 입력 대기
                timeout(-1);
                while (true) {
                    int endCh = getch();
                    if (endCh == '\n' || endCh == '\r' || endCh == KEY_ENTER) break;
                }

                // 마지막 스테이지였으면 게임 종료
                if (isFinal) {
                    endwin();
                    return 0;
                }

                // 다음 스테이지로 전환
                const int nextStage = scoreBoard.getStage() + 1;

                // 기존 Gate 제거 후 새 맵 로드
                gateManager.removeGates();
                gameMap.resetFromFile(kStageMaps[nextStage]);

                // 아이템, 뱀, Gate를 새 맵 기준으로 재생성
                itemManager.resetForNewMap(gameMap);
                snake = std::make_unique<ItemSnake>(gameMap);
                gateManager.spawnGates();
                redWall.resetForNewMap(gameMap);

                // Score Board를 다음 스테이지 목표로 갱신
                scoreBoard.setStage(nextStage);
                scoreBoard.resetForNewStage(snake->getLength());
                tick = 0;
                redraw = true;

                // 비차단 모드 복원
                timeout(100);
            }
        }

        const auto redWallResult = redWall.update(gameMap, *snake, snakeStep);
        if (redWallResult.gameOver) {
            gameOver = true;
            break;
        }
        if (redWallResult.needsRedraw) {
            redraw = true;
            scoreBoard.updateLength(snake->getLength());
        }

        // Gate 교체 카운트(1~3)가 바뀌면 반드시 다시 그림 (초 단위 게임 타이머와 어긋나도 표시 유지)
        const int gateCd = gateManager.respawnCountdown();
        if (gateCd != lastGateCd) {
            redraw = true;
        }

        // 맵/뱀 변화 또는 시간 변화가 있을 때만 화면을 다시 그린다.
        const bool timeTick = (elapsedSec != lastPaintedSecond);
        if (redraw || timeTick) {
            lastPaintedSecond = elapsedSec;
            paintUi();
        }

        lastGateCd = gateCd;
    }

    // ── 게임 오버 처리 (빨간색 팝업) ──
    if (gameOver) {
        clear();
        gameMap.draw(redWall.mapDrawBlinkPhase());
        redWall.drawOverlay();
        scoreBoard.draw(scoreCol, 0);
        scoreBoard.drawGameOverPopup(gameMap.getWidth(), gameMap.getHeight());
        refresh();

        // Enter 키 입력 대기 후 종료
        timeout(-1);
        while (true) {
            int endCh = getch();
            if (endCh == '\n' || endCh == '\r' || endCh == KEY_ENTER) break;
        }
    }

    // ncurses 종료
    endwin();
    return 0;
}
