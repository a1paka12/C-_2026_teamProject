#ifndef SCORE_BOARD_HPP
#define SCORE_BOARD_HPP

class ScoreBoard {
public:
    ScoreBoard();

    // 현재 스테이지 번호를 설정한다. (1~5)
    void setStage(int stageIndex1Based);

    // 스테이지 전환 시 점수/타이머를 초기화한다.
    void resetForNewStage(int initialSnakeLength);

    // 현재 스테이지 클리어 시 호출: 스테이지 소요 시간을 기록한다.
    void finishStageTimer();

    // 게임 오버 시 호출: 현재 스테이지를 F 등급으로 기록한다.
    void markStageFailed();

    void updateLength(int currentLength);
    void addGrowth();
    void addPoison();
    void addGate();
    void setElapsedSeconds(int seconds);

    bool isBodyClear() const;
    bool isGrowthClear() const;
    bool isPoisonClear() const;
    bool isGateClear() const;
    bool isMissionClear() const;

    // 완료한 스테이지의 Grade 문자열 (stageIdx1: 1~5, 미완료 "--")
    const char* getStageGrade(int stageIdx1) const;

    // 완료·기록된 스테이지 중 가장 낮은 Grade
    const char* getOverallGrade() const;

    void draw(int originCol, int originRow, int gateRespawnCountdown = 0) const;
    void drawClearPopup(int mapWidth, int mapHeight, bool isFinalStage) const;
    void drawGameOverPopup(int mapWidth, int mapHeight) const;

    int getStage() const { return stage; }

private:
    void applyGoalsForStage();
    static const char* computeGrade(int stageIdx1, int seconds);
    static int gradeRank(const char* grade);  // 등급 비교용 숫자 (낮을수록 좋음)

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
    int stageStartSec;   // 현재 스테이지 시작 시점의 elapsedSeconds
    int stageTimes[5];   // 0-indexed, 완료 소요 시간(초), -1=미진입, -2=실패(F)

    static constexpr int kPanelWidth = 40;
};

#endif
