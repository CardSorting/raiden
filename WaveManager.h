#pragma once
#include "Enemy.h"
#include <vector>

class WaveManager {
public:
    void Reset();
    void Update(float stageTime, int loop, std::vector<Enemy>& enemies);
    bool ShouldSpawnBoss(float stageTime, bool bossAlive) const;

private:
    int nextCue_ = 0;
    void SpawnCue(int cue, int loop, std::vector<Enemy>& enemies);
    void AddEnemy(std::vector<Enemy>& enemies, EnemyType type, Vector2 pos, Vector2 vel, int loop,
                  int formationId, EnemyMovePattern move, EnemyFirePattern fire,
                  float fireDelay, float fireRate, int maxShots, float phase);
};
