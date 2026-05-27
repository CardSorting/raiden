#pragma once

#include "Powerup.h"
#include "Player.h"
#include "raylib.h"
#include <array>
#include <vector>

enum class ExplosionSize { Small, Medium, Large };
enum class EnemyShotType { Basic, Strong };
enum class PickupType { Powerup, WeaponSwitch, WeaponUpgrade, Bomb, Medal };

class AudioSystem {
public:
    enum class MusicTrack { None, Title, Stage, Boss };
    enum class Cue {
        VulcanShot,
        PlasmaShot,
        MissileLaunch,
        EnemyBullet,
        EnemyStrongShot,
        ExplosionSmall,
        ExplosionMedium,
        ExplosionLarge,
        BossDamage,
        BossPhaseChange,
        BossDefeat,
        PlayerHit,
        PlayerDeath,
        Respawn,
        Bomb,
        BombClear,
        PickupPowerup,
        PickupWeaponSwitch,
        PickupWeaponUpgrade,
        PickupBomb,
        PickupMedal,
        ScoreMilestone,
        ExtraLife,
        MenuMove,
        MenuConfirm,
        MenuCancel,
        InsertCoin,
        PressStart,
        Pause,
        Resume,
        ContinueTick,
        HighScoreTick,
        HighScoreEntry,
        StageStart,
        StageClear,
        GameOver,
        BossWarning,
        BossEntrance,
        LowLife,
        Victory,
        AttractShimmer,
        Denied,
        Count
    };

    AudioSystem() = default;
    ~AudioSystem();

    bool Init();
    void Shutdown();
    void Update();
    bool Ready() const { return ready_ && soundsReady_; }

    void SetVolumes(int sfxVolume, int musicVolume);
    void SetMasterVolume(float volume);

    void PlayMusic(MusicTrack track);
    void StopMusic();

    void PlayPlayerShot(WeaponType weapon);
    void PlayEnemyShot(EnemyShotType type);
    void PlayExplosion(ExplosionSize size);
    void PlayBossDamage();
    void PlayPlayerDeath();
    void PlayPlayerHit();
    void PlayRespawn();
    void PlayBomb();
    void PlayBombClear();
    void PlayEnemyBullet();
    void PlayWeaponSwitch();
    void PlayWeaponUpgrade();
    void PlayPickup(PowerupType type);
    void PlayPickup(PickupType type);
    void PlayScoreMilestone();
    void PlayExtraLife();
    void PlayMenuMove();
    void PlayMenuConfirm();
    void PlayMenuCancel();
    void PlayInsertCoin();
    void PlayPressStart();
    void PlayPause();
    void PlayResume();
    void PlayContinueTick();
    void PlayHighScoreTick();
    void PlayHighScoreEntry();
    void PlayStageStart();
    void PlayStageClear();
    void PlayVictory();
    void PlayGameOver();
    void PlayBossWarning();
    void PlayBossEntrance();
    void PlayBossPhaseChange();
    void PlayBossDefeat();
    void PlayLowLifeWarning();
    void PlayAttractShimmer();
    void PlayDenied();
    void PlaySettingsTick();
    void RunChaosAudit(float seconds = 4.0f);

private:
    enum class Category { Player, Enemy, Explosion, Pickup, UI, Warning, Music };
    enum class Priority { Low, Normal, High, Critical };
    static constexpr int CueCount = static_cast<int>(Cue::Count);

    struct CueBank {
        std::vector<Sound> variants;
        Category category = Category::UI;
        Priority priority = Priority::Normal;
        float baseGain = 1.0f;
        float cooldown = 0.0f;
        double lastPlayed = -1000.0;
    };

    std::array<CueBank, CueCount> cues_{};
    std::array<Sound, 3> music_{};
    MusicTrack currentMusic_ = MusicTrack::None;
    bool ready_ = false;
    bool soundsReady_ = false;
    float masterVolume_ = 1.0f;
    float sfxMaster_ = 0.8f;
    float musicMaster_ = 0.6f;
    double musicDuckUntil_ = -1000.0;
    float currentMusicGain_ = 1.0f;
    unsigned int randomSeed_ = 0x51434452u;
    static constexpr int MaxSfxVoices = 14;

    void LoadCues();
    void UnloadCues();
    bool AddCue(Cue cue, Category category, Priority priority, float baseGain, float cooldown, int variants);
    void PlayCue(Cue cue, float pitch = 1.0f, float gain = 1.0f);
    void ApplyVolumes();
    void ApplyMusicVolume();
    int CountPlaying() const;
    int CountPlaying(Category category) const;
    int VoiceBudget(Category category) const;
    int PriorityRank(Priority priority) const;
    float CongestionGain(int activeVoices, int activeInCategory, Priority priority) const;
    float CategoryHeadroom(Category category, Priority priority) const;
    float RandomPan(Category category);
    bool StealLowerPriorityVoice(Priority incomingPriority, Category incomingCategory);
    float CategoryGain(Category category) const;
    float NextRandom();
    float RandomRange(float lo, float hi);
    Sound& MusicSound(MusicTrack track);
};
