#include "StageDirector.h"

#include <cstddef>

namespace {
enum StageCue {
    EntryDriftSolo,
    EntryNeedlePair,
    EntryWideFeel,
    EntryTopCrest,

    SweepLeftLead,
    SweepRightAnswer,
    SweepCrossReturn,
    DescendingScissors,
    SweepHookShot,

    InterceptVanguard,
    InterceptDelayedSupport,
    InterceptCenterColumn,
    InterceptSideNeedles,
    InterceptRearGuard,

    EncirclementPincer,
    EncirclementClamp,
    EncirclementRelease,
    RecoveryLoneSurvivor,
    RecoveryEscort,
    RecoveryWideCaravan,
    ReassemblyLeader,
    ReassemblyWingFold,
    DebrisDriftLeft,
    DebrisDriftRight,
    BonusParadeLeft,
    BonusParadeRight,
    BonusParadeColumn,
    BonusParadeFinale,
    ThreatHorizonScouts,
    ThreatHorizonBeacon,

    EscalationStagger,
    SynchronizedDive,
    EscalationTurrets,
    CollapseCorridor,
    MirroredScissors,
    LaneDeny,

    GatePatrolLeft,
    GatePatrolRight,
    GateWarningEchoes,
    FinalScout,
    LastDiver,
    FortressCollapseRetreat,
    BossArrivalEscort,
    BossArrivalShadow
};

struct TimedCue {
    float t;
    StageCue cue;
};

struct WaveBlock {
    const char* name;
    float start;
    float duration;
    StageSection section;
    EncounterType encounter;
    StageTransition transition;
    const char* purpose;
    const char* composition;
    const char* escalation;
    const TimedCue* cues;
    int cueCount;
    float intensity;
};

template <typename T, size_t N>
constexpr int CountOf(const T (&)[N]) {
    return (int)N;
}

constexpr TimedCue ActLaunch[] = {
    { 0.55f, EntryDriftSolo },
    { 2.80f, EntryNeedlePair },
    { 5.15f, EntryWideFeel },
    { 8.05f, EntryTopCrest }
};

constexpr TimedCue ActSweep[] = {
    { 0.70f, SweepLeftLead },
    { 3.70f, SweepRightAnswer },
    { 6.70f, SweepCrossReturn },
    { 9.65f, DescendingScissors }
};

constexpr TimedCue ActReassembly[] = {
    { 0.60f, ReassemblyLeader },
    { 2.70f, ReassemblyWingFold }
};

constexpr TimedCue ActIntercept[] = {
    { 0.90f, InterceptVanguard },
    { 4.00f, InterceptDelayedSupport },
    { 7.25f, InterceptCenterColumn },
    { 10.45f, InterceptRearGuard }
};

constexpr TimedCue ActEncirclement[] = {
    { 1.15f, EncirclementPincer },
    { 4.20f, EncirclementClamp },
    { 6.95f, EncirclementRelease }
};

constexpr TimedCue ActRecovery[] = {
    { 0.90f, RecoveryLoneSurvivor },
    { 3.30f, RecoveryEscort },
    { 5.80f, RecoveryWideCaravan }
};

constexpr TimedCue ActDebrisDrift[] = {
    { 0.50f, DebrisDriftLeft },
    { 1.80f, DebrisDriftRight }
};

constexpr TimedCue ActBonusParade[] = {
    { 0.55f, BonusParadeLeft },
    { 2.90f, BonusParadeRight },
    { 5.15f, BonusParadeColumn },
    { 7.70f, BonusParadeFinale }
};

constexpr TimedCue ActThreatHorizon[] = {
    { 1.20f, ThreatHorizonScouts },
    { 4.80f, ThreatHorizonBeacon }
};

constexpr TimedCue ActEscalation[] = {
    { 0.80f, EscalationStagger },
    { 4.35f, SynchronizedDive },
    { 8.10f, EscalationTurrets },
    { 12.70f, CollapseCorridor },
    { 16.65f, MirroredScissors }
};

constexpr TimedCue ActFortressCollapse[] = {
    { 0.70f, FortressCollapseRetreat },
    { 3.90f, DebrisDriftLeft }
};

constexpr TimedCue ActBossArrival[] = {
    { 0.80f, GatePatrolLeft },
    { 2.80f, GatePatrolRight },
    { 4.90f, BossArrivalEscort },
    { 7.10f, BossArrivalShadow }
};

constexpr WaveBlock StageOneBlocks[] = {
    {
        "OPENING ORIENTATION", 0.0f, 11.0f,
        StageSection::Opening, EncounterType::Orientation, StageTransition::Calm,
        "Anchor thumb movement, ship scale, and clean enemy silhouettes immediately.",
        "Solo popcorn, broad mirrored needles, and one readable crossing.",
        "Sparse ships with wide lanes and almost no fire.",
        ActLaunch, CountOf(ActLaunch), 0.16f
    },
    {
        "SWEEP LANGUAGE", 11.0f, 12.0f,
        StageSection::PatternIntro, EncounterType::Sweep, StageTransition::Pressure,
        "Teach broad lateral traversal without requiring edge-perfect dodges.",
        "Alternating lateral popcorn sweeps with one clear center anchor.",
        "Left/right call and response, then a reset instead of a long hook.",
        ActSweep, CountOf(ActSweep), 0.34f
    },
    {
        "FORMATION REASSEMBLY", 23.0f, 4.0f,
        StageSection::Transition, EncounterType::FormationReassembly, StageTransition::Release,
        "Stop pressure quickly and visibly reorganize the stage language.",
        "Non-firing ships fold into synchronized lanes.",
        "A short visual reset before aimed threats take over.",
        ActReassembly, CountOf(ActReassembly), 0.08f
    },
    {
        "AIMED INTERCEPTORS", 27.0f, 13.0f,
        StageSection::Reinforcement, EncounterType::Intercept, StageTransition::Pressure,
        "Reinforce baiting and timing shifts with single-purpose threats.",
        "Aimed popcorn columns and one delayed turret support beat.",
        "Central artillery appears, then space opens before the trap.",
        ActIntercept, CountOf(ActIntercept), 0.43f
    },
    {
        "FALSE RECOVERY PINCH", 40.0f, 7.0f,
        StageSection::Combination, EncounterType::Pincer, StageTransition::Pressure,
        "Create a short commitment test with obvious exits.",
        "One turret pair, crossing pincers, and a broad release.",
        "Pressure closes briefly, then peels away before fatigue sets in.",
        ActEncirclement, CountOf(ActEncirclement), 0.50f
    },
    {
        "BREATHING LANE", 47.0f, 6.0f,
        StageSection::Recovery, EncounterType::Recovery, StageTransition::Release,
        "Reduce projectile density and create emotional reset.",
        "One retreating survivor followed by non-firing escort/caravan traffic.",
        "Slow, low-threat ships restore dodge space and let the player re-center.",
        ActRecovery, CountOf(ActRecovery), 0.14f
    },
    {
        "DEBRIS DRIFT", 53.0f, 3.0f,
        StageSection::Transition, EncounterType::DebrisDrift, StageTransition::Silence,
        "Let the battlefield cool after the trap.",
        "Slow non-firing wreckage silhouettes cross the upper lanes.",
        "A quiet score-breath before the bonus parade.",
        ActDebrisDrift, CountOf(ActDebrisDrift), 0.08f
    },
    {
        "BONUS FORMATION", 56.0f, 11.0f,
        StageSection::Bonus, EncounterType::BonusParade, StageTransition::Celebration,
        "Deliver an early mobile reward beat with low stress and high feedback.",
        "Large synchronized non-firing target parades sweep in musical arcs.",
        "Pure score chase spectacle between compact combat phrases.",
        ActBonusParade, CountOf(ActBonusParade), 0.12f
    },
    {
        "THREAT HORIZON", 67.0f, 6.0f,
        StageSection::Transition, EncounterType::ThreatHorizon, StageTransition::Anticipation,
        "Signal that the reward phase is ending and the fortress is waking.",
        "Slow scouts and a warning beacon enter with minimal fire.",
        "Psychological preparation before the corridor spike.",
        ActThreatHorizon, CountOf(ActThreatHorizon), 0.22f
    },
    {
        "FORTRESS CORRIDOR", 73.0f, 21.0f,
        StageSection::MidStageSpike, EncounterType::CorridorCompression, StageTransition::Spike,
        "Deliver a memorable compression spike in short readable bursts.",
        "Dives, paired turrets, broad corridor walls, and one clean release.",
        "Each layer narrows space in sequence while preserving visible safe lanes.",
        ActEscalation, CountOf(ActEscalation), 0.64f
    },
    {
        "FORTRESS COLLAPSE", 94.0f, 6.0f,
        StageSection::BossRunway, EncounterType::DebrisDrift, StageTransition::Release,
        "Show the stage has been survived before the boss owns the screen.",
        "Retreating ships and drifting wreckage exit with no new bullets.",
        "Combat pressure falls away into approach staging.",
        ActFortressCollapse, CountOf(ActFortressCollapse), 0.16f
    },
    {
        "TACTICAL SILENCE", 100.0f, 6.0f,
        StageSection::BossRunway, EncounterType::TacticalSilence, StageTransition::Silence,
        "Remove threats and let anticipation become the main event.",
        "No enemies, no bullets, only atmosphere.",
        "A deliberate inhale before the carrier arrives.",
        nullptr, 0, 0.0f
    },
    {
        "BOSS ARRIVAL", 106.0f, 8.0f,
        StageSection::BossRunway, EncounterType::BossArrival, StageTransition::Anticipation,
        "Make the boss feel like it arrives instead of spawning.",
        "Sparse gate patrols, escort shadows, and visible gate silhouette.",
        "Slow ceremonial pressure before VANTAGE-9 enters.",
        ActBossArrival, CountOf(ActBossArrival), 0.26f
    }
};

constexpr int BlockCount = CountOf(StageOneBlocks);
constexpr float BossEntryTime = 114.0f;

const char* SectionName(StageSection section) {
    switch (section) {
        case StageSection::Opening: return "OPENING";
        case StageSection::PatternIntro: return "INTRO";
        case StageSection::Reinforcement: return "REINFORCE";
        case StageSection::Combination: return "COMBO";
        case StageSection::Recovery: return "RECOVERY";
        case StageSection::Transition: return "TRANSITION";
        case StageSection::Bonus: return "BONUS";
        case StageSection::MidStageSpike: return "SPIKE";
        case StageSection::BossRunway: return "RUNWAY";
    }
    return "STAGE";
}

const char* EncounterName(EncounterType encounter) {
    switch (encounter) {
        case EncounterType::Orientation: return "ORIENTATION";
        case EncounterType::Sweep: return "SWEEP";
        case EncounterType::Intercept: return "INTERCEPT";
        case EncounterType::Pincer: return "PINCER";
        case EncounterType::CorridorCompression: return "CORRIDOR";
        case EncounterType::Crossfire: return "CROSSFIRE";
        case EncounterType::ChasePressure: return "CHASE";
        case EncounterType::ArtilleryControl: return "ARTILLERY";
        case EncounterType::BulletCurtain: return "CURTAIN";
        case EncounterType::Recovery: return "RECOVERY";
        case EncounterType::FormationReassembly: return "REASSEMBLY";
        case EncounterType::DebrisDrift: return "DEBRIS";
        case EncounterType::ThreatHorizon: return "HORIZON";
        case EncounterType::BonusParade: return "BONUS";
        case EncounterType::TacticalSilence: return "SILENCE";
        case EncounterType::BossArrival: return "BOSS ARRIVAL";
        case EncounterType::BossRunway: return "BOSS RUNWAY";
    }
    return "ENCOUNTER";
}

const char* TransitionName(StageTransition transition) {
    switch (transition) {
        case StageTransition::Calm: return "CALM";
        case StageTransition::Pressure: return "PRESSURE";
        case StageTransition::Release: return "RELEASE";
        case StageTransition::Anticipation: return "ANTICIPATE";
        case StageTransition::Spike: return "SPIKE";
        case StageTransition::Silence: return "SILENCE";
        case StageTransition::Climax: return "CLIMAX";
        case StageTransition::Celebration: return "CELEBRATE";
    }
    return "FLOW";
}
}

void StageDirector::Reset() {
    nextCue_ = 0;
    nextBlock_ = 0;
}

void StageDirector::AddEnemy(std::vector<Enemy>& enemies, EnemyType type, Vector2 pos, Vector2 vel, int loop,
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

void StageDirector::SpawnCue(int cue, int loop, std::vector<Enemy>& enemies) {
    switch (static_cast<StageCue>(cue)) {
        case EntryDriftSolo:
            AddEnemy(enemies, EnemyType::Popcorn, {248.0f, -24.0f}, {0.0f, 82.0f}, loop, 1,
                     EnemyMovePattern::Straight, EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 0.0f);
            break;

        case EntryNeedlePair:
            AddEnemy(enemies, EnemyType::Popcorn, {156.0f, -30.0f}, {10.0f, 78.0f}, loop, 2,
                     EnemyMovePattern::NeedleSweep, EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 0.0f);
            AddEnemy(enemies, EnemyType::Popcorn, {324.0f, -72.0f}, {-10.0f, 78.0f}, loop, 2,
                     EnemyMovePattern::NeedleSweep, EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 3.1f);
            break;

        case EntryWideFeel:
            AddEnemy(enemies, EnemyType::Popcorn, {-22.0f, 80.0f}, {92.0f, 58.0f}, loop, 3,
                     EnemyMovePattern::CrossLane, EnemyFirePattern::AimedSingle, 1.90f, 0.0f, 1, 0.4f);
            AddEnemy(enemies, EnemyType::Popcorn, {502.0f, 30.0f}, {-86.0f, 70.0f}, loop, 3,
                     EnemyMovePattern::CrossLane, EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 2.7f);
            break;

        case EntryTopCrest:
            for (int i = 0; i < 3; ++i) {
                float x = 132.0f + i * 108.0f;
                float vx = (i < 2) ? 18.0f : -18.0f;
                AddEnemy(enemies, EnemyType::Popcorn, {x, -26.0f - i * 20.0f}, {vx, 88.0f}, loop, 4,
                         EnemyMovePattern::Straight, EnemyFirePattern::Hold,
                         0.0f, 0.0f, 0, 0.0f);
            }
            break;

        case SweepLeftLead:
            for (int i = 0; i < 4; ++i) {
                AddEnemy(enemies, EnemyType::Popcorn, {-24.0f - i * 26.0f, 36.0f - i * 28.0f},
                         {116.0f, 78.0f}, loop, 5, EnemyMovePattern::NeedleSweep,
                         i == 2 ? EnemyFirePattern::AimedSingle : EnemyFirePattern::Hold, 1.35f, 0.0f, 1, i * 0.42f);
            }
            break;

        case SweepRightAnswer:
            for (int i = 0; i < 4; ++i) {
                AddEnemy(enemies, EnemyType::Popcorn, {504.0f + i * 26.0f, 24.0f - i * 28.0f},
                         {-116.0f, 80.0f}, loop, 6, EnemyMovePattern::NeedleSweep,
                         i == 2 ? EnemyFirePattern::AimedSingle : EnemyFirePattern::Hold, 1.30f, 0.0f, 1, 2.8f + i * 0.42f);
            }
            break;

        case SweepCrossReturn:
            AddEnemy(enemies, EnemyType::Popcorn, {-26.0f, 142.0f}, {118.0f, 66.0f}, loop, 7,
                     EnemyMovePattern::CrossLane, EnemyFirePattern::AimedSingle, 1.00f, 0.0f, 1, 0.1f);
            AddEnemy(enemies, EnemyType::Popcorn, {506.0f, 92.0f}, {-118.0f, 66.0f}, loop, 7,
                     EnemyMovePattern::CrossLane, EnemyFirePattern::AimedSingle, 1.22f, 0.0f, 1, 3.0f);
            AddEnemy(enemies, EnemyType::Popcorn, {240.0f, -34.0f}, {0.0f, 108.0f}, loop, 7,
                     EnemyMovePattern::Straight, EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 0.0f);
            break;

        case DescendingScissors:
            for (int i = 0; i < 3; ++i) {
                float side = (i % 2 == 0) ? -1.0f : 1.0f;
                AddEnemy(enemies, EnemyType::Popcorn, {240.0f + side * (104.0f + i * 24.0f), -24.0f - i * 32.0f},
                         {-side * 24.0f, 102.0f}, loop, 8, EnemyMovePattern::DescendingScissors,
                         i == 1 ? EnemyFirePattern::AimedSingle : EnemyFirePattern::Hold, 1.28f + i * 0.10f, 0.0f, 1, i * 1.2f);
            }
            break;

        case SweepHookShot:
            for (int i = 0; i < 4; ++i) {
                float side = (i % 2 == 0) ? -1.0f : 1.0f;
                AddEnemy(enemies, EnemyType::Popcorn, {240.0f + side * (130.0f + i * 12.0f), -24.0f - i * 28.0f},
                         {-side * 52.0f, 118.0f}, loop, 9, EnemyMovePattern::DescendingScissors,
                         i >= 2 ? EnemyFirePattern::FanPulse : EnemyFirePattern::AimedSingle,
                         0.86f + i * 0.18f, 1.15f, 1, i * 0.9f);
            }
            break;

        case InterceptVanguard:
            for (int i = 0; i < 2; ++i) {
                float x = 176.0f + i * 128.0f;
                float y = -22.0f - i * 22.0f;
                AddEnemy(enemies, EnemyType::Popcorn, {x, y}, {0.0f, 96.0f}, loop, 10,
                         EnemyMovePattern::Straight, EnemyFirePattern::AimedSingle, 1.45f + i * 0.28f, 0.0f, 1, 0.0f);
            }
            break;

        case InterceptDelayedSupport:
            AddEnemy(enemies, EnemyType::Turret, {108.0f, -30.0f}, {0.0f, 42.0f}, loop, 11,
                     EnemyMovePattern::Default, EnemyFirePattern::Default, 1.45f, 0.0f, 1, 0.0f);
            AddEnemy(enemies, EnemyType::Popcorn, {364.0f, -32.0f}, {-28.0f, 112.0f}, loop, 11,
                     EnemyMovePattern::NeedleSweep, EnemyFirePattern::AimedSingle, 1.55f, 0.0f, 1, 1.8f);
            break;

        case InterceptCenterColumn:
            AddEnemy(enemies, EnemyType::Turret, {240.0f, -34.0f}, {0.0f, 48.0f}, loop, 12,
                     EnemyMovePattern::Default, EnemyFirePattern::TurretBurst, 1.35f, 0.72f, 2, 0.0f);
            for (int i = 0; i < 2; ++i) {
                float side = (i == 0) ? -1.0f : 1.0f;
                AddEnemy(enemies, EnemyType::Popcorn, {240.0f + (i == 0 ? -112.0f : 112.0f), -34.0f - i * 22.0f},
                         {side * 24.0f, 106.0f}, loop, 12, EnemyMovePattern::Straight,
                         EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 0.0f);
            }
            break;

        case InterceptSideNeedles:
            for (int i = 0; i < 6; ++i) {
                bool left = i % 2 == 0;
                AddEnemy(enemies, EnemyType::Popcorn, {left ? -24.0f : 504.0f, 44.0f - i * 26.0f},
                         {left ? 112.0f : -112.0f, 82.0f}, loop, 13, EnemyMovePattern::NeedleSweep,
                         i == 3 ? EnemyFirePattern::FanPulse : EnemyFirePattern::Hold, 1.05f, 1.05f, 1, i * 0.55f);
            }
            break;

        case InterceptRearGuard:
            for (int i = 0; i < 3; ++i) {
                AddEnemy(enemies, EnemyType::Popcorn, {154.0f + i * 86.0f, -28.0f - i * 28.0f},
                         {0.0f, 92.0f}, loop, 14, EnemyMovePattern::CollapseRetreat,
                         EnemyFirePattern::AimedSingle, 1.20f + i * 0.16f, 0.0f, 1, 1.1f + i * 0.7f);
            }
            break;

        case EncirclementPincer:
            AddEnemy(enemies, EnemyType::Turret, {80.0f, -34.0f}, {0.0f, 48.0f}, loop, 15,
                     EnemyMovePattern::Default, EnemyFirePattern::TurretBurst, 1.40f, 0.70f, 1, 0.0f);
            AddEnemy(enemies, EnemyType::Turret, {400.0f, -34.0f}, {0.0f, 48.0f}, loop, 15,
                     EnemyMovePattern::Default, EnemyFirePattern::TurretBurst, 1.70f, 0.70f, 1, 0.0f);
            AddEnemy(enemies, EnemyType::Popcorn, {-24.0f, 210.0f}, {112.0f, 58.0f}, loop, 15,
                     EnemyMovePattern::Pincer, EnemyFirePattern::AimedSingle, 1.05f, 0.0f, 1, 0.0f);
            AddEnemy(enemies, EnemyType::Popcorn, {504.0f, 168.0f}, {-112.0f, 62.0f}, loop, 15,
                     EnemyMovePattern::Pincer, EnemyFirePattern::AimedSingle, 1.24f, 0.0f, 1, 3.14f);
            break;

        case EncirclementClamp:
            for (int i = 0; i < 3; ++i) {
                AddEnemy(enemies, EnemyType::Popcorn, {122.0f + i * 118.0f, -24.0f - i * 22.0f},
                         {0.0f, 102.0f}, loop, 16, EnemyMovePattern::Straight,
                         i == 1 ? EnemyFirePattern::AimedSingle : EnemyFirePattern::Hold, 1.25f + i * 0.18f, 0.0f, 1, 0.0f);
            }
            break;

        case EncirclementRelease:
            for (int i = 0; i < 4; ++i) {
                float side = (i < 2) ? -1.0f : 1.0f;
                AddEnemy(enemies, EnemyType::Popcorn, {240.0f + side * (58.0f + i * 16.0f), -24.0f - i * 20.0f},
                         {side * 18.0f, 112.0f}, loop, 17, EnemyMovePattern::CollapseRetreat,
                         EnemyFirePattern::Hold, 0.0f, 0.0f, 0, side * 1.57f);
            }
            break;

        case RecoveryLoneSurvivor:
            AddEnemy(enemies, EnemyType::Popcorn, {246.0f, -30.0f}, {0.0f, 76.0f}, loop, 18,
                     EnemyMovePattern::CollapseRetreat, EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 1.57f);
            break;

        case RecoveryEscort:
            for (int i = 0; i < 3; ++i) {
                AddEnemy(enemies, EnemyType::Popcorn, {172.0f + i * 68.0f, -28.0f - i * 18.0f},
                         {0.0f, 92.0f}, loop, 19, EnemyMovePattern::Straight,
                         EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 0.0f);
            }
            break;

        case RecoveryWideCaravan:
            for (int i = 0; i < 3; ++i) {
                float x = 104.0f + i * 136.0f;
                AddEnemy(enemies, EnemyType::Popcorn, {x, -28.0f - i * 12.0f}, {0.0f, 96.0f}, loop, 20,
                         EnemyMovePattern::Straight, EnemyFirePattern::Hold,
                         0.0f, 0.0f, 0, 0.0f);
            }
            break;

        case ReassemblyLeader:
            for (int i = 0; i < 3; ++i) {
                AddEnemy(enemies, EnemyType::Popcorn, {168.0f + i * 72.0f, -28.0f - i * 12.0f},
                         {0.0f, 62.0f}, loop, 0, EnemyMovePattern::Straight,
                         EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 0.0f);
            }
            break;

        case ReassemblyWingFold:
            for (int i = 0; i < 4; ++i) {
                bool left = i < 2;
                float y = 82.0f + (i % 2) * 44.0f;
                AddEnemy(enemies, EnemyType::Popcorn, {left ? -24.0f : 504.0f, y},
                         {left ? 76.0f : -76.0f, 18.0f}, loop, 0, EnemyMovePattern::CrossLane,
                         EnemyFirePattern::Hold, 0.0f, 0.0f, 0, left ? i * 0.6f : 3.14f + i * 0.6f);
            }
            break;

        case DebrisDriftLeft:
            for (int i = 0; i < 2; ++i) {
                AddEnemy(enemies, EnemyType::Popcorn, {-26.0f - i * 18.0f, 76.0f + i * 58.0f},
                         {130.0f, 12.0f}, loop, 0, EnemyMovePattern::CrossLane,
                         EnemyFirePattern::Hold, 0.0f, 0.0f, 0, i * 0.8f);
            }
            break;

        case DebrisDriftRight:
            for (int i = 0; i < 2; ++i) {
                AddEnemy(enemies, EnemyType::Popcorn, {506.0f + i * 18.0f, 92.0f + i * 54.0f},
                         {-130.0f, 12.0f}, loop, 0, EnemyMovePattern::CrossLane,
                         EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 3.14f + i * 0.8f);
            }
            break;

        case BonusParadeLeft:
            for (int i = 0; i < 5; ++i) {
                AddEnemy(enemies, EnemyType::Popcorn, {-28.0f - i * 34.0f, 58.0f + i * 42.0f},
                         {148.0f, 3.0f}, loop, 27, EnemyMovePattern::CrossLane,
                         EnemyFirePattern::Hold, 0.0f, 0.0f, 0, i * 0.56f);
            }
            break;

        case BonusParadeRight:
            for (int i = 0; i < 5; ++i) {
                AddEnemy(enemies, EnemyType::Popcorn, {508.0f + i * 34.0f, 58.0f + i * 42.0f},
                         {-148.0f, 3.0f}, loop, 28, EnemyMovePattern::CrossLane,
                         EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 3.14f + i * 0.56f);
            }
            break;

        case BonusParadeColumn:
            for (int i = 0; i < 6; ++i) {
                float x = (i % 2 == 0) ? 128.0f : 352.0f;
                AddEnemy(enemies, EnemyType::Popcorn, {x, -26.0f - i * 36.0f},
                         {0.0f, 70.0f}, loop, 29, EnemyMovePattern::NeedleSweep,
                         EnemyFirePattern::Hold, 0.0f, 0.0f, 0, i * 0.72f);
            }
            break;

        case BonusParadeFinale:
            for (int i = 0; i < 8; ++i) {
                bool left = i < 4;
                float y = 66.0f + (i % 4) * 38.0f;
                AddEnemy(enemies, EnemyType::Popcorn, {left ? -30.0f - i * 16.0f : 510.0f + i * 16.0f, y},
                         {left ? 156.0f : -156.0f, 2.0f}, loop, 30, EnemyMovePattern::NeedleSweep,
                         EnemyFirePattern::Hold, 0.0f, 0.0f, 0, left ? i * 0.64f : 3.14f + i * 0.64f);
            }
            break;

        case ThreatHorizonScouts:
            for (int i = 0; i < 3; ++i) {
                AddEnemy(enemies, EnemyType::Popcorn, {144.0f + i * 96.0f, -30.0f - i * 18.0f},
                         {0.0f, 58.0f}, loop, 0, EnemyMovePattern::Straight,
                         i == 1 ? EnemyFirePattern::AimedSingle : EnemyFirePattern::Hold,
                         1.70f, 0.0f, 1, 0.0f);
            }
            break;

        case ThreatHorizonBeacon:
            AddEnemy(enemies, EnemyType::Turret, {240.0f, -40.0f}, {0.0f, 28.0f}, loop, 0,
                     EnemyMovePattern::GatePatrol, EnemyFirePattern::Hold,
                     0.0f, 0.0f, 0, 1.57f);
            break;

        case EscalationStagger:
            for (int i = 0; i < 3; ++i) {
                float side = (i % 2 == 0) ? -1.0f : 1.0f;
                AddEnemy(enemies, EnemyType::Popcorn, {240.0f + side * (94.0f + i * 24.0f), -22.0f - i * 28.0f},
                         {-side * 28.0f, 104.0f}, loop, 21, EnemyMovePattern::DescendingScissors,
                         i == 1 ? EnemyFirePattern::AimedSingle : EnemyFirePattern::Hold,
                         1.18f + i * 0.14f, 1.10f, 1, i * 0.7f);
            }
            break;

        case SynchronizedDive:
            for (int i = 0; i < 4; ++i) {
                AddEnemy(enemies, EnemyType::Popcorn, {126.0f + i * 76.0f, -26.0f - (i == 1 || i == 2 ? 20.0f : 0.0f)},
                         {0.0f, 128.0f}, loop, 22, EnemyMovePattern::Straight,
                         (i == 1 || i == 2) ? EnemyFirePattern::AimedSingle : EnemyFirePattern::Hold,
                         1.10f, 0.0f, 1, 0.0f);
            }
            break;

        case EscalationTurrets:
            AddEnemy(enemies, EnemyType::Turret, {122.0f, -32.0f}, {0.0f, 54.0f}, loop, 23,
                     EnemyMovePattern::Default, EnemyFirePattern::Default, 1.20f, 0.0f, 1, 0.0f);
            AddEnemy(enemies, EnemyType::Turret, {358.0f, -72.0f}, {0.0f, 54.0f}, loop, 23,
                     EnemyMovePattern::Default, EnemyFirePattern::Default, 1.65f, 0.0f, 1, 0.0f);
            break;

        case CollapseCorridor:
            for (int i = 0; i < 4; ++i) {
                float x = (i < 2) ? 82.0f + i * 56.0f : 398.0f - (i - 2) * 56.0f;
                float vx = (x < 240.0f) ? 36.0f : -36.0f;
                AddEnemy(enemies, EnemyType::Popcorn, {x, -24.0f - i * 28.0f}, {vx, 102.0f}, loop, 24,
                         EnemyMovePattern::Pincer, i == 1 ? EnemyFirePattern::AimedSingle : EnemyFirePattern::Hold,
                         1.22f + i * 0.12f, 1.05f, 1, i * 0.5f);
            }
            break;

        case MirroredScissors:
            for (int i = 0; i < 4; ++i) {
                float side = (i % 2 == 0) ? -1.0f : 1.0f;
                AddEnemy(enemies, EnemyType::Popcorn, {240.0f + side * (112.0f - i * 7.0f), -26.0f - i * 20.0f},
                         {-side * 42.0f, 122.0f}, loop, 25, EnemyMovePattern::DescendingScissors,
                         i == 2 ? EnemyFirePattern::FanPulse : (i == 1 ? EnemyFirePattern::AimedSingle : EnemyFirePattern::Hold),
                         1.05f + i * 0.12f, 1.0f, 1, i * 0.72f);
            }
            break;

        case LaneDeny:
            AddEnemy(enemies, EnemyType::Turret, {92.0f, -36.0f}, {0.0f, 50.0f}, loop, 26,
                     EnemyMovePattern::Default, EnemyFirePattern::TurretBurst, 0.85f, 0.50f, 2, 0.0f);
            AddEnemy(enemies, EnemyType::Turret, {388.0f, -48.0f}, {0.0f, 50.0f}, loop, 26,
                     EnemyMovePattern::Default, EnemyFirePattern::TurretBurst, 1.12f, 0.50f, 2, 0.0f);
            for (int i = 0; i < 2; ++i) {
                AddEnemy(enemies, EnemyType::Popcorn, {202.0f + i * 76.0f, -26.0f - i * 24.0f},
                         {0.0f, 116.0f}, loop, 26, EnemyMovePattern::Straight,
                         EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 0.0f);
            }
            break;

        case GatePatrolLeft:
            AddEnemy(enemies, EnemyType::Turret, {124.0f, -34.0f}, {0.0f, 34.0f}, loop, 0,
                     EnemyMovePattern::GatePatrol, EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 0.0f);
            break;

        case GatePatrolRight:
            AddEnemy(enemies, EnemyType::Turret, {356.0f, -34.0f}, {0.0f, 34.0f}, loop, 0,
                     EnemyMovePattern::GatePatrol, EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 3.14f);
            break;

        case GateWarningEchoes:
            AddEnemy(enemies, EnemyType::Popcorn, {-24.0f, 146.0f}, {104.0f, 62.0f}, loop, 29,
                     EnemyMovePattern::CrossLane, EnemyFirePattern::AimedSingle, 1.35f, 0.0f, 1, 0.0f);
            AddEnemy(enemies, EnemyType::Popcorn, {504.0f, 96.0f}, {-104.0f, 62.0f}, loop, 29,
                     EnemyMovePattern::CrossLane, EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 3.14f);
            break;

        case FinalScout:
            AddEnemy(enemies, EnemyType::Popcorn, {240.0f, -34.0f}, {0.0f, 66.0f}, loop, 30,
                     EnemyMovePattern::Straight, EnemyFirePattern::Hold, 0.0f, 0.0f, 0, 0.0f);
            break;

        case LastDiver:
            for (int i = 0; i < 3; ++i) {
                AddEnemy(enemies, EnemyType::Popcorn, {168.0f + i * 72.0f, -30.0f - i * 20.0f},
                         {0.0f, 104.0f}, loop, 31, EnemyMovePattern::CollapseRetreat,
                         EnemyFirePattern::Hold,
                         0.0f, 0.0f, 0, 1.0f + i * 0.7f);
            }
            break;

        case FortressCollapseRetreat:
            for (int i = 0; i < 4; ++i) {
                float side = (i % 2 == 0) ? -1.0f : 1.0f;
                AddEnemy(enemies, EnemyType::Popcorn, {240.0f + side * (60.0f + i * 18.0f), -24.0f - i * 16.0f},
                         {side * 48.0f, 78.0f}, loop, 0, EnemyMovePattern::CollapseRetreat,
                         EnemyFirePattern::Hold, 0.0f, 0.0f, 0, i * 0.85f);
            }
            break;

        case BossArrivalEscort:
            AddEnemy(enemies, EnemyType::Popcorn, {104.0f, -28.0f}, {18.0f, 58.0f}, loop, 0,
                     EnemyMovePattern::GatePatrol, EnemyFirePattern::Hold,
                     0.0f, 0.0f, 0, 0.3f);
            AddEnemy(enemies, EnemyType::Popcorn, {376.0f, -28.0f}, {-18.0f, 58.0f}, loop, 0,
                     EnemyMovePattern::GatePatrol, EnemyFirePattern::Hold,
                     0.0f, 0.0f, 0, 3.4f);
            break;

        case BossArrivalShadow:
            AddEnemy(enemies, EnemyType::Turret, {240.0f, -48.0f}, {0.0f, 24.0f}, loop, 0,
                     EnemyMovePattern::GatePatrol, EnemyFirePattern::Hold,
                     0.0f, 0.0f, 0, 1.57f);
            break;
    }
}

void StageDirector::Update(float stageTime, int loop, std::vector<Enemy>& enemies) {
    if (stageTime < 0.05f) Reset();

    while (nextBlock_ < BlockCount) {
        const WaveBlock& block = StageOneBlocks[nextBlock_];
        float localTime = stageTime - block.start;
        if (localTime < 0.0f) break;

        while (nextCue_ < block.cueCount && localTime >= block.cues[nextCue_].t) {
            SpawnCue((int)block.cues[nextCue_].cue, loop, enemies);
            ++nextCue_;
        }

        if (stageTime < block.start + block.duration) break;
        ++nextBlock_;
        nextCue_ = 0;
    }
}

bool StageDirector::ShouldSpawnBoss(float stageTime, bool bossAlive) const {
    return stageTime >= BossEntryTime && !bossAlive;
}

float StageDirector::RouteDuration() const {
    return BossEntryTime;
}

int StageDirector::CurrentBlockIndex(float stageTime) const {
    int block = 0;
    for (int i = 0; i < BlockCount; ++i) {
        if (stageTime >= StageOneBlocks[i].start) block = i;
    }
    return block + 1;
}

float StageDirector::CurrentBlockElapsed(float stageTime) const {
    int block = CurrentBlockIndex(stageTime) - 1;
    if (block < 0) block = 0;
    if (block >= BlockCount) block = BlockCount - 1;
    float elapsed = stageTime - StageOneBlocks[block].start;
    return elapsed < 0.0f ? 0.0f : elapsed;
}

const char* StageDirector::CurrentSectionName(float stageTime) const {
    int block = CurrentBlockIndex(stageTime) - 1;
    if (block < 0) block = 0;
    if (block >= BlockCount) block = BlockCount - 1;
    return SectionName(StageOneBlocks[block].section);
}

const char* StageDirector::CurrentEncounterName(float stageTime) const {
    int block = CurrentBlockIndex(stageTime) - 1;
    if (block < 0) block = 0;
    if (block >= BlockCount) block = BlockCount - 1;
    return EncounterName(StageOneBlocks[block].encounter);
}

const char* StageDirector::CurrentBlockName(float stageTime) const {
    int block = CurrentBlockIndex(stageTime) - 1;
    if (block < 0) block = 0;
    if (block >= BlockCount) block = BlockCount - 1;
    return StageOneBlocks[block].name;
}

const char* StageDirector::CurrentBlockPurpose(float stageTime) const {
    int block = CurrentBlockIndex(stageTime) - 1;
    if (block < 0) block = 0;
    if (block >= BlockCount) block = BlockCount - 1;
    return StageOneBlocks[block].purpose;
}

const char* StageDirector::CurrentBlockComposition(float stageTime) const {
    int block = CurrentBlockIndex(stageTime) - 1;
    if (block < 0) block = 0;
    if (block >= BlockCount) block = BlockCount - 1;
    return StageOneBlocks[block].composition;
}

const char* StageDirector::CurrentEscalationIntent(float stageTime) const {
    int block = CurrentBlockIndex(stageTime) - 1;
    if (block < 0) block = 0;
    if (block >= BlockCount) block = BlockCount - 1;
    return StageOneBlocks[block].escalation;
}

const char* StageDirector::CurrentTransitionName(float stageTime) const {
    int block = CurrentBlockIndex(stageTime) - 1;
    if (block < 0) block = 0;
    if (block >= BlockCount) block = BlockCount - 1;
    return TransitionName(StageOneBlocks[block].transition);
}

float StageDirector::CurrentIntensity(float stageTime) const {
    int block = CurrentBlockIndex(stageTime) - 1;
    if (block < 0) block = 0;
    if (block >= BlockCount) block = BlockCount - 1;
    return StageOneBlocks[block].intensity;
}

bool StageDirector::IsRecoveryWindow(float stageTime) const {
    int block = CurrentBlockIndex(stageTime) - 1;
    if (block < 0) block = 0;
    if (block >= BlockCount) block = BlockCount - 1;
    return StageOneBlocks[block].section == StageSection::Recovery ||
           StageOneBlocks[block].transition == StageTransition::Release;
}

bool StageDirector::IsBossRunway(float stageTime) const {
    int block = CurrentBlockIndex(stageTime) - 1;
    if (block < 0) block = 0;
    if (block >= BlockCount) block = BlockCount - 1;
    return StageOneBlocks[block].section == StageSection::BossRunway;
}

bool StageDirector::IsBonusStage(float stageTime) const {
    int block = CurrentBlockIndex(stageTime) - 1;
    if (block < 0) block = 0;
    if (block >= BlockCount) block = BlockCount - 1;
    return StageOneBlocks[block].section == StageSection::Bonus;
}

bool StageDirector::IsTacticalSilence(float stageTime) const {
    int block = CurrentBlockIndex(stageTime) - 1;
    if (block < 0) block = 0;
    if (block >= BlockCount) block = BlockCount - 1;
    return StageOneBlocks[block].encounter == EncounterType::TacticalSilence;
}
