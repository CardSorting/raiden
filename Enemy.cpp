#include "Enemy.h"
#include "Bullet.h"
#include <cmath>

static Vector2 Norm(Vector2 v) {
    float len = std::sqrt(v.x * v.x + v.y * v.y);
    if (len <= 0.0001f) return {0, 1};
    return {v.x / len, v.y / len};
}

Enemy::Enemy(EnemyType t, Vector2 p, int loop, int formId) : type(t), pos(p), formationId(formId) {
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
    enemyBullets.emplace_back(pos, Vector2{d.x * speed, d.y * speed}, 5.5f, damage, BulletOwner::Enemy, PINK);
}

void Enemy::FireRadial(std::vector<Bullet>& enemyBullets, int count, float speed, float angleOffset) const {
    for (int i = 0; i < count; ++i) {
        float a = angleOffset + (float)i / (float)count * 6.2831853f;
        enemyBullets.emplace_back(pos, Vector2{std::cos(a) * speed, std::sin(a) * speed}, 5.0f, 1, BulletOwner::Enemy, MAGENTA);
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
        else pos.x = 240.0f + std::sin(age * 0.9f) * 105.0f;
        if (phaseTimer > 7.5f) { phaseTimer = 0; phase = (phase + 1) % 3; }
        float rate = phase == 0 ? 0.75f : (phase == 1 ? 0.38f : 0.55f);
        if (fireTimer > rate) {
            fireTimer = 0;
            if (phase == 0) FireAimed(playerPos, enemyBullets, 190.0f + loop * 8.0f);
            else if (phase == 1) FireRadial(enemyBullets, 10 + loop, 115.0f + loop * 5.0f, age * 0.8f);
            else {
                FireAimed(playerPos, enemyBullets, 215.0f + loop * 8.0f);
                FireRadial(enemyBullets, 8, 95.0f, -age);
            }
        }
    }
}

void Enemy::Hit(int damage) {
    hp -= damage;
    hitFlashTimer = 0.08f;
    if (hp <= 0) active = false;
}

bool Enemy::Offscreen(int height) const { return pos.y > height + 70 || pos.x < -90 || pos.x > 570; }

void Enemy::Draw(bool debug) const {
    bool flash = hitFlashTimer > 0.0f;
    Color primaryColor = flash ? WHITE : (type == EnemyType::Popcorn ? RED : (type == EnemyType::Turret ? DARKGRAY : DARKPURPLE));
    Color secondaryColor = flash ? WHITE : (type == EnemyType::Popcorn ? MAROON : (type == EnemyType::Turret ? ORANGE : VIOLET));
    Color accentColor = flash ? WHITE : RED;

    if (type == EnemyType::Popcorn) {
        // Flickering engine fire at the back
        if ((int)(age * 16) % 2 == 0) {
            DrawTriangle({pos.x, pos.y - 18}, {pos.x - 4, pos.y - 10}, {pos.x + 4, pos.y - 10}, flash ? WHITE : ORANGE);
        }
        
        // Draw Popcorn ship with wings
        DrawTriangle({pos.x, pos.y + 15}, {pos.x - 14, pos.y - 8}, {pos.x + 14, pos.y - 8}, primaryColor);
        DrawTriangle({pos.x - 14, pos.y - 8}, {pos.x - 18, pos.y - 4}, {pos.x - 10, pos.y - 8}, secondaryColor);
        DrawTriangle({pos.x + 14, pos.y - 8}, {pos.x + 10, pos.y - 8}, {pos.x + 18, pos.y - 4}, secondaryColor);
        DrawCircleV(pos, 6, secondaryColor);
    } else if (type == EnemyType::Turret) {
        // Draw detailed Turret base & gun barrel
        DrawRectangleRounded({pos.x - 18, pos.y - 14, 36, 28}, 0.25f, 6, primaryColor);
        DrawCircleV(pos, 11, secondaryColor);
        // Draw rotating-like indicator
        float angle = age * 2.5f;
        Vector2 barrelEnd = { pos.x + std::cos(angle) * 12.0f, pos.y + std::sin(angle) * 12.0f };
        DrawLineEx(pos, barrelEnd, 4.0f, flash ? WHITE : GRAY);
        DrawCircleV(barrelEnd, 3, accentColor);
    } else {
        // Draw highly detailed Boss
        // Left & Right engines firing
        if ((int)(age * 12) % 2 == 0) {
            DrawTriangle({pos.x - 24, pos.y - 58}, {pos.x - 30, pos.y - 48}, {pos.x - 18, pos.y - 48}, flash ? WHITE : ORANGE);
            DrawTriangle({pos.x + 24, pos.y - 58}, {pos.x + 18, pos.y - 48}, {pos.x + 30, pos.y - 48}, flash ? WHITE : ORANGE);
        }
        
        // Plating and hull structures
        DrawCircleV(pos, 47, primaryColor);
        DrawCircleV({pos.x - 24, pos.y + 8}, 17, secondaryColor);
        DrawCircleV({pos.x + 24, pos.y + 8}, 17, secondaryColor);
        
        // Wing spikes
        DrawTriangle({pos.x - 47, pos.y - 10}, {pos.x - 65, pos.y + 10}, {pos.x - 40, pos.y + 15}, secondaryColor);
        DrawTriangle({pos.x + 47, pos.y - 10}, {pos.x + 40, pos.y + 15}, {pos.x + 65, pos.y + 10}, secondaryColor);
        
        // Glowing cockpit dome
        DrawCircleV({pos.x, pos.y + 20}, 10, flash ? WHITE : SKYBLUE);
        DrawCircleV({pos.x - 3, pos.y + 17}, 3, flash ? WHITE : Fade(WHITE, 0.7f));
        
        // Boss HP Bar
        DrawRectangle((int)pos.x - 50, (int)pos.y - 62, 100, 6, flash ? WHITE : DARKGRAY);
        DrawRectangle((int)pos.x - 50, (int)pos.y - 62, (int)(100.0f * (float)hp / (float)maxHp), 6, flash ? WHITE : LIME);
    }
    if (debug) DrawCircleLines((int)pos.x, (int)pos.y, radius, GREEN);
}
