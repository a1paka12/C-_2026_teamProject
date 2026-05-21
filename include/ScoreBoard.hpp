#ifndef SCORE_BOARD_HPP
#define SCORE_BOARD_HPP

class ScoreBoard {
public:
    ScoreBoard();

    // 현재 스테이지 번호를 설정한다. (1~5)
    // 내부적으로 해당 스테이지의 목표값(Goal)도 함께 갱신된다.
    void setStage(int stageIndex1Based);

    // 스테이지가 전환될 때 점수 카운트를 초기화한다.
    // Growth/Poison/Gate 횟수는 0으로 리셋되고, 뱀 길이는 초기값으로 설정된다.
    void resetForNewStage(int initialSnakeLength);

    // 스테이지 클리어 시 호출: 현재 elapsedSeconds를 해당 스테이지 기록으로 저장한다.
    void finishStageTimer();

    // 게임 오버(사망) 시 호출: 현재 스테이지를 F로 기록한다.
    void markStageFailed();

    // 현재 뱀의 길이를 갱신한다.
    void updateLength(int currentLength);

    void addGrowth();
    void addPoison();
    void addGate();

    // 게임 경과 시간(초 단위)을 갱신한다.
    void setElapsedSeconds(int seconds);

    bool isBodyClear()   const;
    bool isGrowthClear() const;
    bool isPoisonClear() const;
    bool isGateClear()   const;
    bool isMissionClear() const;

    // stageIdx1(1~5) 스테이지의 등급을 반환한다.
    // -1(미진입) → "--", -2(실패) → "F ", 완료 → "A+"~"C "
    const char* getStageGrade(int stageIdx1) const;

    // 완료/실패한 스테이지 중 가장 낮은 등급을 반환한다.
    const char* getOverallGrade() const;

    void draw(int originCol, int originRow, int gateRespawnCountdown = 0) const;
    void drawClearPopup(int mapWidth, int mapHeight, bool isFinalStage) const;
    void drawGameOverPopup(int mapWidth, int mapHeight) const;

    int getStage() const { return stage; }

private:
    void applyGoalsForStage();

    // seconds < 기준이면 해당 등급 반환 (A+/A0/B+/B0/C )
    static const char* computeGrade(int stageIdx1, int seconds);
    // 등급 비교용 숫자 (낮을수록 좋음, "--"는 99로 취급)
    static int gradeRank(const char* grade);
    // 등급에 맞는 ncurses 색 페어 번호와 Bold 여부를 반환한다.
    static void gradeAttr(const char* grade, int& colorPair, bool& bold);

    int stage;
    int currentLength;
    int maxLength;

    int growthCount;
    int poisonCount;
    int gateCount;

    int bodyGoal;
    int growthGoal;
    int poisonGoal;
    int gateGoal;

    int elapsedSeconds;

    // 0-indexed. 값: -1=미진입, -2=실패(F), >=0=클리어 소요 시간(초)
    int stageTimes[5];

    static constexpr int kPanelWidth = 40;
};

#endif
