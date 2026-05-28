// 현재 점수 표시, 스테이지 등급 계산 및 클리어/게임오버 팝업 구현부
#include "ScoreBoard.hpp"
#include <ncurses.h>
#include <cstdio>
#include <algorithm>

// ── 등급 기준 (초 미만이면 해당 등급) ──
// [stage-1][0]=A+ [1]=A0 [2]=B+ [3]=B0, 이상이면 C
static const int kThresholds[5][4] = {
    {  30,  60,  90, 120 },  // Stage 1
    {  45,  75, 105, 135 },  // Stage 2
    {  50,  85, 115, 145 },  // Stage 3
    {  55,  90, 120, 150 },  // Stage 4
    {  65, 100,135, 165 },  // Stage 5
};

static const char* kGradeLabels[] = { "A+", "A0", "B+", "B0", "C ", "F ", "--" };

// ── 생성자 ──
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
      elapsedSeconds(0)
{
    for (int i = 0; i < 5; ++i) stageTimes[i] = -1;
}

// ── 스테이지 설정 (1~5) ──
void ScoreBoard::setStage(int s) {
    if (s < 1) s = 1;
    if (s > 5) s = 5;
    stage = s;
    applyGoalsForStage();
}

// ── 스테이지별 미션 목표 ──
void ScoreBoard::applyGoalsForStage() {
    switch (stage) {
        case 1: bodyGoal = 10; growthGoal = 3; poisonGoal = 1; gateGoal = 1; break;
        case 2: bodyGoal = 12; growthGoal = 4; poisonGoal = 1; gateGoal = 1; break;
        case 3: bodyGoal = 14; growthGoal = 5; poisonGoal = 2; gateGoal = 2; break;
        case 4: bodyGoal = 16; growthGoal = 6; poisonGoal = 2; gateGoal = 2; break;
        case 5:
        default:
            bodyGoal = 18; growthGoal = 7; poisonGoal = 3; gateGoal = 3; break;
    }
}

// ── 스테이지 전환 시 카운트 초기화 (stageTimes는 건드리지 않음) ──
void ScoreBoard::resetForNewStage(int initialSnakeLength) {
    growthCount  = 0;
    poisonCount  = 0;
    gateCount    = 0;
    currentLength = initialSnakeLength;
    maxLength     = initialSnakeLength;
    applyGoalsForStage();
}

// ── 클리어 시 현재 스테이지 시간 기록 ──
void ScoreBoard::finishStageTimer() {
    if (stage >= 1 && stage <= 5)
        stageTimes[stage - 1] = elapsedSeconds;
}

// ── 사망 시 현재 스테이지 F 기록 ──
void ScoreBoard::markStageFailed() {
    if (stage >= 1 && stage <= 5)
        stageTimes[stage - 1] = -2;
}

void ScoreBoard::updateLength(int len) {
    currentLength = len;
    if (currentLength > maxLength) maxLength = currentLength;
}

void ScoreBoard::addGrowth() { ++growthCount; }
void ScoreBoard::addPoison() { ++poisonCount; }
void ScoreBoard::addGate()   { ++gateCount; }

void ScoreBoard::setElapsedSeconds(int sec) {
    if (sec < 0) sec = 0;
    elapsedSeconds = sec;
}

bool ScoreBoard::isBodyClear()   const { return maxLength    >= bodyGoal;   }
bool ScoreBoard::isGrowthClear() const { return growthCount  >= growthGoal; }
bool ScoreBoard::isPoisonClear() const { return poisonCount  >= poisonGoal; }
bool ScoreBoard::isGateClear()   const { return gateCount    >= gateGoal;   }

bool ScoreBoard::isMissionClear() const {
    return isBodyClear() && isGrowthClear() && isPoisonClear() && isGateClear();
}

// ── 등급 계산 ──
const char* ScoreBoard::computeGrade(int stageIdx1, int seconds) {
    if (stageIdx1 < 1 || stageIdx1 > 5) return "C ";
    const int* thr = kThresholds[stageIdx1 - 1];
    if (seconds < thr[0]) return "A+";
    if (seconds < thr[1]) return "A0";
    if (seconds < thr[2]) return "B+";
    if (seconds < thr[3]) return "B0";
    return "C ";
}

const char* ScoreBoard::getStageGrade(int stageIdx1) const {
    if (stageIdx1 < 1 || stageIdx1 > 5) return "--";
    // 한 번 읽어와서 분기에만 쓰고 끝나므로 const
    const int t = stageTimes[stageIdx1 - 1];
    if (t == -2) return "F ";
    if (t <   0) return "--";
    return computeGrade(stageIdx1, t);
}

const char* ScoreBoard::getOverallGrade() const {
    int worst = -1;  // 갱신되므로 비-const
    for (int i = 0; i < 5; ++i) {
        const int t = stageTimes[i];                                       // 이 회차의 기록 (안 바뀜)
        if (t == -1) continue;                                             // 미진입은 제외
        const int r = (t == -2) ? 5 : gradeRank(computeGrade(i + 1, t));   // 등급 순위 (안 바뀜)
        if (r > worst) worst = r;
    }
    if (worst < 0) return "--";
    return kGradeLabels[worst];
}

int ScoreBoard::gradeRank(const char* g) {
    for (int i = 0; i < 7; ++i) {
        if (g[0] == kGradeLabels[i][0] && g[1] == kGradeLabels[i][1]) return i;
    }
    return 99;
}

// ncurses 색 페어 번호와 Bold 여부 반환
// 페어 정의: 9=초록, 10=흰색, 11=노랑, 13=빨강
void ScoreBoard::gradeAttr(const char* g, int& colorPair, bool& bold) {
    if      (g[0] == 'A' && g[1] == '+') { colorPair = 9;  bold = true;  }
    else if (g[0] == 'A' && g[1] == '0') { colorPair = 9;  bold = false; }
    else if (g[0] == 'B' && g[1] == '+') { colorPair = 11; bold = true;  }
    else if (g[0] == 'B' && g[1] == '0') { colorPair = 11; bold = false; }
    else if (g[0] == 'C')                 { colorPair = 10; bold = false; }
    else if (g[0] == 'F')                 { colorPair = 13; bold = true;  }
    else                                  { colorPair = 10; bold = false; }
}

// ---------------------------------------------------------------
// draw: 우측 패널에 Score Board + Ranking을 출력한다.
//
// [Score Board 영역]
// +--------------------------------------+
// | Score Board                          |
// | Stage  : 1 / 5                       |
// | Time   : 01:35                       |
// | Control: Arrow Keys        Quit: Q   |
// |--------------------------------------|
// | Type      Now        Goal     Clear  |
// | Body      5 / 8      10       [ ]    |
// | Growth    2          3        [ ]    |
// | Poison    0          1        [ ]    |
// | Gate      1          1        [V]    |
// +--------------------------------------+
//
// [Ranking 영역]
// +--------------------------------------+
// | Ranking                              |
// | St  Time     Grade                   |
// |  1  00:28     A+                     |
// |  2   --        --                    |
// | ...                                  |
// | Overall:       A+                    |
// +--------------------------------------+
// ---------------------------------------------------------------
void ScoreBoard::draw(int originCol, int originRow, int gateRespawnCountdown) const {
    const bool useColor = has_colors();

    if (useColor) {
        init_pair(9,  COLOR_GREEN,  COLOR_BLACK);
        init_pair(10, COLOR_WHITE,  COLOR_BLACK);
        init_pair(11, COLOR_YELLOW, COLOR_BLACK);
        init_pair(13, COLOR_RED,    COLOR_BLACK);
    }

    const int mm = elapsedSeconds / 60;
    const int ss = elapsedSeconds % 60;

    int y = originRow;
    const int x = originCol;

    // ── Score Board 영역 ──
    if (useColor) attron(COLOR_PAIR(10));
    mvprintw(y++, x, "+--------------------------------------+");
    mvprintw(y++, x, "| Score Board                          |");

    char stageLine[40];
    std::snprintf(stageLine, sizeof(stageLine), "Stage  : %d / 5", stage);
    mvprintw(y++, x, "| %-36s |", stageLine);
    if (useColor) attroff(COLOR_PAIR(10));

    char tbuf[32];
    std::snprintf(tbuf, sizeof(tbuf), "Time   : %02d:%02d", mm, ss);
    if (useColor) attron(COLOR_PAIR(11));
    mvprintw(y++, x, "| %-36s |", tbuf);
    if (useColor) attroff(COLOR_PAIR(11));

    // Gate 교체 직전 경고
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

    const bool bClear  = isBodyClear();
    const bool gClear  = isGrowthClear();
    const bool pClear  = isPoisonClear();
    const bool gtClear = isGateClear();

    char bodyNow[32];
    std::snprintf(bodyNow, sizeof(bodyNow), "%d / %d", currentLength, maxLength);

    auto row = [&](const char* label, const char* nowStr, int goal, bool cleared) {
        const char* mark = cleared ? "[V]" : "[ ]";
        if (useColor) {
            const int pair = cleared ? 9 : 10;
            attron(COLOR_PAIR(pair));
            if (cleared) attron(A_BOLD);
            mvprintw(y++, x, "| %-9s %-10s %-7d %-7s |", label, nowStr, goal, mark);
            if (cleared) attroff(A_BOLD);
            attroff(COLOR_PAIR(pair));
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

    // ── Ranking 영역 ──
    y++;  // 한 줄 띄우기
    if (useColor) attron(COLOR_PAIR(10));
    mvprintw(y++, x, "+--------------------------------------+");
    mvprintw(y++, x, "| Ranking                              |");
    mvprintw(y++, x, "|  St  Time     Grade                  |");
    if (useColor) attroff(COLOR_PAIR(10));

    for (int i = 1; i <= 5; ++i) {
        // 이 한 행을 그리는 동안 안 바뀌는 값들이라 const
        const int t = stageTimes[i - 1];
        const char* grade = getStageGrade(i);

        // 시간 문자열 생성
        char timeBuf[16];
        if (t >= 0)
            std::snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", t / 60, t % 60);
        else
            std::snprintf(timeBuf, sizeof(timeBuf), " -- ");

        // 행 전체를 등급 색으로 출력
        if (useColor) {
            int cp; bool bold;
            gradeAttr(grade, cp, bold);
            attron(COLOR_PAIR(cp));
            if (bold) attron(A_BOLD);
            mvprintw(y++, x, "|  %d   %-7s  %-2s                     |",
                     i, timeBuf, grade);
            if (bold) attroff(A_BOLD);
            attroff(COLOR_PAIR(cp));
        } else {
            mvprintw(y++, x, "|  %d   %-7s  %-2s                     |",
                     i, timeBuf, grade);
        }
    }

    // Overall 등급
    const char* overall = getOverallGrade();
    if (useColor) {
        int cp; bool bold;
        gradeAttr(overall, cp, bold);
        attron(COLOR_PAIR(cp));
        if (bold) attron(A_BOLD);
        mvprintw(y++, x, "| Overall:       %-2s                    |", overall);
        if (bold) attroff(A_BOLD);
        attroff(COLOR_PAIR(cp));
    } else {
        mvprintw(y++, x, "| Overall:       %-2s                    |", overall);
    }

    if (useColor) attron(COLOR_PAIR(10));
    mvprintw(y++, x, "+--------------------------------------+");
    if (useColor) attroff(COLOR_PAIR(10));

    (void)kPanelWidth;
}

// ---------------------------------------------------------------
// drawClearPopup: 스테이지 클리어 팝업 (등급 포함)
// ---------------------------------------------------------------
void ScoreBoard::drawClearPopup(int mapWidth, int mapHeight, bool isFinalStage) const {
    const bool useColor = has_colors();
    if (useColor) {
        init_pair(9,  COLOR_GREEN,  COLOR_BLACK);
        init_pair(11, COLOR_YELLOW, COLOR_BLACK);
        init_pair(13, COLOR_RED,    COLOR_BLACK);
    }

    const int boxW = 30;
    const int boxH = isFinalStage ? 13 : 12;
    const int cx = std::max(0, (mapWidth * 2 - boxW) / 2);
    const int cy = std::max(0, (mapHeight - boxH) / 2);

    int y = cy;

    if (useColor) attron(COLOR_PAIR(9) | A_BOLD);
    else attron(A_BOLD);
    mvprintw(y++, cx, "+----------------------------+");

    if (isFinalStage) {
        mvprintw(y++, cx, "|   ALL STAGES CLEAR!        |");
    } else {
        char title[40];
        std::snprintf(title, sizeof(title), "   STAGE %d CLEAR!", stage);
        mvprintw(y++, cx, "| %-27s|", title);
    }
    mvprintw(y++, cx, "|                            |");
    if (useColor) attroff(COLOR_PAIR(9) | A_BOLD);
    else attroff(A_BOLD);

    // 클리어 시간
    const int mm = elapsedSeconds / 60;
    const int ss = elapsedSeconds % 60;
    char timeLine[40];
    std::snprintf(timeLine, sizeof(timeLine), "   Time   : %02d:%02d", mm, ss);
    if (useColor) attron(COLOR_PAIR(11));
    mvprintw(y++, cx, "| %-27s|", timeLine);
    if (useColor) attroff(COLOR_PAIR(11));

    // 이 스테이지 등급
    const char* grade = getStageGrade(stage);
    {
        char gradeLine[40];
        std::snprintf(gradeLine, sizeof(gradeLine), "   Grade  : %s", grade);
        if (useColor) {
            int cp; bool bold;
            gradeAttr(grade, cp, bold);
            attron(COLOR_PAIR(cp));
            if (bold) attron(A_BOLD);
            mvprintw(y++, cx, "| %-27s|", gradeLine);
            if (bold) attroff(A_BOLD);
            attroff(COLOR_PAIR(cp));
        } else {
            mvprintw(y++, cx, "| %-27s|", gradeLine);
        }
    }

    // 미션 결과
    char bodyLine[40], growthLine[40], poisonLine[40], gateLine[40];
    std::snprintf(bodyLine,   sizeof(bodyLine),
                  "   Body   : %d / %d  (%d)", currentLength, maxLength, bodyGoal);
    std::snprintf(growthLine, sizeof(growthLine),
                  "   Growth : %d / %d   %s", growthCount, growthGoal,
                  isGrowthClear() ? "[V]" : "[ ]");
    std::snprintf(poisonLine, sizeof(poisonLine),
                  "   Poison : %d / %d   %s", poisonCount, poisonGoal,
                  isPoisonClear() ? "[V]" : "[ ]");
    std::snprintf(gateLine,   sizeof(gateLine),
                  "   Gate   : %d / %d   %s", gateCount, gateGoal,
                  isGateClear() ? "[V]" : "[ ]");

    if (useColor) attron(COLOR_PAIR(9) | A_BOLD);
    else attron(A_BOLD);
    mvprintw(y++, cx, "| %-27s|", bodyLine);
    mvprintw(y++, cx, "| %-27s|", growthLine);
    mvprintw(y++, cx, "| %-27s|", poisonLine);
    mvprintw(y++, cx, "| %-27s|", gateLine);
    mvprintw(y++, cx, "|                            |");

    if (isFinalStage) {
        // 전체 종합 등급 표시
        const char* overall = getOverallGrade();
        char oLine[40];
        std::snprintf(oLine, sizeof(oLine), "   Overall: %s", overall);
        if (useColor) attroff(COLOR_PAIR(9) | A_BOLD);
        if (useColor) {
            int cp; bool bold;
            gradeAttr(overall, cp, bold);
            attron(COLOR_PAIR(cp));
            if (bold) attron(A_BOLD);
            mvprintw(y++, cx, "| %-27s|", oLine);
            if (bold) attroff(A_BOLD);
            attroff(COLOR_PAIR(cp));
        } else {
            mvprintw(y++, cx, "| %-27s|", oLine);
        }
        if (useColor) attron(COLOR_PAIR(9) | A_BOLD);
        else attron(A_BOLD);
        mvprintw(y++, cx, "|  Press ENTER to exit.      |");
    } else {
        mvprintw(y++, cx, "|  Press ENTER for next >>   |");
    }

    mvprintw(y++, cx, "+----------------------------+");
    if (useColor) attroff(COLOR_PAIR(9) | A_BOLD);
    else attroff(A_BOLD);
}

// ---------------------------------------------------------------
// drawGameOverPopup: 게임 오버 팝업 (등급 포함)
// ---------------------------------------------------------------
void ScoreBoard::drawGameOverPopup(int mapWidth, int mapHeight) const {
    const bool useColor = has_colors();
    if (useColor) {
        init_pair(13, COLOR_RED, COLOR_BLACK);
        init_pair(9,  COLOR_GREEN,  COLOR_BLACK);
        init_pair(11, COLOR_YELLOW, COLOR_BLACK);
        init_pair(10, COLOR_WHITE,  COLOR_BLACK);
    }

    const int boxW = 30;
    const int boxH = 13;
    const int cx = std::max(0, (mapWidth * 2 - boxW) / 2);
    const int cy = std::max(0, (mapHeight - boxH) / 2);

    int y = cy;

    if (useColor) attron(COLOR_PAIR(13) | A_BOLD);
    else attron(A_BOLD);

    mvprintw(y++, cx, "+----------------------------+");
    mvprintw(y++, cx, "|        GAME OVER!          |");
    mvprintw(y++, cx, "|                            |");

    const int mm = elapsedSeconds / 60;
    const int ss = elapsedSeconds % 60;
    char timeLine[40];
    std::snprintf(timeLine, sizeof(timeLine), "   Time   : %02d:%02d", mm, ss);
    mvprintw(y++, cx, "| %-27s|", timeLine);

    char stageLine[40];
    std::snprintf(stageLine, sizeof(stageLine), "   Stage  : %d / 5", stage);
    mvprintw(y++, cx, "| %-27s|", stageLine);

    char bodyLine[40];
    std::snprintf(bodyLine, sizeof(bodyLine), "   Body   : %d / %d  (%d)",
                  currentLength, maxLength, bodyGoal);
    mvprintw(y++, cx, "| %-27s|", bodyLine);

    char growthLine[40], poisonLine[40];
    std::snprintf(growthLine, sizeof(growthLine), "   Growth : %d / %d", growthCount, growthGoal);
    std::snprintf(poisonLine, sizeof(poisonLine), "   Poison : %d / %d", poisonCount, poisonGoal);
    mvprintw(y++, cx, "| %-27s|", growthLine);
    mvprintw(y++, cx, "| %-27s|", poisonLine);

    mvprintw(y++, cx, "|                            |");
    if (useColor) attroff(COLOR_PAIR(13) | A_BOLD);
    else attroff(A_BOLD);

    // 현재 스테이지 등급 (F)
    const char* grade = getStageGrade(stage);
    char gradeLine[40];
    std::snprintf(gradeLine, sizeof(gradeLine), "   Grade  : %s", grade);
    if (useColor) {
        int cp; bool bold;
        gradeAttr(grade, cp, bold);
        attron(COLOR_PAIR(cp));
        if (bold) attron(A_BOLD);
        mvprintw(y++, cx, "| %-27s|", gradeLine);
        if (bold) attroff(A_BOLD);
        attroff(COLOR_PAIR(cp));
    } else {
        mvprintw(y++, cx, "| %-27s|", gradeLine);
    }

    // Overall 등급
    const char* overall = getOverallGrade();
    char oLine[40];
    std::snprintf(oLine, sizeof(oLine), "   Overall: %s", overall);
    if (useColor) {
        int cp; bool bold;
        gradeAttr(overall, cp, bold);
        attron(COLOR_PAIR(cp));
        if (bold) attron(A_BOLD);
        mvprintw(y++, cx, "| %-27s|", oLine);
        if (bold) attroff(A_BOLD);
        attroff(COLOR_PAIR(cp));
    } else {
        mvprintw(y++, cx, "| %-27s|", oLine);
    }

    if (useColor) attron(COLOR_PAIR(13) | A_BOLD);
    else attron(A_BOLD);
    mvprintw(y++, cx, "|  Press ENTER to exit.      |");
    mvprintw(y++, cx, "+----------------------------+");
    if (useColor) attroff(COLOR_PAIR(13) | A_BOLD);
    else attroff(A_BOLD);
}
