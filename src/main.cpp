#include <ncurses.h>
#include "Map.hpp"
#include "Gate.hpp"
#include "Snake.hpp"
#include <string>

// 변경 확인
// 혁주 확인1234

int main() {
    // Initialize ncurses
    initscr();
    start_color();
    noecho();
    curs_set(0); 
    keypad(stdscr, TRUE);

    // Milestone 1 Requirement: Display map from 2D array (loaded from file)
    // Starting with Stage 1 (Map 1)
    std::string mapPath = "data/map1.txt";
    Map gameMap(mapPath);

    Snake snake(gameMap);

    GateManager gateManager(gameMap);
    gateManager.spawnGates();

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

        // 게이트 매니저 업데이트 (시간 체크)
        if (gateManager.update()) {
            redraw = true;
        }

        // 3틱(300ms)마다 뱀 1칸 이동
        tick++;
        if (tick % 3 == 0) {
            if (!snake.move(gameMap)) {
                gameOver = true;
                break;
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
