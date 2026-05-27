#include "Bullet.h"
#include "Enemy.h"
#include "SpriteManager.h"
#include "Effects.h"
#include <cmath>


static Vector2 NormalizeOrZero(Vector2 v) {
    float len = std::sqrt(v.x * v.x + v.y * v.y);
    if (len <= 0.0001f) return {0, 0};
    return {v.x / len, v.y / len};
}

Bullet::Bullet(Vector2 p, Vector2 v, float r, int d, BulletOwner o, Color c, bool seek)
    : pos(p), vel(v), radius(r), damage(d), owner(o), homing(seek), color(c), trailCount_(0) {
    for (int i = 0; i < 6; ++i) trail_[i] = p;
}

void Bullet::Update(float dt, const std::vector<Enemy>& enemies, Effects& effects) {
    for (int i = 5; i > 0; --i) {
        trail_[i] = trail_[i - 1];
    }
    trail_[0] = pos;
    if (trailCount_ < 6) trailCount_++;

    if (homing && owner == BulletOwner::Player && !enemies.empty()) {
        const Enemy* target = nullptr;
        float best = 100000000.0f;
        for (const auto& e : enemies) {
            if (!e.active) continue;
            float dx = e.pos.x - pos.x;
            float dy = e.pos.y - pos.y;
            float d2 = dx * dx + dy * dy;
            if (d2 < best) { best = d2; target = &e; }
        }
        if (target) {
            Vector2 desired = NormalizeOrZero({target->pos.x - pos.x, target->pos.y - pos.y});
            float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
            vel.x = vel.x * 0.91f + desired.x * speed * 0.09f;
            vel.y = vel.y * 0.91f + desired.y * speed * 0.09f;
        }
    }

    if (homing && owner == BulletOwner::Player && GetRandomValue(0, 100) < 35) {
        effects.Smoke(pos, Color{110, 110, 115, 180});
    }

    pos.x += vel.x * dt;
    pos.y += vel.y * dt;
}

void Bullet::Draw(bool debug) const {
    float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
    float angle = 0.0f;
    if (speed > 0.0001f) {
        angle = std::atan2(vel.y, vel.x) * 57.29578f + 90.0f; // degrees, facing up as base
    }

    if (owner == BulletOwner::Player) {
        if (homing) {
            // Draw Homing Missile
            BeginBlendMode(BLEND_ADDITIVE);
            for (int i = 0; i < trailCount_; ++i) {
                float alpha = 1.0f - (float)i / 6.0f;
                // Add green/orange exhaust plumes
                DrawCircleV(trail_[i], radius * (1.2f - (float)i / 6.0f), Fade(ORANGE, alpha * 0.65f));
                DrawCircleV(trail_[i], radius * (0.6f - (float)i / 8.0f), Fade(YELLOW, alpha * 0.55f));
            }
            EndBlendMode();
            SpriteManager::Instance().Draw(SpriteId::BulletMissile, pos, angle, 1.4f);
        } else {
            // Check if Vulcan or Plasma
            bool isPlasma = (color.r == SKYBLUE.r && color.g == SKYBLUE.g && color.b == SKYBLUE.b) || 
                             (color.r == BLUE.r && color.g == BLUE.g && color.b == BLUE.b);
            
            if (isPlasma) {
                // Draw spinning Plasma orb with additive motion smears
                float spin = (float)GetTime() * 450.0f;
                BeginBlendMode(BLEND_ADDITIVE);
                for (int i = 0; i < trailCount_; ++i) {
                    float alpha = 1.0f - (float)i / (float)trailCount_;
                    DrawCircleV(trail_[i], radius * (1.3f - (float)i * 0.15f), Fade(color, alpha * 0.6f));
                    DrawCircleV(trail_[i], radius * (0.8f - (float)i * 0.1f), Fade(WHITE, alpha * 0.4f));
                }
                EndBlendMode();
                SpriteManager::Instance().Draw(SpriteId::BulletPlasma, pos, spin, 1.4f);
            } else {
                // Draw Vulcan laser line with motion smear needles (white core, orange border)
                BeginBlendMode(BLEND_ADDITIVE);
                for (int i = 0; i < trailCount_; ++i) {
                    float alpha = 1.0f - (float)i / (float)trailCount_;
                    // Outer neon border
                    DrawLineEx(trail_[i], trail_[std::max(0, i - 1)], 3.5f * alpha, Fade(color, alpha * 0.5f));
                    // Inner white core
                    DrawLineEx(trail_[i], trail_[std::max(0, i - 1)], 1.5f * alpha, Fade(WHITE, alpha * 0.8f));
                }
                EndBlendMode();
                SpriteManager::Instance().Draw(SpriteId::BulletVulcan, pos, angle, 1.4f);
            }
        }
    } else {
        // Enemy bullets: First draw black safety mask for contrast separation!
        for (int i = trailCount_ - 1; i >= 0; --i) {
            float alpha = 0.66f - (float)i * 0.075f;
            DrawCircleV(trail_[i], radius * (1.34f - (float)i * 0.08f), Fade(BLACK, std::max(0.18f, alpha)));
        }
        DrawCircleV(pos, radius * 1.42f, Fade(BLACK, 0.86f));

        // Then draw neon glowing trails additively
        BeginBlendMode(BLEND_ADDITIVE);
        for (int i = 0; i < trailCount_; ++i) {
            float alpha = 1.0f - (float)i / (float)trailCount_;
            DrawCircleV(trail_[i], radius * (1.22f - (float)i * 0.09f), Fade(color, alpha * 0.72f));
            DrawCircleV(trail_[i], radius * (0.68f - (float)i * 0.06f), Fade(WHITE, alpha * 0.50f));
        }
        EndBlendMode();

        // Draw basic shapes or sprite representations
        if (radius >= 6.8f) {
            float spin = (float)GetTime() * -250.0f;
            SpriteManager::Instance().Draw(SpriteId::BulletEnemyStrong, pos, spin, 1.3f);
        } else {
            SpriteManager::Instance().Draw(SpriteId::BulletEnemyBasic, pos, 0.0f, 1.3f);
        }
        
        // Draw the high-contrast white core and neon border on top
        DrawCircleV(pos, radius * 0.55f, WHITE);
        DrawCircleLines((int)pos.x, (int)pos.y, (int)(radius + 1.0f), Fade(color, 0.95f));
        DrawCircleLines((int)pos.x, (int)pos.y, (int)(radius + 2.0f), Fade(color, 0.45f));
    }

    if (debug) DrawCircleLines((int)pos.x, (int)pos.y, radius, GREEN);
}


bool Bullet::Offscreen(int width, int height) const {
    return pos.x < -40 || pos.x > width + 40 || pos.y < -60 || pos.y > height + 60;
}
