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
        FormationBonus,
        ExtraLife,
        MenuMove,
        MenuConfirm,
        MenuCancel,
        InsertCoin,
        PressStart,
        Pause,
        Resume,
        ContinueTick,
        BonusTick,
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
    void SetMusicDucked(bool ducked);

    void PlayMusic(MusicTrack track);
    void StopMusic();

    void PlayPlayerShot(WeaponType weapon);
    void PlayEnemyShot(EnemyShotType type);
    void PlayEnemyShotAt(EnemyShotType type, float x, float screenWidth);
    void PlayExplosion(ExplosionSize size);
    void PlayExplosionAt(ExplosionSize size, float x, float screenWidth);
    void PlayBossDamage();
    void PlayBossDamageAt(float x, float screenWidth);
    void PlayPlayerDeath();
    void PlayPlayerHit();
    void PlayRespawn();
    void PlayBomb();
    void PlayBombClear();
    void PlayEnemyBullet();
    void PlayWeaponSwitch();
    void PlayWeaponUpgrade();
    void PlayMedal(int chain);
    void PlayMedalAt(int chain, float x, float screenWidth);
    void PlayPickup(PowerupType type);
    void PlayPickupAt(PowerupType type, float x, float screenWidth);
    void PlayPickup(PickupType type);
    void PlayPickupAt(PickupType type, float x, float screenWidth);
    void PlayScoreMilestone();
    void PlayFormationBonus();
    void PlayFormationBonusAt(float x, float screenWidth);
    void PlayExtraLife();
    void PlayMenuMove();
    void PlayMenuConfirm();
    void PlayMenuCancel();
    void PlayInsertCoin();
    void PlayPressStart();
    void PlayPause();
    void PlayResume();
    void PlayContinueTick();
    void PlayBonusTick(int step = 0);
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
    static bool ExportProceduralBank(const char* directory);

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
    bool musicManuallyDucked_ = false;
    unsigned int randomSeed_ = 0x51434452u;
    int runtimePlayed_ = 0;
    int runtimeCooldownDrops_ = 0;
    int runtimePressureDrops_ = 0;
    int runtimeStolenVoices_ = 0;
    int runtimeFocusStops_ = 0;
    static constexpr int MaxSfxVoices = 14;

    void LoadCues();
    void UnloadCues();
    bool AddCue(Cue cue, Category category, Priority priority, float baseGain, float cooldown, int variants);
    void PlayCue(Cue cue, float pitch = 1.0f, float gain = 1.0f, float pan = -1.0f);
    void ApplyVolumes();
    void ApplyMusicVolume();
    int CountPlaying() const;
    int CountPlaying(Category category) const;
    int VoiceBudget(Category category) const;
    int PriorityRank(Priority priority) const;
    float CongestionGain(int activeVoices, int activeInCategory, Priority priority) const;
    float CategoryHeadroom(Category category, Priority priority) const;
    float RandomPan(Category category);
    float PositionPan(float x, float screenWidth, Category category);
    bool StealLowerPriorityVoice(Priority incomingPriority, Category incomingCategory);
    void StopVoicesForFocus(Priority minimumPriorityToKeep, bool keepUi, bool keepWarning);
    void ResetRuntimeStats();
    float CategoryGain(Category category) const;
    float NextRandom();
    float RandomRange(float lo, float hi);
    Sound& MusicSound(MusicTrack track);
};
