#pragma once
#include "raylib.h"
#include "Bullet.h"
#include "Enemy.h"
#include <vector>

enum class WeaponType { Vulcan, Plasma, Missile };

class Player {
public:
    Vector2 pos{240, 560};
    float spriteRadius = 16.0f;
    float hitRadius = 4.0f;
    int lives = 3;
    int bombs = 3;
    int score = 0;
    int weaponLevel = 1;
    WeaponType weapon = WeaponType::Vulcan;
    bool invulnerable = false;
    float invulnTimer = 0.0f;
    bool isDemo = false;
    bool triggerBomb_ = false;
    float animationTime = 0.0f;

    void ResetForNewGame();
    void ResetAfterHit();
    void Update(float dt, const std::vector<Enemy>& enemies = {}, const std::vector<Bullet>& enemyBullets = {});
    void Draw(bool debug) const;
    void TryShoot(std::vector<Bullet>& playerBullets);
    bool TryBomb();
    void NextWeapon();
    void UpgradeWeapon();
    const char* WeaponName() const;

private:
    float shootTimer_ = 0.0f;
};

