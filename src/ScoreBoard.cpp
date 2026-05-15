#include "ScoreBoard.hpp"
#include <ncurses.h>
#include <cstdio>
#include <algorithm>

// ── Grade 기준 (스테이지별 초 단위 임계값) ──
// 4개 경계: 이하이면 S→A→B→C, 그보다 느리면 D (값이 클수록 같은 등급에 허용되는 시간이 길다)
static const int kGradeThresholds[5][4] = {
    {  45,  90,  130,  170 },  // Stage 1 (기존 대비 여유)
    {  55, 100,  140,  185 },  // Stage 2
    {  70, 115,  155,  200 },  // Stage 3
    {  75, 125,  170,  210 },  // Stage 4
    {  90, 140,  190,  240 },  // Stage 5
};

static const char* kGradeLabels[] = { "S ", "A ", "B ", "C ", "D ", "F " };

// 팝업 등: 등급 글자별 강조색 (전체를 초록으로 칠하지 않음)
static int gradeRowColorPair(char c) {
    if (c == 'S') return 11; // 노랑 — 최상위만 눈에 띄게
    if (c == 'A') return 9;  // 시안
    if (c == 'B' || c == 'C') return 13; // 자홍
    if (c == 'D') return 10; // 흰색
    if (c == 'F' || c == '-') return 12; // 빨강 / 미기록
    return 10;
}

const char* ScoreBoard::computeGrade(int stageIdx1, int seconds) {
    if (stageIdx1 < 1 || stageIdx1 > 5 || seconds < 0) return "--";
    const int* t = kGradeThresholds[stageIdx1 - 1];
    if (seconds <= t[0]) return "S ";
    if (seconds <= t[1]) return "A ";
    if (seconds <= t[2]) return "B ";
    if (seconds <= t[3]) return "C ";
    return "D ";
}

int ScoreBoard::gradeRank(const char* grade) {
    for (int i = 0; i < 6; ++i) {
        if (grade[0] == kGradeLabels[i][0] && grade[1] == kGradeLabels[i][1])
            return i;
    }
    return 99;
}

ScoreBoard::ScoreBoard()
    : stage(1),
      currentLength(0),
      maxLength(0),
      growthCount(0),
      poisonCount(0),
      gateCount(0),
      bodyGoal(10),
      growthGoal(3),
      poisonGoal(1),
      gateGoal(1),
      elapsedSeconds(0),
      stageStartSec(0) {
    for (int i = 0; i < 5; ++i) stageTimes[i] = -1;
}

void ScoreBoard::setStage(int stageIndex1Based) {
    if (stageIndex1Based < 1) stageIndex1Based = 1;
    if (stageIndex1Based > 5) stageIndex1Based = 5;
    stage = stageIndex1Based;
    applyGoalsForStage();
}

void ScoreBoard::applyGoalsForStage() {
    switch (stage) {
        case 1: bodyGoal = 10; growthGoal = 3; poisonGoal = 1; gateGoal = 1; break;
        case 2: bodyGoal = 12; growthGoal = 4; poisonGoal = 1; gateGoal = 1; break;
        case 3: bodyGoal = 14; growthGoal = 5; poisonGoal = 2; gateGoal = 2; break;
        case 4: bodyGoal = 16; growthGoal = 6; poisonGoal = 2; gateGoal = 2; break;
        case 5:
        default: bodyGoal = 18; growthGoal = 7; poisonGoal = 3; gateGoal = 3; break;
    }
}

void ScoreBoard::resetForNewStage(int initialSnakeLength) {
    growthCount = 0;
    poisonCount = 0;
    gateCount = 0;
    currentLength = initialSnakeLength;
    maxLength = initialSnakeLength;
    stageStartSec = elapsedSeconds;  // 현재 시점을 스테이지 시작으로 기록
    applyGoalsForStage();
}

void ScoreBoard::finishStageTimer() {
    if (stage >= 1 && stage <= 5) {
        stageTimes[stage - 1] = elapsedSeconds - stageStartSec;
        if (stageTimes[stage - 1] < 0) stageTimes[stage - 1] = 0;
    }
}

void ScoreBoard::updateLength(int len) {
    currentLength = len;
    if (currentLength > maxLength) maxLength = currentLength;
}

void ScoreBoard::addGrowth() { ++growthCount; }
void ScoreBoard::addPoison() { ++poisonCount; }
void ScoreBoard::addGate() { ++gateCount; }

void ScoreBoard::setElapsedSeconds(int seconds) {
    if (seconds < 0) seconds = 0;
    elapsedSeconds = seconds;
}

bool ScoreBoard::isBodyClear()   const { return maxLength >= bodyGoal; }
bool ScoreBoard::isGrowthClear() const { return growthCount >= growthGoal; }
bool ScoreBoard::isPoisonClear() const { return poisonCount >= poisonGoal; }
bool ScoreBoard::isGateClear()   const { return gateCount >= gateGoal; }

bool ScoreBoard::isMissionClear() const {
    return isBodyClear() && isGrowthClear() && isPoisonClear() && isGateClear();
}

const char* ScoreBoard::getStageGrade(int stageIdx1) const {
    if (stageIdx1 < 1 || stageIdx1 > 5) return "--";
    int t = stageTimes[stageIdx1 - 1];
    if (t == -2) return "F ";
    if (t < 0)   return "--";
    return computeGrade(stageIdx1, t);
}

const char* ScoreBoard::getOverallGrade() const {
    int worst = -1;
    for (int i = 0; i < 5; ++i) {
        int t = stageTimes[i];
        if (t == -1) continue;  // 미진입 스테이지는 제외
        int r = (t == -2) ? gradeRank("F ") : gradeRank(computeGrade(i + 1, t));
        if (r > worst) worst = r;
    }
    if (worst < 0) return "--";
    return kGradeLabels[worst];
}

void ScoreBoard::markStageFailed() {
    if (stage >= 1 && stage <= 5)
        stageTimes[stage - 1] = -2;
}

// ---------------------------------------------------------------
// draw: 우측 패널 Score Board 출력
// ---------------------------------------------------------------
void ScoreBoard::draw(int originCol, int originRow, int gateRespawnCountdown) const {
    const bool useColor = has_colors();
    if (useColor) {
        init_pair(9,  COLOR_CYAN,   COLOR_BLACK);  // 강조 문구(미션 클리어 등)
        init_pair(10, COLOR_WHITE,  COLOR_BLACK); // 기본
        init_pair(11, COLOR_YELLOW, COLOR_BLACK); // 시간·달성 행
    }

    int mm = elapsedSeconds / 60;
    int ss = elapsedSeconds % 60;
    int stageElapsed = elapsedSeconds - stageStartSec;
    int sm = stageElapsed / 60;
    int sse = stageElapsed % 60;

    int y = originRow;
    int x = originCol;

    if (useColor) attron(COLOR_PAIR(10));
    mvprintw(y++, x, "+--------------------------------------+");
    mvprintw(y++, x, "| Score Board                          |");

    char stageLine[40];
    std::snprintf(stageLine, sizeof(stageLine), "Stage  : %d / 5", stage);
    mvprintw(y++, x, "| %-36s |", stageLine);
    if (useColor) attroff(COLOR_PAIR(10));

    // 총 경과 시간 (노란색)
    char tbuf[40];
    std::snprintf(tbuf, sizeof(tbuf), "Total  : %02d:%02d  Stage: %02d:%02d", mm, ss, sm, sse);
    if (useColor) attron(COLOR_PAIR(11));
    mvprintw(y++, x, "| %-36s |", tbuf);
    if (useColor) attroff(COLOR_PAIR(11));

    // Gate 교체 경고
    if (gateRespawnCountdown > 0) {
        char wbuf[40];
        std::snprintf(wbuf, sizeof(wbuf), ">> GATE %ds <<", gateRespawnCountdown);
        if (useColor) {
            init_pair(12, COLOR_BLACK, COLOR_YELLOW);
            attron(COLOR_PAIR(12) | A_BOLD);
        }
        mvprintw(y++, x, "| %-36s |", wbuf);
        if (useColor) attroff(COLOR_PAIR(12) | A_BOLD);
    }

    mvprintw(y++, x, "| Control: Arrow Keys        Quit: Q   |");
    mvprintw(y++, x, "|--------------------------------------|");
    mvprintw(y++, x, "| Type      Now        Goal     Clear  |");

    bool bClear  = isBodyClear();
    bool gClear  = isGrowthClear();
    bool pClear  = isPoisonClear();
    bool gtClear = isGateClear();

    char bodyNow[32];
    std::snprintf(bodyNow, sizeof(bodyNow), "%d / %d", currentLength, maxLength);

    auto row = [&](const char* label, const char* nowStr, int goal, bool cleared) {
        const char* mark = cleared ? "[V]" : "[ ]";
        if (useColor) {
            // 달성 행은 노랑 굵게, 미달성은 흰색(초록 일색 방지)
            attron(COLOR_PAIR(cleared ? 11 : 10));
            if (cleared) attron(A_BOLD);
            mvprintw(y++, x, "| %-9s %-10s %-7d %-7s |", label, nowStr, goal, mark);
            if (cleared) attroff(A_BOLD);
            attroff(COLOR_PAIR(cleared ? 11 : 10));
        } else {
            mvprintw(y++, x, "| %-9s %-10s %-7d %-7s |", label, nowStr, goal, mark);
        }
    };

    row("Body", bodyNow, bodyGoal, bClear);

    char gbuf[16], pbuf[16], gtbuf[16];
    std::snprintf(gbuf,  sizeof(gbuf),  "%d", growthCount);
    std::snprintf(pbuf,  sizeof(pbuf),  "%d", poisonCount);
    std::snprintf(gtbuf, sizeof(gtbuf), "%d", gateCount);

    row("Growth", gbuf,  growthGoal, gClear);
    row("Poison", pbuf,  poisonGoal, pClear);
    row("Gate",   gtbuf, gateGoal,   gtClear);

    if (useColor) attron(COLOR_PAIR(10));
    mvprintw(y++, x, "+--------------------------------------+");
    if (useColor) attroff(COLOR_PAIR(10));

    if (isMissionClear()) {
        if (useColor) attron(COLOR_PAIR(9) | A_BOLD);
        else attron(A_BOLD);
        mvprintw(y++, x, "|        *** MISSION CLEAR ***         |");
        mvprintw(y++, x, "+--------------------------------------+");
        if (useColor) attroff(COLOR_PAIR(9) | A_BOLD);
        else attroff(A_BOLD);
    }

    (void)kPanelWidth;
}

// ---------------------------------------------------------------
// drawClearPopup: 스테이지 클리어 (Grade는 팝업에만, 가독성·색 분리)
// ---------------------------------------------------------------
void ScoreBoard::drawClearPopup(int mapWidth, int mapHeight, bool isFinalStage) const {
    const bool useColor = has_colors();
    if (useColor) {
        init_pair(9,  COLOR_CYAN,    COLOR_BLACK);
        init_pair(10, COLOR_WHITE,   COLOR_BLACK);
        init_pair(11, COLOR_YELLOW,  COLOR_BLACK);
        init_pair(12, COLOR_RED,     COLOR_BLACK);
        init_pair(13, COLOR_MAGENTA, COLOR_BLACK);
    }

    const int boxW = 42;
    const int boxH = isFinalStage ? 22 : 17;
    const int cx = std::max(0, (mapWidth * 2 - boxW) / 2);
    const int cy = std::max(0, (mapHeight - boxH) / 2);

    int y = cy;

    auto inner = [&](const char* text, int colorPair, bool bold) {
        if (useColor) {
            attroff(A_STANDOUT);
            attron(COLOR_PAIR(colorPair));
            if (bold) attron(A_BOLD);
        } else if (bold) {
            attron(A_BOLD);
        }
        mvprintw(y++, cx, "| %-40s|", text);
        if (useColor) {
            if (bold) attroff(A_BOLD);
            attroff(COLOR_PAIR(colorPair));
        } else if (bold) {
            attroff(A_BOLD);
        }
    };

    const char* border = "+----------------------------------------+";

    if (useColor) attron(COLOR_PAIR(9) | A_BOLD);
    else attron(A_BOLD);
    mvprintw(y++, cx, "%s", border);
    if (useColor) attroff(COLOR_PAIR(9) | A_BOLD);
    else attroff(A_BOLD);

    char line[96];
    if (isFinalStage) {
        std::snprintf(line, sizeof(line), "     ALL STAGES CLEAR!");
        inner(line, 11, true);
    } else {
        std::snprintf(line, sizeof(line), "     STAGE %d CLEAR!", stage);
        inner(line, 11, true);
    }

    inner("", 10, false);

    int stageTime = (stage >= 1 && stage <= 5) ? stageTimes[stage - 1] : -1;
    if (stageTime >= 0) {
        const char* gNow = getStageGrade(stage);
        std::snprintf(line, sizeof(line), "  Stage time : %02d:%02d",
                      stageTime / 60, stageTime % 60);
        inner(line, 10, false);
        std::snprintf(line, sizeof(line), "  Grade      : %s", gNow);
        inner(line, gradeRowColorPair(gNow[0]), true);
    }

    inner("  ------------------------------", 9, false);

    std::snprintf(line, sizeof(line), "  Mission");
    inner(line, 9, true);

    std::snprintf(line, sizeof(line), "  Body   : %d / %d  (goal %d)",
                  currentLength, maxLength, bodyGoal);
    inner(line, 10, false);

    std::snprintf(line, sizeof(line), "  Growth : %d / %d   %s",
                  growthCount, growthGoal, isGrowthClear() ? "[OK]" : "[  ]");
    inner(line, 10, false);

    std::snprintf(line, sizeof(line), "  Poison : %d / %d   %s",
                  poisonCount, poisonGoal, isPoisonClear() ? "[OK]" : "[  ]");
    inner(line, 10, false);

    std::snprintf(line, sizeof(line), "  Gate   : %d / %d   %s",
                  gateCount, gateGoal, isGateClear() ? "[OK]" : "[  ]");
    inner(line, 10, false);

    inner("", 10, false);

    if (isFinalStage) {
        std::snprintf(line, sizeof(line), "  --- Stage Grades (time) ---");
        inner(line, 9, true);
        for (int i = 1; i <= 5; ++i) {
            const int st = stageTimes[i - 1];
            const char* g = getStageGrade(i);
            if (st >= 0) {
                std::snprintf(line, sizeof(line), "  #%d   %02d:%02d      Grade %s",
                              i, st / 60, st % 60, g);
            } else if (st == -2) {
                std::snprintf(line, sizeof(line), "  #%d   (failed)    Grade %s", i, g);
            } else {
                std::snprintf(line, sizeof(line), "  #%d   --          %s", i, g);
            }
            const char key = (st == -2 || g[0] == '-') ? 'F' : g[0];
            inner(line, gradeRowColorPair(key), false);
        }
        std::snprintf(line, sizeof(line), "  Overall Grade : %s", getOverallGrade());
        const char* og = getOverallGrade();
        inner(line, gradeRowColorPair(og[0]), true);
        inner("", 10, false);
        inner("  Press ENTER to exit.", 11, false);
    } else {
        inner("  Press ENTER for next stage >>", 11, false);
    }

    if (useColor) attron(COLOR_PAIR(9) | A_BOLD);
    else attron(A_BOLD);
    mvprintw(y++, cx, "%s", border);
    if (useColor) attroff(COLOR_PAIR(9) | A_BOLD);
    else attroff(A_BOLD);
}

// ---------------------------------------------------------------
// drawGameOverPopup: 게임 오버 (Grade 요약, 색·너비는 클리어 팝업과 통일)
// ---------------------------------------------------------------
void ScoreBoard::drawGameOverPopup(int mapWidth, int mapHeight) const {
    const bool useColor = has_colors();
    if (useColor) {
        init_pair(9,  COLOR_CYAN,    COLOR_BLACK);
        init_pair(10, COLOR_WHITE,   COLOR_BLACK);
        init_pair(11, COLOR_YELLOW,  COLOR_BLACK);
        init_pair(12, COLOR_RED,     COLOR_BLACK);
        init_pair(13, COLOR_MAGENTA, COLOR_BLACK);
    }

    int completedCount = 0;
    int failedStageIdx = -1;
    for (int i = 0; i < 5; ++i) {
        if (stageTimes[i] >= 0) ++completedCount;
        if (stageTimes[i] == -2) failedStageIdx = i + 1;
    }
    const int resultLines = completedCount + (failedStageIdx >= 0 ? 1 : 0);

    const int boxW = 42;
    const int boxH = 13 + (resultLines > 0 ? resultLines + 3 : 0);
    const int cx = std::max(0, (mapWidth * 2 - boxW) / 2);
    const int cy = std::max(0, (mapHeight - boxH) / 2);

    int y = cy;
    char line[96];

    auto inner = [&](const char* text, int colorPair, bool bold) {
        if (useColor) {
            attroff(A_STANDOUT);
            attron(COLOR_PAIR(colorPair));
            if (bold) attron(A_BOLD);
        } else if (bold) {
            attron(A_BOLD);
        }
        mvprintw(y++, cx, "| %-40s|", text);
        if (useColor) {
            if (bold) attroff(A_BOLD);
            attroff(COLOR_PAIR(colorPair));
        } else if (bold) {
            attroff(A_BOLD);
        }
    };

    const char* border = "+----------------------------------------+";

    if (useColor) attron(COLOR_PAIR(9) | A_BOLD);
    else attron(A_BOLD);
    mvprintw(y++, cx, "%s", border);
    if (useColor) attroff(COLOR_PAIR(9) | A_BOLD);
    else attroff(A_BOLD);

    inner("         GAME OVER!", 12, true);

    inner("", 10, false);

    int mm = elapsedSeconds / 60;
    int ss = elapsedSeconds % 60;
    std::snprintf(line, sizeof(line), "  Total time : %02d:%02d", mm, ss);
    inner(line, 11, false);

    std::snprintf(line, sizeof(line), "  Stage        : %d / 5", stage);
    inner(line, 10, false);

    std::snprintf(line, sizeof(line), "  Body (goal)  : %d / %d  (need %d)",
                  currentLength, maxLength, bodyGoal);
    inner(line, 10, false);

    std::snprintf(line, sizeof(line), "  Growth / goal: %d / %d", growthCount, growthGoal);
    inner(line, 10, false);

    std::snprintf(line, sizeof(line), "  Poison / goal: %d / %d", poisonCount, poisonGoal);
    inner(line, 10, false);

    inner("", 10, false);

    if (resultLines > 0) {
        std::snprintf(line, sizeof(line), "  --- Stage Grades (time) ---");
        inner(line, 9, true);
        for (int i = 1; i <= 5; ++i) {
            const int t = stageTimes[i - 1];
            if (t == -1) continue;
            const char* g = getStageGrade(i);
            if (t >= 0) {
                std::snprintf(line, sizeof(line), "  #%d   %02d:%02d      Grade %s",
                              i, t / 60, t % 60, g);
            } else {
                std::snprintf(line, sizeof(line), "  #%d   (failed)    Grade %s", i, g);
            }
            const char c = (t == -2 || g[0] == '-') ? 'F' : g[0];
            inner(line, gradeRowColorPair(c), t == -2);
        }
        inner("", 10, false);
    }

    inner("  Press ENTER to exit.", 11, false);

    if (useColor) attron(COLOR_PAIR(9) | A_BOLD);
    else attron(A_BOLD);
    mvprintw(y++, cx, "%s", border);
    if (useColor) attroff(COLOR_PAIR(9) | A_BOLD);
    else attroff(A_BOLD);
}
