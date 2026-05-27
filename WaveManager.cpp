#include "WaveManager.h"

#include <cstddef>

namespace {
enum StageCue {
    EntryDriftSolo,
    EntryNeedlePair,
    EntryWideFeel,
    InterceptVanguard,
    InterceptDelayedSupport,
    NeedleSweepLeft,
    NeedleSweepRight,
    DescendingScissors,
    EncirclementPincer,
    EncirclementClamp,
    EncirclementRelease,
    RecoveryLoneSurvivor,
    RecoveryEscort,
    EscalationStagger,
    EscalationTurrets,
    CollapseCorridor,
    SynchronizedDive,
    GatePatrolLeft,
    GateWarningEchoes,
    FinalScout
};

struct TimedCue {
    float t;
    StageCue cue;
};

constexpr TimedCue StageOne[] = {
    // ENTRY: spare, readable movement with no immediate fire.
    { 1.20f, EntryDriftSolo },
    { 4.10f, EntryNeedlePair },
    { 6.80f, EntryWideFeel },

    // INTERCEPT: compact squads begin to aim at the player.
    { 9.30f, InterceptVanguard },
    { 12.35f, InterceptDelayedSupport },

    // SWEEP: the screen becomes diagonal and lateral.
    { 16.20f, NeedleSweepLeft },
    { 19.20f, NeedleSweepRight },
    { 22.40f, DescendingScissors },

    // ENCIRCLEMENT: multi-angle pressure and a brief panic peak.
    { 26.70f, EncirclementPincer },
    { 30.00f, EncirclementClamp },
    { 33.15f, EncirclementRelease },

    // RECOVERY: a visible reset and pickup opportunity.
    { 38.60f, RecoveryLoneSurvivor },
    { 42.40f, RecoveryEscort },

    // ESCALATION: denser but phrased; threats arrive in sentences.
    { 48.00f, EscalationStagger },
    { 53.10f, SynchronizedDive },
    { 58.10f, EscalationTurrets },
    { 63.40f, CollapseCorridor },

    // GATE WARNING: fewer enemies, stronger staging, more silence.
    { 72.80f, GatePatrolLeft },
    { 78.40f, GateWarningEchoes },
    { 83.20f, FinalScout }
};
}

void WaveManager::Reset() {
    nextCue_ = 0;
}

void WaveManager::AddEnemy(std::vector<Enemy>& enemies, EnemyType type, Vector2 pos, Vector2 vel, int loop,
                           int formationId, EnemyMovePattern move, EnemyFirePattern fire,
                           float fireDelay, float fireRate, int maxShots, float phase) {
    Enemy e(type, pos, loop, formationId);
    e.vel = vel;
    e.movePattern = move;
    e.firePattern = fire;
    e.fireDelay = fireDelay;
    e.fireRate = fireRate;
    e.maxShots = maxShots;
    e.scriptPhase = phase;
    if (move != EnemyMovePattern::Default) e.drift = 0.0f;
    enemies.push_back(e);
}

void WaveManager::SpawnCue(int cue, int loop, std::vector<Enemy>& enemies) {
    switch (static_cast<StageCue>(cue)) {
        case EntryDriftSolo:
            AddEnemy(enemies, EnemyType::Popcorn, {248.0f, -24.0f}, {0.0f, 82.0f}, loop, 0,
                     EnemyMovePattern::Straight, EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 0.0f);
            break;

        case EntryNeedlePair:
            AddEnemy(enemies, EnemyType::Popcorn, {170.0f, -30.0f}, {14.0f, 84.0f}, loop, 0,
                     EnemyMovePattern::NeedleSweep, EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 0.0f);
            AddEnemy(enemies, EnemyType::Popcorn, {318.0f, -72.0f}, {-14.0f, 84.0f}, loop, 0,
                     EnemyMovePattern::NeedleSweep, EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 3.1f);
            break;

        case EntryWideFeel:
            AddEnemy(enemies, EnemyType::Popcorn, {-22.0f, 80.0f}, {92.0f, 58.0f}, loop, 0,
                     EnemyMovePattern::CrossLane, EnemyFirePattern::AimedSingle, 1.55f, 0.0f, 1, 0.4f);
            AddEnemy(enemies, EnemyType::Popcorn, {502.0f, 30.0f}, {-86.0f, 70.0f}, loop, 0,
                     EnemyMovePattern::CrossLane, EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 2.7f);
            break;

        case InterceptVanguard:
            for (int i = 0; i < 3; ++i) {
                float x = 170.0f + i * 70.0f;
                float y = -22.0f - i * 22.0f;
                AddEnemy(enemies, EnemyType::Popcorn, {x, y}, {0.0f, 106.0f}, loop, 1,
                         EnemyMovePattern::Straight, EnemyFirePattern::AimedSingle, 1.05f + i * 0.18f, 0.0f, 1, 0.0f);
            }
            break;

        case InterceptDelayedSupport:
            AddEnemy(enemies, EnemyType::Turret, {108.0f, -30.0f}, {0.0f, 42.0f}, loop, 0,
                     EnemyMovePattern::Default, EnemyFirePattern::Default, 0.80f, 0.0f, 2, 0.0f);
            AddEnemy(enemies, EnemyType::Popcorn, {364.0f, -32.0f}, {-28.0f, 112.0f}, loop, 0,
                     EnemyMovePattern::NeedleSweep, EnemyFirePattern::AimedSingle, 1.30f, 0.0f, 1, 1.8f);
            break;

        case NeedleSweepLeft:
            for (int i = 0; i < 5; ++i) {
                AddEnemy(enemies, EnemyType::Popcorn, {-24.0f - i * 26.0f, 36.0f - i * 28.0f},
                         {126.0f, 86.0f}, loop, 3, EnemyMovePattern::NeedleSweep,
                         i == 2 ? EnemyFirePattern::AimedSingle : EnemyFirePattern::Hold, 1.15f, 0.0f, 1, i * 0.42f);
            }
            break;

        case NeedleSweepRight:
            for (int i = 0; i < 5; ++i) {
                AddEnemy(enemies, EnemyType::Popcorn, {504.0f + i * 26.0f, 24.0f - i * 28.0f},
                         {-126.0f, 88.0f}, loop, 4, EnemyMovePattern::NeedleSweep,
                         i == 2 ? EnemyFirePattern::AimedSingle : EnemyFirePattern::Hold, 1.10f, 0.0f, 1, 2.8f + i * 0.42f);
            }
            break;

        case DescendingScissors:
            for (int i = 0; i < 5; ++i) {
                float side = (i % 2 == 0) ? -1.0f : 1.0f;
                AddEnemy(enemies, EnemyType::Popcorn, {240.0f + side * (88.0f + i * 20.0f), -24.0f - i * 28.0f},
                         {-side * 30.0f, 112.0f}, loop, 5, EnemyMovePattern::DescendingScissors,
                         i == 1 ? EnemyFirePattern::AimedSingle : EnemyFirePattern::Hold, 1.05f + i * 0.08f, 0.0f, 1, i * 1.2f);
            }
            break;

        case EncirclementPincer:
            AddEnemy(enemies, EnemyType::Turret, {80.0f, -34.0f}, {0.0f, 48.0f}, loop, 0,
                     EnemyMovePattern::Default, EnemyFirePattern::TurretBurst, 0.92f, 0.48f, 3, 0.0f);
            AddEnemy(enemies, EnemyType::Turret, {400.0f, -34.0f}, {0.0f, 48.0f}, loop, 0,
                     EnemyMovePattern::Default, EnemyFirePattern::TurretBurst, 1.22f, 0.48f, 3, 0.0f);
            AddEnemy(enemies, EnemyType::Popcorn, {-24.0f, 210.0f}, {112.0f, 58.0f}, loop, 0,
                     EnemyMovePattern::Pincer, EnemyFirePattern::AimedSingle, 1.05f, 0.0f, 1, 0.0f);
            AddEnemy(enemies, EnemyType::Popcorn, {504.0f, 168.0f}, {-112.0f, 62.0f}, loop, 0,
                     EnemyMovePattern::Pincer, EnemyFirePattern::AimedSingle, 1.24f, 0.0f, 1, 3.14f);
            break;

        case EncirclementClamp:
            for (int i = 0; i < 4; ++i) {
                AddEnemy(enemies, EnemyType::Popcorn, {112.0f + i * 86.0f, -24.0f - i * 18.0f},
                         {0.0f, 112.0f}, loop, 7, EnemyMovePattern::Straight,
                         EnemyFirePattern::AimedSingle, 0.84f + i * 0.15f, 0.0f, 1, 0.0f);
            }
            break;

        case EncirclementRelease:
            for (int i = 0; i < 4; ++i) {
                float side = (i < 2) ? -1.0f : 1.0f;
                AddEnemy(enemies, EnemyType::Popcorn, {240.0f + side * (58.0f + i * 16.0f), -24.0f - i * 20.0f},
                         {side * 18.0f, 112.0f}, loop, 0, EnemyMovePattern::CollapseRetreat,
                         EnemyFirePattern::Hold, 0.0f, 0.0f, 0, side * 1.57f);
            }
            break;

        case RecoveryLoneSurvivor:
            AddEnemy(enemies, EnemyType::Popcorn, {246.0f, -30.0f}, {0.0f, 76.0f}, loop, 8,
                     EnemyMovePattern::CollapseRetreat, EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 1.57f);
            break;

        case RecoveryEscort:
            for (int i = 0; i < 3; ++i) {
                AddEnemy(enemies, EnemyType::Popcorn, {172.0f + i * 68.0f, -28.0f - i * 18.0f},
                         {0.0f, 92.0f}, loop, 9, EnemyMovePattern::Straight,
                         EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 0.0f);
            }
            break;

        case EscalationStagger:
            for (int i = 0; i < 5; ++i) {
                float side = (i % 2 == 0) ? -1.0f : 1.0f;
                AddEnemy(enemies, EnemyType::Popcorn, {240.0f + side * (76.0f + i * 20.0f), -22.0f - i * 24.0f},
                         {-side * 36.0f, 112.0f}, loop, 0, EnemyMovePattern::DescendingScissors,
                         i == 2 ? EnemyFirePattern::FanPulse : EnemyFirePattern::AimedSingle,
                         0.92f + i * 0.12f, 1.10f, 1, i * 0.7f);
            }
            break;

        case EscalationTurrets:
            AddEnemy(enemies, EnemyType::Turret, {122.0f, -32.0f}, {0.0f, 54.0f}, loop, 0,
                     EnemyMovePattern::Default, EnemyFirePattern::Default, 0.65f, 0.0f, 3, 0.0f);
            AddEnemy(enemies, EnemyType::Turret, {358.0f, -72.0f}, {0.0f, 54.0f}, loop, 0,
                     EnemyMovePattern::Default, EnemyFirePattern::Default, 1.15f, 0.0f, 3, 0.0f);
            break;

        case CollapseCorridor:
            for (int i = 0; i < 6; ++i) {
                float x = (i < 3) ? 82.0f + i * 38.0f : 398.0f - (i - 3) * 38.0f;
                float vx = (x < 240.0f) ? 36.0f : -36.0f;
                AddEnemy(enemies, EnemyType::Popcorn, {x, -24.0f - i * 22.0f}, {vx, 108.0f}, loop, 0,
                         EnemyMovePattern::Pincer, i == 2 ? EnemyFirePattern::FanPulse : EnemyFirePattern::AimedSingle,
                         0.98f + i * 0.10f, 1.05f, 1, i * 0.5f);
            }
            break;

        case SynchronizedDive:
            for (int i = 0; i < 4; ++i) {
                AddEnemy(enemies, EnemyType::Popcorn, {126.0f + i * 76.0f, -26.0f - std::abs(1 - i) * 20.0f},
                         {0.0f, 128.0f}, loop, 0, EnemyMovePattern::Straight,
                         EnemyFirePattern::AimedSingle, 1.02f, 0.0f, 1, 0.0f);
            }
            break;

        case GatePatrolLeft:
            AddEnemy(enemies, EnemyType::Turret, {124.0f, -34.0f}, {0.0f, 34.0f}, loop, 0,
                     EnemyMovePattern::GatePatrol, EnemyFirePattern::TurretBurst, 1.35f, 0.58f, 2, 0.0f);
            break;

        case GateWarningEchoes:
            AddEnemy(enemies, EnemyType::Popcorn, {-24.0f, 146.0f}, {104.0f, 62.0f}, loop, 0,
                     EnemyMovePattern::CrossLane, EnemyFirePattern::AimedSingle, 1.35f, 0.0f, 1, 0.0f);
            AddEnemy(enemies, EnemyType::Popcorn, {504.0f, 96.0f}, {-104.0f, 62.0f}, loop, 0,
                     EnemyMovePattern::CrossLane, EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 3.14f);
            break;

        case FinalScout:
            AddEnemy(enemies, EnemyType::Popcorn, {240.0f, -34.0f}, {0.0f, 66.0f}, loop, 0,
                     EnemyMovePattern::Straight, EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 0.0f);
            break;
    }
}

void WaveManager::Update(float stageTime, int loop, std::vector<Enemy>& enemies) {
    if (stageTime < 0.05f) nextCue_ = 0;

    constexpr int CueCount = (int)(sizeof(StageOne) / sizeof(StageOne[0]));
    while (nextCue_ < CueCount && stageTime >= StageOne[nextCue_].t) {
        SpawnCue((int)StageOne[nextCue_].cue, loop, enemies);
        ++nextCue_;
    }
}

bool WaveManager::ShouldSpawnBoss(float stageTime, bool bossAlive) const {
    return stageTime >= 90.0f && !bossAlive;
}
