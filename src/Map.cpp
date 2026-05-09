#include "Map.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

Map::Map(const std::string& filePath) : height(0), width(0) {
    loadFromFile(filePath);
    validateAndResize();
}

void Map::loadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::vector<int> row;
        for (char c : line) {
            if (isdigit(c)) row.push_back(c - '0');
        }
        if (!row.empty()) {
            data.push_back(row);
            width = std::max(width, (int)row.size());
        }
    }
    height = data.size();
}

void Map::validateAndResize() {
    int targetSize = 25; // User requested 25x25

    if (height < targetSize) data.resize(targetSize, std::vector<int>(width, 0));
    for (int i = 0; i < (int)data.size(); ++i) {
        if ((int)data[i].size() < targetSize) data[i].resize(targetSize, 0);
    }

    height = data.size();
    width = data[0].size();
}

void Map::draw() const {
    // Basic color setup (should be called in main, but we use it here)
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);    // Default
    init_pair(2, COLOR_BLUE, COLOR_BLUE);      // Wall
    init_pair(3, COLOR_CYAN, COLOR_CYAN);      // Immune Wall
    init_pair(4, COLOR_YELLOW, COLOR_YELLOW);  // Head (배경을 노란색으로)
    init_pair(5, COLOR_GREEN, COLOR_GREEN);    // Body (배경을 초록색으로)
    init_pair(6, COLOR_MAGENTA, COLOR_MAGENTA); // Gate

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int val = data[y][x];
            int drawX = x * 2; // Double the horizontal coordinate
            
            switch (val) {
                case 1: 
                    attron(COLOR_PAIR(2));
                    mvaddch(y, drawX, ' '); 
                    mvaddch(y, drawX + 1, ' '); 
                    attroff(COLOR_PAIR(2));
                    break;
                case 2: 
                    attron(COLOR_PAIR(3));
                    mvaddch(y, drawX, ' '); 
                    mvaddch(y, drawX + 1, ' '); 
                    attroff(COLOR_PAIR(3));
                    break;
                case 3: 
                    attron(COLOR_PAIR(4));
                    mvaddch(y, drawX, ' '); 
                    mvaddch(y, drawX + 1, ' '); 
                    attroff(COLOR_PAIR(4));
                    break;
                case 4:
                    attron(COLOR_PAIR(5));
                    mvaddch(y, drawX, ' ');
                    mvaddch(y, drawX + 1, ' ');
                    attroff(COLOR_PAIR(5));
                    break;
                case 7: // Gate
                    attron(COLOR_PAIR(6));
                    mvaddch(y, drawX, ' ');
                    mvaddch(y, drawX + 1, ' ');
                    attroff(COLOR_PAIR(6));
                    break;
                default: 
                    mvaddch(y, drawX, ' '); 
                    mvaddch(y, drawX + 1, ' '); 
                    break;
            }
        }
    }
}

void Map::setCell(int y, int x, int value) {
    if (y >= 0 && y < height && x >= 0 && x < width) data[y][x] = value;
}

int Map::getCell(int y, int x) const {
    if (y >= 0 && y < height && x >= 0 && x < width) return data[y][x];
    return -1;
}
