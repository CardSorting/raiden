#pragma once
#include "Enemy.h"
#include <vector>

enum class StageSection {
    Opening,
    PatternIntro,
    Reinforcement,
    Combination,
    Recovery,
    MidStageSpike,
    BossRunway
};

enum class EncounterType {
    Orientation,
    Sweep,
    Intercept,
    Pincer,
    CorridorCompression,
    Crossfire,
    ChasePressure,
    ArtilleryControl,
    BulletCurtain,
    Recovery,
    BossRunway
};

enum class StageTransition {
    Calm,
    Pressure,
    Release,
    Anticipation,
    Spike,
    Silence,
    Climax
};

class StageDirector {
public:
    void Reset();
    void Update(float stageTime, int loop, std::vector<Enemy>& enemies);
    bool ShouldSpawnBoss(float stageTime, bool bossAlive) const;
    float RouteDuration() const;
    int CurrentBlockIndex(float stageTime) const;
    float CurrentBlockElapsed(float stageTime) const;
    const char* CurrentSectionName(float stageTime) const;
    const char* CurrentEncounterName(float stageTime) const;
    const char* CurrentBlockName(float stageTime) const;
    const char* CurrentBlockPurpose(float stageTime) const;
    const char* CurrentBlockComposition(float stageTime) const;
    const char* CurrentEscalationIntent(float stageTime) const;
    const char* CurrentTransitionName(float stageTime) const;
    float CurrentIntensity(float stageTime) const;
    bool IsRecoveryWindow(float stageTime) const;
    bool IsBossRunway(float stageTime) const;

private:
    int nextCue_ = 0;
    int nextBlock_ = 0;
    void SpawnCue(int cue, int loop, std::vector<Enemy>& enemies);
    void AddEnemy(std::vector<Enemy>& enemies, EnemyType type, Vector2 pos, Vector2 vel, int loop,
                  int formationId, EnemyMovePattern move, EnemyFirePattern fire,
                  float fireDelay, float fireRate, int maxShots, float phase);
};
