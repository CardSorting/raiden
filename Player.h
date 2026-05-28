#pragma once
#include "raylib.h"
#include "Bullet.h"
#include "Enemy.h"
#include <vector>

class Effects;

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
    int controlLayout = 0;
    float tilt_ = 0.0f;             // Horizontal banking/tilt animation state
    float muzzleFlashTimer_ = 0.0f;  // Screen duration timer for weapon fire sparks
    float recoilOffset_ = 0.0f;      // Recoil shift offset
    float recoilVel_ = 0.0f;         // Recoil speed velocity for spring physics
    float scaleY_ = 1.0f;            // Squash/stretch recoil scaling factor
    float scaleYVel_ = 0.0f;         // Scale elasticity speed velocity for spring physics
    float heat_ = 0.0f;              // Wing vent heat level for ember particles
    Vector2 pastPositions_[8]{};    // Ring buffer of past positions for ghost trails
    int positionCount_ = 0;          // Count of elements in the past positions buffer

    void ResetForNewGame();
    void ResetAfterHit();
    void Update(float dt, Effects& effects, const std::vector<Enemy>& enemies = {}, const std::vector<Bullet>& enemyBullets = {});
    void Draw(bool debug) const;
    void TryShoot(std::vector<Bullet>& playerBullets);
    bool BombPressed() const;
    bool TryBomb();
    void NextWeapon();
    void UpgradeWeapon();
    const char* WeaponName() const;

private:
    float shootTimer_ = 0.0f;
    bool pointerControlActive_ = false;
    float pointerIdleTimer_ = 0.0f;
};
