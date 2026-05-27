#include "Bullet.h"
#include "Enemy.h"
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

void Bullet::Update(float dt, const std::vector<Enemy>& enemies) {
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
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;
}

void Bullet::Draw(bool debug) const {
    if (homing) {
        for (int i = 0; i < trailCount_; ++i) {
            float alpha = 1.0f - (float)i / 6.0f;
            DrawCircleV(trail_[i], radius * (1.0f - (float)i / 10.0f), Fade(color, alpha * 0.45f));
        }
        DrawCircleV(pos, radius, color);
    } else {
        float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
        Vector2 dir = { 0, -1 };
        if (speed > 0.0001f) {
            dir = { vel.x / speed, vel.y / speed };
        }
        float bulletLen = radius * 2.8f;
        Vector2 start = pos;
        Vector2 end = { pos.x - dir.x * bulletLen, pos.y - dir.y * bulletLen };
        DrawLineEx(start, end, radius * 1.5f, color);
        if (radius >= 6.0f) {
            DrawLineEx(start, end, radius * 0.6f, WHITE);
        }
    }
    if (owner == BulletOwner::Enemy) DrawCircleLines((int)pos.x, (int)pos.y, radius + 2, Fade(WHITE, 0.45f));
    if (debug) DrawCircleLines((int)pos.x, (int)pos.y, radius, GREEN);
}

bool Bullet::Offscreen(int width, int height) const {
    return pos.x < -40 || pos.x > width + 40 || pos.y < -60 || pos.y > height + 60;
}
