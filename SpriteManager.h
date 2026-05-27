#pragma once
#include "raylib.h"
#include <unordered_map>
#include <string>

enum class SpriteId {
    PlayerIdle,
    PlayerSoftLeft,
    PlayerLeft,
    PlayerSoftRight,
    PlayerRight,
    Popcorn,
    TurretBase,
    TurretBarrel,
    TurretBarrelRecoil,
    Boss,
    BossDamaged,
    PowerupW,
    PowerupP,
    PowerupB,
    PowerupMedal,
    BulletVulcan,
    BulletPlasma,
    BulletMissile,
    BulletEnemyBasic,
    BulletEnemyStrong,
    Explosion0,
    Explosion1,
    Explosion2,
    Explosion3,
    Explosion4,
    Explosion5,
    Explosion6,
    Explosion7,
    MuzzleVulcan,
    MuzzlePlasma,
    MuzzleMissile,
    ShieldBubble,
    AsteroidChunk,
    CloudForeground,
    SpaceStationLeft,
    SpaceStationLeftCore,
    SpaceStationRight,
    SpaceStationRightCore,
    MiniBombCapsule,
    DebrisPlayerWingLeft,
    DebrisPlayerWingRight,
    DebrisEnemyWing,
    DebrisEnemyThruster,
    DebrisBossCore
};

class SpriteManager {
public:
    static SpriteManager& Instance() {
        static SpriteManager instance;
        return instance;
    }

    SpriteManager(const SpriteManager&) = delete;
    SpriteManager& operator=(const SpriteManager&) = delete;

    void Init();
    void Cleanup();

    Texture2D GetTexture(SpriteId id) const;
    void Draw(SpriteId id, Vector2 pos, float rotation = 0.0f, float scale = 1.0f, Color tint = WHITE) const;
    void DrawRect(SpriteId id, Rectangle src, Rectangle dest, Vector2 origin, float rotation, Color tint = WHITE) const;

private:
    SpriteManager() = default;
    ~SpriteManager();

    std::unordered_map<SpriteId, Texture2D> textures_;

    Texture2D GenerateSprite(int w, int h, const char* grid, const std::unordered_map<char, Color>& palette);
    Texture2D GenerateProceduralCloud();
    Texture2D GenerateProceduralAsteroid();
};
