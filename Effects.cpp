#include "Effects.h"
#include <algorithm>
#include <cmath>

void Effects::Update(float dt) {
    for (auto& p : particles_) {
        p.life -= dt;
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.vel.x *= 0.985f;
        p.vel.y *= 0.985f;
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(), [](const Particle& p){ return p.life <= 0; }), particles_.end());

    for (auto& tp : textParticles_) {
        tp.life -= dt;
        tp.pos.y -= 25.0f * dt;
    }
    textParticles_.erase(std::remove_if(textParticles_.begin(), textParticles_.end(), [](const TextParticle& tp){ return tp.life <= 0; }), textParticles_.end());

    shakeTime_ = std::max(0.0f, shakeTime_ - dt);
}

void Effects::Draw() const {
    for (const auto& p : particles_) {
        float a = p.life / p.maxLife;
        DrawCircleV(p.pos, p.radius * (0.35f + a), Fade(p.color, a));
    }

    for (const auto& tp : textParticles_) {
        float a = tp.life / tp.maxLife;
        int fontSize = 10;
        int tw = MeasureText(tp.text, fontSize);
        DrawText(tp.text, (int)(tp.pos.x - tw / 2), (int)tp.pos.y, fontSize, Fade(tp.color, a));
    }
}

void Effects::Clear() {
    particles_.clear();
    textParticles_.clear();
    shakeTime_ = 0.0f;
    shakeAmount_ = 0.0f;
}

void Effects::AddText(Vector2 pos, const char* text, Color color) {
    TextParticle tp;
    tp.pos = pos;
    // Simple snprintf is safe and clean
    std::snprintf(tp.text, sizeof(tp.text), "%s", text);
    tp.color = color;
    tp.life = tp.maxLife = 0.8f;
    textParticles_.push_back(tp);
}

void Effects::Explosion(Vector2 pos, Color color, int count) {
    for (int i = 0; i < count; ++i) {
        float angle = (float)i / (float)count * 6.2831853f + (float)GetRandomValue(-10, 10) * 0.01f;
        float speed = (float)GetRandomValue(45, 185);
        Particle p;
        p.pos = pos;
        p.vel = {std::cos(angle) * speed, std::sin(angle) * speed};
        p.life = p.maxLife = (float)GetRandomValue(25, 70) / 100.0f;
        p.radius = (float)GetRandomValue(2, 5);
        p.color = color;
        particles_.push_back(p);
    }
}

void Effects::Spark(Vector2 pos, Color color) {
    Explosion(pos, color, 6);
}

void Effects::Shake(float amount, float time) {
    shakeAmount_ = std::max(shakeAmount_, amount);
    shakeTime_ = std::max(shakeTime_, time);
}

Vector2 Effects::ShakeOffset() const {
    if (shakeTime_ <= 0.0f) return {0, 0};
    return {(float)GetRandomValue(-100, 100) / 100.0f * shakeAmount_, (float)GetRandomValue(-100, 100) / 100.0f * shakeAmount_};
}
