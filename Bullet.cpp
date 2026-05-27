#include "Bullet.h"
#include "Enemy.h"
#include <cmath>

static Vector2 NormalizeOrZero(Vector2 v) {
    float len = std::sqrt(v.x * v.x + v.y * v.y);
    if (len <= 0.0001f) return {0, 0};
    return {v.x / len, v.y / len};
}

Bullet::Bullet(Vector2 p, Vector2 v, float r, int d, BulletOwner o, Color c, bool seek)
    : pos(p), vel(v), radius(r), damage(d), owner(o), homing(seek), color(c) {}

void Bullet::Update(float dt, const std::vector<Enemy>& enemies) {
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
    DrawCircleV(pos, radius, color);
    if (owner == BulletOwner::Enemy) DrawCircleLines((int)pos.x, (int)pos.y, radius + 2, Fade(WHITE, 0.45f));
    if (debug) DrawCircleLines((int)pos.x, (int)pos.y, radius, GREEN);
}

bool Bullet::Offscreen(int width, int height) const {
    return pos.x < -40 || pos.x > width + 40 || pos.y < -60 || pos.y > height + 60;
}
