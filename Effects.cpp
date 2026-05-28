#include "Effects.h"
#include "SpriteManager.h"
#include <algorithm>
#include <cmath>
#include <cstring>

void Effects::Update(float dt) {
    // 1. Update classic particles
    for (auto& p : particles_) {
        p.life -= dt;
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.vel.x *= 0.975f;
        p.vel.y *= 0.975f;
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(), [](const Particle& p){ return p.life <= 0; }), particles_.end());

    // 2. Update animated explosion frames
    for (auto& ae : animatedExplosions_) {
        ae.life += dt;
    }
    animatedExplosions_.erase(std::remove_if(animatedExplosions_.begin(), animatedExplosions_.end(), [](const AnimatedExplosion& ae){ return ae.life >= ae.maxLife; }), animatedExplosions_.end());

    // 3. Update Shockwaves
    for (auto& sw : shockwaves_) {
        sw.radius += sw.speed * dt;
    }
    // Erase completed shockwaves
    auto swIt = std::remove_if(shockwaves_.begin(), shockwaves_.end(), [](const Shockwave& sw){ return sw.radius >= sw.maxRadius; });
    shockwaves_.erase(swIt, shockwaves_.end());

    // 4. Update Embers
    for (auto& em : embers_) {
        em.life -= dt;
        em.pos.x += em.vel.x * dt;
        em.pos.y += em.vel.y * dt;
        em.vel.x *= 0.985f;
        em.vel.y *= 0.985f;
    }
    embers_.erase(std::remove_if(embers_.begin(), embers_.end(), [](const Ember& em){ return em.life <= 0; }), embers_.end());

    // 5. Update physical shrapnel Debris (affected by gravity so it drops downwards)
    for (auto& d : debris_) {
        d.life -= dt;
        d.pos.x += d.vel.x * dt;
        d.pos.y += d.vel.y * dt;
        d.vel.y += d.gravity * dt;
        d.rotation += d.spinSpeed * dt;
    }
    debris_.erase(std::remove_if(debris_.begin(), debris_.end(), [](const Debris& d){ return d.life <= 0; }), debris_.end());

    // 6. Update text indicators
    for (auto& tp : textParticles_) {
        tp.life -= dt;
        tp.pos.y -= 25.0f * dt;
    }
    textParticles_.erase(std::remove_if(textParticles_.begin(), textParticles_.end(), [](const TextParticle& tp){ return tp.life <= 0; }), textParticles_.end());

    auto trimOldest = [](auto& items, size_t maxCount) {
        if (items.size() > maxCount) {
            items.erase(items.begin(), items.begin() + (long)(items.size() - maxCount));
        }
    };
    trimOldest(particles_, 160);
    trimOldest(animatedExplosions_, 24);
    trimOldest(shockwaves_, 10);
    trimOldest(embers_, 64);
    trimOldest(debris_, 80);
    trimOldest(textParticles_, 18);

    shakeTime_ = std::max(0.0f, shakeTime_ - dt);
}

void Effects::Draw() const {
    // 1. Draw animated spritesheet fireballs
    for (const auto& ae : animatedExplosions_) {
        if (ae.life < 0.0f) continue; // delayed trigger
        int frame = (int)(ae.life / ae.maxLife * 8.0f);
        frame = std::clamp(frame, 0, 7);
        SpriteId spriteId = static_cast<SpriteId>((int)SpriteId::Explosion0 + frame);
        SpriteManager::Instance().Draw(spriteId, ae.pos, ae.rotation, ae.scale);
    }

    // 2. Draw additive glowing particles (shockwaves, fire embers)
    BeginBlendMode(BLEND_ADDITIVE);
    
    // Draw expanding shockwave rings
    for (const auto& sw : shockwaves_) {
        float ratio = sw.radius / sw.maxRadius;
        float alpha = 1.0f - ratio;
        DrawCircleLines((int)sw.pos.x, (int)sw.pos.y, sw.radius, Fade(sw.color, alpha * 0.75f));
        DrawCircleLines((int)sw.pos.x, (int)sw.pos.y, sw.radius - 2.5f, Fade(sw.color, alpha * 0.35f));
    }
    
    // Draw fire embers
    for (const auto& em : embers_) {
        float ratio = em.life / em.maxLife;
        DrawCircleV(em.pos, em.radius * (0.6f + ratio * 0.8f), Fade(em.color, ratio * 0.9f));
    }
    
    EndBlendMode();

    // 3. Draw physical shrapnel debris chunks (rotated rectangles or custom sprites!)
    for (const auto& d : debris_) {
        float ratio = d.life / d.maxLife;
        if (d.useSprite) {
            SpriteManager::Instance().Draw(d.spriteId, d.pos, d.rotation, d.size, Fade(WHITE, ratio));
        } else {
            Rectangle dest = { d.pos.x, d.pos.y, d.size, d.size };
            Vector2 origin = { d.size / 2.0f, d.size / 2.0f };
            DrawRectanglePro(dest, origin, d.rotation, Fade(d.color, ratio));
        }
    }

    // 4. Draw classic ash & spark particles
    for (const auto& p : particles_) {
        float a = p.life / p.maxLife;
        Color col = p.color;
        
        bool isThruster = (p.color.r == SKYBLUE.r && p.color.g == SKYBLUE.g && p.color.b == SKYBLUE.b);
        bool isSmoke = (std::abs((int)p.color.r - (int)p.color.g) < 16 &&
                        std::abs((int)p.color.g - (int)p.color.b) < 16 &&
                        p.color.r < 135 && p.color.a < 230);
        if (isSmoke) {
            float sz = p.radius * (1.0f + (1.0f - a) * 0.9f);
            DrawCircleV(p.pos, sz, Fade(p.color, a * 0.24f));
            continue;
        }
        if (!isThruster) {
            if (a > 0.82f) {
                col = WHITE;
            } else if (a > 0.55f) {
                col = YELLOW;
            } else if (a > 0.32f) {
                col = ORANGE;
            } else if (a > 0.15f) {
                col = RED;
            } else {
                col = Color{80, 80, 85, 255}; // dark ash gray
            }
        }

        float speedSq = p.vel.x * p.vel.x + p.vel.y * p.vel.y;
        if (speedSq > 6000.0f) {
            float dtOffset = 0.018f;
            Vector2 tail = { p.pos.x - p.vel.x * dtOffset * a, p.pos.y - p.vel.y * dtOffset * a };
            float thickness = p.radius * (0.4f + a * 0.6f);
            if (thickness < 1.0f) thickness = 1.0f;
            DrawLineEx(p.pos, tail, thickness, Fade(col, a));
        } else {
            float sz = p.radius * (0.45f + a * 0.75f);
            DrawCircleV(p.pos, sz / 2.0f, Fade(col, a));
        }
    }

    // 5. Draw score / event indicators
    for (const auto& tp : textParticles_) {
        float a = tp.life / tp.maxLife;
        int fontSize = std::strlen(tp.text) <= 8 ? 12 : 10;
        int tw = MeasureText(tp.text, fontSize);
        
        DrawText(tp.text, (int)(tp.pos.x - tw / 2) + 1, (int)tp.pos.y + 1, fontSize, Fade(BLACK, a * 0.65f));
        DrawText(tp.text, (int)(tp.pos.x - tw / 2), (int)tp.pos.y, fontSize, Fade(tp.color, a));
    }
}

void Effects::Clear() {
    particles_.clear();
    animatedExplosions_.clear();
    shockwaves_.clear();
    embers_.clear();
    debris_.clear();
    textParticles_.clear();
    shakeTime_ = 0.0f;
    shakeAmount_ = 0.0f;
}

void Effects::AddText(Vector2 pos, const char* text, Color color) {
    TextParticle tp;
    tp.pos = pos;
    std::snprintf(tp.text, sizeof(tp.text), "%s", text);
    tp.color = color;
    tp.life = tp.maxLife = 0.72f;
    textParticles_.push_back(tp);
}

void Effects::Explosion(Vector2 pos, Color color, int count, SpriteId debrisSprite) {
    // 1. Trigger animated explosion sprite frame sequences
    AnimatedExplosion ae;
    ae.pos = pos;
    ae.life = 0.0f;
    ae.maxLife = (count > 50) ? 0.58f : ((count > 15) ? 0.36f : 0.28f);
    ae.scale = (count > 50) ? 3.0f : ((count > 15) ? 1.55f : 0.82f);
    ae.rotation = (float)GetRandomValue(0, 360);
    animatedExplosions_.push_back(ae);

    // 2. Cluster delayed secondary explosions for large carrier/boss ships
    if (count > 50) {
        for (int step = 0; step < 5; ++step) {
            AnimatedExplosion subAe;
            subAe.pos = { pos.x + GetRandomValue(-35, 35), pos.y + GetRandomValue(-35, 35) };
            subAe.life = -0.07f * (float)(step + 1); // delayed trigger times
            subAe.maxLife = 0.34f;
            subAe.scale = (float)GetRandomValue(12, 22) / 10.0f;
            subAe.rotation = (float)GetRandomValue(0, 360);
            animatedExplosions_.push_back(subAe);
        }
    }

    // 3. Trigger Additive Shockwave Rings
    Shockwave sw;
    sw.pos = pos;
    sw.radius = 0.0f;
    sw.maxRadius = (count > 50) ? 150.0f : ((count > 15) ? 65.0f : 30.0f);
    sw.speed = (count > 50) ? 240.0f : 160.0f;
    sw.color = (count > 50) ? VIOLET : ((count > 15) ? ORANGE : GOLD);
    shockwaves_.push_back(sw);

    // 4. Spawn Additive Embers (bright slow hot sparks)
    int emberCount = (count > 50) ? 18 : ((count > 15) ? 7 : 2);
    for (int i = 0; i < emberCount; ++i) {
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(20, 85);
        Ember em;
        em.pos = pos;
        em.vel = { std::cos(angle) * speed, std::sin(angle) * speed };
        em.life = em.maxLife = (float)GetRandomValue(35, 78) / 100.0f;
        em.radius = (float)GetRandomValue(12, 25) / 10.0f;
        em.color = (GetRandomValue(0, 1) == 0) ? GOLD : ORANGE;
        embers_.push_back(em);
    }

    // 5. Spawn Physical Shrapnel Debris (rotated gravity-affected panels or custom components)
    int debrisCount = (count > 50) ? 12 : ((count > 15) ? 5 : 2);
    for (int i = 0; i < debrisCount; ++i) {
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(35, 140);
        Debris d;
        d.pos = pos;
        d.vel = { std::cos(angle) * speed, std::sin(angle) * speed - (float)GetRandomValue(24, 58) };
        d.rotation = (float)GetRandomValue(0, 360);
        d.spinSpeed = (float)GetRandomValue(-240, 240);
        d.gravity = (float)GetRandomValue(105, 185);
        d.life = d.maxLife = (float)GetRandomValue(45, 100) / 100.0f;
        
        if (count > 50) {
            d.useSprite = true;
            if (i % 3 == 0) d.spriteId = SpriteId::DebrisBossCore;
            else if (i % 3 == 1) d.spriteId = SpriteId::DebrisEnemyWing;
            else d.spriteId = SpriteId::DebrisEnemyThruster;
            d.size = (float)GetRandomValue(10, 20) / 10.0f;
        } else if (debrisSprite == SpriteId::DebrisPlayerWingLeft || debrisSprite == SpriteId::DebrisPlayerWingRight) {
            d.spriteId = (i % 2 == 0) ? SpriteId::DebrisPlayerWingLeft : SpriteId::DebrisPlayerWingRight;
            d.useSprite = true;
            d.size = (float)GetRandomValue(9, 16) / 10.0f;
        } else if (debrisSprite != SpriteId::AsteroidChunk) {
            d.spriteId = debrisSprite;
            d.useSprite = true;
            d.size = (float)GetRandomValue(8, 16) / 10.0f;
        } else {
            d.useSprite = false;
            d.size = (float)GetRandomValue(2, 5);
        }
        
        d.color = (GetRandomValue(0, 2) == 0) ? Color{110, 110, 115, 255} : Color{150, 45, 45, 255}; // steel grey or burning metal red
        debris_.push_back(d);
    }

    // 6. Spawn classic Ash particles
    int particleCount = (count > 50) ? 24 : std::min(count, 18);
    for (int i = 0; i < particleCount; ++i) {
        float angle = (float)i / (float)particleCount * 6.2831853f + (float)GetRandomValue(-15, 15) * 0.01f;
        float speed = (float)GetRandomValue(65, 205);
        Particle p;
        p.pos = pos;
        p.vel = {std::cos(angle) * speed, std::sin(angle) * speed};
        p.life = p.maxLife = (float)GetRandomValue(18, 58) / 100.0f;
        p.radius = (float)GetRandomValue(2, 6);
        p.color = color;
        particles_.push_back(p);
    }
}

void Effects::Spark(Vector2 pos, Color color, Vector2 biasVel) {
    // Spark particles
    int count = 8;
    for (int i = 0; i < count; ++i) {
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(85, 210);
        Particle p;
        p.pos = pos;
        p.vel = { std::cos(angle) * speed + biasVel.x * 0.45f, std::sin(angle) * speed + biasVel.y * 0.45f };
        p.life = p.maxLife = (float)GetRandomValue(9, 24) / 100.0f;
        p.radius = (float)GetRandomValue(15, 30) / 10.0f;
        p.color = color;
        particles_.push_back(p);
    }
}

void Effects::EngineExhaust(Vector2 pos, Color color, Vector2 biasVel) {
    Particle p;
    p.pos = pos;
    // Launch downwards with bias tilt
    float angle = (float)GetRandomValue(82, 98) * DEG2RAD;
    float speed = (float)GetRandomValue(60, 120);
    p.vel = { std::cos(angle) * speed + biasVel.x, std::sin(angle) * speed + biasVel.y };
    p.life = p.maxLife = (float)GetRandomValue(12, 28) / 100.0f;
    p.radius = (float)GetRandomValue(10, 22) / 10.0f;
    p.color = color;
    particles_.push_back(p);
}

void Effects::Smoke(Vector2 pos, Color color) {
    Particle p;
    p.pos = pos;
    // Slow drifting smoke particles
    float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
    float speed = (float)GetRandomValue(10, 32);
    p.vel = { std::cos(angle) * speed, std::sin(angle) * speed + 20.0f };
    p.life = p.maxLife = (float)GetRandomValue(34, 72) / 100.0f;
    p.radius = (float)GetRandomValue(28, 62) / 10.0f;
    p.color = color;
    particles_.push_back(p);
}

void Effects::DebrisShower(Vector2 pos, Color color, int count) {
    for (int i = 0; i < count; ++i) {
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(50, 200);
        Debris d;
        d.pos = pos;
        // Upward launch bias so it bounces and arcs down
        d.vel = { std::cos(angle) * speed, std::sin(angle) * speed - (float)GetRandomValue(35, 70) };
        d.rotation = (float)GetRandomValue(0, 360);
        d.spinSpeed = (float)GetRandomValue(-360, 360);
        d.gravity = (float)GetRandomValue(95, 210);
        d.life = d.maxLife = (float)GetRandomValue(55, 135) / 100.0f;
        d.size = (float)GetRandomValue(3, 11);
        d.color = color;
        debris_.push_back(d);
    }
}

void Effects::Shake(float amount, float time) {
    shakeAmount_ = std::max(shakeAmount_, amount);
    shakeTime_ = std::max(shakeTime_, time);
}

Vector2 Effects::ShakeOffset() const {
    if (shakeTime_ <= 0.0f) return {0, 0};
    return {(float)GetRandomValue(-100, 100) / 100.0f * shakeAmount_, (float)GetRandomValue(-100, 100) / 100.0f * shakeAmount_};
}
