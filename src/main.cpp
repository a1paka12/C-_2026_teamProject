#include <ncurses.h>
#include "Map.hpp"
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

    gameMap.draw();
    mvprintw(gameMap.getHeight() + 1, 0, "Phase 1: Map Display (Stage 1)");
    mvprintw(gameMap.getHeight() + 2, 0, "Map File: %s", mapPath.c_str());
    mvprintw(gameMap.getHeight() + 3, 0, "Press 'q' to exit Phase 1...");
    refresh();

    int ch;
    while((ch = getch()) != 'q') {
        // Wait for 'q'
    }

    endwin();
    return 0;
}
