#include "Powerup.h"

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
    DrawCircleV(pos, radius, c);
    DrawCircleLines((int)pos.x, (int)pos.y, radius + 2, WHITE);
    int w = MeasureText(label, 10);
    DrawText(label, (int)(pos.x - w / 2), (int)(pos.y - 5), 10, BLACK);
}

bool Powerup::Offscreen(int height) const {
    return pos.y > height + 30;
}
