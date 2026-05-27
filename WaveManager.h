#pragma once
#include "Enemy.h"
#include <vector>

class WaveManager {
public:
    void Reset();
    void Update(float stageTime, int loop, std::vector<Enemy>& enemies);
    bool ShouldSpawnBoss(float stageTime, bool bossAlive) const;

private:
    int lastSecond_ = -1;
    void SpawnPair(std::vector<Enemy>& enemies, float y, int loop, bool inward);
};
