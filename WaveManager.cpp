#include "WaveManager.h"

void WaveManager::Reset() {
    lastSecond_ = -1;
}

void WaveManager::SpawnPair(std::vector<Enemy>& enemies, float y, int loop, bool inward) {
    Enemy left(EnemyType::Popcorn, {-24, y}, loop, 0);
    Enemy right(EnemyType::Popcorn, {504, y - 22}, loop, 0);
    left.vel = inward ? Vector2{86.0f, 82.0f} : Vector2{64.0f, 96.0f};
    right.vel = inward ? Vector2{-86.0f, 82.0f} : Vector2{-64.0f, 96.0f};
    enemies.push_back(left);
    enemies.push_back(right);
}

void WaveManager::Update(float stageTime, int loop, std::vector<Enemy>& enemies) {
    int s = (int)stageTime;
    if (s == lastSecond_) return;
    lastSecond_ = s;
    int beat = s % 90;

    // Standard popcorn pairs
    if (beat < 84 && beat % 6 == 1 && beat != 13 && beat != 43 && beat != 73) {
        SpawnPair(enemies, -20, loop, (beat / 6) % 2 == 0);
    }
    
    // Standard turrets
    if (beat < 82 && beat % 10 == 4) enemies.emplace_back(EnemyType::Turret, Vector2{80.0f, -28.0f}, loop, 0);
    if (beat < 82 && beat % 10 == 8) enemies.emplace_back(EnemyType::Turret, Vector2{400.0f, -28.0f}, loop, 0);
    
    // Formations
    // Beat 13: V-formation of 5 Popcorns (formationId = 1)
    if (beat == 13) {
        float xCoords[5] = { 120.0f, 180.0f, 240.0f, 300.0f, 360.0f };
        float yOffsets[5] = { -20.0f, -45.0f, -70.0f, -45.0f, -20.0f };
        for (int i = 0; i < 5; ++i) {
            Enemy e(EnemyType::Popcorn, { xCoords[i], yOffsets[i] }, loop, 1);
            e.vel = { 0.0f, 115.0f };
            enemies.push_back(e);
        }
    }
    
    // Beat 43: Left diagonal formation (formationId = 2)
    if (beat == 43) {
        for (int i = 0; i < 5; ++i) {
            Enemy e(EnemyType::Popcorn, { 60.0f + i * 40.0f, -20.0f - i * 30.0f }, loop, 2);
            e.vel = { 45.0f, 105.0f };
            enemies.push_back(e);
        }
    }

    // Beat 73: Right diagonal formation (formationId = 3)
    if (beat == 73) {
        for (int i = 0; i < 5; ++i) {
            Enemy e(EnemyType::Popcorn, { 420.0f - i * 40.0f, -20.0f - i * 30.0f }, loop, 3);
            e.vel = { -45.0f, 105.0f };
            enemies.push_back(e);
        }
    }
    
    // Random popcorn trio drops
    if (beat < 76 && beat % 15 == 11 && beat != 11 && beat != 41 && beat != 71) {
        enemies.emplace_back(EnemyType::Popcorn, Vector2{130.0f, -25.0f}, loop, 0);
        enemies.emplace_back(EnemyType::Popcorn, Vector2{240.0f, -55.0f}, loop, 0);
        enemies.emplace_back(EnemyType::Popcorn, Vector2{350.0f, -25.0f}, loop, 0);
    }
}

bool WaveManager::ShouldSpawnBoss(float stageTime, bool bossAlive) const {
    return stageTime >= 90.0f && !bossAlive;
}
