#pragma once
#include "raylib.h"
#include <vector>

struct Enemy;

enum class BulletOwner { Player, Enemy };

struct Bullet {
    Vector2 pos{};
    Vector2 vel{};
    float radius = 4.0f;
    int damage = 1;
    BulletOwner owner = BulletOwner::Player;
    bool active = true;
    bool homing = false;
    Color color = WHITE;
    Vector2 trail_[6]{};
    int trailCount_ = 0;

    Bullet() = default;
    Bullet(Vector2 p, Vector2 v, float r, int d, BulletOwner o, Color c, bool seek = false);

    void Update(float dt, const std::vector<Enemy>& enemies);
    void Draw(bool debug) const;
    bool Offscreen(int width, int height) const;
};
