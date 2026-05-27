#include "Player.h"
#include <algorithm>
#include <cmath>

void Player::ResetForNewGame() {
    pos = {240, 560};
    lives = 3;
    bombs = 3;
    score = 0;
    weaponLevel = 1;
    weapon = WeaponType::Vulcan;
    invulnerable = false;
    invulnTimer = 0.0f;
    shootTimer_ = 0.0f;
    isDemo = false;
    triggerBomb_ = false;
    animationTime = 0.0f;
}

void Player::ResetAfterHit() {
    pos = {240, 560};
    invulnerable = true;
    invulnTimer = 2.0f;
    weaponLevel = std::max(1, weaponLevel - 1);
    triggerBomb_ = false;
}

void Player::Update(float dt, const std::vector<Enemy>& enemies, const std::vector<Bullet>& enemyBullets) {
    Vector2 dir{0, 0};
    if (isDemo) {
        // Simple AI logic
        float targetX = 240.0f;
        const Enemy* closestEnemy = nullptr;
        float minDist = 999999.0f;
        for (const auto& e : enemies) {
            if (!e.active) continue;
            float dy = pos.y - e.pos.y;
            if (dy > 0) { // Enemy is above player
                float dist = std::abs(e.pos.x - pos.x) + dy;
                if (dist < minDist) {
                    minDist = dist;
                    closestEnemy = &e;
                }
            }
        }
        if (closestEnemy) {
            targetX = closestEnemy->pos.x;
        }

        // Steer horizontally to target
        if (pos.x < targetX - 6.0f) dir.x += 1.0f;
        else if (pos.x > targetX + 6.0f) dir.x -= 1.0f;

        // Try to hover around y = 520
        if (pos.y < 500.0f) dir.y += 0.5f;
        else if (pos.y > 540.0f) dir.y -= 0.5f;

        // Dodge bullets
        bool bulletVeryClose = false;
        for (const auto& b : enemyBullets) {
            if (!b.active) continue;
            float dx = pos.x - b.pos.x;
            float dy = pos.y - b.pos.y;
            float distSq = dx * dx + dy * dy;
            if (distSq < 96.0f * 96.0f) {
                float dist = std::sqrt(distSq);
                if (dist > 0.001f) {
                    float force = (96.0f - dist) / 96.0f;
                    dir.x += (dx / dist) * force * 3.2f;
                    dir.y += (dy / dist) * force * 3.2f;
                }
                if (distSq < 38.0f * 38.0f) {
                    bulletVeryClose = true;
                }
            }
        }

        // Trigger bomb if in severe danger
        if (bulletVeryClose && bombs > 0 && !invulnerable) {
            triggerBomb_ = true;
        }
    } else {
        // Keyboard movement
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) dir.x -= 1;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) dir.x += 1;
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) dir.y -= 1;
        if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) dir.y += 1;

        // Gamepad movement
        if (IsGamepadAvailable(0)) {
            float gdx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
            float gdy = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
            if (std::abs(gdx) > 0.2f) dir.x += gdx;
            if (std::abs(gdy) > 0.2f) dir.y += gdy;

            if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) dir.x -= 1;
            if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) dir.x += 1;
            if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) dir.y -= 1;
            if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) dir.y += 1;
        }
    }

    if (dir.x != 0 || dir.y != 0) {
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        dir.x /= len; dir.y /= len;
    }
    
    bool focusMode = !isDemo && (IsKeyDown(KEY_LEFT_SHIFT) || (IsGamepadAvailable(0) && (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1) || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1))));
    float speed = focusMode ? 145.0f : 245.0f;
    pos.x = std::clamp(pos.x + dir.x * speed * dt, 24.0f, 456.0f);
    pos.y = std::clamp(pos.y + dir.y * speed * dt, 72.0f, 610.0f);
    shootTimer_ = std::max(0.0f, shootTimer_ - dt);
    animationTime += dt;
    if (invulnerable) {
        invulnTimer -= dt;
        if (invulnTimer <= 0.0f) { invulnerable = false; invulnTimer = 0.0f; }
    }
}

void Player::TryShoot(std::vector<Bullet>& bullets) {
    bool shootPressed = isDemo || IsKeyDown(KEY_Z) || IsKeyDown(KEY_SPACE) || (IsGamepadAvailable(0) && (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)));
    if (!shootPressed || shootTimer_ > 0.0f) return;
    int level = std::clamp(weaponLevel, 1, 4);
    if (weapon == WeaponType::Vulcan) {
        shootTimer_ = 0.105f;
        bullets.emplace_back(Vector2{pos.x, pos.y - 18}, Vector2{0, -520}, 4, 1, BulletOwner::Player, YELLOW);
        if (level >= 2) {
            bullets.emplace_back(Vector2{pos.x - 10, pos.y - 10}, Vector2{-65, -500}, 4, 1, BulletOwner::Player, GOLD);
            bullets.emplace_back(Vector2{pos.x + 10, pos.y - 10}, Vector2{65, -500}, 4, 1, BulletOwner::Player, GOLD);
        }
        if (level >= 4) {
            bullets.emplace_back(Vector2{pos.x - 16, pos.y}, Vector2{-130, -470}, 4, 1, BulletOwner::Player, ORANGE);
            bullets.emplace_back(Vector2{pos.x + 16, pos.y}, Vector2{130, -470}, 4, 1, BulletOwner::Player, ORANGE);
        }
    } else if (weapon == WeaponType::Plasma) {
        shootTimer_ = 0.16f;
        bullets.emplace_back(Vector2{pos.x, pos.y - 20}, Vector2{0, -390}, 8 + level, 2, BulletOwner::Player, SKYBLUE);
        if (level >= 2) {
            bullets.emplace_back(Vector2{pos.x - 18, pos.y - 8}, Vector2{-90, -345}, 7, 1, BulletOwner::Player, BLUE);
            bullets.emplace_back(Vector2{pos.x + 18, pos.y - 8}, Vector2{90, -345}, 7, 1, BulletOwner::Player, BLUE);
        }
    } else {
        shootTimer_ = 0.20f;
        bullets.emplace_back(Vector2{pos.x, pos.y - 18}, Vector2{0, -440}, 4, 1, BulletOwner::Player, LIME);
        bullets.emplace_back(Vector2{pos.x - 15, pos.y}, Vector2{-45, -260}, 6, 2, BulletOwner::Player, GREEN, true);
        if (level >= 3) bullets.emplace_back(Vector2{pos.x + 15, pos.y}, Vector2{45, -260}, 6, 2, BulletOwner::Player, GREEN, true);
    }
}

bool Player::TryBomb() {
    bool bombPressed = IsKeyPressed(KEY_X) || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) || (isDemo && triggerBomb_);
    if (!bombPressed || bombs <= 0) return false;
    --bombs;
    invulnerable = true;
    invulnTimer = 1.1f;
    triggerBomb_ = false;
    return true;
}

void Player::NextWeapon() {
    if (weapon == WeaponType::Vulcan) weapon = WeaponType::Plasma;
    else if (weapon == WeaponType::Plasma) weapon = WeaponType::Missile;
    else weapon = WeaponType::Vulcan;
}

void Player::UpgradeWeapon() { weaponLevel = std::min(4, weaponLevel + 1); }

const char* Player::WeaponName() const {
    if (weapon == WeaponType::Vulcan) return "VULCAN";
    if (weapon == WeaponType::Plasma) return "PLASMA";
    return "MISSILE";
}

void Player::Draw(bool debug) const {
    bool blink = invulnerable && ((int)(invulnTimer * 12) % 2 == 0);
    
    // Engine thrusters - flickering plume
    if (!blink) {
        float flameH = 10.0f + std::sin(animationTime * 45.0f) * 4.0f;
        DrawTriangle({pos.x - 6, pos.y + 14}, {pos.x - 9, pos.y + 14 + flameH}, {pos.x - 3, pos.y + 14}, SKYBLUE);
        DrawTriangle({pos.x - 5, pos.y + 14}, {pos.x - 7, pos.y + 14 + (flameH * 0.6f)}, {pos.x - 4, pos.y + 14}, WHITE);
        
        DrawTriangle({pos.x + 6, pos.y + 14}, {pos.x + 3, pos.y + 14}, {pos.x + 9, pos.y + 14 + flameH}, SKYBLUE);
        DrawTriangle({pos.x + 5, pos.y + 14}, {pos.x + 4, pos.y + 14}, {pos.x + 7, pos.y + 14 + (flameH * 0.6f)}, WHITE);
    }
    
    if (!blink) {
        // Detailed Wings
        DrawTriangle({pos.x, pos.y - 12}, {pos.x - 18, pos.y + 6}, {pos.x - 6, pos.y + 6}, GRAY);
        DrawTriangle({pos.x, pos.y - 12}, {pos.x + 6, pos.y + 6}, {pos.x + 18, pos.y + 6}, GRAY);
        
        // Fuselage
        DrawTriangle({pos.x, pos.y - 22}, {pos.x - 12, pos.y + 14}, {pos.x + 12, pos.y + 14}, RAYWHITE);
        DrawTriangle({pos.x, pos.y - 12}, {pos.x - 6, pos.y + 10}, {pos.x + 6, pos.y + 10}, BLUE);
        
        // Glowing canopy
        DrawCircleV({pos.x, pos.y - 2}, 4, SKYBLUE);
        DrawCircleV({pos.x - 1, pos.y - 3}, 1, WHITE);
        
        DrawRectangle((int)pos.x - 2, (int)pos.y + 12, 4, 8, ORANGE);
    }
    DrawCircleV(pos, hitRadius, RED);
    
    if (invulnerable) {
        float bubbleRadius = spriteRadius + 4.0f + std::sin(animationTime * 18.0f) * 2.0f;
        DrawCircleLines((int)pos.x, (int)pos.y, bubbleRadius, Fade(SKYBLUE, 0.75f));
        float angle = animationTime * 5.0f;
        Vector2 orb1 = { pos.x + std::cos(angle) * bubbleRadius, pos.y + std::sin(angle) * bubbleRadius };
        Vector2 orb2 = { pos.x + std::cos(angle + 3.14159f) * bubbleRadius, pos.y + std::sin(angle + 3.14159f) * bubbleRadius };
        DrawCircleV(orb1, 3.0f, SKYBLUE);
        DrawCircleV(orb2, 3.0f, SKYBLUE);
    }
    
    if (debug) {
        DrawCircleLines((int)pos.x, (int)pos.y, spriteRadius, GREEN);
        DrawCircleLines((int)pos.x, (int)pos.y, hitRadius, RED);
    }
}
