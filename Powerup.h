#pragma once
#include "raylib.h"

enum class PowerupType { WeaponChange, Upgrade, Bomb, Medal };

struct Powerup {
    PowerupType type = PowerupType::Medal;
    Vector2 pos{};
    Vector2 vel{0, 70};
    float radius = 10.0f;
    bool active = true;

    Powerup() = default;
    Powerup(PowerupType t, Vector2 p);

    void Update(float dt);
    void Draw() const;
    bool Offscreen(int height) const;
};
