#pragma once
#include "raylib.h"
#include "SpriteManager.h"
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

struct AnimatedExplosion {
    Vector2 pos{};
    float life = 0.0f;
    float maxLife = 0.45f;
    float scale = 1.0f;
    float rotation = 0.0f;
};

struct Shockwave {
    Vector2 pos{};
    float radius = 0.0f;
    float maxRadius = 45.0f;
    float speed = 160.0f;
    Color color = WHITE;
};

struct Ember {
    Vector2 pos{};
    Vector2 vel{};
    float life = 1.0f;
    float maxLife = 1.0f;
    float radius = 1.5f;
    Color color = WHITE;
};

struct Debris {
    Vector2 pos{};
    Vector2 vel{};
    float rotation = 0.0f;
    float spinSpeed = 0.0f;
    float life = 1.0f;
    float maxLife = 1.0f;
    float size = 3.0f;
    Color color = WHITE;
    SpriteId spriteId = SpriteId::AsteroidChunk;
    bool useSprite = false;
};

class Effects {
public:
    void Update(float dt);
    void Draw() const;
    void Clear();
    void Explosion(Vector2 pos, Color color, int count = 24, SpriteId debrisSprite = SpriteId::AsteroidChunk);
    void Spark(Vector2 pos, Color color, Vector2 biasVel = {0.0f, 0.0f});
    void EngineExhaust(Vector2 pos, Color color = ORANGE, Vector2 biasVel = {0.0f, 0.0f});
    void Smoke(Vector2 pos, Color color = GRAY);
    void DebrisShower(Vector2 pos, Color color, int count);
    void Shake(float amount, float time);
    Vector2 ShakeOffset() const;
    void AddText(Vector2 pos, const char* text, Color color = WHITE);

private:
    std::vector<Particle> particles_;
    std::vector<TextParticle> textParticles_;
    std::vector<AnimatedExplosion> animatedExplosions_;
    std::vector<Shockwave> shockwaves_;
    std::vector<Ember> embers_;
    std::vector<Debris> debris_;
    float shakeTime_ = 0.0f;
    float shakeAmount_ = 0.0f;
};


