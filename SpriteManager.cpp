#include "SpriteManager.h"
#include <cmath>

SpriteManager::~SpriteManager() {
    Cleanup();
}

void SpriteManager::Init() {
    // Define Palettes
    std::unordered_map<char, Color> palPlayer = {
        {'.', BLANK},
        {'K', Color{12, 12, 16, 255}},     // Outline
        {'D', Color{50, 60, 75, 255}},     // Dark Steel
        {'S', Color{105, 120, 140, 255}},  // Steel Blue
        {'L', Color{170, 185, 200, 255}},  // Light Steel
        {'W', Color{255, 255, 255, 255}},  // Specular Highlight
        {'R', Color{160, 15, 25, 255}},    // Dark Crimson Trim
        {'r', Color{230, 35, 45, 255}},    // Bright Scarlet
        {'Y', Color{245, 190, 15, 255}},   // Gold Wingtip
        {'C', Color{0, 150, 220, 255}},    // Canopy Glass
        {'c', Color{130, 215, 255, 255}},  // Canopy Glow
        {'O', Color{255, 100, 0, 255}},    // Thruster Core Orange
        {'o', Color{255, 200, 0, 255}}     // Thruster Spark Yellow
    };

    std::unordered_map<char, Color> palPopcorn = {
        {'.', BLANK},
        {'K', Color{15, 10, 20, 255}},
        {'P', Color{110, 15, 50, 255}},    // Purple Hull
        {'p', Color{200, 25, 95, 255}},    // Bright Magenta Vents
        {'G', Color{0, 240, 190, 255}},    // Glowing Vents
        {'D', Color{35, 35, 40, 255}},     // Dark grey plating
        {'S', Color{80, 80, 90, 255}},     // Grey plating
        {'O', Color{255, 90, 0, 255}}      // Thruster
    };

    std::unordered_map<char, Color> palTurretBase = {
        {'.', BLANK},
        {'K', Color{10, 15, 10, 255}},
        {'H', Color{30, 55, 30, 255}},     // Military Dark Green
        {'h', Color{65, 105, 65, 255}},    // Military Light Green
        {'D', Color{45, 45, 50, 255}},     // Dark Metal
        {'S', Color{95, 95, 105, 255}},    // Grey Metal
        {'Y', Color{225, 175, 10, 255}}    // Yellow Warning Stripes
    };

    std::unordered_map<char, Color> palTurretBarrel = {
        {'.', BLANK},
        {'K', Color{10, 10, 12, 255}},
        {'D', Color{40, 40, 45, 255}},     // Dark Gunmetal
        {'S', Color{85, 85, 95, 255}}      // Grey Gunmetal
    };

    std::unordered_map<char, Color> palBoss = {
        {'.', BLANK},
        {'K', Color{15, 15, 20, 255}},
        {'B', Color{40, 45, 55, 255}},     // Deep Dark Steel
        {'b', Color{75, 80, 95, 255}},     // Armored Plating Grey
        {'M', Color{125, 130, 150, 255}},  // Light Metal Plating
        {'P', Color{85, 15, 105, 255}},    // Purple trim
        {'p', Color{165, 40, 200, 255}},   // Glowing violet trim
        {'R', Color{145, 10, 20, 255}},    // Dark Red Core
        {'r', Color{255, 35, 45, 255}},    // Glowing neon red core
        {'G', Color{0, 245, 95, 255}},     // Green sensor lights
        {'Y', Color{235, 190, 15, 255}}    // Gold exhaust grates
    };

    std::unordered_map<char, Color> palBossDamaged = palBoss;
    palBossDamaged['B'] = Color{35, 30, 30, 255};
    palBossDamaged['b'] = Color{60, 55, 55, 255};
    palBossDamaged['M'] = Color{95, 90, 90, 255};
    palBossDamaged['P'] = Color{65, 10, 75, 255};
    palBossDamaged['p'] = Color{110, 30, 130, 255};

    std::unordered_map<char, Color> palPowerup = {
        {'.', BLANK},
        {'K', Color{20, 20, 25, 255}},
        {'G', Color{250, 195, 15, 255}},   // Gold
        {'Y', Color{255, 240, 130, 255}},  // Highlight Yellow
        {'S', Color{0, 160, 230, 255}},    // Sky Blue (W)
        {'s', Color{140, 220, 255, 255}},  // Light Blue (W)
        {'O', Color{255, 100, 0, 255}},    // Orange (P)
        {'o', Color{255, 205, 110, 255}},  // Light Orange (P)
        {'P', Color{150, 30, 200, 255}},   // Purple (B)
        {'p', Color{230, 130, 255, 255}},  // Light Purple (B)
        {'W', Color{255, 255, 255, 255}}   // White text
    };

    std::unordered_map<char, Color> palBullet = {
        {'.', BLANK},
        {'K', Color{30, 10, 10, 200}},
        {'Y', Color{255, 225, 0, 255}},
        {'W', Color{255, 255, 255, 255}},
        {'C', Color{0, 230, 255, 255}},
        {'G', Color{10, 210, 10, 255}},
        {'R', Color{255, 0, 100, 255}},
        {'r', Color{255, 100, 0, 255}}
    };

    std::unordered_map<char, Color> palExplosion = {
        {'.', BLANK},
        {'W', Color{255, 255, 255, 255}},
        {'Y', Color{255, 230, 100, 255}},
        {'O', Color{255, 120, 20, 255}},
        {'R', Color{200, 30, 10, 255}},
        {'D', Color{80, 80, 85, 255}},
        {'d', Color{45, 45, 48, 255}}
    };

    std::unordered_map<char, Color> palShield = {
        {'.', BLANK},
        {'C', Color{0, 200, 255, 110}},
        {'c', Color{120, 240, 255, 200}},
        {'W', Color{255, 255, 255, 255}}
    };

    std::unordered_map<char, Color> palSpaceStation = {
        {'.', BLANK},
        {'K', Color{10, 12, 16, 255}},     // Dark Outline
        {'D', Color{35, 40, 50, 255}},     // Dark Steel Plating
        {'S', Color{60, 68, 85, 255}},     // Medium Steel Plating
        {'L', Color{100, 110, 130, 255}},  // Light Steel Plating
        {'Y', Color{235, 175, 15, 255}},   // Warning Stripe Yellow
        {'C', Color{0, 220, 255, 255}},    // Neon Cyan glow
        {'c', Color{100, 240, 255, 255}},  // Light Cyan
        {'R', Color{230, 30, 40, 255}},    // Neon Red beacon
        {'B', Color{20, 22, 28, 255}}      // Dark background / shadows
    };

    const char* gridSpaceStationLeft =
        "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDKKK"
        "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDKKK"
        "DDLLLLLLLLLLLLLLLLLLLLLLLLDDKKK."
        "DDLSSSSSSSSSSSSSSSSSSSSSSLDDKK.."
        "DDLSSSSSSSSSSSSSSSSSSSSSSLDDKK.."
        "DDLSSSSSSSSSSSSSSSSSSSSSSLDDKK.."
        "DDLSSSSSSYYYYYYYSSSSSSSSSLDDKK.."
        "DDLSSSSSYYYYYYYSSSSSSSSSSLDDKK.."
        "DDLSSSSYYYYYYYSSSSSSSSSSSLDDKK.."
        "DDLSSSYYYYYYYSSSSSSSSSSSSLDDKK.."
        "DDLSSYYYYYYYSSSSSSSSSSSSSLDDKK.."
        "DDLSSSSSSSSSSSSSSSSSSSSSSLDDKK.."
        "DDLSSSSSSSSSSSSSSSSSSSSSSLDDKK.."
        "DDLLLLLLLLLLLLLLLLLLLLLLLLDDKKK."
        "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDKKK"
        "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDKKK"
        "KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK"
        "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDKKK"
        "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDKKK"
        "DDLLLLLLLLLLLLLLLLLLLLLLLLDDKKK."
        "DDLSSSSSSSSSSSSSSSSSSSSSSLDDKK.."
        "DDLSSSSSSYYYYYYYSSSSSSSSSLDDKK.."
        "DDLSSSSSYYYYYYYSSSSSSSSSSLDDKK.."
        "DDLSSSSYYYYYYYSSSSSSSSSSSLDDKK.."
        "DDLSSSYYYYYYYSSSSSSSSSSSSLDDKK.."
        "DDLSSYYYYYYYSSSSSSSSSSSSSLDDKK.."
        "DDLSSSSSSSSSSSSSSSSSSSSSSLDDKK.."
        "DDLSSSSSSSSSSSSSSSSSSSSSSLDDKK.."
        "DDLLLLLLLLLLLLLLLLLLLLLLLLDDKKK."
        "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDKKK"
        "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDKKK"
        "................................";

    const char* gridSpaceStationLeftCore =
        "DDDDDDDDDDRRRRDDDDDDDDDDDDDDDKKK"
        "DDDDDDDDDDRrrRDDDDDDDDDDDDDDDKKK"
        "DDLLLLLLLLRRRRLLLLLLLLLLLLDDKKK."
        "DDLSSSSSSSSSSSSSSSSSSSSSSLDDKK.."
        "DDLSSSSSSSSSSSSSSSSSSSSSSLDDKK.."
        "DDLSSDDDDDDDDDDDDDDDDDDSSLDDKK.."
        "DDLSSDCCCCCCCCCCCCCCDDSSLDDKK.."
        "DDLSSDCccccccccccccCDDSSLDDKK.."
        "DDLSSDCCCCCCCCCCCCCCDDSSLDDKK.."
        "DDLSSDDDDDDDDDDDDDDDDDDSSLDDKK.."
        "DDLSSSSSSSSSSSSSSSSSSSSSSLDDKK.."
        "DDLSSSSSSSSSSSSSSSSSSSSSSLDDKK.."
        "DDLLLLLLLLLLLLLLLLLLLLLLLLDDKKK."
        "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDKKK"
        "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDKKK"
        "KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK"
        "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDKKK"
        "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDKKK"
        "DDLLLLLLLLLLLLLLLLLLLLLLLLDDKKK."
        "DDLSSSSSSSSSSSSSSSSSSSSSSLDDKK.."
        "DDLSSDDDDDDDDDDDDDDDDDDSSLDDKK.."
        "DDLSSDCCCCCCCCCCCCCCDDSSLDDKK.."
        "DDLSSDCccccccccccccCDDSSLDDKK.."
        "DDLSSDCCCCCCCCCCCCCCDDSSLDDKK.."
        "DDLSSDDDDDDDDDDDDDDDDDDSSLDDKK.."
        "DDLSSSSSSSSSSSSSSSSSSSSSSLDDKK.."
        "DDLSSSSSSSSSSSSSSSSSSSSSSLDDKK.."
        "DDLLLLLLLLLLLLLLLLLLLLLLLLDDKKK."
        "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDKKK"
        "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDKKK"
        "KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK"
        "................................";

    const char* gridSpaceStationRight =
        "KKKDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
        "KKKDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
        ".KKKDDLLLLLLLLLLLLLLLLLLLLLLLLDD"
        "..KKDDSLSSSSSSSSSSSSSSSSSSSSSSLD"
        "..KKDDSLSSSSSSSSSSSSSSSSSSSSSSLD"
        "..KKDDSLSSSSSSSSSSSSSSSSSSSSSSLD"
        "..KKDDSLSSYYYYYYYSSSSSSSSSSSSLD"
        "..KKDDSLSSYYYYYYYSSSSSSSSSSSSLD"
        "..KKDDSLSSYYYYYYYSSSSSSSSSSSSLD"
        "..KKDDSLSSSSSSSSSSSSSSSSSSSSSSLD"
        "..KKDDSLSSSSSSSSSSSSSSSSSSSSSSLD"
        "..KKDDSLSSSSSSSSSSSSSSSSSSSSSSLD"
        ".KKKDDLLLLLLLLLLLLLLLLLLLLLLLLDD"
        "KKKDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
        "KKKDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
        "KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK"
        "KKKDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
        "KKKDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
        ".KKKDDLLLLLLLLLLLLLLLLLLLLLLLLDD"
        "..KKDDSLSSSSSSSSSSSSSSSSSSSSSSLD"
        "..KKDDSLSSYYYYYYYSSSSSSSSSSSSLD"
        "..KKDDSLSSYYYYYYYSSSSSSSSSSSSLD"
        "..KKDDSLSSYYYYYYYSSSSSSSSSSSSLD"
        "..KKDDSLSSSSSSSSSSSSSSSSSSSSSSLD"
        "..KKDDSLSSSSSSSSSSSSSSSSSSSSSSLD"
        "..KKDDSLSSSSSSSSSSSSSSSSSSSSSSLD"
        "..KKDDSLSSSSSSSSSSSSSSSSSSSSSSLD"
        ".KKKDDLLLLLLLLLLLLLLLLLLLLLLLLDD"
        "KKKDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
        "KKKDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
        "KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK"
        "................................";

    const char* gridSpaceStationRightCore =
        "KKKDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
        "KKKDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
        ".KKKDDLLLLLLLLLLLLLLLLLLLLLLLLDD"
        "..KKDDSLSSSSSSSSSSSSSSSSSSSSSSLD"
        "..KKDDSLSSSSSSSSSSSSSSSSSSSSSSLD"
        "..KKDDSLSSDDDDDDDDDDDDDDDDDDSSLD"
        "..KKDDSLSSDCCCCCCCCCCCCCCDDSSLD"
        "..KKDDSLSSDCccccccccccccCDDSSLD"
        "..KKDDSLSSDCCCCCCCCCCCCCCDDSSLD"
        "..KKDDSLSSDDDDDDDDDDDDDDDDDDSSLD"
        "..KKDDSLSSSSSSSSSSSSSSSSSSSSSSLD"
        "..KKDDSLSSSSSSSSSSSSSSSSSSSSSSLD"
        ".KKKDDLLLLLLLLLLLLLLLLLLLLLLLLDD"
        "KKKDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
        "KKKDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
        "KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK"
        "KKKDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
        "KKKDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
        ".KKKDDLLLLLLLLLLLLLLLLLLLLLLLLDD"
        "..KKDDSLSSSSSSSSSSSSSSSSSSSSSSLD"
        "..KKDDSLSSDDDDDDDDDDDDDDDDDDSSLD"
        "..KKDDSLSSDCCCCCCCCCCCCCCDDSSLD"
        "..KKDDSLSSDCccccccccccccCDDSSLD"
        "..KKDDSLSSDCCCCCCCCCCCCCCDDSSLD"
        "..KKDDSLSSDDDDDDDDDDDDDDDDDDSSLD"
        "..KKDDSLSSSSSSSSSSSSSSSSSSSSSSLD"
        "..KKDDSLSSSSSSSSSSSSSSSSSSSSSSLD"
        ".KKKDDLLLLLLLLLLLLLLLLLLLLLLLLDD"
        "KKKDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
        "KKKDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
        "KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK"
        "................................";

    const char* gridMiniBomb =
        "...KKKKK...."
        "..KKPPPWWK.."
        ".KKPPPPPPWK."
        ".KPPPPPPPPK."
        ".KPPPPPPPPK."
        ".KPPPPPPPPK."
        ".KPPPPPPPPK."
        ".KPPPPPPPPK."
        ".KPPPPPPPPK."
        ".KKPPPPPPWK."
        "..KKPPPWWK.."
        "...KKKKK....";

    // 1. PLAYER SHIP IDLE
    const char* gridPlayerIdle = 
        "...........KK..........."
        "..........KccK.........."
        "..........KCCK.........."
        ".........KSLCCK........."
        ".........KSLCCK........."
        "........KSSLCCK........."
        "........KSRrrRK........."
        ".......KSSRrrRSK........"
        ".......KSRRrrRRK........"
        "......KSSRRrrRRK........"
        "......KSRRRrrRRRK......."
        ".....KSSRRRrrRRRSK......"
        "....KSSRRYYrrYYRRRSK...."
        "...KSSSRRYYrrYYRRRSSSK.."
        "..KSSSSRRYYrrYYRRRSSSSK."
        ".KSSSDSSRRYYrrYYRRSSDSSK"
        "KSSSDDDSSRRYYrrYYRRSDDDS"
        "KKKKKKKKKKSRRKSSKKKKKKKK"
        "........KSRRK..........."
        "........KSRRK..........."
        ".......KOOSSOOK........."
        "........KOOOK..........."
        ".........KOK............"
        "........................";

    // 1b. PLAYER SHIP SOFT LEFT
    const char* gridPlayerSoftLeft = 
        "..........KK............"
        ".........KccK..........."
        ".........KCCK..........."
        "........KSLCCK.........."
        "........KSLCCK.........."
        ".......KSSLCCK.........."
        ".......KSRrrRK.........."
        "......KSSRrrRSK........."
        "......KSRRrrRRK........."
        ".....KSSRRrrRRK........."
        ".....KSRRRrrRRRK........"
        "....KSSRRRrrRRRSK......."
        "...KSSRRYYrrYYRRRSK....."
        "..KSSSRRYYrrYYRRRSSSK..."
        ".KSSSSRRYYrrYYRRRSSSSK.."
        "KSSSDSSRRYYrrYYRRSSDSSK."
        "SSSDDDSSRRYYrrYYRRSDDDSK"
        "KKKKKKKKKSRRKSSKKKKKKKKK"
        ".......KSRRK............"
        ".......KSRRK............"
        "......KOOSSOOK.........."
        ".......KOOOK............"
        "........KOK............."
        "........................";

    // 2. PLAYER SHIP ROLL LEFT
    const char* gridPlayerLeft = 
        ".........KK............."
        "........KccK............"
        "........KCCK............"
        ".......KSLCCK..........."
        ".......KSLCCK..........."
        "......KSSLCCK..........."
        "......KSRrrRK..........."
        ".....KSSRrrRSK.........."
        ".....KSRRrrRRK.........."
        "....KSSRRrrRRK.........."
        "....KSRRRrrRRRK........."
        "...KSSRRRrrRRRSK........"
        "..KSSRRYYrrYYRRRSK......"
        ".KSSSRRYYrrYYRRRSSSK...."
        "KSSSSRRYYrrYYRRRSSSSK..."
        "SSSDSSRRYYrrYYRRSSDSSK.."
        "SDDDSSRRYYrrYYRRSDDDSSK."
        "KKKKKKKKSRRKSSKKKKKKKKKK"
        "......KSRRK............."
        "......KSRRK............."
        ".....KOOSSOOK..........."
        "......KOOOK............."
        ".......KOK.............."
        "........................";

    // 2b. PLAYER SHIP SOFT RIGHT
    const char* gridPlayerSoftRight = 
        "............KK.........."
        "...........KccK........."
        "...........KCCK........."
        "..........KSLCCK........"
        "..........KSLCCK........"
        ".........KSSLCCK........"
        ".........KSRrrRK........"
        "........KSSRrrRSK......."
        "........KSRRrrRRK......."
        "........KSSRRrrRRK......"
        ".......KSRRRrrRRRK......"
        "......KSSRRRrrRRRSK....."
        ".....KSSRRYYrrYYRRRSK..."
        "....KSSSRRYYrrYYRRRSSSK."
        "...KSSSSRRYYrrYYRRRSSSSK"
        "..KSSSDSSRRYYrrYYRRSSDSS"
        ".KSSSDDDSSRRYYrrYYRRSDDD"
        "KKKKKKKKKSRRKSSKKKKKKKKK"
        "............KSRRK......."
        "............KSRRK......."
        "..........KOOSSOOK......"
        "...........KOOOK........"
        "............KOK........."
        "........................";

    // 3. PLAYER SHIP ROLL RIGHT
    const char* gridPlayerRight = 
        ".............KK........."
        "............KccK........"
        "............KCCK........"
        "...........KSLCCK......."
        "...........KSLCCK......."
        "..........KSSLCCK......."
        "..........KSRrrRK......."
        ".........KSSRrrRSK......"
        ".........KSRRrrRRK......"
        "..........KSSRRrrRRK...."
        ".........KSRRRrrRRRK...."
        "........KSSRRRrrRRRSK..."
        "......KSSRRYYrrYYRRRSK.."
        "....KSSSRRYYrrYYRRRSSSK."
        "...KSSSSRRYYrrYYRRRSSSSK"
        "..KSSSDSSRRYYrrYYRRSSDSS"
        ".KSSSDDDSSRRYYrrYYRRSDDD"
        "KKKKKKKKKKSRRKSSKKKKKKKK"
        ".............KSRRK......"
        ".............KSRRK......"
        "...........KOOSSOOK....."
        "............KOOOK......."
        ".............KOK........"
        "........................";

    // Debris grids (12x12)
    const char* gridDebrisPlayerWingLeft =
        "....KKKK...."
        "...KSSSDK..."
        "..KSSSDDK..."
        ".KSSSDDK...."
        ".KSSSDK....."
        "KSSSDK......"
        "KSRRK......."
        "KSRK........"
        "KKK........."
        "............"
        "............"
        "............";

    const char* gridDebrisPlayerWingRight =
        "....KKKK...."
        "...KDSSSK..."
        "...KDDSSK.."
        "....KDDSSK."
        ".....KDSSK."
        "......KDSSK"
        ".......KRRK"
        "........KRK"
        ".........KK"
        "............"
        "............"
        "............";

    const char* gridDebrisEnemyWing =
        "...KKKKK...."
        "..KPPPDDK..."
        ".KPPPDDK...."
        "KPPPDDK....."
        "KPDDK......."
        "KKK........."
        "............"
        "............"
        "............"
        "............"
        "............"
        "............";

    const char* gridDebrisEnemyThruster =
        "...KKKKK...."
        "..KOOOOOK..."
        ".KOOOOOK...."
        "KKKKKKKK...."
        "............"
        "............"
        "............"
        "............"
        "............"
        "............"
        "............"
        "............";

    const char* gridDebrisBossCore =
        "....KKKK...."
        "...KRRRRK..."
        "..KrrrrrrK.."
        ".KrrWWWWrrK."
        ".KrrWWWWrrK."
        "..KrrrrrrK.."
        "...KRRRRK..."
        "....KKKK...."
        "............"
        "............"
        "............"
        "............";

    const char* gridTurretBarrelRecoil = 
        "............"
        "............"
        "....KKKK...."
        "....KSSK...."
        "....KDDK...."
        "....KDDK...."
        "....KDDK...."
        "...KKDDKK..."
        "..KSSDSSSK.."
        ".KDDDDDDDDK."
        "KKKKKKKKKKKK"
        "............";

    // 4. POPCORN ENEMY
    const char* gridPopcorn = 
        "....KK....KK...."
        "...KPpK..KPpK..."
        "...KPPKKKKPPK..."
        "..KDPPPGGPPDK.."
        "..KDDDPGGPDDK.."
        ".KKDDDDPPDDDKK."
        "KSDDDDPPPDDDDDK"
        "KSSSDDPPPDDSSSK"
        "KSSSDKKKKKKDSSK"
        "KKKKKPPPPPPKKKK"
        "....KPPPPPPK...."
        "....KPPPPPPK...."
        "....KPpKKpPK...."
        "....KOK..KOK...."
        ".....K....K....."
        "................";

    // 5. TURRET BASE
    const char* gridTurretBase = 
        "......KKKKKKKK......"
        "....KKHhhhhhHKK...."
        "...KHHhhhhhhhHHK..."
        "..KHHhhhhYhhhhHHK.."
        ".KHHhhhhYYYhhhhHHK."
        "KHHhhhhYYYYYhhhhHHK"
        "KHHhhhYYYYYYYhhhHHK"
        "KHHhhYYYYYYYYYhhHHK"
        "KHHhYYYYYYYYYYYhHHK"
        "KHHHHHHHHHHHHHHHHHK"
        "KDSSSSSSSSSSSSSSSDK"
        "KDSSSDDDDDDDDDSSSDK"
        "KDSSDDDDDDDDDDDSSDK"
        "KDSDDDDDDDDDDDDDSDK"
        "KDDDDDDDDDDDDDDDDDK"
        "KDDDDDDDDDDDDDDDDDK"
        "KDDDDDDDDDDDDDDDDDK"
        "KKKKKKKKKKKKKKKKKKK"
        "..................."
        "...................";

    // 6. TURRET BARREL
    const char* gridTurretBarrel = 
        "....KKKK...."
        "....KSSK...."
        "....KSSK...."
        "....KDDK...."
        "....KDDK...."
        "....KDDK...."
        "....KDDK...."
        "...KKDDKK..."
        "..KSSDSSSK.."
        ".KDDDDDDDDK."
        "KKKKKKKKKKKK"
        "............";

    // 7. POWERUP W
    const char* gridPowerupW = 
        "KKKKKKKKKKKKKKKK"
        "KGGGGGGGGGGGGGGK"
        "KGGYYYYYYYYYYGGK"
        "KGYWWWWWWWWWWYGK"
        "KGYW.S.W.S.W.YGK"
        "KGYW.S.W.S.W.YGK"
        "KGYW.S.W.S.W.YGK"
        "KGYW.S.W.S.W.YGK"
        "KGYW.S.W.S.W.YGK"
        "KGYW.SSWSSW..YGK"
        "KGYW..SSSSS..YGK"
        "KGYWWWWWWWWWWYGK"
        "KGGYYYYYYYYYYGGK"
        "KGGGGGGGGGGGGGGK"
        "KKKKKKKKKKKKKKKK"
        "................";

    // 8. POWERUP P
    const char* gridPowerupP = 
        "KKKKKKKKKKKKKKKK"
        "KGGGGGGGGGGGGGGK"
        "KGGYYYYYYYYYYGGK"
        "KGYWWWWWWWWWWYGK"
        "KGYW..OOOOO..YGK"
        "KGYW..O...O..YGK"
        "KGYW..OOOOO..YGK"
        "KGYW..O......YGK"
        "KGYW..O......YGK"
        "KGYW..O......YGK"
        "KGYW..O......YGK"
        "KGYWWWWWWWWWWYGK"
        "KGGYYYYYYYYYYGGK"
        "KGGGGGGGGGGGGGGK"
        "KKKKKKKKKKKKKKKK"
        "................";

    // 9. POWERUP B
    const char* gridPowerupB = 
        "KKKKKKKKKKKKKKKK"
        "KGGGGGGGGGGGGGGK"
        "KGGYYYYYYYYYYGGK"
        "KGYWWWWWWWWWWYGK"
        "KGYW..PPPPP..YGK"
        "KGYW..P...P..YGK"
        "KGYW..PPPPP..YGK"
        "KGYW..P...P..YGK"
        "KGYW..P...P..YGK"
        "KGYW..PPPPP..YGK"
        "KGYW.........YGK"
        "KGYWWWWWWWWWWYGK"
        "KGGYYYYYYYYYYGGK"
        "KGGGGGGGGGGGGGGK"
        "KKKKKKKKKKKKKKKK"
        "................";

    // 10. POWERUP MEDAL
    const char* gridPowerupMedal = 
        "....KKKKKKKK...."
        "..KKGGGGGGGGKK.."
        ".KGGYYYYYYYYGGK."
        "KGGYYWWWWWWYYGGK"
        "KGYWYYYYYYYYWYGK"
        "KGYWYWWWWWWYWYGK"
        "KGYWYWYYYYWYWYGK"
        "KGYWYWYYYYWYWYGK"
        "KGYWYWYYYYWYWYGK"
        "KGYWYWWWWWWYWYGK"
        "KGYWYYYYYYYYWYGK"
        "KGGYYWWWWWWYYGGK"
        ".KGGYYYYYYYYGGK."
        "..KKGGGGGGGGKK.."
        "....KKKKKKKK...."
        "................";

    // 11. BULLET VULCAN
    const char* gridBulletVulcan = 
        ".KK."
        "KWWK"
        "KYYK"
        "KYYK"
        "KYYK"
        "KYYK"
        "KYYK"
        "KYYK"
        "KYYK"
        "KYYK"
        ".KK."
        "....";

    // 12. BULLET PLASMA
    const char* gridBulletPlasma = 
        "....KKKK...."
        "..KKCCCCKK.."
        ".KCCCCCCWCK."
        "KCCCCWWWWWCK"
        "KCCCWWWWWWWK"
        "KCCCWWWWWWWK"
        "KCCCWWWWWWWK"
        "KCCCCWWWWWCK"
        ".KCCCCCCWCK."
        "..KKCCCCKK.."
        "....KKKK...."
        "............";

    // 13. BULLET MISSILE
    const char* gridBulletMissile = 
        "...KK..."
        "..KrrK.."
        "..KGGK.."
        "..KGGK.."
        "..KGGK.."
        "..KGGK.."
        "..KGGK.."
        "..KGGK.."
        "..KGGK.."
        "..KGGK.."
        ".KKGGKK."
        "KWWKKWWK"
        "Krr..rrK"
        "K......K"
        "........"
        "........";

    // 14. BULLET ENEMY BASIC
    const char* gridBulletEnemyBasic = 
        "..KKKK.."
        ".KWRRWK."
        "KWRRRRWK"
        "KRRRRRRK"
        "KRRRRRRK"
        "KWRRRRWK"
        ".KWRRWK."
        "..KKKK..";

    // 15. BULLET ENEMY STRONG
    const char* gridBulletEnemyStrong = 
        "....KKKK...."
        "...KrrrrK..."
        "..KrrrrrrK.."
        ".KrrrWWrrrK."
        "KrrrWWWWrrrK"
        "KrrrWWWWrrrK"
        "KrrrWWWWrrrK"
        ".KrrrWWrrrK."
        "..KrrrrrrK.."
        "...KrrrrK..."
        "....KKKK...."
        "............";

    // 16. SHIELD BUBBLE
    const char* gridShieldBubble = 
        "............KKKKKK............"
        "........KKKcccccccKKK........."
        "......KKcccccccccccccKK......"
        "....KKcccccccccccccccccKK...."
        "   KcccccccCCCCCCCCcccccccK   "
        "  KccccccCCCCCCCCCCCccccccK  "
        " KccccccCCCCCCCCCCCCCccccccK "
        " KcccccCCCCCCCCCCCCCCCcccccK "
        "KcccccCCCCCCCCCCCCCCCCCcccccK"
        "KcccccCCCCCCCCCCCCCCCCCcccccK"
        "KcccccCCCCCCCCCCCCCCCCCcccccK"
        " KcccccCCCCCCCCCCCCCCCcccccK "
        " KccccccCCCCCCCCCCCCCccccccK "
        "  KccccccCCCCCCCCCCCccccccK  "
        "   KcccccccCCCCCCCCcccccccK   "
        "....KKcccccccccccccccccKK...."
        "......KKcccccccccccccKK......"
        "........KKKcccccccKKK........."
        "............KKKKKK............";

    // 17. MUZZLE VULCAN
    const char* gridMuzzleVulcan = 
        "....KK...."
        "...KYYK..."
        "..KYWWYK.."
        ".KYWWWWYK."
        "KKWWWWWWKK"
        "KKWWWWWWKK"
        ".KYWWWWYK."
        "..KYWWYK.."
        "...KYYK..."
        "....KK....";

    // 18. MUZZLE PLASMA
    const char* gridMuzzlePlasma = 
        "....KKKK...."
        "..KKcWWcKK.."
        ".KcCCCCCCcK."
        "KcCWWWWWWcCK"
        "KcWWWWWWWWcK"
        "KcWWWWWWWWcK"
        "KcCWWWWWWcCK"
        ".KcCCCCCCcK."
        "..KKcWWcKK.."
        "....KKKK....";

    // 19. MUZZLE MISSILE
    const char* gridMuzzleMissile = 
        "....KK...."
        "...KrrK..."
        "..KrYWrK.."
        ".KrYWWYrK."
        "KrrWWWWrrK"
        "KrrWWWWrrK"
        ".KrYWWYrK."
        "..KrYWrK.."
        "...KrrK..."
        "....KK....";

    // 20. BOSS MAIN SPRITE (64x64)
    // We construct a grid representing a heavy spacecraft core.
    // For Boss, we will use a shorter format or programmatically generate left/right wing spikes to keep string sizes sane,
    // but we can also write out a structured text grid of 64 lines of 64 characters. Let's do a beautiful structured layout!
    // Since 64x64 is 4096 chars, writing it out line-by-line is fine.
    // To ensure the boss looks outstanding, let's write it carefully.
    std::string bossGrid = "";
    for (int y = 0; y < 64; ++y) {
        std::string row = "";
        for (int x = 0; x < 64; ++x) {
            float dx = (float)(x - 32);
            float dy = (float)(y - 32);
            float d = std::sqrt(dx*dx + dy*dy);
            
            char pixel = '.';
            if (d < 30.0f) {
                pixel = 'b'; // default armored plate grey
                
                // Outlines
                if (d > 28.5f) pixel = 'K';
                else if (std::abs(dx) < 2.0f && std::abs(dy) < 20.0f) pixel = 'r'; // Central red glowing strip
                else if (std::abs(dx) < 6.0f && std::abs(dy) < 6.0f) pixel = 'R';  // Crimson weapon core shadow
                else if (std::abs(dx) > 20.0f && std::abs(dy) < 8.0f) pixel = 'P';  // Purple trim on side panels
                else if (std::abs(dx) > 24.0f && std::abs(dy) < 6.0f) pixel = 'p';  // Violet glow lights
                else if (dy < -24.0f && std::abs(dx) > 12.0f && std::abs(dx) < 18.0f) pixel = 'Y'; // Gold exhausts grates
                else if (dy > 20.0f && std::abs(dx) < 12.0f) pixel = 'B';           // Dark metal rear vents
                
                // Horizontal panel cuts
                if (std::abs((int)dy % 8) == 0 && d < 26.0f) pixel = 'K';
                // Vertical panel cuts
                if (std::abs((int)dx % 12) == 0 && d < 26.0f) pixel = 'K';
                // Highlight specular
                if (dx < -4.0f && dx > -20.0f && dy < -4.0f && dy > -20.0f && pixel == 'b') pixel = 'M';
            }
            
            // Add custom weapon pods on wings
            if (dy > -10.0f && dy < 10.0f) {
                if (std::abs(dx) > 28.0f && std::abs(dx) < 32.0f) {
                    pixel = 'K';
                }
                if (std::abs(dx) >= 30.0f && std::abs(dx) <= 31.0f && dy < 2.0f) {
                    pixel = 'G'; // Glowing green sensor lights
                }
            }
            
            row += pixel;
        }
        bossGrid += row + "\n";
    }

    // 21. EXPLOSION SPRITE SHEET (8 frames of 24x24 pixels)
    // We define each frame procedurally to create a classic pixel expansion.
    const char* expFrames[8] = {
        // Frame 0 (small core flash)
        "........................"
        "........................"
        "........................"
        "........................"
        "..........KKKK.........."
        ".........KWWWWK........."
        "........KWWWWWWK........"
        "........KWWWWWWK........"
        ".........KWWWWK........."
        "..........KKKK.........."
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................",

        // Frame 1
        "........................"
        "........................"
        "........................"
        ".........KKKKKK........."
        ".......KKYYYYYYKK......."
        "......KYYYYWWYYYYK......"
        ".....KYYYWWWWWWYYYK....."
        ".....KYYWWWWWWWWYYK....."
        ".....KYYWWWWWWWWYYK....."
        ".....KYYYWWWWWWYYYK....."
        "......KYYYYWWYYYYK......"
        ".......KKYYYYYYKK......."
        ".........KKKKKK........."
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................",

        // Frame 2
        "........................"
        "........................"
        "........KKKKKKKK........"
        "......KKYYYYYYYYKK......"
        ".....KYYYYOOOOYYYYK....."
        "....KYYYOOOOOOOOYYYK...."
        "....KYYOOOOOOOOOOYYK...."
        "....KYYOOOOOOOOOOYYK...."
        "....KYYYOOOOOOOOYYYK...."
        ".....KYYYYOOOOYYYYK....."
        "......KKYYYYYYYYKK......"
        "........KKKKKKKK........"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................",

        // Frame 3
        "........................"
        ".......KKKKKKKKKK......."
        ".....KKYYYYYYYYYYKK....."
        "....KYYYYOOOOOOYYYYK...."
        "...KYYYOOOOOOOOOOYYYK..."
        "...KYYOOOOO  OOOOOYYK..."
        "...KYYOOOO    OOOOYYK..."
        "...KYYOOOOO  OOOOOYYK..."
        "...KYYYOOOOOOOOOOYYYK..."
        "....KYYYYOOOOOOYYYYK...."
        ".....KKYYYYYYYYYYKK....."
        ".......KKKKKKKKKK......."
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................",

        // Frame 4
        "......KKKKKKKKKKKK......"
        "....KKRROOOOOOOORRKK...."
        "...KRROOOOOOOOOOOORRK..."
        "..KROOOOO      OOOOORK.."
        "..KROOOO        OOOORK.."
        "..KROOO          OOORK.."
        "..KROOO          OOORK.."
        "..KROOOO        OOOORK.."
        "..KROOOOO      OOOOORK.."
        "...KRROOOOOOOOOOOORRK..."
        "....KKRROOOOOOOORRKK...."
        "......KKKKKKKKKKKK......"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................",

        // Frame 5
        "....KKKK..KKKK..KKKK...."
        "...KRRRRKKRRRRKKRRRRK..."
        "..KRRR  RRRR  RRRR  RK.."
        "..KRR    RR    RR    K.."
        "..KRR    RR    RR    K.."
        "..KRRR  RRRR  RRRR  RK.."
        "...KRRRRKKRRRRKKRRRRK..."
        "....KKKK..KKKK..KKKK...."
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................",

        // Frame 6
        "...KKKK........KKKK....."
        "..KDDDDK......KDDDDK...."
        "..KDD  DK....KD  DDK...."
        "..KD    K....K    DK...."
        "..KDD  DK....KD  DDK...."
        "..KDDDDK......KDDDDK...."
        "...KKKK........KKKK....."
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................",

        // Frame 7
        "....KK..........KK......"
        "...KddK........KddK....."
        "...K..K........K..K....."
        "....KK..........KK......"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
        "........................"
    };

    // Parse and Generate Textures
    textures_[SpriteId::PlayerIdle]   = GenerateSprite(24, 24, gridPlayerIdle, palPlayer);
    textures_[SpriteId::PlayerSoftLeft] = GenerateSprite(24, 24, gridPlayerSoftLeft, palPlayer);
    textures_[SpriteId::PlayerLeft]   = GenerateSprite(24, 24, gridPlayerLeft, palPlayer);
    textures_[SpriteId::PlayerSoftRight] = GenerateSprite(24, 24, gridPlayerSoftRight, palPlayer);
    textures_[SpriteId::PlayerRight]  = GenerateSprite(24, 24, gridPlayerRight, palPlayer);
    textures_[SpriteId::Popcorn]      = GenerateSprite(16, 16, gridPopcorn, palPopcorn);
    textures_[SpriteId::TurretBase]   = GenerateSprite(19, 20, gridTurretBase, palTurretBase);
    textures_[SpriteId::TurretBarrel] = GenerateSprite(12, 12, gridTurretBarrel, palTurretBarrel);
    textures_[SpriteId::TurretBarrelRecoil] = GenerateSprite(12, 12, gridTurretBarrelRecoil, palTurretBarrel);
    textures_[SpriteId::Boss]         = GenerateSprite(64, 64, bossGrid.c_str(), palBoss);
    textures_[SpriteId::BossDamaged]  = GenerateSprite(64, 64, bossGrid.c_str(), palBossDamaged);
    textures_[SpriteId::PowerupW]     = GenerateSprite(16, 16, gridPowerupW, palPowerup);
    textures_[SpriteId::PowerupP]     = GenerateSprite(16, 16, gridPowerupP, palPowerup);
    textures_[SpriteId::PowerupB]     = GenerateSprite(16, 16, gridPowerupB, palPowerup);
    textures_[SpriteId::PowerupMedal] = GenerateSprite(16, 16, gridPowerupMedal, palPowerup);
    textures_[SpriteId::BulletVulcan] = GenerateSprite(4, 12, gridBulletVulcan, palBullet);
    textures_[SpriteId::BulletPlasma] = GenerateSprite(12, 12, gridBulletPlasma, palBullet);
    textures_[SpriteId::BulletMissile] = GenerateSprite(8, 16, gridBulletMissile, palBullet);
    textures_[SpriteId::BulletEnemyBasic] = GenerateSprite(8, 8, gridBulletEnemyBasic, palBullet);
    textures_[SpriteId::BulletEnemyStrong] = GenerateSprite(12, 12, gridBulletEnemyStrong, palBullet);
    textures_[SpriteId::ShieldBubble] = GenerateSprite(30, 19, gridShieldBubble, palShield);
    textures_[SpriteId::MuzzleVulcan] = GenerateSprite(10, 10, gridMuzzleVulcan, palBullet);
    textures_[SpriteId::MuzzlePlasma] = GenerateSprite(12, 12, gridMuzzlePlasma, palBullet);
    textures_[SpriteId::MuzzleMissile] = GenerateSprite(10, 10, gridMuzzleMissile, palBullet);

    // Explosions
    textures_[SpriteId::Explosion0] = GenerateSprite(24, 24, expFrames[0], palExplosion);
    textures_[SpriteId::Explosion1] = GenerateSprite(24, 24, expFrames[1], palExplosion);
    textures_[SpriteId::Explosion2] = GenerateSprite(24, 24, expFrames[2], palExplosion);
    textures_[SpriteId::Explosion3] = GenerateSprite(24, 24, expFrames[3], palExplosion);
    textures_[SpriteId::Explosion4] = GenerateSprite(24, 24, expFrames[4], palExplosion);
    textures_[SpriteId::Explosion5] = GenerateSprite(24, 24, expFrames[5], palExplosion);
    textures_[SpriteId::Explosion6] = GenerateSprite(24, 24, expFrames[6], palExplosion);
    textures_[SpriteId::Explosion7] = GenerateSprite(24, 24, expFrames[7], palExplosion);

    // Procedural larger structures
    textures_[SpriteId::AsteroidChunk]   = GenerateProceduralAsteroid();
    textures_[SpriteId::CloudForeground] = GenerateProceduralCloud();

    // Space station background tiles
    textures_[SpriteId::SpaceStationLeft]      = GenerateSprite(32, 32, gridSpaceStationLeft, palSpaceStation);
    textures_[SpriteId::SpaceStationLeftCore]  = GenerateSprite(32, 32, gridSpaceStationLeftCore, palSpaceStation);
    textures_[SpriteId::SpaceStationRight]     = GenerateSprite(32, 32, gridSpaceStationRight, palSpaceStation);
    textures_[SpriteId::SpaceStationRightCore]    = GenerateSprite(32, 32, gridSpaceStationRightCore, palSpaceStation);
    textures_[SpriteId::MiniBombCapsule]       = GenerateSprite(12, 12, gridMiniBomb, palPowerup);
    textures_[SpriteId::DebrisPlayerWingLeft]  = GenerateSprite(12, 12, gridDebrisPlayerWingLeft, palPlayer);
    textures_[SpriteId::DebrisPlayerWingRight] = GenerateSprite(12, 12, gridDebrisPlayerWingRight, palPlayer);
    textures_[SpriteId::DebrisEnemyWing]       = GenerateSprite(12, 12, gridDebrisEnemyWing, palPopcorn);
    textures_[SpriteId::DebrisEnemyThruster]   = GenerateSprite(12, 12, gridDebrisEnemyThruster, palPopcorn);
    textures_[SpriteId::DebrisBossCore]        = GenerateSprite(12, 12, gridDebrisBossCore, palBoss);
}

void SpriteManager::Cleanup() {
    for (auto& pair : textures_) {
        UnloadTexture(pair.second);
    }
    textures_.clear();
}

Texture2D SpriteManager::GetTexture(SpriteId id) const {
    auto it = textures_.find(id);
    if (it != textures_.end()) return it->second;
    return Texture2D{};
}

void SpriteManager::Draw(SpriteId id, Vector2 pos, float rotation, float scale, Color tint) const {
    Texture2D tex = GetTexture(id);
    if (tex.id == 0) return;

    Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
    Rectangle dest = { pos.x, pos.y, (float)tex.width * scale, (float)tex.height * scale };
    Vector2 origin = { (float)tex.width * scale / 2.0f, (float)tex.height * scale / 2.0f };
    DrawTexturePro(tex, src, dest, origin, rotation, tint);
}

void SpriteManager::DrawRect(SpriteId id, Rectangle src, Rectangle dest, Vector2 origin, float rotation, Color tint) const {
    Texture2D tex = GetTexture(id);
    if (tex.id == 0) return;
    DrawTexturePro(tex, src, dest, origin, rotation, tint);
}

Texture2D SpriteManager::GenerateSprite(int w, int h, const char* grid, const std::unordered_map<char, Color>& palette) {
    Image image = GenImageColor(w, h, BLANK);
    int x = 0;
    int y = 0;
    for (int i = 0; grid[i] != '\0'; ++i) {
        char c = grid[i];
        if (c == '\n' || c == '\r' || c == ' ' || c == '\t') continue;
        if (x < w && y < h) {
            Color color = BLANK;
            auto it = palette.find(c);
            if (it != palette.end()) {
                color = it->second;
            }
            ImageDrawPixel(&image, x, y, color);
        }
        x++;
        if (x >= w) {
            x = 0;
            y++;
        }
    }
    Texture2D tex = LoadTextureFromImage(image);
    UnloadImage(image);
    // Force nearest-neighbor filtering to retain pixelated retro sharpness!
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    return tex;
}

Texture2D SpriteManager::GenerateProceduralCloud() {
    // Generate a 128x128 soft cloud texture using overlapping alpha-blended circles
    int w = 128;
    int h = 128;
    Image image = GenImageColor(w, h, BLANK);

    // Draw central body
    for (int i = 0; i < 28; ++i) {
        int cx = 64 + GetRandomValue(-24, 24);
        int cy = 64 + GetRandomValue(-18, 18);
        int r = GetRandomValue(16, 32);
        Color col = Fade(WHITE, (float)GetRandomValue(10, 32) / 255.0f);
        ImageDrawCircle(&image, cx, cy, r, col);
    }
    
    // Draw some highlights
    for (int i = 0; i < 8; ++i) {
        int cx = 64 + GetRandomValue(-18, 18);
        int cy = 54 + GetRandomValue(-10, 10);
        int r = GetRandomValue(10, 18);
        Color col = Fade(RAYWHITE, (float)GetRandomValue(5, 15) / 255.0f);
        ImageDrawCircle(&image, cx, cy, r, col);
    }

    Texture2D tex = LoadTextureFromImage(image);
    UnloadImage(image);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR); // Clouds can be smooth
    return tex;
}

Texture2D SpriteManager::GenerateProceduralAsteroid() {
    int w = 32;
    int h = 32;
    Image image = GenImageColor(w, h, BLANK);

    // Color definitions
    Color outline = Color{12, 10, 8, 255};
    Color darkBody = Color{65, 50, 45, 255};
    Color midBody = Color{100, 80, 70, 255};
    Color lightBody = Color{145, 120, 105, 255};

    Vector2 center = {16, 16};
    float radius = 11.0f;

    // Generate random radial offsets
    float offsets[16];
    for (int i = 0; i < 16; ++i) {
        offsets[i] = (float)GetRandomValue(-2, 3);
    }

    // Draw outline and body
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float dx = (float)(x - center.x);
            float dy = (float)(y - center.y);
            float angle = std::atan2(dy, dx);
            if (angle < 0) angle += 2.0f * 3.14159f;
            int offsetIndex = (int)(angle / (2.0f * 3.14159f) * 16.0f) % 16;
            float currentRadius = radius + offsets[offsetIndex];
            
            float dist = std::sqrt(dx*dx + dy*dy);
            if (dist < currentRadius) {
                Color col = midBody;
                if (dist > currentRadius - 1.0f) col = outline;
                else if (dx < -2.0f && dy < -2.0f) col = lightBody; // Light highlight
                else if (dx > 2.0f && dy > 2.0f) col = darkBody;   // Shadow
                
                // Add some crater textures
                if (std::abs(dist - 5.0f) < 1.0f && dx < 0 && dy > 0) col = darkBody;
                if (std::abs(dist - 5.0f) < 0.5f && dx < -0.5f && dy > 0.5f) col = outline;
                
                ImageDrawPixel(&image, x, y, col);
            }
        }
    }

    Texture2D tex = LoadTextureFromImage(image);
    UnloadImage(image);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    return tex;
}
