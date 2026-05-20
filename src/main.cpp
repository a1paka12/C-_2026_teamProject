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
#include "Zone.hpp"

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
    ZoneManager zoneManager(gameMap);  // 슬로우/패스트 존 (시간 제한 스폰)
    RedWallProjectileManager redWall(gameMap);

    // Score Board 초기화
    ScoreBoard scoreBoard;
    scoreBoard.setStage(startStage);
    scoreBoard.resetForNewStage(snake->getLength());

    // Score Board 시작 열 위치: 맵 가로 크기(셀 * 2칸) + 여백 2칸
    const int scoreCol = gameMap.getWidth() * 2 + 2;

    // ── 하단 알림 (맵 아래 한 줄: 존·아이템·미스터리·빨간 블록·속도 변화) ──
    // 맵 draw()가 COLOR_PAIR 1~18을 매 프레임 재정의하므로, 알림은 20번대만 사용
    std::string statusMsg;
    int statusMsgTicks = 0;
    enum StatusStyle {
        STATUS_DEFAULT,  // 흰 글자
        STATUS_WARN,     // 노랑 (속도 변화 등)
        STATUS_GOOD,     // 초록 (성장·패스트 존)
        STATUS_BAD,      // 빨강 (독·빨간 블록)
        STATUS_INFO      // 시안 (슬로우·무적·미스터리)
    };
    StatusStyle statusStyle = STATUS_DEFAULT;

    auto showStatus = [&](const char* msg, int ticks = 20, StatusStyle style = STATUS_DEFAULT) {
        statusMsg = msg;
        statusMsgTicks = ticks;
        statusStyle = style;
    };

    // 화면 전체를 다시 그리는 람다 함수 (맵 + Score Board + Gate 교체 카운트다운)
    auto paintUi = [&]() {
        clear();
        gameMap.draw(redWall.mapDrawBlinkPhase());
        redWall.drawOverlay();
        const int gateCd = gateManager.respawnCountdown();
        scoreBoard.draw(scoreCol, 0, gateCd);
        
        // 하단 알림 출력 (검정 배경 + 글자색 — 맵 7번(초록/초록)과 겹치지 않음)
        if (!statusMsg.empty()) {
            int statusPair = 20;
            if (has_colors()) {
                init_pair(20, COLOR_WHITE, COLOR_BLACK);
                init_pair(21, COLOR_YELLOW, COLOR_BLACK);
                init_pair(22, COLOR_GREEN, COLOR_BLACK);
                init_pair(23, COLOR_RED, COLOR_BLACK);
                init_pair(24, COLOR_CYAN, COLOR_BLACK);
                switch (statusStyle) {
                    case STATUS_WARN: statusPair = 21; break;
                    case STATUS_GOOD: statusPair = 22; break;
                    case STATUS_BAD:  statusPair = 23; break;
                    case STATUS_INFO: statusPair = 24; break;
                    default:          statusPair = 20; break;
                }
                attron(COLOR_PAIR(statusPair) | A_BOLD);
            } else {
                attron(A_BOLD);
            }
            mvprintw(gameMap.getHeight() + 1, 0, "%s", statusMsg.c_str());
            if (has_colors()) {
                attroff(COLOR_PAIR(statusPair) | A_BOLD);
            } else {
                attroff(A_BOLD);
            }
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
    int currentTimeout = 100;
    
    // 스테이지 시작 시간 (속도 계산용)
    int stageStartSec = 0;
    
    // 아이템 효과에 의한 속도 조절
    int speedModifierFromItems = 0;
    int growthCountForSpeed = 0;
    int poisonCountForSpeed = 0;
    int lastHeadZone = ZONE_NONE;

    // ── 메인 게임 루프 ──
    while (true) {
        ch = getch();
        if (ch == 'q' || ch == 'Q') break;  // Q키로 게임 종료

        if (statusMsgTicks > 0) statusMsgTicks--;
        else statusMsg.clear();

        // 게임 경과 시간을 Score Board에 반영
        const int elapsedSec = static_cast<int>(
            std::chrono::duration_cast<std::chrono::seconds>(clock::now() - gameStart).count());
        scoreBoard.setElapsedSeconds(elapsedSec);

        // 속도 계산 (스테이지 경과 시간 + 아이템 + 머리 위치 존)
        int stageElapsedSec = elapsedSec - stageStartSec;
        int baseTimeout = 100 - (stageElapsedSec / 15) * 10;
        std::pair<int, int> headPos = snake->getHeadPos();
        const int headZone = gameMap.getZone(headPos.first, headPos.second);
        const int zoneTimeoutDelta = gameMap.getZoneTimeoutDelta(headPos.first, headPos.second);
        int targetTimeout = baseTimeout - speedModifierFromItems + zoneTimeoutDelta;
        if (targetTimeout < 30) targetTimeout = 30;
        if (targetTimeout > 180) targetTimeout = 180;

        if (headZone != lastHeadZone) {
            if (headZone == ZONE_SLOW)
                showStatus("Slow Zone", 15, STATUS_INFO);
            else if (headZone == ZONE_FAST)
                showStatus("Fast Zone", 15, STATUS_GOOD);
            lastHeadZone = headZone;
        }

        if (targetTimeout < currentTimeout) {
            currentTimeout = targetTimeout;
            if (statusMsgTicks <= 0)
                showStatus("Speed Up!", 20, STATUS_WARN);
        } else if (targetTimeout > currentTimeout) {
            currentTimeout = targetTimeout;
            if (statusMsgTicks <= 0)
                showStatus("Speed Down!", 20, STATUS_WARN);
        }
        timeout(currentTimeout);

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

        // 속도 존 업데이트 (일정 시간 후 사라지고 다른 위치에 재생성)
        if (zoneManager.update(gameMap)) {
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

            // 이동 결과에 따라 Score Board·하단 알림 반영
            if (moveResult == SNAKE_MOVE_GROWTH) {
                scoreBoard.addGrowth();
                growthCountForSpeed++;
                if (growthCountForSpeed >= 4) {
                    growthCountForSpeed = 0;
                    speedModifierFromItems += 10;
                }
                showStatus("Growth! Length +1", 20, STATUS_GOOD);
            } else if (moveResult == SNAKE_MOVE_MYSTERY_GROWTH) {
                scoreBoard.addGrowth();
                growthCountForSpeed++;
                if (growthCountForSpeed >= 4) {
                    growthCountForSpeed = 0;
                    speedModifierFromItems += 10;
                }
                showStatus("Mystery Box: Growth! (+1)", 22, STATUS_INFO);
            } else if (moveResult == SNAKE_MOVE_POISON) {
                scoreBoard.addPoison();
                poisonCountForSpeed++;
                if (poisonCountForSpeed >= 3) {
                    poisonCountForSpeed = 0;
                    speedModifierFromItems -= 10;
                }
                showStatus("Poison! Length -1", 20, STATUS_BAD);
            } else if (moveResult == SNAKE_MOVE_MYSTERY_POISON) {
                scoreBoard.addPoison();
                poisonCountForSpeed++;
                if (poisonCountForSpeed >= 3) {
                    poisonCountForSpeed = 0;
                    speedModifierFromItems -= 10;
                }
                showStatus("Mystery Box: Poison! (-1)", 22, STATUS_INFO);
            } else if (moveResult == SNAKE_MOVE_INVINCIBLE) {
                showStatus("Invincible! (5s)", 22, STATUS_INFO);
            } else if (moveResult == SNAKE_MOVE_MYSTERY_INVINCIBLE) {
                showStatus("Mystery Box: Invincible! (5s)", 22, STATUS_INFO);
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
                zoneManager.resetForNewMap(gameMap);
                snake = std::make_unique<ItemSnake>(gameMap);
                gateManager.spawnGates();
                redWall.resetForNewMap(gameMap);

                // Score Board를 다음 스테이지 목표로 갱신
                scoreBoard.setStage(nextStage);
                scoreBoard.resetForNewStage(snake->getLength());
                tick = 0;
                redraw = true;
                
                // 속도 및 스테이지 시작 시간 초기화
                stageStartSec = elapsedSec;
                speedModifierFromItems = 0;
                growthCountForSpeed = 0;
                poisonCountForSpeed = 0;
                currentTimeout = 100;
                lastHeadZone = ZONE_NONE;

                // 비차단 모드 복원
                timeout(currentTimeout);
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
        if (redWallResult.snakeHitRedWall) {
            showStatus("Red Block Hit! Length -3", 25, STATUS_BAD);
            redraw = true;
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
