#include "Powerup.h"
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
    const char* label = "$";
    if (type == PowerupType::WeaponChange) { c = SKYBLUE; label = "W"; }
    if (type == PowerupType::Upgrade) { c = ORANGE; label = "P"; }
    if (type == PowerupType::Bomb) { c = PURPLE; label = "B"; }
    
    float time = (float)GetTime();
    float pulse = 1.0f + 1.5f * std::sin(time * 12.0f);
    
    // Draw pulsing outer glow
    DrawCircleLines((int)pos.x, (int)pos.y, (int)(radius + 3.0f + pulse), Fade(c, 0.45f));
    DrawCircleLines((int)pos.x, (int)pos.y, (int)radius, Fade(WHITE, 0.7f));
    
    // Draw rotating vector diamond
    float angle = time * 2.8f;
    float dRadius = radius - 1.0f;
    Vector2 p1 = { pos.x + std::cos(angle) * dRadius, pos.y + std::sin(angle) * dRadius };
    Vector2 p2 = { pos.x + std::cos(angle + 1.57079f) * dRadius, pos.y + std::sin(angle + 1.57079f) * dRadius };
    Vector2 p3 = { pos.x + std::cos(angle + 3.14159f) * dRadius, pos.y + std::sin(angle + 3.14159f) * dRadius };
    Vector2 p4 = { pos.x + std::cos(angle + 4.71239f) * dRadius, pos.y + std::sin(angle + 4.71239f) * dRadius };
    
    DrawLineEx(p1, p2, 1.5f, c);
    DrawLineEx(p2, p3, 1.5f, c);
    DrawLineEx(p3, p4, 1.5f, c);
    DrawLineEx(p4, p1, 1.5f, c);
    
    int w = MeasureText(label, 10);
    DrawText(label, (int)(pos.x - w / 2), (int)(pos.y - 5), 10, RAYWHITE);
}

bool Powerup::Offscreen(int height) const {
    return pos.y > height + 30;
}
