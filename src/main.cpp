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

int main() {
    // ncurses 초기화
    initscr();
    start_color();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    // 게임 시작 시각을 기록 (경과 시간 계산용)
    using clock = std::chrono::steady_clock;
    const auto gameStart = clock::now();

    // Stage 1 맵 로드 및 게임 객체 생성
    Map gameMap(kStageMaps[1]);
    auto snake = std::make_unique<ItemSnake>(gameMap);

    GateManager gateManager(gameMap);
    gateManager.spawnGates();

    ItemManager itemManager(gameMap);

    // Score Board 초기화 (Stage 1, 초기 뱀 길이 설정)
    ScoreBoard scoreBoard;
    scoreBoard.setStage(1);
    scoreBoard.resetForNewStage(snake->getLength());

    // Score Board 시작 열 위치: 맵 가로 크기(셀 * 2칸) + 여백 2칸
    const int scoreCol = gameMap.getWidth() * 2 + 2;

    // 화면 전체를 다시 그리는 람다 함수 (맵 + Score Board)
    auto paintUi = [&]() {
        clear();
        gameMap.draw();
        scoreBoard.draw(scoreCol, 0);
        refresh();
    };

    // 초기 화면 출력
    paintUi();

    // 100ms마다 getch()가 반환하도록 비차단 모드 설정
    timeout(100);

    int ch = 0;
    int tick = 0;
    bool gameOver = false;
    int lastCountdown = 0;
    int lastPaintedSecond = -1;

    // ── 메인 게임 루프 ──
    while (true) {
        ch = getch();
        if (ch == 'q' || ch == 'Q') break;  // Q키로 게임 종료

        // 게임 경과 시간을 Score Board에 반영
        const int elapsedSec = static_cast<int>(
            std::chrono::duration_cast<std::chrono::seconds>(clock::now() - gameStart).count());
        scoreBoard.setElapsedSeconds(elapsedSec);

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
        tick++;
        if (tick % 3 == 0) {
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
            if (moveResult == SNAKE_MOVE_GROWTH) {
                scoreBoard.addGrowth();   // Growth Item 획득
            } else if (moveResult == SNAKE_MOVE_POISON) {
                scoreBoard.addPoison();   // Poison Item 획득
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
                gameMap.draw();
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

                // Score Board를 다음 스테이지 목표로 갱신
                scoreBoard.setStage(nextStage);
                scoreBoard.resetForNewStage(snake->getLength());
                tick = 0;
                redraw = true;

                // 비차단 모드 복원
                timeout(100);
            }
        }

        // 맵/뱀 변화 또는 시간 변화가 있을 때만 화면을 다시 그린다.
        const bool timeTick = (elapsedSec != lastPaintedSecond);
        if (redraw || timeTick) {
            lastPaintedSecond = elapsedSec;
            paintUi();
        }

        // ── Gate 재생성 카운트다운 표시 (3/2/1초) ──
        int cd = gateManager.respawnCountdown();
        if (cd != lastCountdown) {
            if (lastCountdown != 0 && cd == 0) {
                paintUi();
            }

            // Gate 위치에 남은 초를 숫자로 표시
            if (cd != 0) {
                GatePos g1 = gateManager.getGate1();
                GatePos g2 = gateManager.getGate2();
                int drawX1 = g1.x * 2;
                int drawX2 = g2.x * 2;

                attron(COLOR_PAIR(6) | A_BOLD);
                mvaddch(g1.y, drawX1, '0' + cd);
                mvaddch(g1.y, drawX1 + 1, ' ');
                mvaddch(g2.y, drawX2, '0' + cd);
                mvaddch(g2.y, drawX2 + 1, ' ');
                attroff(COLOR_PAIR(6) | A_BOLD);
            }

            refresh();
            lastCountdown = cd;
        }
    }

    // ── 게임 오버 처리 (빨간색 팝업) ──
    if (gameOver) {
        clear();
        gameMap.draw();
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
