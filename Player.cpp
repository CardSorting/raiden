#include "Player.h"
#include "Effects.h"
#include "SpriteManager.h"
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
    tilt_ = 0.0f;
    muzzleFlashTimer_ = 0.0f;
    recoilOffset_ = 0.0f;
    recoilVel_ = 0.0f;
    scaleY_ = 1.0f;
    scaleYVel_ = 0.0f;
    heat_ = 0.0f;
    positionCount_ = 0;
    for (int i = 0; i < 8; ++i) pastPositions_[i] = pos;
}

void Player::ResetAfterHit() {
    pos = {240, 560};
    invulnerable = true;
    invulnTimer = 2.0f;
    weaponLevel = std::max(1, weaponLevel - 1);
    triggerBomb_ = false;
    tilt_ = 0.0f;
    muzzleFlashTimer_ = 0.0f;
    recoilOffset_ = 0.0f;
    recoilVel_ = 0.0f;
    scaleY_ = 1.0f;
    scaleYVel_ = 0.0f;
    heat_ = 0.0f;
    positionCount_ = 0;
    for (int i = 0; i < 8; ++i) pastPositions_[i] = pos;
}

void Player::Update(float dt, Effects& effects, const std::vector<Enemy>& enemies, const std::vector<Bullet>& enemyBullets) {
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
        if (controlLayout == 0) {
            if (IsKeyDown(KEY_LEFT)) dir.x -= 1;
            if (IsKeyDown(KEY_RIGHT)) dir.x += 1;
            if (IsKeyDown(KEY_UP)) dir.y -= 1;
            if (IsKeyDown(KEY_DOWN)) dir.y += 1;
        } else {
            if (IsKeyDown(KEY_A)) dir.x -= 1;
            if (IsKeyDown(KEY_D)) dir.x += 1;
            if (IsKeyDown(KEY_W)) dir.y -= 1;
            if (IsKeyDown(KEY_S)) dir.y += 1;
        }

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

    // Update coordinates history for ghost trails
    for (int i = 7; i > 0; --i) {
        pastPositions_[i] = pastPositions_[i - 1];
    }
    pastPositions_[0] = pos;
    if (positionCount_ < 8) positionCount_++;

    // Momentum-Based Banking (exponential drag interpolation)
    float targetTilt = dir.x * 2.0f;
    float tiltSpeed = (dir.x == 0.0f) ? 14.0f : 10.0f; // centers slightly quicker than it banks
    tilt_ += (targetTilt - tilt_) * tiltSpeed * dt;

    if (muzzleFlashTimer_ > 0.0f) {
        muzzleFlashTimer_ -= dt;
    }

    // Physical Spring-Damper Recoil Physics
    // Recoil offset spring system (target: 0.0f)
    float recoilK = 220.0f;
    float recoilDamping = 18.0f;
    float recoilForce = -recoilK * recoilOffset_ - recoilDamping * recoilVel_;
    recoilVel_ += recoilForce * dt;
    recoilOffset_ += recoilVel_ * dt;
    if (recoilOffset_ < 0.0f) {
        recoilOffset_ = 0.0f;
        recoilVel_ = 0.0f;
    }

    // scaleY_ spring system (target: 1.0f)
    float scaleK = 380.0f;
    float scaleDamping = 20.0f;
    float scaleForce = -scaleK * (scaleY_ - 1.0f) - scaleDamping * scaleYVel_;
    scaleYVel_ += scaleForce * dt;
    scaleY_ += scaleYVel_ * dt;

    // Decay wing vent heat
    heat_ = std::max(0.0f, heat_ - 1.8f * dt);
    if (heat_ > 0.2f && GetRandomValue(0, 100) < (int)(heat_ * 60)) {
        effects.Spark({ pos.x - 14.0f + GetRandomValue(-2, 2), pos.y + 2.0f }, ORANGE, {dir.x * -100.0f, 40.0f});
        effects.Spark({ pos.x + 14.0f + GetRandomValue(-2, 2), pos.y + 2.0f }, ORANGE, {dir.x * -100.0f, 40.0f});
    }

    // Spawn procedural vectored engine exhaust particles
    if (GetRandomValue(0, 100) < 65) {
        Color thrusterColor = focusMode ? SKYBLUE : (GetRandomValue(0, 1) == 0 ? ORANGE : GOLD);
        
        // Calculate dynamic vectored exhaust direction based on ship movement and roll tilt
        // Plumes tilt opposite to movement direction and banking angle
        float tiltExhaustX = -tilt_ * 30.0f - dir.x * 50.0f;
        float exhaustY = focusMode ? 40.0f : (dir.y < 0.0f ? 140.0f : 80.0f);
        
        // Asymmetric plume speeds depending on left/right banking
        float leftThrustMult = 1.0f;
        float rightThrustMult = 1.0f;
        if (dir.x < 0.0f) { rightThrustMult = 1.35f; leftThrustMult = 0.65f; }
        else if (dir.x > 0.0f) { leftThrustMult = 1.35f; rightThrustMult = 0.65f; }
        if (dir.y < 0.0f) { leftThrustMult *= 1.25f; rightThrustMult *= 1.25f; }
        
        effects.EngineExhaust({ pos.x - 6.0f, pos.y + 9.0f }, thrusterColor, { tiltExhaustX - 10.0f, exhaustY * leftThrustMult });
        effects.EngineExhaust({ pos.x + 6.0f, pos.y + 9.0f }, thrusterColor, { tiltExhaustX + 10.0f, exhaustY * rightThrustMult });
    }
}

void Player::TryShoot(std::vector<Bullet>& bullets) {
    bool shootPressed = isDemo;
    if (!shootPressed) {
        if (controlLayout == 0) {
            shootPressed = IsKeyDown(KEY_Z) || IsKeyDown(KEY_SPACE);
        } else {
            shootPressed = IsKeyDown(KEY_J) || IsKeyDown(KEY_SPACE);
        }
        if (IsGamepadAvailable(0)) {
            shootPressed = shootPressed || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
        }
    }
    if (!shootPressed || shootTimer_ > 0.0f) return;
    
    int level = std::clamp(weaponLevel, 1, 4);
    if (weapon == WeaponType::Vulcan) {
        recoilVel_ += 130.0f;
        scaleYVel_ -= 9.0f;
        heat_ = std::min(1.0f, heat_ + 0.05f);
        muzzleFlashTimer_ = 0.04f;
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
        recoilVel_ += 360.0f;
        scaleYVel_ -= 28.0f;
        heat_ = std::min(1.0f, heat_ + 0.20f);
        muzzleFlashTimer_ = 0.08f;
        shootTimer_ = 0.16f;
        bullets.emplace_back(Vector2{pos.x, pos.y - 20}, Vector2{0, -390}, 8 + level, 2, BulletOwner::Player, SKYBLUE);
        if (level >= 2) {
            bullets.emplace_back(Vector2{pos.x - 18, pos.y - 8}, Vector2{-90, -345}, 7, 1, BulletOwner::Player, BLUE);
            bullets.emplace_back(Vector2{pos.x + 18, pos.y - 8}, Vector2{90, -345}, 7, 1, BulletOwner::Player, BLUE);
        }
    } else {
        recoilVel_ += 210.0f;
        scaleYVel_ -= 16.0f;
        heat_ = std::min(1.0f, heat_ + 0.12f);
        muzzleFlashTimer_ = 0.06f;
        shootTimer_ = 0.20f;
        bullets.emplace_back(Vector2{pos.x, pos.y - 18}, Vector2{0, -440}, 4, 1, BulletOwner::Player, LIME);
        bullets.emplace_back(Vector2{pos.x - 15, pos.y}, Vector2{-45, -260}, 6, 2, BulletOwner::Player, GREEN, true);
        if (level >= 3) bullets.emplace_back(Vector2{pos.x + 15, pos.y}, Vector2{45, -260}, 6, 2, BulletOwner::Player, GREEN, true);
    }
}

bool Player::BombPressed() const {
    bool bombPressed = false;
    if (isDemo) {
        bombPressed = triggerBomb_;
    } else {
        if (controlLayout == 0) {
            bombPressed = IsKeyPressed(KEY_X);
        } else {
            bombPressed = IsKeyPressed(KEY_K);
        }
        if (IsGamepadAvailable(0)) {
            bombPressed = bombPressed || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
        }
    }
    return bombPressed;
}

bool Player::TryBomb() {
    if (!BombPressed() || bombs <= 0) return false;
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
    Vector2 drawPos = { pos.x, pos.y + recoilOffset_ };
    Color tint = WHITE;

    if (invulnerable && invulnTimer > 1.2f) {
        // Glitch state visual offset & red shift
        drawPos.x += (float)GetRandomValue(-20, 20) / 10.0f;
        tint = (GetRandomValue(0, 1) == 0) ? Color{255, 60, 60, 255} : Color{255, 255, 255, 120};
    }
    
    // 1. Engine thrusters plume flickering
    bool focusMode = !isDemo && (IsKeyDown(KEY_LEFT_SHIFT) || (IsGamepadAvailable(0) && (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1) || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1))));
    
    float plumeBase = 1.0f + 0.35f * std::sin(animationTime * 48.0f);
    float leftPlumeScale = plumeBase;
    float rightPlumeScale = plumeBase;
    
    float dx = 0.0f;
    float dy = 0.0f;
    if (positionCount_ >= 2) {
        dx = pos.x - pastPositions_[1].x;
        dy = pos.y - pastPositions_[1].y;
    }
    
    if (dy < -0.1f) {
        leftPlumeScale *= 1.35f;
        rightPlumeScale *= 1.35f;
    } else if (dy > 0.1f) {
        leftPlumeScale *= 0.6f;
        rightPlumeScale *= 0.6f;
    }
    
    if (dx < -0.1f) {
        rightPlumeScale *= 1.3f;
        leftPlumeScale *= 0.7f;
    } else if (dx > 0.1f) {
        leftPlumeScale *= 1.3f;
        rightPlumeScale *= 0.7f;
    }
    
    if (focusMode) {
        leftPlumeScale *= 0.75f;
        rightPlumeScale *= 0.75f;
    }

    float tiltOffset = -tilt_ * 3.2f;

    if (!blink) {
        Color plumeColor1 = focusMode ? Color{0, 150, 255, 180} : Color{255, 100, 0, 180};
        Color plumeColor2 = focusMode ? Color{100, 220, 255, 230} : Color{255, 220, 0, 230};
        Color glowColor = focusMode ? SKYBLUE : ORANGE;

        // Emissive engine exhaust bloom (additive circular gradients)
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleGradient((int)(drawPos.x - 6.0f), (int)(drawPos.y + 11.0f), 12.0f * leftPlumeScale, Fade(glowColor, 0.45f), Fade(BLACK, 0.0f));
        DrawCircleGradient((int)(drawPos.x + 6.0f), (int)(drawPos.y + 11.0f), 12.0f * rightPlumeScale, Fade(glowColor, 0.45f), Fade(BLACK, 0.0f));
        EndBlendMode();

        // Draw left nozzle plume
        DrawTriangle(
            {drawPos.x - 6.0f, drawPos.y + 11.0f},
            {drawPos.x - 6.0f + tiltOffset, drawPos.y + 11.0f + 14.0f * leftPlumeScale},
            {drawPos.x - 4.0f, drawPos.y + 11.0f},
            plumeColor1
        );
        DrawTriangle(
            {drawPos.x - 5.5f, drawPos.y + 11.0f},
            {drawPos.x - 5.5f + tiltOffset, drawPos.y + 11.0f + 8.0f * leftPlumeScale},
            {drawPos.x - 4.5f, drawPos.y + 11.0f},
            plumeColor2
        );

        // Draw right nozzle plume
        DrawTriangle(
            {drawPos.x + 6.0f, drawPos.y + 11.0f},
            {drawPos.x + 4.0f, drawPos.y + 11.0f},
            {drawPos.x + 6.0f + tiltOffset, drawPos.y + 11.0f + 14.0f * rightPlumeScale},
            plumeColor1
        );
        DrawTriangle(
            {drawPos.x + 5.5f, drawPos.y + 11.0f},
            {drawPos.x + 4.5f, drawPos.y + 11.0f},
            {drawPos.x + 5.5f + tiltOffset, drawPos.y + 11.0f + 8.0f * rightPlumeScale},
            plumeColor2
        );
    }
    
    // 1b. Ghost afterimage trails
    if (!blink && !focusMode && (dx * dx + dy * dy > 0.02f)) {
        BeginBlendMode(BLEND_ADDITIVE);
        for (int i = 1; i < 4; ++i) {
            int idx = i * 2;
            if (idx >= positionCount_) break;
            Vector2 ghostPos = pastPositions_[idx];
            
            float alpha = 0.38f - (float)i * 0.11f;
            if (alpha <= 0.0f) continue;
            
            SpriteId spriteId = SpriteId::PlayerIdle;
            if (tilt_ < -1.1f) spriteId = SpriteId::PlayerLeft;
            else if (tilt_ < -0.3f) spriteId = SpriteId::PlayerSoftLeft;
            else if (tilt_ > 1.1f) spriteId = SpriteId::PlayerRight;
            else if (tilt_ > 0.3f) spriteId = SpriteId::PlayerSoftRight;
            
            Texture2D tex = SpriteManager::Instance().GetTexture(spriteId);
            if (tex.id != 0) {
                Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
                Rectangle dest = { ghostPos.x, ghostPos.y + recoilOffset_, (float)tex.width * 1.5f, (float)tex.height * 1.5f * scaleY_ };
                Vector2 origin = { (float)tex.width * 1.5f / 2.0f, (float)tex.height * 1.5f * scaleY_ / 2.0f };
                Color trailColor = Fade(SKYBLUE, alpha);
                SpriteManager::Instance().DrawRect(spriteId, src, dest, origin, 0.0f, trailColor);
            }
        }
        EndBlendMode();
    }

    // 2. Render Player Ship Sprite with 5 Banking States and Recoil Squash
    if (!blink) {
        SpriteId spriteId = SpriteId::PlayerIdle;
        if (tilt_ < -1.1f) spriteId = SpriteId::PlayerLeft;
        else if (tilt_ < -0.3f) spriteId = SpriteId::PlayerSoftLeft;
        else if (tilt_ > 1.1f) spriteId = SpriteId::PlayerRight;
        else if (tilt_ > 0.3f) spriteId = SpriteId::PlayerSoftRight;
        
        Texture2D tex = SpriteManager::Instance().GetTexture(spriteId);
        if (tex.id != 0) {
            Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
            Rectangle dest = { drawPos.x, drawPos.y, (float)tex.width * 1.5f, (float)tex.height * 1.5f * scaleY_ };
            Vector2 origin = { (float)tex.width * 1.5f / 2.0f, (float)tex.height * 1.5f * scaleY_ / 2.0f };
            SpriteManager::Instance().DrawRect(spriteId, src, dest, origin, 0.0f, tint);
        }

        // Additive glowing canopy cockpit lighting
        BeginBlendMode(BLEND_ADDITIVE);
        float pulseCyan = 0.5f + 0.5f * std::sin(animationTime * 12.0f);
        Vector2 canopyPos = { drawPos.x, drawPos.y - 9.0f * scaleY_ };
        DrawCircleGradient((int)canopyPos.x, (int)canopyPos.y, 6.0f + 2.5f * pulseCyan, Fade(SKYBLUE, 0.55f * (0.6f + 0.4f * pulseCyan)), Fade(BLACK, 0.0f));
        DrawCircleV(canopyPos, 2.0f, Fade(WHITE, 0.7f));
        EndBlendMode();

        // Low-health critical warning pulse
        if (lives == 1) {
            float alertPulse = 0.5f + 0.5f * std::sin(animationTime * 16.0f);
            DrawCircleLines((int)drawPos.x, (int)drawPos.y, 22.0f, Fade(RED, alertPulse * 0.7f));
        }
    }

    // 3. Render Muzzle Flashes
    if (muzzleFlashTimer_ > 0.0f) {
        SpriteId flashId = SpriteId::MuzzleVulcan;
        if (weapon == WeaponType::Plasma) flashId = SpriteId::MuzzlePlasma;
        else if (weapon == WeaponType::Missile) flashId = SpriteId::MuzzleMissile;

        int level = std::clamp(weaponLevel, 1, 4);
        if (weapon == WeaponType::Vulcan) {
            SpriteManager::Instance().Draw(flashId, {drawPos.x, drawPos.y - 18}, 0.0f, 1.2f);
            if (level >= 2) {
                SpriteManager::Instance().Draw(flashId, {drawPos.x - 10, drawPos.y - 10}, -15.0f, 1.0f);
                SpriteManager::Instance().Draw(flashId, {drawPos.x + 10, drawPos.y - 10}, 15.0f, 1.0f);
            }
            if (level >= 4) {
                SpriteManager::Instance().Draw(flashId, {drawPos.x - 16, drawPos.y}, -30.0f, 1.0f);
                SpriteManager::Instance().Draw(flashId, {drawPos.x + 16, drawPos.y}, 30.0f, 1.0f);
            }
        } else if (weapon == WeaponType::Plasma) {
            SpriteManager::Instance().Draw(flashId, {drawPos.x, drawPos.y - 20}, 0.0f, 1.4f);
            if (level >= 2) {
                SpriteManager::Instance().Draw(flashId, {drawPos.x - 18, drawPos.y - 8}, -20.0f, 1.1f);
                SpriteManager::Instance().Draw(flashId, {drawPos.x + 18, drawPos.y - 8}, 20.0f, 1.1f);
            }
        } else { // Missile
            SpriteManager::Instance().Draw(flashId, {drawPos.x, drawPos.y - 18}, 0.0f, 1.2f);
            SpriteManager::Instance().Draw(flashId, {drawPos.x - 15, drawPos.y}, -10.0f, 1.0f);
            if (level >= 3) {
                SpriteManager::Instance().Draw(flashId, {drawPos.x + 15, drawPos.y}, 10.0f, 1.0f);
            }
        }
    }

    // 4. Render Invulnerability Shield Bubble (with electric arcing)
    if (invulnerable) {
        float scale = 1.35f + 0.12f * std::sin(animationTime * 20.0f);
        SpriteManager::Instance().Draw(SpriteId::ShieldBubble, drawPos, animationTime * 180.0f, scale, Fade(SKYBLUE, 0.75f));
        
        // Additive electric shimmer rings and arcs
        BeginBlendMode(BLEND_ADDITIVE);
        float pulseRadius = 14.0f + 16.0f * fmodf(animationTime * 1.5f, 1.0f);
        DrawCircleLines((int)drawPos.x, (int)drawPos.y, pulseRadius, Fade(SKYBLUE, 1.0f - (pulseRadius - 14.0f) / 16.0f));
        
        for (int i = 0; i < 3; ++i) {
            float angle1 = (float)GetRandomValue(0, 360) * DEG2RAD;
            float angle2 = angle1 + (float)GetRandomValue(20, 50) * DEG2RAD;
            float r = spriteRadius * (1.1f + 0.2f * std::sin(animationTime * 35.0f));
            Vector2 p1 = { drawPos.x + std::cos(angle1) * r, drawPos.y + std::sin(angle1) * r };
            Vector2 p2 = { drawPos.x + std::cos(angle2) * r, drawPos.y + std::sin(angle2) * r };
            DrawLineV(p1, p2, Fade(WHITE, 0.75f));
        }
        EndBlendMode();
    }
    
    // Always render small visible red hitbox point
    DrawCircleV(drawPos, hitRadius, RED);
    
    if (debug) {
        DrawCircleLines((int)pos.x, (int)pos.y, spriteRadius, GREEN);
        DrawCircleLines((int)pos.x, (int)pos.y, hitRadius, RED);
    }
}

