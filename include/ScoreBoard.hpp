#ifndef SCORE_BOARD_HPP
#define SCORE_BOARD_HPP

// 5단계: 우측 패널에 점수(Score Board)와 미션(Mission)을 통합 표시하는 클래스.
// 과제 기본안의 Score Board + Mission을 하나의 표로 합쳐서,
// 현재 상태(Now), 목표값(Goal), 달성 여부(Clear)를 한눈에 볼 수 있도록 구성한다.
class ScoreBoard {
public:
    // 생성자: 모든 점수와 목표값을 기본값(Stage 1)으로 초기화한다.
    ScoreBoard();

    // 현재 스테이지 번호를 설정한다. (1~4)
    // 내부적으로 해당 스테이지의 목표값(Goal)도 함께 갱신된다.
    void setStage(int stageIndex1Based);

    // 스테이지가 전환될 때 점수 카운트를 초기화한다.
    // Growth/Poison/Gate 횟수는 0으로 리셋되고, 뱀 길이는 초기값으로 설정된다.
    void resetForNewStage(int initialSnakeLength);

    // 현재 뱀의 길이를 갱신한다.
    // 현재 길이가 최대 길이보다 크면 최대 길이도 함께 갱신된다.
    void updateLength(int currentLength);

    // Growth Item을 획득했을 때 호출한다. 획득 수가 1 증가한다.
    void addGrowth();

    // Poison Item을 획득했을 때 호출한다. 획득 수가 1 증가한다.
    void addPoison();

    // Gate를 통과했을 때 호출한다. 사용 횟수가 1 증가한다.
    void addGate();

    // 게임 경과 시간(초 단위)을 갱신한다. Score Board에서 MM:SS 형태로 표시된다.
    void setElapsedSeconds(int seconds);

    // 각 항목별 미션 달성 여부를 반환한다.
    // Body는 게임 중 최대 길이(maxLength)가 목표 이상인지로 판단한다.
    bool isBodyClear() const;
    bool isGrowthClear() const;
    bool isPoisonClear() const;
    bool isGateClear() const;

    // 모든 미션(Body, Growth, Poison, Gate)이 달성되었는지 반환한다.
    // true이면 다음 스테이지로 진행할 수 있다.
    bool isMissionClear() const;

    // ncurses를 이용해 Score Board를 화면에 출력한다.
    // originCol: 패널 시작 열 (맵 오른쪽), originRow: 패널 시작 행
    // gateRespawnCountdown: Gate 위치 교체 직전 남은 초(1~3)일 때 경고 한 줄 표시, 0이면 생략
    void draw(int originCol, int originRow, int gateRespawnCountdown = 0) const;

    // 스테이지 클리어 시 화면 중앙에 초록색 결과 팝업을 출력한다.
    // isFinalStage가 true이면 "ALL STAGES CLEAR" 메시지를 출력한다.
    void drawClearPopup(int mapWidth, int mapHeight, bool isFinalStage) const;

    // 게임 오버 시 화면 중앙에 빨간색 팝업을 출력한다.
    void drawGameOverPopup(int mapWidth, int mapHeight) const;

    // 현재 스테이지 번호를 반환한다.
    int getStage() const { return stage; }

private:
    // 현재 스테이지에 맞는 목표값(bodyGoal 등)을 설정하는 내부 함수
    void applyGoalsForStage();

    int stage;           // 현재 스테이지 번호 (1~4)
    int currentLength;   // 뱀의 현재 길이
    int maxLength;       // 게임 중 기록된 뱀의 최대 길이

    int growthCount;     // Growth Item 획득 횟수
    int poisonCount;     // Poison Item 획득 횟수
    int gateCount;       // Gate 사용 횟수

    int bodyGoal;        // Body 미션 목표 (최대 길이가 이 이상이면 달성)
    int growthGoal;      // Growth 미션 목표
    int poisonGoal;      // Poison 미션 목표
    int gateGoal;        // Gate 미션 목표

    int elapsedSeconds;  // 게임 경과 시간 (초)

    static constexpr int kPanelWidth = 40; // Score Board 패널의 가로 폭 (문자 수)
};

#endif
