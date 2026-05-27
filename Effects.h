#pragma once
#include "raylib.h"
#include <vector>

struct Particle {
    Vector2 pos{};
    Vector2 vel{};
    float life = 1.0f;
    float maxLife = 1.0f;
    float radius = 2.0f;
    Color color = WHITE;
};

struct TextParticle {
    Vector2 pos{};
    char text[32]{};
    Color color = WHITE;
    float life = 1.0f;
    float maxLife = 1.0f;
};

class Effects {
public:
    void Update(float dt);
    void Draw() const;
    void Clear();
    void Explosion(Vector2 pos, Color color, int count = 24);
    void Spark(Vector2 pos, Color color);
    void Shake(float amount, float time);
    Vector2 ShakeOffset() const;
    void AddText(Vector2 pos, const char* text, Color color = WHITE);

private:
    std::vector<Particle> particles_;
    std::vector<TextParticle> textParticles_;
    float shakeTime_ = 0.0f;
    float shakeAmount_ = 0.0f;
};
