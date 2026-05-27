#pragma once
#include "raylib.h"
#include <vector>

struct Bullet;
class Effects;

enum class EnemyType { Popcorn, Turret, Miniboss };

struct Enemy {
    EnemyType type = EnemyType::Popcorn;
    Vector2 pos{};
    Vector2 vel{};
    float radius = 14.0f;
    int hp = 3;
    int maxHp = 3;
    int scoreValue = 100;
    bool active = true;
    float age = 0.0f;
    float fireTimer = 0.0f;
    float phaseTimer = 0.0f;
    int phase = 0;
    float drift = 0.0f;
    float hitFlashTimer = 0.0f;
    int formationId = 0;
    float aimAngle = 0.0f;
    bool bossPanelsShed = false;  // Has boss shed its wing armor panels?

    Enemy() = default;
    Enemy(EnemyType t, Vector2 p, int loop, int formId = 0);

    void Update(float dt, Vector2 playerPos, std::vector<Bullet>& enemyBullets, int loop);
    void Draw(bool debug) const;
    void Hit(int damage, Effects& effects);
    bool Offscreen(int height) const;
    bool IsBoss() const { return type == EnemyType::Miniboss; }

private:
    void FireAimed(Vector2 playerPos, std::vector<Bullet>& enemyBullets, float speed, int damage = 1) const;
    void FireRadial(std::vector<Bullet>& enemyBullets, int count, float speed, float angleOffset) const;
};
