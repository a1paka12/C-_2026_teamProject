#include "ScoreBoard.hpp"
#include <ncurses.h>
#include <cstdio>
#include <algorithm>

// 생성자: Stage 1 기본값으로 초기화
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
      elapsedSeconds(0) {}

// 스테이지 번호를 설정하고 목표값을 갱신한다. (1~4 범위로 제한)
void ScoreBoard::setStage(int stageIndex1Based) {
    if (stageIndex1Based < 1) stageIndex1Based = 1;
    if (stageIndex1Based > 4) stageIndex1Based = 4;
    stage = stageIndex1Based;
    applyGoalsForStage();
}

// 스테이지별 미션 목표값을 설정한다.
// Stage 1: Body 10, Growth 3, Poison 1, Gate 1
// Stage 2: Body 12, Growth 4, Poison 1, Gate 1
// Stage 3: Body 14, Growth 5, Poison 2, Gate 2
// Stage 4: Body 16, Growth 6, Poison 2, Gate 2
void ScoreBoard::applyGoalsForStage() {
    switch (stage) {
        case 1:
            bodyGoal = 10;
            growthGoal = 3;
            poisonGoal = 1;
            gateGoal = 1;
            break;
        case 2:
            bodyGoal = 12;
            growthGoal = 4;
            poisonGoal = 1;
            gateGoal = 1;
            break;
        case 3:
            bodyGoal = 14;
            growthGoal = 5;
            poisonGoal = 2;
            gateGoal = 2;
            break;
        case 4:
        default:
            bodyGoal = 16;
            growthGoal = 6;
            poisonGoal = 2;
            gateGoal = 2;
            break;
    }
}

// 스테이지 전환 시 점수 카운트를 초기화한다.
// 게임 시간(elapsedSeconds)은 리셋하지 않고 누적한다.
void ScoreBoard::resetForNewStage(int initialSnakeLength) {
    growthCount = 0;
    poisonCount = 0;
    gateCount = 0;
    currentLength = initialSnakeLength;
    maxLength = initialSnakeLength;
    applyGoalsForStage();
}

// 현재 뱀 길이를 갱신하고, 최대 길이도 함께 갱신한다.
void ScoreBoard::updateLength(int len) {
    currentLength = len;
    if (currentLength > maxLength) maxLength = currentLength;
}

// 각 아이템/게이트 획득 시 호출하여 카운트를 증가시킨다.
void ScoreBoard::addGrowth() { ++growthCount; }

void ScoreBoard::addPoison() { ++poisonCount; }

void ScoreBoard::addGate() { ++gateCount; }

// 게임 경과 시간을 설정한다. (음수 방지)
void ScoreBoard::setElapsedSeconds(int seconds) {
    if (seconds < 0) seconds = 0;
    elapsedSeconds = seconds;
}

// 각 미션 달성 여부 판단 함수들.
// Body는 "현재 길이"가 아니라 "게임 중 최대 길이"를 기준으로 판단한다.
bool ScoreBoard::isBodyClear() const { return maxLength >= bodyGoal; }

bool ScoreBoard::isGrowthClear() const { return growthCount >= growthGoal; }

bool ScoreBoard::isPoisonClear() const { return poisonCount >= poisonGoal; }

bool ScoreBoard::isGateClear() const { return gateCount >= gateGoal; }

// 모든 미션이 달성되면 true를 반환한다. (다음 스테이지 진행 조건)
bool ScoreBoard::isMissionClear() const {
    return isBodyClear() && isGrowthClear() && isPoisonClear() && isGateClear();
}

// ---------------------------------------------------------------
// draw: ncurses를 이용해 Score Board를 우측 패널에 출력한다.
//
// 출력 형태:
// +--------------------------------------+
// | Score Board                          |
// | Stage  : 1 / 4                       |
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
// 달성한 항목은 초록색 + 굵게(Bold)로 강조하고, [V]를 표시한다.
// 모든 미션 달성 시 하단에 "*** MISSION CLEAR ***" 메시지를 출력한다.
// ---------------------------------------------------------------
void ScoreBoard::draw(int originCol, int originRow, int gateRespawnCountdown) const {
    const bool useColor = has_colors();

    // 색상 쌍 설정: 9=초록(달성), 10=흰색(기본), 11=노랑(시간)
    if (useColor) {
        init_pair(9, COLOR_GREEN, COLOR_BLACK);
        init_pair(10, COLOR_WHITE, COLOR_BLACK);
        init_pair(11, COLOR_YELLOW, COLOR_BLACK);
    }

    // 경과 시간을 MM:SS 형태로 변환 (한 번 계산해서 그대로 사용 → const)
    const int mm = elapsedSeconds / 60;
    const int ss = elapsedSeconds % 60;

    int y = originRow;          // 줄마다 ++ 하므로 변경 가능
    const int x = originCol;    // 열은 고정

    // ── 상단 제목 영역 ──
    if (useColor) attron(COLOR_PAIR(10));
    mvprintw(y++, x, "+--------------------------------------+");
    mvprintw(y++, x, "| Score Board                          |");

    // 현재 스테이지 번호 표시
    char stageLine[40];
    std::snprintf(stageLine, sizeof(stageLine), "Stage  : %d / 4", stage);
    mvprintw(y++, x, "| %-36s |", stageLine);
    if (useColor) attroff(COLOR_PAIR(10));

    // 게임 경과 시간 표시 (노란색)
    char tbuf[32];
    std::snprintf(tbuf, sizeof(tbuf), "Time   : %02d:%02d", mm, ss);
    if (useColor) attron(COLOR_PAIR(11));
    mvprintw(y++, x, "| %-36s |", tbuf);
    if (useColor) attroff(COLOR_PAIR(11));

    // Gate 20초 주기 교체 직전(남은 1~3초): 우측 패널에 고대비 경고 (맵 위 숫자와 병행)
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

    // 조작키 안내 및 구분선
    mvprintw(y++, x, "| Control: Arrow Keys        Quit: Q   |");
    mvprintw(y++, x, "|--------------------------------------|");

    // ── 점수 테이블 헤더 ──
    mvprintw(y++, x, "| Type      Now        Goal     Clear  |");

    // 각 항목의 달성 여부를 미리 계산 (그 뒤로는 읽기만 함 → const)
    const bool bClear = isBodyClear();
    const bool gClear = isGrowthClear();
    const bool pClear = isPoisonClear();
    const bool gtClear = isGateClear();

    // Body의 Now 열: "현재 길이 / 최대 길이" 형태
    char bodyNow[32];
    std::snprintf(bodyNow, sizeof(bodyNow), "%d / %d", currentLength, maxLength);

    // 점수 테이블 한 행을 출력하는 람다 함수.
    // 달성한 항목은 초록색(COLOR_PAIR 9) + Bold, 미달성은 흰색(COLOR_PAIR 10).
    auto row = [&](const char* label, const char* nowStr, int goal, bool cleared) {
        const char* mark = cleared ? "[V]" : "[ ]";
        if (useColor) {
            const int pair = cleared ? 9 : 10;  // 행을 그리는 동안 색은 고정
            attron(COLOR_PAIR(pair));
            if (cleared) attron(A_BOLD);
            mvprintw(y++, x, "| %-9s %-10s %-7d %-7s |", label, nowStr, goal, mark);
            if (cleared) attroff(A_BOLD);
            attroff(COLOR_PAIR(pair));
        } else {
            mvprintw(y++, x, "| %-9s %-10s %-7d %-7s |", label, nowStr, goal, mark);
        }
    };

    // ── 점수 테이블 본문 (Body / Growth / Poison / Gate) ──
    row("Body", bodyNow, bodyGoal, bClear);

    char gbuf[16];
    char pbuf[16];
    char gtbuf[16];
    std::snprintf(gbuf, sizeof(gbuf), "%d", growthCount);
    std::snprintf(pbuf, sizeof(pbuf), "%d", poisonCount);
    std::snprintf(gtbuf, sizeof(gtbuf), "%d", gateCount);

    row("Growth", gbuf, growthGoal, gClear);
    row("Poison", pbuf, poisonGoal, pClear);
    row("Gate", gtbuf, gateGoal, gtClear);

    // ── 하단 테두리 ──
    if (useColor) attron(COLOR_PAIR(10));
    mvprintw(y++, x, "+--------------------------------------+");
    if (useColor) attroff(COLOR_PAIR(10));

    // 모든 미션 달성 시 "MISSION CLEAR" 메시지를 초록색 Bold로 출력
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
// drawClearPopup: 스테이지 클리어 시 화면 중앙에 결과 팝업을 출력한다.
//
// 출력 형태 (스테이지 1~3 클리어):
// +=========================+
// |   STAGE 1 CLEAR!        |
// |   Time   : 01:35        |
// |   Body   : 5 / 10       |
// |   Growth : 3 / 3   [V]  |
// |   Poison : 1 / 1   [V]  |
// |   Gate   : 1 / 1   [V]  |
// |                         |
// | Press ENTER for next    |
// +=========================+
//
// 마지막 스테이지(4) 클리어 시에는 "ALL STAGES CLEAR!"가 표시된다.
// ---------------------------------------------------------------
void ScoreBoard::drawClearPopup(int mapWidth, int mapHeight, bool isFinalStage) const {
    const bool useColor = has_colors();
    if (useColor) {
        init_pair(9, COLOR_GREEN, COLOR_BLACK);
        init_pair(11, COLOR_YELLOW, COLOR_BLACK);
    }

    const int boxW = 30;
    const int boxH = isFinalStage ? 12 : 11;
    const int cx = std::max(0, (mapWidth * 2 - boxW) / 2);
    const int cy = std::max(0, (mapHeight - boxH) / 2);

    int y = cy;

    // 상단 테두리
    if (useColor) attron(COLOR_PAIR(9) | A_BOLD);
    else attron(A_BOLD);
    mvprintw(y++, cx, "+----------------------------+");

    // 타이틀
    if (isFinalStage) {
        mvprintw(y++, cx, "|   ALL STAGES CLEAR!        |");
    } else {
        char title[40];
        std::snprintf(title, sizeof(title), "   STAGE %d CLEAR!", stage);
        mvprintw(y++, cx, "| %-27s|", title);
    }

    // 구분선
    mvprintw(y++, cx, "|                            |");

    // 이 스테이지에 걸린 시간 (스테이지 시작 시점부터 클리어까지)
    const int mm = elapsedSeconds / 60;
    const int ss = elapsedSeconds % 60;
    char timeLine[40];
    std::snprintf(timeLine, sizeof(timeLine), "   Stage Time : %02d:%02d", mm, ss);
    mvprintw(y++, cx, "| %-27s|", timeLine);

    // Body: 현재길이 / 최대길이 (목표)
    char bodyLine[40];
    std::snprintf(bodyLine, sizeof(bodyLine), "   Body   : %d / %d  (%d)",
                  currentLength, maxLength, bodyGoal);
    mvprintw(y++, cx, "| %-27s|", bodyLine);

    // Growth
    char growthLine[40];
    std::snprintf(growthLine, sizeof(growthLine), "   Growth : %d / %d   %s",
                  growthCount, growthGoal, isGrowthClear() ? "[V]" : "[ ]");
    mvprintw(y++, cx, "| %-27s|", growthLine);

    // Poison
    char poisonLine[40];
    std::snprintf(poisonLine, sizeof(poisonLine), "   Poison : %d / %d   %s",
                  poisonCount, poisonGoal, isPoisonClear() ? "[V]" : "[ ]");
    mvprintw(y++, cx, "| %-27s|", poisonLine);

    // Gate
    char gateLine[40];
    std::snprintf(gateLine, sizeof(gateLine), "   Gate   : %d / %d   %s",
                  gateCount, gateGoal, isGateClear() ? "[V]" : "[ ]");
    mvprintw(y++, cx, "| %-27s|", gateLine);

    // 빈 줄
    mvprintw(y++, cx, "|                            |");

    // 안내 메시지
    if (isFinalStage) {
        mvprintw(y++, cx, "|  Press ENTER to exit.      |");
    } else {
        mvprintw(y++, cx, "|  Press ENTER for next >>   |");
    }

    // 하단 테두리
    mvprintw(y++, cx, "+----------------------------+");

    if (useColor) attroff(COLOR_PAIR(9) | A_BOLD);
    else attroff(A_BOLD);
}

// ---------------------------------------------------------------
// drawGameOverPopup: 게임 오버 시 화면 중앙에 빨간색 팝업을 출력한다.
// ---------------------------------------------------------------
void ScoreBoard::drawGameOverPopup(int mapWidth, int mapHeight) const {
    const bool useColor = has_colors();
    if (useColor) {
        init_pair(12, COLOR_RED, COLOR_BLACK);
    }

    const int boxW = 30;
    const int boxH = 11;
    const int cx = std::max(0, (mapWidth * 2 - boxW) / 2);
    const int cy = std::max(0, (mapHeight - boxH) / 2);

    int y = cy;

    if (useColor) attron(COLOR_PAIR(12) | A_BOLD);
    else attron(A_BOLD);

    mvprintw(y++, cx, "+----------------------------+");
    mvprintw(y++, cx, "|        GAME OVER!          |");
    mvprintw(y++, cx, "|                            |");

    // 경과 시간 (한 번만 계산)
    const int mm = elapsedSeconds / 60;
    const int ss = elapsedSeconds % 60;
    char timeLine[40];
    std::snprintf(timeLine, sizeof(timeLine), "   Time   : %02d:%02d", mm, ss);
    mvprintw(y++, cx, "| %-27s|", timeLine);

    // 스테이지
    char stageLine[40];
    std::snprintf(stageLine, sizeof(stageLine), "   Stage  : %d / 4", stage);
    mvprintw(y++, cx, "| %-27s|", stageLine);

    // Body
    char bodyLine[40];
    std::snprintf(bodyLine, sizeof(bodyLine), "   Body   : %d / %d  (%d)",
                  currentLength, maxLength, bodyGoal);
    mvprintw(y++, cx, "| %-27s|", bodyLine);

    // Growth / Poison / Gate
    char growthLine[40];
    std::snprintf(growthLine, sizeof(growthLine), "   Growth : %d / %d",
                  growthCount, growthGoal);
    mvprintw(y++, cx, "| %-27s|", growthLine);

    char poisonLine[40];
    std::snprintf(poisonLine, sizeof(poisonLine), "   Poison : %d / %d",
                  poisonCount, poisonGoal);
    mvprintw(y++, cx, "| %-27s|", poisonLine);

    mvprintw(y++, cx, "|                            |");
    mvprintw(y++, cx, "|  Press ENTER to exit.      |");
    mvprintw(y++, cx, "+----------------------------+");

    if (useColor) attroff(COLOR_PAIR(12) | A_BOLD);
    else attroff(A_BOLD);
}
