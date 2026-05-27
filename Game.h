#pragma once
#include "raylib.h"
#include "Player.h"
#include "Enemy.h"
#include "Powerup.h"
#include "Effects.h"
#include "WaveManager.h"
#include <vector>

class Game {
public:
    Game();
    ~Game();
    void Run();

private:
    enum class State { Title, HowTo, Playing, Paused, StageClear, GameOver, Settings, HighScores, NameEntry };
    static constexpr int ScreenW = 480;
    static constexpr int ScreenH = 640;

    struct HighScoreEntry {
        char initials[4];
        int score;
        int loop;
    };

    State state_ = State::Title;
    State returnFromHowTo_ = State::Title;
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
    float stageTime_ = 0.0f;
    int loop_ = 1;
    float clearTimer_ = 0.0f;
    float scrollA_ = 0.0f;
    float scrollB_ = 0.0f;
    bool bossSpawned_ = false;

    // Generated Sounds
    Sound shootSound_{};
    Sound boomSound_{};
    Sound pickupSound_{};
    Sound menuSelectSound_{};
    Sound menuConfirmSound_{};
    Sound playerHitSound_{};
    Sound gameOverSound_{};
    Sound stageClearSound_{};
    Sound bgmSound_{};
    Sound bossKlaxonSound_{};
    bool audioReady_ = false;
    bool soundsReady_ = false;

    // Settings
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
    float bossDeathTimer_ = 0.0f;
    Vector2 bossDeathPos_{};

    // Formation tracking counters
    int formationCount_[10]{};

    void StartGame();
    void ReturnToTitle();
    void NextLoop();
    void Update(float dt);
    void UpdatePlaying(float dt);
    void UpdateSettings();
    void UpdateNameEntry();
    void Draw();
    void DrawBackground() const;
    void DrawHud() const;
    void DrawCenteredText(const char* title, const char* subtitle) const;
    void DrawTitleMenu() const;
    void DrawHowTo() const;
    void DrawHighScores() const;
    void DrawSettings() const;
    void DrawNameEntry() const;
    void HandleCollisions();
    void Cleanup();
    void SpawnDrop(Vector2 pos, EnemyType source);
    bool BossAlive() const;
    void LoadGeneratedSounds();
    void UnloadGeneratedSounds();

    // Settings and High Score Persist Helpers
    void LoadSettings();
    void SaveSettings();
    void LoadHighScores();
    void SaveHighScores();
    bool CheckHighScore(int score) const;
    void InsertHighScore(const char* initials, int score, int loop);

    // Dynamic State Transition Helpers
    void StartTransition(State target);
    void UpdateTransition(float dt);
    void DrawTransition() const;
};

