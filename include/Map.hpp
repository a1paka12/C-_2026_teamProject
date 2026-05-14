#ifndef MAP_H
#define MAP_H

#include <vector>
#include <string>
#include <ncurses.h>

class Map {
public:
    Map(const std::string& filePath);

    // (5단계 ScoreBoard에서 추가) 스테이지 전환 시 같은 인스턴스에 새 맵 파일을 다시 로드
    void resetFromFile(const std::string& filePath);
    
    void draw() const;
    void setCell(int y, int x, int value);
    int getCell(int y, int x) const;
    
    int getHeight() const { return height; }
    int getWidth() const { return width; }

private:
    int height;
    int width;
    std::vector<std::vector<int>> data;
    
    void loadFromFile(const std::string& filePath);
    void validateAndResize();
};

#endif
