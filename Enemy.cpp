#include "Enemy.h"
#include "Bullet.h"
#include "SpriteManager.h"
#include "Effects.h"
#include <cmath>

static Vector2 Norm(Vector2 v) {
    float len = std::sqrt(v.x * v.x + v.y * v.y);
    if (len <= 0.0001f) return {0, 1};
    return {v.x / len, v.y / len};
}

Enemy::Enemy(EnemyType t, Vector2 p, int loop, int formId) : type(t), pos(p), formationId(formId), aimAngle(0.0f), bossPanelsShed(false) {
    if (type == EnemyType::Popcorn) {
        radius = 13; hp = maxHp = 2 + loop / 2; scoreValue = 120; vel = {0, 105}; drift = (float)GetRandomValue(-65, 65);
    } else if (type == EnemyType::Turret) {
        radius = 18; hp = maxHp = 7 + loop * 2; scoreValue = 420; vel = {0, 42};
    } else {
        radius = 46; hp = maxHp = 140 + loop * 45; scoreValue = 6000 + loop * 1000; vel = {0, 42};
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

void Enemy::Update(float dt, Vector2 playerPos, std::vector<Bullet>& enemyBullets, int loop) {
    age += dt;
    fireTimer += dt;
    phaseTimer += dt;
    if (hitFlashTimer > 0.0f) hitFlashTimer -= dt;

    if (type == EnemyType::Popcorn) {
        pos.x += (vel.x + std::sin(age * 4.2f) * drift) * dt;
        pos.y += vel.y * dt;
        if (fireTimer > 1.25f - std::min(0.45f, loop * 0.04f) && pos.y > 40 && pos.y < 430) {
            fireTimer = 0;
            FireAimed(playerPos, enemyBullets, 145.0f + loop * 12.0f);
        }
    } else if (type == EnemyType::Turret) {
        pos.y += vel.y * dt;
        if (pos.y > 118) vel.y = 15.0f;
        if (fireTimer > 0.92f - std::min(0.25f, loop * 0.025f) && pos.y > 20 && pos.y < 520) {
            fireTimer = 0;
            FireAimed(playerPos, enemyBullets, 170.0f + loop * 9.0f);
        }
    } else {
        if (pos.y < 110) pos.y += 38.0f * dt;
        else {
            float speedMult = (hp < maxHp / 2) ? 1.6f : 1.0f;
            pos.x = 240.0f + std::sin(age * 0.9f * speedMult) * 105.0f;
        }

        // Structural wobble under damage (instability wobble)
        if (hp < maxHp / 2) {
            float damageRatio = 1.0f - (float)hp / (float)maxHp;
            pos.x += std::sin(age * 33.0f) * 3.2f * damageRatio + std::sin(age * 7.2f) * 2.8f;
            pos.y += std::cos(age * 25.0f) * 1.8f * damageRatio + std::sin(age * 5.0f) * 1.1f;
        }
        
        bool phase2 = (hp < maxHp / 2);
        if (phaseTimer > 7.5f) { phaseTimer = 0; phase = (phase + 1) % 3; }
        
        float rate = phase2 ? (phase == 0 ? 0.35f : 0.22f) : (phase == 0 ? 0.75f : (phase == 1 ? 0.38f : 0.55f));
        if (fireTimer > rate) {
            fireTimer = 0;
            if (phase2) {
                if (phase == 0) {
                    FireAimed(playerPos, enemyBullets, 220.0f + loop * 10.0f);
                } else if (phase == 1) {
                    FireRadial(enemyBullets, 14 + loop, 135.0f + loop * 5.0f, age * 1.5f);
                } else {
                    FireAimed(playerPos, enemyBullets, 240.0f + loop * 10.0f);
                    FireRadial(enemyBullets, 6, 85.0f, -age * 0.8f);
                }
            } else {
                if (phase == 0) FireAimed(playerPos, enemyBullets, 190.0f + loop * 8.0f);
                else if (phase == 1) FireRadial(enemyBullets, 10 + loop, 115.0f + loop * 5.0f, age * 0.8f);
                else {
                    FireAimed(playerPos, enemyBullets, 215.0f + loop * 8.0f);
                    FireRadial(enemyBullets, 8, 95.0f, -age);
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
    if (IsBoss() && hp < maxHp / 2 && !bossPanelsShed) {
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
        // Draw Turret Base
        SpriteManager::Instance().Draw(SpriteId::TurretBase, pos, 0.0f, 1.8f, tint);
        // Draw Barrel pointing directly at player (with recoil frame shift if recently fired)
        SpriteId barrelId = (fireTimer < 0.12f) ? SpriteId::TurretBarrelRecoil : SpriteId::TurretBarrel;
        SpriteManager::Instance().Draw(barrelId, pos, aimAngle, 1.8f, tint);
    } else {
        // Draw boss
        bool phase2 = (hp < maxHp / 2);
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
        
        // Draw premium arcade Boss HP Bar
        Color hpColor = flash ? WHITE : (phase2 ? RED : LIME);
        int barY = (int)pos.y - 68;
        
        // Draw HP bar outline bezel
        DrawRectangle((int)pos.x - 52, barY - 1, 104, 8, DARKGRAY);
        DrawRectangle((int)pos.x - 51, barY, 102, 6, BLACK);
        DrawRectangle((int)pos.x - 51, barY, (int)(102.0f * (float)hp / (float)maxHp), 6, hpColor);
        
        // "BOSS" text above bar
        int textW = MeasureText("WARNING: HOSTILE CARRIER", 10);
        DrawText("WARNING: HOSTILE CARRIER", (int)pos.x - textW / 2, barY - 12, 10, phase2 ? RED : GOLD);
    }
    
    if (debug) DrawCircleLines((int)pos.x, (int)pos.y, radius, GREEN);
}
