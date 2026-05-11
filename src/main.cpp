#include <ncurses.h>
#include "Map.hpp"
#include "Gate.hpp"
#include "Item.hpp"
#include "ItemSnake.hpp"
#include <string>

int main() {
    // ncurses 초기화
    initscr();
    start_color();
    noecho();
    curs_set(0); 
    keypad(stdscr, TRUE);

    // 1단계 요구사항: 2차원 배열에서 맵 표시 (파일에서 로드)
    // 스테이지 1 (Map 1) 시작
    std::string mapPath = "data/map1.txt";
    Map gameMap(mapPath);

    ItemSnake snake(gameMap);

    GateManager gateManager(gameMap);
    gateManager.spawnGates();

    ItemManager itemManager(gameMap);

    gameMap.draw();
    mvprintw(gameMap.getHeight() + 1, 0, "Phase 1: Map Display (Stage 1)");
    mvprintw(gameMap.getHeight() + 2, 0, "Map File: %s", mapPath.c_str());
    mvprintw(gameMap.getHeight() + 3, 0, "Press 'q' to exit Phase 1...");
    refresh();

    // Gate 갱신을 위해 getch를 주기적으로(비차단) 호출
    timeout(100); // ms

    int ch = 0;
    int tick = 0;
    bool gameOver = false;
    int lastCountdown = 0;

    while (true) {
        ch = getch();
        if (ch == 'q') break;

        bool redraw = false;

        // 키 입력 처리 (방향 전환)
        if (ch != ERR) {
            if (!snake.updateDirection(ch)) {
                gameOver = true;
                break;
            }
        }

        // 아이템 매니저 업데이트 (시간 체크) - 1틱마다 체크
        if (itemManager.update(gameMap)) {
            redraw = true;
        }

        // 게이트 매니저 업데이트 (시간 체크)
        if (gateManager.update()) {
            redraw = true;
        }

        // 3틱(300ms)마다 뱀 1칸 이동
        tick++;
        if (tick % 3 == 0) {
            int moveResult = snake.move(gameMap, gateManager);
            if (moveResult == -1) {
                gameOver = true;
                break;
            }
            gateManager.afterSnakeMove();
            if (moveResult == 5 || moveResult == 6) {
                // 아이템을 먹었으므로, 현재 머리 위치의 아이템을 리스트에서 제거
                // 머리 좌표가 필요하지만, ItemManager 내부에서 위치 비교로 지우도록 할 수 있도록
                // 여기서는 Map 배열에 이미 머리(3)가 덮어씌워졌으므로 아이템 자체는 Map에서 사라진 상태.
                // 다만 ItemManager 내부 리스트에서도 지우려면 Snake의 현재 머리 좌표를 가져와야 함.
                // 그러나 편의상 다음 update()에서 맵 데이터 불일치(5/6 아님)로 알아서 제거될 수 있음.
                // 좀 더 안전하게 명시적으로 지우려면 Snake에서 좌표를 반환받아야 하지만, 
                // ItemManager::update에서 `if (map.getCell(y,x) != type) erase` 하는 로직이 이미 있다면 처리됨.
                // (현재 ItemManager.cpp 에 관련 로직을 작성했음)
            }
            redraw = true;
        }

        // 맵이나 뱀이 변했을 때만 화면 다시 그리기
        if (redraw) {
            clear();
            gameMap.draw();
            mvprintw(gameMap.getHeight() + 1, 0, "Phase 1: Map Display (Stage 1)");
            mvprintw(gameMap.getHeight() + 2, 0, "Map File: %s", mapPath.c_str());
            mvprintw(gameMap.getHeight() + 3, 0, "Press 'q' to exit Phase 1...");
            refresh();
        }

        // Gate 위치 변경(재생성) 직전 3/2/1 카운트다운을 Gate 블럭 위에 표시
        int cd = gateManager.respawnCountdown();
        if (cd != lastCountdown) {
            // 숫자를 지우기 위해(3->2->1->0) 필요한 경우 redraw
            if (lastCountdown != 0 && cd == 0) {
                clear();
                gameMap.draw();
                mvprintw(gameMap.getHeight() + 1, 0, "Phase 1: Map Display (Stage 1)");
                mvprintw(gameMap.getHeight() + 2, 0, "Map File: %s", mapPath.c_str());
                mvprintw(gameMap.getHeight() + 3, 0, "Press 'q' to exit Phase 1...");
            }

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

    // 게임 오버 처리
    if (gameOver) {
        clear();
        gameMap.draw();
        mvprintw(gameMap.getHeight() / 2, (gameMap.getWidth() * 2) / 2 - 8, "=================");
        mvprintw(gameMap.getHeight() / 2 + 1, (gameMap.getWidth() * 2) / 2 - 8, "   GAME OVER!    ");
        mvprintw(gameMap.getHeight() / 2 + 2, (gameMap.getWidth() * 2) / 2 - 8, " Press ENTER to  ");
        mvprintw(gameMap.getHeight() / 2 + 3, (gameMap.getWidth() * 2) / 2 - 8, "   exit game.    ");
        mvprintw(gameMap.getHeight() / 2 + 4, (gameMap.getWidth() * 2) / 2 - 8, "=================");
        refresh();
        
        timeout(-1); // Blocking 모드로 변경 (키 입력 대기)
        while (true) {
            int endCh = getch();
            if (endCh == '\n' || endCh == '\r' || endCh == KEY_ENTER) break;
        }
    }

    endwin();
    return 0;
}
