#include "Enemy.h"
#include "Bullet.h"
#include "SpriteManager.h"
#include "Effects.h"
#include <cmath>
#include <algorithm>

static Vector2 Norm(Vector2 v) {
    float len = std::sqrt(v.x * v.x + v.y * v.y);
    if (len <= 0.0001f) return {0, 1};
    return {v.x / len, v.y / len};
}

static float WrapAngle(float a) {
    while (a < -PI) a += PI * 2.0f;
    while (a > PI) a -= PI * 2.0f;
    return a;
}

Enemy::Enemy(EnemyType t, Vector2 p, int loop, int formId) : type(t), pos(p), formationId(formId), aimAngle(0.0f), bossPanelsShed(false) {
    if (type == EnemyType::Popcorn) {
        radius = 13; hp = maxHp = 2 + loop / 2; scoreValue = 120; vel = {0, 105}; drift = (float)GetRandomValue(-65, 65);
    } else if (type == EnemyType::Turret) {
        radius = 18; hp = maxHp = 7 + loop * 2; scoreValue = 420; vel = {0, 42};
    } else {
        radius = 48; hp = maxHp = 260 + loop * 55; scoreValue = 9000 + loop * 1250; vel = {0, 58}; phase = 0;
    }
}

void Enemy::FireAimed(Vector2 playerPos, std::vector<Bullet>& enemyBullets, float speed, int damage) const {
    Vector2 d = Norm({playerPos.x - pos.x, playerPos.y - pos.y});
    float size = IsBoss() ? 7.5f : 5.5f;
    Color c = IsBoss() ? Color{255, 42, 76, 255} : PINK;
    enemyBullets.emplace_back(pos, Vector2{d.x * speed, d.y * speed}, size, damage, BulletOwner::Enemy, c);
}

void Enemy::FireRadial(std::vector<Bullet>& enemyBullets, int count, float speed, float angleOffset) const {
    float size = IsBoss() ? 7.0f : 5.0f;
    Color c = IsBoss() ? Color{255, 72, 128, 255} : MAGENTA;
    for (int i = 0; i < count; ++i) {
        float a = angleOffset + (float)i / (float)count * 6.2831853f;
        enemyBullets.emplace_back(pos, Vector2{std::cos(a) * speed, std::sin(a) * speed}, size, 1, BulletOwner::Enemy, c);
    }
}

void Enemy::FireAimedFan(Vector2 playerPos, std::vector<Bullet>& enemyBullets, int count, float speed, float spread, Color color, float size) const {
    Vector2 d = Norm({playerPos.x - pos.x, playerPos.y - pos.y});
    float base = std::atan2(d.y, d.x);
    int center = count / 2;
    for (int i = 0; i < count; ++i) {
        float a = base + (float)(i - center) * spread;
        enemyBullets.emplace_back(pos, Vector2{std::cos(a) * speed, std::sin(a) * speed}, size, 1, BulletOwner::Enemy, color);
    }
}

void Enemy::FireRadialCustom(std::vector<Bullet>& enemyBullets, int count, float speed, float angleOffset, Color color, float size) const {
    for (int i = 0; i < count; ++i) {
        float a = angleOffset + (float)i / (float)count * PI * 2.0f;
        enemyBullets.emplace_back(pos, Vector2{std::cos(a) * speed, std::sin(a) * speed}, size, 1, BulletOwner::Enemy, color);
    }
}

void Enemy::FireRadialGap(std::vector<Bullet>& enemyBullets, int count, float speed, float angleOffset, float gapCenter, float gapWidth, Color color, float size) const {
    for (int i = 0; i < count; ++i) {
        float a = angleOffset + (float)i / (float)count * PI * 2.0f;
        if (std::abs(WrapAngle(a - gapCenter)) < gapWidth * 0.5f) continue;
        enemyBullets.emplace_back(pos, Vector2{std::cos(a) * speed, std::sin(a) * speed}, size, 1, BulletOwner::Enemy, color);
    }
}

bool Enemy::ReadyToFire(float rate) {
    if (age < fireDelay) return false;
    if (maxShots >= 0 && shotsFired >= maxShots) return false;
    if (fireTimer <= rate) return false;
    fireTimer = 0.0f;
    ++shotsFired;
    return true;
}

void Enemy::SetBossPhase(BossAttackPhase nextPhase) {
    int next = static_cast<int>(nextPhase);
    if (phase == next) return;
    phase = next;
    phaseTimer = 0.0f;
    fireTimer = -0.85f;
}

void Enemy::BeginBossCombat() {
    if (!IsBoss()) return;
    SetBossPhase(BossAttackPhase::AzureNeedle);
}

const char* Enemy::BossAttackTitle() const {
    switch (BossPhase()) {
        case BossAttackPhase::AzureNeedle: return "Gate Sign: Needle Lattice";
        case BossAttackPhase::FallingStar: return "Vector Sign: Falling Star Lock";
        case BossAttackPhase::BrokenHalo: return "Circuit Sign: White Halo Denial";
        case BossAttackPhase::Overload: return "Final Sign: Core Collapse Mandate";
        case BossAttackPhase::Entrance:
        default: return "Hostile Frame: VANTAGE-9";
    }
}

void Enemy::Update(float dt, Vector2 playerPos, std::vector<Bullet>& enemyBullets, int loop) {
    age += dt;
    fireTimer += dt;
    phaseTimer += dt;
    if (hitFlashTimer > 0.0f) hitFlashTimer -= dt;

    if (type == EnemyType::Popcorn) {
        switch (movePattern) {
            case EnemyMovePattern::Straight:
                pos.x += vel.x * dt;
                pos.y += vel.y * dt;
                break;
            case EnemyMovePattern::NeedleSweep:
                pos.x += (vel.x + std::sin(age * 5.2f + scriptPhase) * 18.0f) * dt;
                pos.y += vel.y * dt;
                break;
            case EnemyMovePattern::DescendingScissors: {
                float weave = std::sin(age * 2.8f + scriptPhase) * 54.0f;
                pos.x += (vel.x + weave) * dt;
                pos.y += (vel.y + std::max(0.0f, age - 1.0f) * 22.0f) * dt;
                break;
            }
            case EnemyMovePattern::CrossLane:
                pos.x += (vel.x + std::sin(age * 4.4f + scriptPhase) * 10.0f) * dt;
                pos.y += (vel.y + std::cos(age * 2.0f + scriptPhase) * 20.0f) * dt;
                break;
            case EnemyMovePattern::OrbitalDrift:
                pos.x += (vel.x + std::cos(age * 3.0f + scriptPhase) * 34.0f) * dt;
                pos.y += (vel.y + std::sin(age * 2.2f + scriptPhase) * 16.0f) * dt;
                break;
            case EnemyMovePattern::Pincer:
                pos.x += (vel.x * (1.0f + std::min(age, 1.6f) * 0.22f)) * dt;
                pos.y += vel.y * dt;
                break;
            case EnemyMovePattern::CollapseRetreat: {
                float turn = age > 1.35f ? std::min(1.0f, (age - 1.35f) * 1.4f) : 0.0f;
                pos.x += (vel.x + std::sin(scriptPhase) * 95.0f * turn) * dt;
                pos.y += (vel.y - 72.0f * turn) * dt;
                break;
            }
            default:
                pos.x += (vel.x + std::sin(age * 4.2f) * drift) * dt;
                pos.y += vel.y * dt;
                break;
        }

        if (pos.y > 36 && pos.y < 430) {
            if (firePattern == EnemyFirePattern::Default) {
                if (ReadyToFire(1.25f - std::min(0.45f, loop * 0.04f))) {
                    FireAimed(playerPos, enemyBullets, 145.0f + loop * 12.0f);
                }
            } else if (firePattern == EnemyFirePattern::AimedSingle) {
                if (ReadyToFire(0.0f)) FireAimed(playerPos, enemyBullets, 148.0f + loop * 10.0f);
            } else if (firePattern == EnemyFirePattern::AimedPulse) {
                float rate = fireRate > 0.0f ? fireRate : 0.82f;
                if (ReadyToFire(rate)) FireAimed(playerPos, enemyBullets, 152.0f + loop * 11.0f);
            } else if (firePattern == EnemyFirePattern::FanPulse) {
                float rate = fireRate > 0.0f ? fireRate : 1.05f;
                if (ReadyToFire(rate)) {
                    FireAimedFan(playerPos, enemyBullets, 3, 140.0f + loop * 9.0f, 0.12f, Color{255, 96, 196, 255}, 5.0f);
                }
            }
        }
    } else if (type == EnemyType::Turret) {
        if (movePattern == EnemyMovePattern::GatePatrol) {
            pos.y += vel.y * dt;
            if (pos.y > 84.0f) vel.y = 9.0f;
            pos.x += std::sin(age * 1.15f + scriptPhase) * 18.0f * dt;
        } else {
            pos.y += vel.y * dt;
            if (pos.y > 118) vel.y = 15.0f;
        }

        if (pos.y > 20 && pos.y < 520 && firePattern != EnemyFirePattern::Hold) {
            if (firePattern == EnemyFirePattern::TurretBurst) {
                float rate = fireRate > 0.0f ? fireRate : 0.38f;
                if (ReadyToFire(rate)) {
                    FireAimed(playerPos, enemyBullets, 176.0f + loop * 9.0f);
                }
            } else {
                if (ReadyToFire(0.92f - std::min(0.25f, loop * 0.025f))) {
                    FireAimed(playerPos, enemyBullets, 170.0f + loop * 9.0f);
                }
            }
        }
    } else {
        BossAttackPhase current = BossPhase();
        if (current == BossAttackPhase::Entrance) {
            pos.x = 240.0f + std::sin(age * 3.0f) * 6.0f;
            if (pos.y < 108.0f) pos.y = std::min(108.0f, pos.y + vel.y * dt);
        } else {
            if (hp <= maxHp / 4) {
                SetBossPhase(BossAttackPhase::Overload);
            } else if (hp <= maxHp / 2) {
                SetBossPhase(BossAttackPhase::BrokenHalo);
            } else if (current == BossAttackPhase::AzureNeedle && phaseTimer > 8.5f) {
                SetBossPhase(BossAttackPhase::FallingStar);
            } else if (current == BossAttackPhase::FallingStar && phaseTimer > 8.0f) {
                SetBossPhase(BossAttackPhase::AzureNeedle);
            }
            current = BossPhase();

            if (current == BossAttackPhase::AzureNeedle) {
                pos.x = 240.0f + std::sin(phaseTimer * 0.78f) * 118.0f;
                pos.y = 108.0f + std::sin(phaseTimer * 1.35f) * 7.0f;
                float rate = std::max(0.58f, 0.98f - loop * 0.035f);
                if (fireTimer > rate) {
                    fireTimer = 0.0f;
                    int burst = ((int)(phaseTimer / rate) % 4 == 3) ? 3 : 5;
                    FireAimedFan(playerPos, enemyBullets, burst, 180.0f + loop * 9.0f, 0.105f, Color{72, 190, 255, 255}, 6.4f);
                    if (((int)(phaseTimer / rate) % 3) == 1) {
                        Vector2 left = {pos.x - 54.0f, pos.y + 2.0f};
                        Vector2 right = {pos.x + 54.0f, pos.y + 2.0f};
                        Vector2 dl = Norm({playerPos.x - left.x, playerPos.y - left.y});
                        Vector2 dr = Norm({playerPos.x - right.x, playerPos.y - right.y});
                        enemyBullets.emplace_back(left, Vector2{dl.x * (168.0f + loop * 7.0f), dl.y * (168.0f + loop * 7.0f)}, 5.5f, 1, BulletOwner::Enemy, Color{255, 228, 92, 255});
                        enemyBullets.emplace_back(right, Vector2{dr.x * (168.0f + loop * 7.0f), dr.y * (168.0f + loop * 7.0f)}, 5.5f, 1, BulletOwner::Enemy, Color{255, 228, 92, 255});
                    }
                }
            } else if (current == BossAttackPhase::FallingStar) {
                pos.x = 240.0f + std::sin(phaseTimer * 0.72f) * 92.0f;
                pos.y = 114.0f + std::sin(phaseTimer * 1.44f) * 18.0f;
                float rate = std::max(0.29f, 0.43f - loop * 0.012f);
                if (fireTimer > rate) {
                    fireTimer = 0.0f;
                    int count = 18 + std::min(6, loop);
                    float speed = 78.0f + loop * 4.5f;
                    FireRadialGap(enemyBullets, count, speed, phaseTimer * 1.22f, PI * 0.5f, 0.46f, Color{255, 96, 196, 255}, 5.8f);
                    if (((int)(phaseTimer * 2.4f) % 3) == 0) {
                        float ring = 34.0f;
                        for (int i = 0; i < 6; ++i) {
                            float a = phaseTimer * 0.95f + (float)i / 6.0f * PI * 2.0f;
                            Vector2 origin = {pos.x + std::cos(a) * ring, pos.y + std::sin(a) * ring};
                            enemyBullets.emplace_back(origin, Vector2{std::cos(a) * speed * 0.72f, std::sin(a) * speed * 0.72f}, 5.2f, 1, BulletOwner::Enemy, Color{80, 230, 255, 255});
                        }
                    }
                }
            } else if (current == BossAttackPhase::BrokenHalo) {
                float damageRatio = 1.0f - (float)hp / (float)maxHp;
                pos.x = 240.0f + std::sin(phaseTimer * 1.25f) * 58.0f + std::sin(age * 29.0f) * 2.4f * damageRatio;
                pos.y = 112.0f + std::sin(phaseTimer * 2.6f) * 9.0f + std::cos(age * 23.0f) * 1.6f * damageRatio;
                float cycle = std::fmod(phaseTimer, 3.6f);
                if (cycle < 2.55f && fireTimer > 0.46f) {
                    fireTimer = 0.0f;
                    float gap = PI * 0.5f + std::sin(phaseTimer * 0.7f) * 0.62f;
                    FireRadialGap(enemyBullets, 28 + std::min(8, loop), 92.0f + loop * 4.0f, phaseTimer * 1.65f, gap, 0.72f, Color{255, 126, 38, 255}, 5.8f);
                    if (((int)(phaseTimer * 2.0f) % 4) == 1) {
                        FireAimedFan(playerPos, enemyBullets, 3, 205.0f + loop * 7.0f, 0.08f, Color{255, 56, 82, 255}, 6.8f);
                    }
                }
            } else if (current == BossAttackPhase::Overload) {
                pos.x = 240.0f + std::sin(phaseTimer * 2.35f) * 128.0f + std::sin(age * 31.0f) * 4.0f;
                pos.y = 103.0f + std::sin(phaseTimer * 4.1f) * 14.0f;
                float cycle = std::fmod(phaseTimer, 3.15f);
                if (cycle < 2.12f && fireTimer > std::max(0.18f, 0.25f - loop * 0.01f)) {
                    fireTimer = 0.0f;
                    float safeLane = PI * 0.5f + std::sin(phaseTimer * 0.9f) * 0.42f;
                    FireRadialGap(enemyBullets, 18 + std::min(6, loop), 118.0f + loop * 4.0f, age * 2.35f, safeLane, 0.64f, Color{255, 40, 108, 255}, 6.2f);
                    if (((int)(phaseTimer * 3.2f) % 2) == 0) {
                        for (int i = 0; i < 8; ++i) {
                            float a = -age * 1.45f + (float)i / 8.0f * PI * 2.0f;
                            Vector2 origin = {pos.x + std::cos(a) * 56.0f, pos.y + std::sin(a) * 56.0f};
                            Vector2 inward = Norm({pos.x - origin.x, pos.y + 28.0f - origin.y});
                            enemyBullets.emplace_back(origin, Vector2{inward.x * (92.0f + loop * 3.0f), inward.y * (92.0f + loop * 3.0f)}, 5.2f, 1, BulletOwner::Enemy, Color{128, 230, 255, 255});
                        }
                    }
                }
            }
        }
    }

    // Dynamic rotation angle calculation pointing to player
    if (type == EnemyType::Turret || type == EnemyType::Miniboss) {
        float dx = playerPos.x - pos.x;
        float dy = playerPos.y - pos.y;
        aimAngle = std::atan2(dy, dx) * 57.29578f + 90.0f; // degrees, facing up as base
    }
}

void Enemy::Hit(int damage, Effects& effects) {
    hp -= damage;
    hitFlashTimer = 0.08f;

    // Direct impact flash spark burst
    effects.Spark(pos, WHITE);

    // Boss Phase 2 Armor Shedding transition
    if (IsBoss() && hp <= maxHp / 2 && !bossPanelsShed) {
        bossPanelsShed = true;
            effects.DebrisShower(pos, Color{125, 130, 150, 255}, 18);
            effects.Explosion(pos, Color{255, 70, 80, 255}, 26, SpriteId::DebrisBossCore);
            effects.Spark(pos, WHITE, {0.0f, -80.0f});
            effects.Shake(13.0f, 0.42f);
        effects.AddText(pos, "ARMOR BREACH", RED);
    }

    if (hp <= 0) active = false;
}

bool Enemy::Offscreen(int height) const { return pos.y > height + 70 || pos.x < -90 || pos.x > 570; }

void Enemy::Draw(bool debug) const {
    bool flash = hitFlashTimer > 0.0f;
    Color tint = flash ? Color{ 255, 100, 100, 255 } : WHITE;

    if (type == EnemyType::Popcorn) {
        if (fireDelay > age && fireDelay - age < 0.45f && firePattern != EnemyFirePattern::Hold) {
            float telegraph = 1.0f - (fireDelay - age) / 0.45f;
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleLines((int)pos.x, (int)pos.y, 17.0f + telegraph * 6.0f, Fade(Color{90, 210, 255, 255}, 0.45f * telegraph));
            EndBlendMode();
        }

        // Flickering engine plume at top (moving downwards)
        if ((int)(age * 18) % 2 == 0) {
            float plumeH = 8.0f + std::sin(age * 50.0f) * 3.0f;
            DrawTriangle(
                {pos.x - 3, pos.y - 12},
                {pos.x, pos.y - 12 - plumeH},
                {pos.x + 3, pos.y - 12},
                ORANGE
            );
        }
        
        // Draw Popcorn ship sprite rotated 180 degrees
        SpriteManager::Instance().Draw(SpriteId::Popcorn, pos, 180.0f, 1.8f, tint);
    } else if (type == EnemyType::Turret) {
        if (fireDelay > age && fireDelay - age < 0.55f && firePattern != EnemyFirePattern::Hold) {
            float telegraph = 1.0f - (fireDelay - age) / 0.55f;
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleLines((int)pos.x, (int)pos.y, 22.0f + telegraph * 8.0f, Fade(ORANGE, 0.5f * telegraph));
            EndBlendMode();
        }

        // Draw Turret Base
        SpriteManager::Instance().Draw(SpriteId::TurretBase, pos, 0.0f, 1.8f, tint);
        // Draw Barrel pointing directly at player (with recoil frame shift if recently fired)
        SpriteId barrelId = (fireTimer < 0.12f) ? SpriteId::TurretBarrelRecoil : SpriteId::TurretBarrel;
        SpriteManager::Instance().Draw(barrelId, pos, aimAngle, 1.8f, tint);
    } else {
        // Draw boss
        bool phase2 = (hp <= maxHp / 2);
        SpriteId bossSprite = phase2 ? SpriteId::BossDamaged : SpriteId::Boss;

        // Double engine plumes at the top (facing down)
        float flameRate = phase2 ? 24.0f : 14.0f;
        if ((int)(age * flameRate) % 2 == 0) {
            float plumeH = (phase2 ? 20.0f : 15.0f) + std::sin(age * 54.0f) * (phase2 ? 7.0f : 5.0f);
            DrawTriangle({pos.x - 24, pos.y - 48}, {pos.x - 28, pos.y - 48 - plumeH}, {pos.x - 20, pos.y - 48}, ORANGE);
            DrawTriangle({pos.x - 23, pos.y - 48}, {pos.x - 25, pos.y - 48 - (plumeH * 0.6f)}, {pos.x - 21, pos.y - 48}, YELLOW);
            
            DrawTriangle({pos.x + 24, pos.y - 48}, {pos.x + 20, pos.y - 48}, {pos.x + 28, pos.y - 48 - plumeH}, ORANGE);
            DrawTriangle({pos.x + 23, pos.y - 48}, {pos.x + 21, pos.y - 48}, {pos.x + 25, pos.y - 48 - (plumeH * 0.6f)}, YELLOW);
        }

        // Draw Boss sprite facing down (180 degrees)
        SpriteManager::Instance().Draw(bossSprite, pos, 180.0f, 1.8f, tint);

        // VANTAGE-9 gatekeeper silhouette fins, drawn procedurally to separate it from regular carriers.
        Color gateColor = phase2 ? Color{180, 38, 60, 255} : Color{70, 150, 190, 255};
        DrawTriangle({pos.x - 62.0f, pos.y - 18.0f}, {pos.x - 88.0f, pos.y + 2.0f}, {pos.x - 58.0f, pos.y + 18.0f}, Fade(gateColor, 0.86f));
        DrawTriangle({pos.x + 62.0f, pos.y - 18.0f}, {pos.x + 58.0f, pos.y + 18.0f}, {pos.x + 88.0f, pos.y + 2.0f}, Fade(gateColor, 0.86f));
        DrawLineEx({pos.x - 82.0f, pos.y + 2.0f}, {pos.x - 50.0f, pos.y + 2.0f}, 2.0f, Fade(WHITE, 0.34f));
        DrawLineEx({pos.x + 50.0f, pos.y + 2.0f}, {pos.x + 82.0f, pos.y + 2.0f}, 2.0f, Fade(WHITE, 0.34f));

        // Rotating gate segments make VANTAGE-9 read as a mechanical lock instead of a normal ship.
        BossAttackPhase current = BossPhase();
        float ringRot = age * (phase2 ? 54.0f : 34.0f);
        float ringRadius = (current == BossAttackPhase::Overload) ? 54.0f : 44.0f;
        BeginBlendMode(BLEND_ADDITIVE);
        for (int i = 0; i < 4; ++i) {
            float start = ringRot + i * 90.0f + ((current == BossAttackPhase::BrokenHalo) ? (i % 2 == 0 ? 8.0f : -13.0f) : 0.0f);
            float sweep = (current == BossAttackPhase::FallingStar) ? 34.0f : 24.0f;
            DrawRing(pos, ringRadius - 2.0f, ringRadius + 1.6f, start, start + sweep, 8, Fade(gateColor, 0.34f));
        }
        DrawCircleLines((int)pos.x, (int)pos.y, ringRadius + 5.0f, Fade(gateColor, phase2 ? 0.26f : 0.18f));
        EndBlendMode();
        
        // Draw the core glowing center (Weak Point Indicator) and emissive wing lights
        float pulse = 0.5f + 0.5f * std::sin(age * 12.0f);
        Color coreColor = phase2 ? Color{ 255, 40, 40, 255 } : Color{ 255, 170, 0, 255 };
        if (flash) coreColor = WHITE;
        
        BeginBlendMode(BLEND_ADDITIVE);
        // Outer glowing core aura
        DrawCircleV(pos, (phase2 ? 14.0f : 8.0f) + 4.0f * pulse, Fade(coreColor, 0.55f + 0.30f * pulse));
        // Inner hot core
        DrawCircleV(pos, phase2 ? 5.0f : 3.5f, WHITE);
        if (phase2) {
            DrawCircleLines((int)pos.x, (int)pos.y, 18.0f + 3.0f * pulse, Fade(RED, 0.68f));
            DrawLineEx({pos.x - 17.0f, pos.y - 5.0f}, {pos.x - 7.0f, pos.y + 6.0f}, 1.5f, Fade(ORANGE, 0.8f));
            DrawLineEx({pos.x + 17.0f, pos.y - 6.0f}, {pos.x + 6.0f, pos.y + 7.0f}, 1.5f, Fade(ORANGE, 0.8f));
        }
        
        // Emissive green wingtip sensors
        float sensorPulse = 0.6f + 0.4f * std::sin(age * 8.0f);
        DrawCircleV({ pos.x - 55.0f, pos.y - 4.0f }, 2.5f, Fade(LIME, sensorPulse));
        DrawCircleV({ pos.x + 55.0f, pos.y - 4.0f }, 2.5f, Fade(LIME, sensorPulse));
        EndBlendMode();

        // Mechanical damage arcs/sparks from wings
        if (phase2) {
            BeginBlendMode(BLEND_ADDITIVE);
            for (int i = 0; i < 2; ++i) {
                Vector2 wp = (i == 0) ? Vector2{ pos.x - 30.0f, pos.y - 8.0f } : Vector2{ pos.x + 30.0f, pos.y - 8.0f };
                if (GetRandomValue(0, 10) < 4) {
                    float sz = (float)GetRandomValue(3, 7);
                    DrawCircleV(wp, sz, (GetRandomValue(0, 1) == 0) ? ORANGE : RED);
                    DrawLineV(wp, { wp.x + GetRandomValue(-12, 12), wp.y + GetRandomValue(8, 24) }, GOLD);
                }
            }
            EndBlendMode();
        }

        // Attack telegraphs: thin, high-contrast danger hints before each named pattern resolves.
        BeginBlendMode(BLEND_ADDITIVE);
        if (current == BossAttackPhase::AzureNeedle && fireTimer > 0.68f) {
            float shotAngle = (aimAngle - 90.0f) * DEG2RAD;
            Vector2 end = { pos.x + std::cos(shotAngle) * 540.0f, pos.y + std::sin(shotAngle) * 540.0f };
            DrawLineEx(pos, end, 2.0f, Fade(Color{72, 190, 255, 255}, 0.34f));
            DrawLineEx({pos.x - 7.0f, pos.y}, {end.x - 20.0f, end.y}, 1.0f, Fade(SKYBLUE, 0.18f));
            DrawLineEx({pos.x + 7.0f, pos.y}, {end.x + 20.0f, end.y}, 1.0f, Fade(SKYBLUE, 0.18f));
        } else if (current == BossAttackPhase::FallingStar && fireTimer > 0.25f) {
            float r = 26.0f + std::sin(age * 18.0f) * 2.0f;
            DrawCircleLines((int)pos.x, (int)pos.y, r, Fade(Color{255, 96, 196, 255}, 0.62f));
            DrawCircleLines((int)pos.x, (int)pos.y, r + 15.0f, Fade(SKYBLUE, 0.28f));
        } else if (current == BossAttackPhase::BrokenHalo) {
            float cycle = std::fmod(phaseTimer, 3.6f);
            if (cycle < 2.55f && fireTimer > 0.28f) {
                float gap = PI * 0.5f + std::sin(phaseTimer * 0.7f) * 0.62f;
                Vector2 safe = { pos.x + std::cos(gap) * 76.0f, pos.y + std::sin(gap) * 76.0f };
                DrawCircleLines((int)pos.x, (int)pos.y, 42.0f + 5.0f * pulse, Fade(ORANGE, 0.48f));
                DrawLineEx(pos, safe, 4.0f, Fade(LIME, 0.20f));
            }
        } else if (current == BossAttackPhase::Overload) {
            float cycle = std::fmod(phaseTimer, 3.15f);
            if (cycle < 2.12f) {
                DrawLineEx({pos.x - 64.0f, pos.y - 38.0f}, {pos.x + 64.0f, pos.y + 38.0f}, 2.5f, Fade(RED, 0.32f));
                DrawLineEx({pos.x + 64.0f, pos.y - 38.0f}, {pos.x - 64.0f, pos.y + 38.0f}, 2.5f, Fade(RED, 0.32f));
            } else {
                DrawCircleLines((int)pos.x, (int)pos.y, 50.0f, Fade(SKYBLUE, 0.24f));
            }
        }
        EndBlendMode();
        
        // Draw premium arcade Boss HP Bar
        Color hpColor = flash ? WHITE : (phase2 ? RED : LIME);
        int barY = (int)pos.y - 68;
        
        // Draw HP bar outline bezel
        DrawRectangle((int)pos.x - 52, barY - 1, 104, 8, DARKGRAY);
        DrawRectangle((int)pos.x - 51, barY, 102, 6, BLACK);
        DrawRectangle((int)pos.x - 51, barY, (int)(102.0f * (float)hp / (float)maxHp), 6, hpColor);
        
        // "BOSS" text above bar
        const char* tag = phase2 ? "VANTAGE-9 // CORE EXPOSED" : "VANTAGE-9 // ORBITAL GATEKEEPER";
        int textW = MeasureText(tag, 10);
        DrawText(tag, (int)pos.x - textW / 2, barY - 12, 10, phase2 ? RED : GOLD);
    }
    
    if (debug) DrawCircleLines((int)pos.x, (int)pos.y, radius, GREEN);
}
