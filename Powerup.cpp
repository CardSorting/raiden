#include "Powerup.h"
#include "SpriteManager.h"
#include <cmath>

Powerup::Powerup(PowerupType t, Vector2 p) : type(t), pos(p) {
    vel = {0, 58.0f};
    radius = (type == PowerupType::Medal) ? 8.0f : 10.0f;
}

void Powerup::Update(float dt) {
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;
}

void Powerup::Draw() const {
    Color c = GOLD;
    SpriteId spriteId = SpriteId::PowerupMedal;
    if (type == PowerupType::WeaponChange) { c = SKYBLUE; spriteId = SpriteId::PowerupW; }
    else if (type == PowerupType::Upgrade) { c = ORANGE; spriteId = SpriteId::PowerupP; }
    else if (type == PowerupType::Bomb) { c = PURPLE; spriteId = SpriteId::PowerupB; }
    
    float time = (float)GetTime();
    float pulse = 1.0f + 1.5f * std::sin(time * 12.0f);
    float bob = std::sin(time * 6.0f) * 3.5f;
    Vector2 drawPos = { pos.x, pos.y + bob };
    
    // Draw pulsing outer glow rings
    DrawCircleLines((int)pos.x, (int)(pos.y + bob), (int)(radius + 3.0f + pulse), Fade(c, 0.45f));
    DrawCircleLines((int)pos.x, (int)(pos.y + bob), (int)radius, Fade(WHITE, 0.7f));
    
    // Draw high-quality pixel art powerup icon
    SpriteManager::Instance().Draw(spriteId, drawPos, 0.0f, 1.4f);
}

bool Powerup::Offscreen(int height) const {
    return pos.y > height + 30;
}

