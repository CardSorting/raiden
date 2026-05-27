#pragma once
#include "raylib.h"
#include "Player.h"
#include "Enemy.h"
#include "Powerup.h"
#include "Effects.h"
#include "WaveManager.h"
#include "AudioSystem.h"
#include <vector>

class Game {
public:
    Game();
    ~Game();
    void Run();

private:
    enum class State { Title, HowTo, Playing, Paused, StageClear, GameOver, Settings, HighScores, NameEntry, ExitConfirm, ClearScoresConfirm, DifficultySelect, Continue };
    static constexpr int ScreenW = 480;
    static constexpr int ScreenH = 640;

    struct HighScoreEntry {
        char initials[4];
        int score;
        int loop;
    };

    State state_ = State::Title;
    State returnFromHowTo_ = State::Title;
    State returnFromSettings_ = State::Title;
    State returnFromExitConfirm_ = State::Title;
    Player player_;
    std::vector<Bullet> playerBullets_;
    std::vector<Bullet> enemyBullets_;
    std::vector<Enemy> enemies_;
    std::vector<Powerup> powerups_;
    Effects effects_;
    WaveManager waves_;
    bool debug_ = false;
    bool shouldExit_ = false;
    int titleSelection_ = 0;
    int pauseSelection_ = 0;
    int gameOverSelection_ = 0;
    int exitConfirmSelection_ = 0;
    int clearScoresSelection_ = 0;
    float stageTime_ = 0.0f;
    int loop_ = 1;
    float clearTimer_ = 0.0f;
    float stageClearTickTimer_ = 0.0f;
    int stageClearTicksPlayed_ = 0;
    int stageClearBonus_ = 0;
    float gameOverTimer_ = 0.0f;
    int credits_ = 0;
    int difficulty_ = 0; // 0 = Normal, 1 = Ace
    float continueTimer_ = 0.0f;
    int difficultySelection_ = 0;

    enum class InputType { KeyboardGamepad, Mouse };
    InputType lastInputType_ = InputType::KeyboardGamepad;
    Vector2 lastMousePos_ = { -1.0f, -1.0f };
    float scrollA_ = 0.0f;
    float scrollB_ = 0.0f;
    bool bossSpawned_ = false;
    bool bossPhaseChanged_ = false;

    AudioSystem audio_;

    // Settings
    int masterVolume_ = 9;
    int sfxVolume_ = 8;
    int bgmVolume_ = 6;
    bool screenShakeEnabled_ = true;
    bool hitboxOverlayEnabled_ = false;
    bool isFullscreen_ = false;
    int controlLayout_ = 0;
    int settingsSelection_ = 0;

    // Leaderboard Data
    std::vector<HighScoreEntry> highScores_;
    char nameEntryBuf_[4] = "AAA";
    int nameEntryIndex_ = 0;

    // Attract Mode & Transitions
    float titleTimer_ = 0.0f;
    bool demoMode_ = false;
    float transitionAlpha_ = 0.0f;
    bool transitioning_ = false;
    float transitionTimer_ = 0.0f;
    State transitionTarget_ = State::Title;
    float tutorialTimer_ = 0.0f;
    float settingsFeedbackTimer_ = 0.0f;
    const char* settingsFeedbackText_ = nullptr;
    Color settingsFeedbackColor_ = WHITE;

    // Starfield Parallax Data
    struct Star {
        Vector2 pos;
        float speed;
        Color color;
    };
    static constexpr int NumStars = 80;
    Star stars_[NumStars]{};

    // Boss Warning & Death Sequence state fields
    float bossWarningTimer_ = 0.0f;
    float bossWarningKlaxonTimer_ = 0.0f;
    float bossDeathTimer_ = 0.0f;
    float bossDeathExplosionTimer_ = 0.0f;
    Vector2 bossDeathPos_{};
    float lowLifeAudioTimer_ = 0.0f;
    float enemyShotAudioTimer_ = 0.0f;
    float playerShotShakeTimer_ = 0.0f;
    float medalChainTimer_ = 0.0f;
    int medalChain_ = 0;

    // Visual Scaling, CRT Shader, and Cabinet Bezels
    bool crtShaderEnabled_ = true;
    bool cleanPixelMode_ = false;
    int crtCurvature_ = 3;
    int crtScanline_ = 3;
    int crtMask_ = 4;
    int crtBloom_ = 2;
    int crtVignette_ = 3;
    int crtGlare_ = 2;
    int aspectMode_ = 0; // 0 = Fit, 1 = Integer, 2 = Stretch
    bool drawBezel_ = true;
    RenderTexture2D screenTarget_{};
    Shader crtShader_{};

    // Parallax background asteroid & cloud structures
    struct AsteroidInstance {
        Vector2 pos;
        float speed;
        float rotation;
        float spinSpeed;
        float scale;
    };
    struct CloudInstance {
        Vector2 pos;
        float speed;
        float scale;
    };
    std::vector<AsteroidInstance> backgroundAsteroids_;
    std::vector<CloudInstance> backgroundClouds_;
    float nebulaScroll_ = 0.0f;

    struct BackgroundLaser {
        Vector2 start;
        Vector2 end;
        float life;
        float maxLife;
        Color color;
    };
    struct BackgroundSpark {
        Vector2 pos;
        Vector2 vel;
        float life;
        float maxLife;
        Color color;
    };
    std::vector<BackgroundLaser> bgLasers_;
    std::vector<BackgroundSpark> bgSparks_;
    float spaceStationScrollY_ = 0.0f;
    float screenFlashTimer_ = 0.0f;

    void DrawCabinetBezel(float rx, float ry, float rw, float rh) const;

    // Formation tracking counters
    int formationCount_[10]{};
    int nextScoreMilestone_ = 10000;
    int nextExtraLifeScore_ = 50000;


    void StartGame();

    void ReturnToTitle();
    void NextLoop();
    void Update(float dt);
    void UpdatePlaying(float dt);
    void UpdateSettings();
    void UpdateNameEntry();
    void UpdateExitConfirm();
    void UpdateClearScoresConfirm();
    void UpdateDifficultySelect();
    void UpdateContinue(float dt);
    void Draw();
    void DrawBackground() const;
    void DrawHud() const;
    void DrawCenteredText(const char* title, const char* subtitle) const;
    void DrawTitleMenu() const;
    void DrawHowTo() const;
    void DrawHighScores() const;
    void DrawSettings() const;
    void DrawNameEntry() const;
    void DrawExitConfirm() const;
    void DrawClearScoresConfirm() const;
    void DrawDifficultySelect() const;
    void DrawContinue() const;
    void HandleCollisions();
    void Cleanup();
    void SpawnDrop(Vector2 pos, EnemyType source);
    bool BossAlive() const;
    void CheckScoreMilestones();
    // Settings and High Score Persist Helpers
    void LoadSettings();
    void ApplyVolumeSettings();
    void SaveSettings();
    void ResetSettingsToDefault();
    void ClearScores();
    void LoadHighScores();
    void SaveHighScores();
    bool CheckHighScore(int score) const;
    void InsertHighScore(const char* initials, int score, int loop);

    // Dynamic State Transition Helpers
    void StartTransition(State target);
    void UpdateTransition(float dt);
    void DrawTransition() const;
};
