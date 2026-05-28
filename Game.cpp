#include "Game.h"
#include "SpriteManager.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

static const char* VS_CODE = 
    "#version 330\n"
    "in vec3 vertexPosition;\n"
    "in vec2 vertexTexCoord;\n"
    "in vec4 vertexColor;\n"
    "out vec2 fragTexCoord;\n"
    "out vec4 fragColor;\n"
    "uniform mat4 mvp;\n"
    "void main() {\n"
    "    fragTexCoord = vertexTexCoord;\n"
    "    fragColor = vertexColor;\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char* FS_CODE = 
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "out vec4 finalColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "\n"
    "uniform float curvature;\n"
    "uniform float scanlineWeight;\n"
    "uniform float maskStrength;\n"
    "uniform float bloomStrength;\n"
    "uniform float vignetteStrength;\n"
    "uniform float glareStrength;\n"
    "const float scanlineFreq = 360.0;\n"
    "const float chromaticOffset = 0.0011;\n"
    "const float brightness = 1.18;\n"
    "\n"
    "vec2 curve(vec2 uv) {\n"
    "    uv = uv * 2.0 - 1.0;\n"
    "    vec2 offset = abs(uv.yx) * curvature;\n"
    "    uv = uv + uv * offset * offset;\n"
    "    uv = uv * 0.5 + 0.5;\n"
    "    return uv;\n"
    "}\n"
    "\n"
    "void main() {\n"
    "    vec2 uv = curve(fragTexCoord);\n"
    "    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {\n"
    "        finalColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
    "        return;\n"
    "    }\n"
    "    vec2 offset = uv - vec2(0.5);\n"
    "    vec4 col;\n"
    "    vec2 offRed = offset * chromaticOffset;\n"
    "    col.r = texture(texture0, uv - offRed).r;\n"
    "    col.g = texture(texture0, uv).g;\n"
    "    col.b = texture(texture0, uv + offRed).b;\n"
    "    col.a = texture(texture0, uv).a;\n"
    "\n"
    "    // Horizontal electron-beam glow bleed\n"
    "    vec4 bleed = texture(texture0, uv + vec2(-0.0015, 0.0)) * 0.22 +\n"
    "                 texture(texture0, uv + vec2(-0.0008, 0.0)) * 0.35 +\n"
    "                 texture(texture0, uv + vec2(0.0008, 0.0)) * 0.35 +\n"
    "                 texture(texture0, uv + vec2(0.0015, 0.0)) * 0.22;\n"
    "    col.rgb = mix(col.rgb, bleed.rgb, bloomStrength);\n"
    "\n"
    "    // Curved scanlines\n"
    "    float scanline = sin(uv.y * scanlineFreq * 3.14159265 * 2.0);\n"
    "    scanline = mix(1.0, (scanline + 1.0) * 0.5, scanlineWeight);\n"
    "    col.rgb *= scanline;\n"
    "\n"
    "    // High-fidelity shadow slot mask\n"
    "    vec2 maskCoord = uv * vec2(480.0, 640.0);\n"
    "    float xMod = mod(maskCoord.x, 3.0);\n"
    "    float mRed = 1.0;\n"
    "    float mGreen = 1.0;\n"
    "    float mBlue = 1.0;\n"
    "    if (xMod < 1.0) { mRed = 1.0; mGreen = 0.88; mBlue = 0.84; }\n"
    "    else if (xMod < 2.0) { mRed = 0.88; mGreen = 1.0; mBlue = 0.88; }\n"
    "    else { mRed = 0.84; mGreen = 0.88; mBlue = 1.0; }\n"
    "\n"
    "    float yMod = mod(maskCoord.y + step(1.5, xMod) * 1.5, 3.0);\n"
    "    if (yMod < 0.6) {\n"
    "        mRed *= maskStrength;\n"
    "        mGreen *= maskStrength;\n"
    "        mBlue *= maskStrength;\n"
    "    }\n"
    "    col.r *= mRed;\n"
    "    col.g *= mGreen;\n"
    "    col.b *= mBlue;\n"
    "\n"
    "    // Corner vignette\n"
    "    float vignette = uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y);\n"
    "    vignette = clamp(pow(16.0 * vignette, vignetteStrength), 0.0, 1.0);\n"
    "    col.rgb *= vignette;\n"
    "\n"
    "    // Cabinet ambient marquee glare reflection\n"
    "    float glare = 1.0 - length(uv - vec2(0.35, 0.25));\n"
    "    glare = clamp(pow(glare, 5.0) * glareStrength, 0.0, 1.0);\n"
    "    col.rgb += vec3(glare);\n"
    "\n"
    "    col.rgb *= brightness;\n"
    "    finalColor = col * colDiffuse * fragColor;\n"
    "}";

static std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

static bool CircleHit(Vector2 a, float ar, Vector2 b, float br) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float r = ar + br;
    return dx * dx + dy * dy <= r * r;
}

static constexpr int SettingsCount = 20;
static constexpr int VisibleSettingsRows = 11;
static constexpr float BossDeathDuration = 1.68f;
static constexpr int DiagBulletWarnThreshold = 42;
static constexpr int DiagEnemyWarnThreshold = 14;
static constexpr int DiagCombinedBulletThreshold = 30;
static constexpr int DiagCombinedEnemyThreshold = 9;
static constexpr int DiagRecoveryBulletThreshold = 2;
static constexpr int DiagRecoveryEnemyThreshold = 6;

const char* Game::CharacterCallsigns::Callsign(CommsSpeaker speaker) {
    switch (speaker) {
        case CommsSpeaker::Ace: return "ACE";
        case CommsSpeaker::Control: return "MOTHER-3";
        case CommsSpeaker::Wrench: return "WRENCH";
        case CommsSpeaker::Shade: return "SHADE";
        case CommsSpeaker::Boss: return "VANTAGE-9";
    }
    return "COMMS";
}

const char* Game::CharacterCallsigns::Name(CommsSpeaker speaker) {
    switch (speaker) {
        case CommsSpeaker::Ace: return "Liora Vance";
        case CommsSpeaker::Control: return "Mara Quill";
        case CommsSpeaker::Wrench: return "Nix Calder";
        case CommsSpeaker::Shade: return "Ivo Renn";
        case CommsSpeaker::Boss: return "Orbital Gatekeeper";
    }
    return "Unknown";
}

Color Game::CharacterCallsigns::ColorFor(CommsSpeaker speaker) {
    switch (speaker) {
        case CommsSpeaker::Ace: return Color{80, 210, 255, 255};
        case CommsSpeaker::Control: return Color{110, 255, 178, 255};
        case CommsSpeaker::Wrench: return Color{255, 184, 76, 255};
        case CommsSpeaker::Shade: return Color{190, 128, 255, 255};
        case CommsSpeaker::Boss: return Color{255, 76, 110, 255};
    }
    return SKYBLUE;
}

void Game::CommsPortrait::Draw(Vector2 pos, CommsSpeaker speaker, float moodPulse) const {
    Color frame = CharacterCallsigns::ColorFor(speaker);
    bool rival = speaker == CommsSpeaker::Boss || speaker == CommsSpeaker::Shade;
    bool engineer = speaker == CommsSpeaker::Wrench;
    bool control = speaker == CommsSpeaker::Control;
    Color core = rival ? Color{80, 18, 58, 255} : (engineer ? Color{92, 48, 8, 255} : Color{10, 52, 88, 255});
    DrawRectangle((int)pos.x, (int)pos.y, 62, 62, Fade(BLACK, 0.86f));
    DrawRectangleLines((int)pos.x, (int)pos.y, 62, 62, Fade(frame, 0.88f));
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV({pos.x + 31.0f, pos.y + 29.0f}, 24.0f, Fade(core, 0.72f));
    DrawCircleLines((int)(pos.x + 31.0f), (int)(pos.y + 29.0f), 26.0f + moodPulse * 3.0f, Fade(frame, 0.65f));
    EndBlendMode();
    if (speaker == CommsSpeaker::Boss) {
        DrawTriangle({pos.x + 18.0f, pos.y + 18.0f}, {pos.x + 46.0f, pos.y + 22.0f}, {pos.x + 31.0f, pos.y + 42.0f}, Fade(frame, 0.88f));
        DrawLineEx({pos.x + 15.0f, pos.y + 45.0f}, {pos.x + 48.0f, pos.y + 15.0f}, 2.0f, Fade(WHITE, 0.58f));
    } else if (speaker == CommsSpeaker::Shade) {
        DrawTriangle({pos.x + 16.0f, pos.y + 38.0f}, {pos.x + 47.0f, pos.y + 18.0f}, {pos.x + 42.0f, pos.y + 44.0f}, Fade(frame, 0.84f));
        DrawLineEx({pos.x + 20.0f, pos.y + 22.0f}, {pos.x + 42.0f, pos.y + 22.0f}, 2.0f, Fade(WHITE, 0.54f));
    } else if (engineer) {
        DrawRectangle((int)pos.x + 18, (int)pos.y + 20, 28, 24, Fade(frame, 0.84f));
        DrawCircleV({pos.x + 25.0f, pos.y + 32.0f}, 3.0f, WHITE);
        DrawCircleV({pos.x + 39.0f, pos.y + 32.0f}, 3.0f, WHITE);
    } else if (control) {
        DrawCircleLines((int)(pos.x + 31.0f), (int)(pos.y + 31.0f), 15.0f, Fade(frame, 0.84f));
        DrawLineEx({pos.x + 31.0f, pos.y + 13.0f}, {pos.x + 31.0f, pos.y + 49.0f}, 1.5f, Fade(WHITE, 0.58f));
        DrawLineEx({pos.x + 13.0f, pos.y + 31.0f}, {pos.x + 49.0f, pos.y + 31.0f}, 1.5f, Fade(WHITE, 0.58f));
    } else {
        DrawTriangle({pos.x + 31.0f, pos.y + 13.0f}, {pos.x + 14.0f, pos.y + 46.0f}, {pos.x + 48.0f, pos.y + 46.0f}, Fade(frame, 0.86f));
        DrawCircleV({pos.x + 31.0f, pos.y + 33.0f}, 6.0f, Fade(WHITE, 0.72f));
    }
}

void Game::DialogueBox::Draw(const char* speaker, const char* line, CommsSpeaker portraitSpeaker, const CommsPortrait& portrait) const {
    bool rightSpeaker = portraitSpeaker == CommsSpeaker::Boss || portraitSpeaker == CommsSpeaker::Shade;
    Color speakerColor = CharacterCallsigns::ColorFor(portraitSpeaker);
    DrawRectangle(16, 468, 448, 120, Fade(BLACK, 0.90f));
    DrawRectangleLines(16, 468, 448, 120, Fade(speakerColor, 0.80f));
    float pulse = 0.5f + 0.5f * std::sin((float)GetTime() * 8.0f);
    Vector2 portraitPos = rightSpeaker ? Vector2{386.0f, 486.0f} : Vector2{32.0f, 486.0f};
    portrait.Draw(portraitPos, portraitSpeaker, pulse);

    int textX = rightSpeaker ? 36 : 108;
    int textW = 342;
    DrawText(speaker, textX, 486, 14, speakerColor);
    DrawLine(textX, 506, textX + textW, 506, Fade(speakerColor, 0.35f));

    std::string copy(line);
    int y = 520;
    while (!copy.empty()) {
        size_t take = std::min<size_t>(44, copy.size());
        if (take < copy.size()) {
            size_t space = copy.rfind(' ', take);
            if (space != std::string::npos && space > 0) take = space;
        }
        std::string row = copy.substr(0, take);
        DrawText(row.c_str(), textX, y, 13, RAYWHITE);
        y += 18;
        if (take >= copy.size()) break;
        copy.erase(0, take + (copy[take] == ' ' ? 1 : 0));
    }

    const char* prompt = "ENTER / ACTION: ADVANCE";
    DrawText(prompt, 464 - MeasureText(prompt, 10) - 12, 572, 10, Fade(GRAY, 0.84f));
}

void Game::ScriptedDialogue::Begin(const std::vector<Line>& lines) {
    lines_ = lines;
    index_ = 0;
    lineTimer_ = 0.0f;
    active_ = !lines_.empty();
}

void Game::ScriptedDialogue::Update(float dt) {
    if (!active_) return;
    lineTimer_ += dt;
    bool advance = lineTimer_ > 2.65f || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) advance = true;
    if (!advance) return;
    lineTimer_ = 0.0f;
    ++index_;
    if (index_ >= lines_.size()) active_ = false;
}

void Game::ScriptedDialogue::Draw(const DialogueBox& box, const CommsPortrait& portrait) const {
    if (!active_ || index_ >= lines_.size()) return;
    box.Draw(lines_[index_].speaker, lines_[index_].text, lines_[index_].portraitSpeaker, portrait);
}

void Game::ScriptedDialogue::Reset() {
    lines_.clear();
    index_ = 0;
    lineTimer_ = 0.0f;
    active_ = false;
}

bool Game::CommsQueue::Push(const CommsMessage& message) {
    if (active_ && message.priority <= current_.priority && timer_ > 0.75f) {
        if (pending_.size() < 3) pending_.push_back(message);
        return false;
    }
    if (active_ && message.priority > current_.priority) {
        pending_.insert(pending_.begin(), current_);
    }
    current_ = message;
    active_ = true;
    timer_ = message.duration;
    cooldown_ = 0.42f;
    return true;
}

void Game::CommsQueue::Update(float dt) {
    if (cooldown_ > 0.0f) cooldown_ -= dt;
    if (active_) {
        timer_ -= dt;
        if (timer_ > 0.0f) return;
        active_ = false;
    }
    if (!active_ && cooldown_ <= 0.0f && !pending_.empty()) {
        size_t best = 0;
        for (size_t i = 1; i < pending_.size(); ++i) {
            if (pending_[i].priority > pending_[best].priority) best = i;
        }
        current_ = pending_[best];
        pending_.erase(pending_.begin() + (long)best);
        active_ = true;
        timer_ = current_.duration;
        cooldown_ = 0.42f;
    }
}

void Game::CommsQueue::Draw(const CommsPortrait& portrait) const {
    if (!active_) return;
    float alpha = std::clamp(timer_ < 0.35f ? timer_ / 0.35f : 1.0f, 0.0f, 1.0f);
    Color frame = CharacterCallsigns::ColorFor(current_.speaker);
    DrawRectangle(12, 44, 328, 72, Fade(BLACK, 0.82f * alpha));
    DrawRectangleLines(12, 44, 328, 72, Fade(frame, 0.78f * alpha));
    float pulse = 0.5f + 0.5f * std::sin((float)GetTime() * 9.5f);
    portrait.Draw({20.0f, 49.0f}, current_.speaker, pulse);
    DrawText(CharacterCallsigns::Callsign(current_.speaker), 91, 54, 13, Fade(frame, alpha));
    DrawText(CharacterCallsigns::Name(current_.speaker), 176, 55, 10, Fade(GRAY, alpha * 0.88f));
    DrawLine(90, 72, 326, 72, Fade(frame, 0.28f * alpha));
    DrawText(current_.line1, 91, 80, 12, Fade(RAYWHITE, alpha));
    if (current_.line2 && current_.line2[0] != '\0') {
        DrawText(current_.line2, 91, 96, 12, Fade(RAYWHITE, alpha));
    }
}

void Game::CommsQueue::Reset() {
    pending_.clear();
    current_ = {};
    active_ = false;
    timer_ = 0.0f;
    cooldown_ = 0.0f;
}

void Game::BossAttackNameOverlay::Show(const char* title) {
    title_ = title;
    timer_ = 2.25f;
}

void Game::BossAttackNameOverlay::Update(float dt) {
    if (timer_ > 0.0f) timer_ -= dt;
}

void Game::BossAttackNameOverlay::Draw() const {
    if (timer_ <= 0.0f || title_.empty()) return;
    float alpha = std::clamp(timer_ < 0.35f ? timer_ / 0.35f : 1.0f, 0.0f, 1.0f);
    DrawRectangle(0, 62, 480, 48, Fade(BLACK, 0.76f * alpha));
    DrawLine(0, 62, 480, 62, Fade(GOLD, 0.64f * alpha));
    DrawLine(0, 110, 480, 110, Fade(SKYBLUE, 0.42f * alpha));
    const char* label = "BOSS ATTACK";
    DrawText(label, 240 - MeasureText(label, 10) / 2, 68, 10, Fade(GRAY, alpha));
    DrawText(title_.c_str(), 240 - MeasureText(title_.c_str(), 17) / 2, 83, 17, Fade(GOLD, alpha));
}

void Game::BossAttackNameOverlay::Reset() {
    title_.clear();
    timer_ = 0.0f;
}

void Game::MissionBriefingOverlay::Begin() {
    timer_ = 5.6f;
}

void Game::MissionBriefingOverlay::Update(float dt) {
    if (timer_ > 0.0f) timer_ -= dt;
}

void Game::MissionBriefingOverlay::Draw() const {
    if (timer_ <= 0.0f) return;
    float alpha = std::clamp(timer_ < 0.75f ? timer_ / 0.75f : 1.0f, 0.0f, 1.0f);
    DrawRectangle(0, 78, 480, 126, Fade(BLACK, 0.72f * alpha));
    DrawLine(32, 78, 448, 78, Fade(SKYBLUE, 0.55f * alpha));
    DrawLine(32, 204, 448, 204, Fade(SKYBLUE, 0.30f * alpha));
    const char* title = "STAGE 1: SKY CIRCUIT";
    DrawText(title, 240 - MeasureText(title, 26) / 2, 94, 26, Fade(GOLD, alpha));
    DrawText("Unauthorized Entry Through the Upper Gate", 62, 126, 13, Fade(SKYBLUE, alpha));
    DrawText("SOLACE-IX // LAUNCH CLEARANCE DENIED", 62, 150, 13, Fade(Color{255, 112, 132, 255}, alpha));
    DrawText("Manual override accepted.", 62, 170, 13, Fade(RAYWHITE, alpha));
    DrawText("ACE: Liora Vance // Keep the channel open.", 62, 190, 12, Fade(GRAY, alpha));
}

void Game::MissionBriefingOverlay::Reset() {
    timer_ = 0.0f;
}

void Game::StageClearDebrief::Draw(float clearTimer, int loop, int lives, int bombs, int stageClearBonus) const {
    DrawRectangle(0, 0, 480, 640, Fade(BLACK, 0.65f));
    DrawRectangle(30, 86, 420, 468, Fade(BLACK, 0.87f));
    DrawRectangleLines(30, 86, 420, 468, Fade(GOLD, 0.8f));

    const char* title = "STAGE CLEAR";
    DrawText(title, 240 - MeasureText(title, 32) / 2, 106, 32, GOLD);
    DrawText("MOTHER-3: Gate is open. Barely.", 52, 154, 12, RAYWHITE);
    DrawText("WRENCH: SOLACE is scorched, but flying.", 52, 172, 12, Color{255, 184, 76, 255});
    DrawText("SHADE: So the locked door was personal.", 52, 190, 12, Color{190, 128, 255, 255});
    DrawText("ACE: No. It was waiting.", 52, 208, 12, SKYBLUE);
    DrawLine(50, 232, 430, 232, Fade(GOLD, 0.4f));

    int baseBonus = 2000;
    int livesBonus = lives * 500;
    int bombsBonus = bombs * 200;
    int totalBonus = stageClearBonus > 0 ? stageClearBonus : (baseBonus + livesBonus + bombsBonus) * loop;
    int shownTotalBonus = totalBonus;
    if (clearTimer < 3.45f) {
        float reveal = std::clamp((clearTimer - 0.92f) / 2.53f, 0.0f, 1.0f);
        shownTotalBonus = (int)((float)totalBonus * reveal);
    }

    DrawText("MISSION PERFORMANCE RESULTS", 60, 254, 15, SKYBLUE);
    DrawText("Base Clear Bonus:", 60, 288, 15, RAYWHITE);
    DrawText(TextFormat("+%d", baseBonus), 322, 288, 15, LIME);
    DrawText(TextFormat("Survivor Lives (%d):", lives), 60, 320, 15, RAYWHITE);
    DrawText(TextFormat("+%d", livesBonus), 322, 320, 15, LIME);
    DrawText(TextFormat("Remaining Bombs (%d):", bombs), 60, 352, 15, RAYWHITE);
    DrawText(TextFormat("+%d", bombsBonus), 322, 352, 15, LIME);
    DrawLine(60, 388, 420, 388, Fade(GRAY, 0.5f));
    DrawText("TOTAL STAGE BONUS:", 60, 410, 18, GOLD);
    DrawText(TextFormat("+%d", shownTotalBonus), 322, 410, 18, GOLD);

    const char* hook = "NEXT: THE EMPTY ORBITAL CHOIR";
    DrawText(hook, 240 - MeasureText(hook, 15) / 2, 468, 15, Color{255, 112, 132, 255});
    const char* loopText = TextFormat("ADVANCING TO LOOP %d", loop + 1);
    DrawText(loopText, 240 - MeasureText(loopText, 14) / 2, 500, 14, SKYBLUE);
    const char* prompt = "Press ENTER / Action A to continue";
    DrawText(prompt, 240 - MeasureText(prompt, 13) / 2, 526, 13, RAYWHITE);
}

Game::Game() {
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(ScreenW, ScreenH, "Sky Circuit - raylib arcade shooter");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    audio_.Init();
    
    // Initialize Pixel Art Sprites, Shaders, and Render Targets
    SpriteManager::Instance().Init();
    screenTarget_ = LoadRenderTexture(ScreenW, ScreenH);
    SetTextureFilter(screenTarget_.texture, TEXTURE_FILTER_POINT);
    crtShader_ = LoadShaderFromMemory(VS_CODE, FS_CODE);
    
    lastMousePos_ = GetMousePosition();

    
    // Load high scores and settings
    LoadHighScores();
    
    LoadSettings(); // Loads & applies settings to the audio files
    audio_.PlayCabinetBoot();
    audio_.PlayMusic(AudioSystem::MusicTrack::Title);

    // Initialize stars parallax
    for (int i = 0; i < NumStars; ++i) {
        stars_[i].pos.x = (float)GetRandomValue(0, ScreenW);
        stars_[i].pos.y = (float)GetRandomValue(0, ScreenH);
        
        int band = GetRandomValue(0, 2);
        if (band == 0) {
            stars_[i].speed = (float)GetRandomValue(15, 30);
            stars_[i].color = Fade(DARKBLUE, 0.4f);
        } else if (band == 1) {
            stars_[i].speed = (float)GetRandomValue(45, 75);
            stars_[i].color = Fade(SKYBLUE, 0.6f);
        } else {
            stars_[i].speed = (float)GetRandomValue(100, 160);
            stars_[i].color = Fade(RAYWHITE, 0.8f);
        }
    }

    // Populate background asteroids
    for (int i = 0; i < 8; ++i) {
        AsteroidInstance ast;
        ast.pos = { (float)GetRandomValue(0, ScreenW), (float)GetRandomValue(-ScreenH, ScreenH) };
        ast.speed = (float)GetRandomValue(35, 60);
        ast.rotation = (float)GetRandomValue(0, 360);
        ast.spinSpeed = (float)GetRandomValue(-30, 30);
        ast.scale = (float)GetRandomValue(8, 20) / 10.0f;
        backgroundAsteroids_.push_back(ast);
    }

    // Populate background clouds
    for (int i = 0; i < 5; ++i) {
        CloudInstance cld;
        cld.pos = { (float)GetRandomValue(-20, ScreenW - 20), (float)GetRandomValue(-ScreenH, ScreenH) };
        cld.speed = (float)GetRandomValue(70, 110);
        cld.scale = (float)GetRandomValue(15, 30) / 10.0f;
        backgroundClouds_.push_back(cld);
    }
}

Game::~Game() {
    audio_.Shutdown();
    if (crtShader_.id != 0) UnloadShader(crtShader_);
    UnloadRenderTexture(screenTarget_);
    SpriteManager::Instance().Cleanup();
    CloseWindow();
}

void Game::Run() {
    while (!WindowShouldClose() && !shouldExit_) {
        Update(GetFrameTime());
        
        // 1. Render gameplay at 480x640 to offscreen virtual canvas
        BeginTextureMode(screenTarget_);
        ClearBackground(BLACK);
        Draw();
        EndTextureMode();
        
        // 2. Render virtual canvas to window viewport with scaling/CRT shaders
        BeginDrawing();
        ClearBackground(BLACK);
        
        float scale = std::min((float)GetScreenWidth() / ScreenW, (float)GetScreenHeight() / ScreenH);
        if (aspectMode_ == 1) { // Integer scaling
            scale = std::max(1.0f, std::floor(scale));
        }
        
        float w = ScreenW * scale;
        float h = ScreenH * scale;
        if (aspectMode_ == 2) { // Stretch scaling
            w = (float)GetScreenWidth();
            h = (float)GetScreenHeight();
        }
        
        float x = (GetScreenWidth() - w) / 2.0f;
        float y = (GetScreenHeight() - h) / 2.0f;
        
        // Draw bezel cabinet panels if borders exist
        if (drawBezel_ && !cleanPixelMode_ && aspectMode_ != 2 && (x > 10.0f || y > 10.0f)) {
            DrawCabinetBezel(x, y, w, h);
        }
        
        // Apply CRT shader
        if (crtShaderEnabled_ && !cleanPixelMode_ && crtShader_.id != 0) {
            float curvVal = (float)crtCurvature_ * 0.0075f;
            float scanVal = (float)crtScanline_ * 0.032f;
            float maskVal = 1.0f - (float)crtMask_ * 0.026f;
            float bloomVal = (float)crtBloom_ * 0.028f;
            float vignVal = (float)crtVignette_ * 0.028f;
            float glareVal = (float)crtGlare_ * 0.008f;

            SetShaderValue(crtShader_, GetShaderLocation(crtShader_, "curvature"), &curvVal, SHADER_UNIFORM_FLOAT);
            SetShaderValue(crtShader_, GetShaderLocation(crtShader_, "scanlineWeight"), &scanVal, SHADER_UNIFORM_FLOAT);
            SetShaderValue(crtShader_, GetShaderLocation(crtShader_, "maskStrength"), &maskVal, SHADER_UNIFORM_FLOAT);
            SetShaderValue(crtShader_, GetShaderLocation(crtShader_, "bloomStrength"), &bloomVal, SHADER_UNIFORM_FLOAT);
            SetShaderValue(crtShader_, GetShaderLocation(crtShader_, "vignetteStrength"), &vignVal, SHADER_UNIFORM_FLOAT);
            SetShaderValue(crtShader_, GetShaderLocation(crtShader_, "glareStrength"), &glareVal, SHADER_UNIFORM_FLOAT);

            BeginShaderMode(crtShader_);
        }
        
        // Draw render texture (flipped vertically because raylib textures are flipped in memory)
        DrawTexturePro(
            screenTarget_.texture,
            Rectangle{0, 0, (float)screenTarget_.texture.width, -(float)screenTarget_.texture.height},
            Rectangle{x, y, w, h},
            Vector2{0, 0},
            0.0f,
            WHITE
        );
        
        if (crtShaderEnabled_ && !cleanPixelMode_ && crtShader_.id != 0) {
            EndShaderMode();
        }
        
        EndDrawing();
    }
}


void Game::StartGame() {
    player_.ResetForNewGame();
    player_.lives = (difficulty_ == 0) ? 3 : 2;
    player_.isDemo = demoMode_;
    player_.controlLayout = controlLayout_;
    playerBullets_.clear(); enemyBullets_.clear(); enemies_.clear(); powerups_.clear();
    bgLasers_.clear();
    bgSparks_.clear();
    spaceStationScrollY_ = 0.0f;
    screenFlashTimer_ = 0.0f;
    effects_.Clear();
    stageDirector_.Reset();
    stageRecoveryActive_ = false;
    stageBossRunwayActive_ = false;
    diagLastBlock_ = -1;
    diagSludgeWarned_ = false;
    diagLastDeathBlock_ = "NONE";
    diagLastDeathCause_ = "NONE";
    stageTime_ = 0.0f;
    clearTimer_ = 0.0f;
    stageClearTickTimer_ = 0.0f;
    stageClearTicksPlayed_ = 0;
    stageClearBonus_ = 0;
    loop_ = 1;
    bossSpawned_ = false;
    bossPhaseChanged_ = false;
    bossWarningTimer_ = 0.0f;
    bossWarningKlaxonTimer_ = 0.0f;
    bossDeathTimer_ = 0.0f;
    bossDeathExplosionTimer_ = 0.0f;
    bossClearDelayTimer_ = 0.0f;
    lowLifeAudioTimer_ = 0.0f;
    enemyShotAudioTimer_ = 0.0f;
    playerShotShakeTimer_ = 0.0f;
    medalChainTimer_ = 0.0f;
    medalChain_ = 0;
    nextScoreMilestone_ = 10000;
    nextExtraLifeScore_ = 50000;
    tutorialTimer_ = 4.0f;
    ResetStorySystems();
    missionBriefing_.Begin();
    for (int i = 0; i < 32; ++i) formationCount_[i] = 0;
    state_ = State::Playing;
    
    audio_.SetMusicDucked(false);
    audio_.PlayMusic(AudioSystem::MusicTrack::Stage);
    if (!demoMode_) audio_.PlayStageStart();
}

void Game::ReturnToTitle() {
    titleSelection_ = 0;
    returnFromHowTo_ = State::Title;
    player_.ResetForNewGame();
    player_.controlLayout = controlLayout_;
    playerBullets_.clear(); enemyBullets_.clear(); enemies_.clear(); powerups_.clear();
    effects_.Clear();
    stageDirector_.Reset();
    stageRecoveryActive_ = false;
    stageBossRunwayActive_ = false;
    diagLastBlock_ = -1;
    diagSludgeWarned_ = false;
    diagLastDeathBlock_ = "NONE";
    diagLastDeathCause_ = "NONE";
    stageTime_ = 0.0f;
    clearTimer_ = 0.0f;
    stageClearTickTimer_ = 0.0f;
    stageClearTicksPlayed_ = 0;
    stageClearBonus_ = 0;
    loop_ = 1;
    bossSpawned_ = false;
    bossPhaseChanged_ = false;
    demoMode_ = false;
    bossWarningTimer_ = 0.0f;
    bossWarningKlaxonTimer_ = 0.0f;
    bossDeathTimer_ = 0.0f;
    bossDeathExplosionTimer_ = 0.0f;
    bossClearDelayTimer_ = 0.0f;
    lowLifeAudioTimer_ = 0.0f;
    enemyShotAudioTimer_ = 0.0f;
    playerShotShakeTimer_ = 0.0f;
    medalChainTimer_ = 0.0f;
    medalChain_ = 0;
    ResetStorySystems();
    for (int i = 0; i < 32; ++i) formationCount_[i] = 0;
    state_ = State::Title;
    
    audio_.SetMusicDucked(false);
    audio_.PlayMusic(AudioSystem::MusicTrack::Title);
}

void Game::NextLoop() {
    ++loop_;
    stageTime_ = 0.0f;
    clearTimer_ = 0.0f;
    stageClearTickTimer_ = 0.0f;
    stageClearTicksPlayed_ = 0;
    stageClearBonus_ = 0;
    bossSpawned_ = false;
    bossPhaseChanged_ = false;
    bossWarningTimer_ = 0.0f;
    bossWarningKlaxonTimer_ = 0.0f;
    bossDeathTimer_ = 0.0f;
    bossDeathExplosionTimer_ = 0.0f;
    bossClearDelayTimer_ = 0.0f;
    lowLifeAudioTimer_ = 0.0f;
    enemyShotAudioTimer_ = 0.0f;
    playerShotShakeTimer_ = 0.0f;
    medalChainTimer_ = 0.0f;
    medalChain_ = 0;
    ResetStorySystems();
    missionBriefing_.Begin();
    for (int i = 0; i < 32; ++i) formationCount_[i] = 0;
    playerBullets_.clear(); enemyBullets_.clear(); enemies_.clear(); powerups_.clear();
    stageDirector_.Reset();
    stageRecoveryActive_ = false;
    stageBossRunwayActive_ = false;
    diagLastBlock_ = -1;
    diagSludgeWarned_ = false;
    diagLastDeathBlock_ = "NONE";
    diagLastDeathCause_ = "NONE";
    player_.bombs = std::min(6, player_.bombs + 1);
    state_ = State::Playing;
    audio_.SetMusicDucked(false);
    audio_.PlayMusic(AudioSystem::MusicTrack::Stage);
    audio_.PlayStageStart();
}

void Game::Update(float dt) {
    // Coin insertion trigger (C key or Select button on Gamepad)
    bool coinPressed = IsKeyPressed(KEY_C);
    if (IsGamepadAvailable(0)) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_LEFT)) coinPressed = true;
    }
    if (coinPressed) {
        if (credits_ < 99) {
            credits_++;
            audio_.PlayInsertCoin();
            effects_.AddText({ 240, 320 }, "CREDIT +1", GOLD);
        } else {
            audio_.PlayDenied();
            effects_.AddText({ 240, 320 }, "CREDIT MAX", RED);
        }
    }

    // Detect active input device for hybrid navigation focus
    bool anyKeyboardGamepad = 
        IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) ||
        IsKeyPressed(KEY_W) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_D) ||
        IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE) ||
        IsKeyPressed(KEY_P) || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_X) || IsKeyPressed(KEY_J) || IsKeyPressed(KEY_K) ||
        IsKeyPressed(KEY_C);
        
    if (IsGamepadAvailable(0)) {
        for (int b = 0; b < 15; ++b) {
            if (IsGamepadButtonPressed(0, b)) {
                anyKeyboardGamepad = true;
                break;
            }
        }
    }
    if (anyKeyboardGamepad) {
        lastInputType_ = InputType::KeyboardGamepad;
    }
    
    Vector2 mPos = GetMousePosition();
    float mDist = std::sqrt((mPos.x - lastMousePos_.x) * (mPos.x - lastMousePos_.x) + 
                            (mPos.y - lastMousePos_.y) * (mPos.y - lastMousePos_.y));
    if (mDist > 4.0f || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        lastInputType_ = InputType::Mouse;
        lastMousePos_ = mPos;
    }

    if (IsKeyPressed(KEY_F1)) {
        debug_ = !debug_;
        hitboxOverlayEnabled_ = debug_;
        SaveSettings();
        audio_.PlaySettingsTick();
    }
    
    scrollA_ += (70.0f + loop_ * 5.0f) * dt;
    scrollB_ += (32.0f + loop_ * 2.5f) * dt;
    if (scrollA_ > ScreenH) scrollA_ -= ScreenH;
    if (scrollB_ > ScreenH) scrollB_ -= ScreenH;

    for (int i = 0; i < NumStars; ++i) {
        stars_[i].pos.y += stars_[i].speed * dt;
        if (stars_[i].pos.y > ScreenH) {
            stars_[i].pos.y = -10;
            stars_[i].pos.x = (float)GetRandomValue(0, ScreenW);
        }
    }

    // Scroll parallax background layers
    nebulaScroll_ += 5.5f * dt;
    if (nebulaScroll_ > ScreenH) nebulaScroll_ -= ScreenH;

    for (auto& ast : backgroundAsteroids_) {
        ast.pos.y += ast.speed * dt;
        ast.rotation += ast.spinSpeed * dt;
        if (ast.pos.y > ScreenH + 30) {
            ast.pos.y = -40;
            ast.pos.x = (float)GetRandomValue(0, ScreenW);
            ast.rotation = (float)GetRandomValue(0, 360);
        }
    }

    for (auto& cld : backgroundClouds_) {
        cld.pos.y += cld.speed * dt;
        if (cld.pos.y > ScreenH + 80) {
            cld.pos.y = -90;
            cld.pos.x = (float)GetRandomValue(-20, ScreenW - 20);
        }
    }

    // Scroll space station walls
    spaceStationScrollY_ += 31.0f * dt;
    if (spaceStationScrollY_ > 32.0f) spaceStationScrollY_ -= 32.0f;

    // Update screen flash
    if (screenFlashTimer_ > 0.0f) {
        screenFlashTimer_ -= dt;
    }

    // Update distant battles (bgLasers_ and bgSparks_)
    for (auto& l : bgLasers_) {
        l.life -= dt;
    }
    bgLasers_.erase(std::remove_if(bgLasers_.begin(), bgLasers_.end(), [](const BackgroundLaser& l) { return l.life <= 0.0f; }), bgLasers_.end());

    for (auto& s : bgSparks_) {
        s.life -= dt;
        s.pos.x += s.vel.x * dt;
        s.pos.y += s.vel.y * dt;
    }
    bgSparks_.erase(std::remove_if(bgSparks_.begin(), bgSparks_.end(), [](const BackgroundSpark& s) { return s.life <= 0.0f; }), bgSparks_.end());

    // Spawn new distant lasers & sparks
    if (state_ == State::Playing && GetRandomValue(0, 100) < 2 && bgLasers_.size() < 3) {
        BackgroundLaser l;
        float startY = (float)GetRandomValue(50, ScreenH - 150);
        float angle = (float)GetRandomValue(-20, 20) * DEG2RAD;
        bool directionLeft = GetRandomValue(0, 1) == 0;
        l.start = directionLeft ? Vector2{ 0, startY } : Vector2{ (float)ScreenW, startY };
        float length = (float)GetRandomValue(150, 300);
        l.end = { l.start.x + (directionLeft ? 1.0f : -1.0f) * cosf(angle) * length, l.start.y + sinf(angle) * length };
        l.maxLife = l.life = (float)GetRandomValue(10, 25) / 100.0f; // fast flash
        l.color = (GetRandomValue(0, 1) == 0) ? RED : LIME;
        bgLasers_.push_back(l);

        // Spawn background sparks
        int sparkCount = GetRandomValue(3, 6);
        Vector2 sparkOrigin = (GetRandomValue(0, 1) == 0) ? l.start : l.end;
        for (int i = 0; i < sparkCount; ++i) {
            BackgroundSpark s;
            s.pos = sparkOrigin;
            float sAngle = (float)GetRandomValue(0, 360) * DEG2RAD;
            float speed = (float)GetRandomValue(25, 60);
            s.vel = { cosf(sAngle) * speed, sinf(sAngle) * speed };
            s.maxLife = s.life = (float)GetRandomValue(20, 60) / 100.0f;
            s.color = l.color;
            bgSparks_.push_back(s);
        }
    }

    
    effects_.Update(dt);
    if (settingsFeedbackTimer_ > 0.0f) {
        settingsFeedbackTimer_ -= dt;
    }
    
    audio_.Update();
    
    if (transitioning_) {
        UpdateTransition(dt);
        return;
    }
    
    // Attract mode input exit trigger
    if (state_ == State::Playing && demoMode_) {
        bool inputDetected = false;
        for (int k = 32; k < 348; ++k) {
            if (IsKeyPressed(k)) { inputDetected = true; break; }
        }
        if (IsGamepadAvailable(0)) {
            for (int b = 0; b < 15; ++b) {
                if (IsGamepadButtonPressed(0, b)) { inputDetected = true; break; }
            }
        }
        if (inputDetected) {
            demoMode_ = false;
            audio_.PlayMenuConfirm();
            ReturnToTitle();
            return;
        }
    }
    
    if (state_ == State::Title) {
        titleTimer_ += dt;
        static int lastAttractShimmer = -1;
        int attractShimmer = (int)(titleTimer_ / 5.0f);
        if (attractShimmer != lastAttractShimmer) {
            lastAttractShimmer = attractShimmer;
            audio_.PlayAttractShimmer();
        }
        if (titleTimer_ > 10.0f) {
            demoMode_ = true;
            titleTimer_ = 0.0f;
            StartGame();
        } else {
            bool keyUp = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
            bool keyDown = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
            bool keyConfirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
            
            if (IsGamepadAvailable(0)) {
                if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) keyUp = true;
                if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) keyDown = true;
                if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) keyConfirm = true;
            }
            
            Vector2 mousePos = GetMousePosition();
            static Vector2 lastMousePos = { -1.0f, -1.0f };
            bool mouseMoved = (std::abs(mousePos.x - lastMousePos.x) > 0.5f || std::abs(mousePos.y - lastMousePos.y) > 0.5f);
            lastMousePos = mousePos;
            
            if (lastInputType_ == InputType::Mouse && mouseMoved) {
                for (int i = 0; i < 5; ++i) {
                    int y = 220 + i * 32;
                    if (mousePos.x >= 120 && mousePos.x <= 360 && mousePos.y >= y - 6 && mousePos.y <= y + 20) {
                        if (titleSelection_ != i) {
                            titleSelection_ = i;
                            titleTimer_ = 0.0f;
                            audio_.PlayMenuMove();
                        }
                    }
                }
            }
            
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                for (int i = 0; i < 5; ++i) {
                    int y = 220 + i * 32;
                    if (mousePos.x >= 120 && mousePos.x <= 360 && mousePos.y >= y - 6 && mousePos.y <= y + 20) {
                        titleSelection_ = i;
                        keyConfirm = true;
                    }
                }
            }
            
            bool keyEscape = IsKeyPressed(KEY_ESCAPE);
            if (IsGamepadAvailable(0)) {
                if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_LEFT)) keyEscape = true;
            }
            
            if (keyEscape) {
                audio_.PlayMenuConfirm();
                exitConfirmSelection_ = 0;
                returnFromExitConfirm_ = State::Title;
                titleTimer_ = 0.0f;
                StartTransition(State::ExitConfirm);
            }
            
            if (keyUp) {
                titleSelection_ = (titleSelection_ + 4) % 5;
                titleTimer_ = 0.0f;
                audio_.PlayMenuMove();
            }
            if (keyDown) {
                titleSelection_ = (titleSelection_ + 1) % 5;
                titleTimer_ = 0.0f;
                audio_.PlayMenuMove();
            }
            
            if (keyConfirm) {
                titleTimer_ = 0.0f;
                if (titleSelection_ == 0) {
                    if (credits_ > 0 || demoMode_) {
                        audio_.PlayPressStart();
                        difficultySelection_ = 0;
                        if (demoMode_) {
                            difficulty_ = 0;
                            StartTransition(State::Playing);
                        } else {
                            StartTransition(State::DifficultySelect);
                        }
                    } else {
                        audio_.PlayDenied();
                        effects_.AddText({ 240, 215 }, "INSERT COIN!", RED);
                    }
                } else {
                    audio_.PlayMenuConfirm();
                    if (titleSelection_ == 1) {
                        returnFromHowTo_ = State::Title;
                        StartTransition(State::HowTo);
                    } else if (titleSelection_ == 2) {
                        returnFromSettings_ = State::Title;
                        isFullscreen_ = IsWindowFullscreen();
                        StartTransition(State::Settings);
                    } else if (titleSelection_ == 3) {
                        StartTransition(State::HighScores);
                    } else {
                        exitConfirmSelection_ = 0;
                        returnFromExitConfirm_ = State::Title;
                        StartTransition(State::ExitConfirm);
                    }
                }
            }
        }
    } else if (state_ == State::HighScores) {
        bool backPressed = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE);
        if (IsGamepadAvailable(0)) {
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
                backPressed = true;
            }
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mousePos = GetMousePosition();
            if (mousePos.x >= 140 && mousePos.x <= 340 && mousePos.y >= 520 && mousePos.y <= 556) {
                backPressed = true;
            }
        }
        if (backPressed) {
            audio_.PlayMenuConfirm();
            StartTransition(State::Title);
        }
    } else if (state_ == State::Settings) {
        UpdateSettings();
    } else if (state_ == State::DifficultySelect) {
        UpdateDifficultySelect();
    } else if (state_ == State::Continue) {
        UpdateContinue(dt);
    } else if (state_ == State::ExitConfirm) {
        UpdateExitConfirm();
    } else if (state_ == State::ClearScoresConfirm) {
        UpdateClearScoresConfirm();
    } else if (state_ == State::NameEntry) {
        UpdateNameEntry();
    } else if (state_ == State::HowTo) {
        bool backPressed = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE);
        if (IsGamepadAvailable(0)) {
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
                backPressed = true;
            }
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mousePos = GetMousePosition();
            if (mousePos.x >= 140 && mousePos.x <= 340 && mousePos.y >= 520 && mousePos.y <= 556) {
                backPressed = true;
            }
        }
        if (backPressed) {
            audio_.PlayMenuConfirm();
            StartTransition(returnFromHowTo_);
        }
    } else if (state_ == State::Playing) {
        bool pausePressed = IsKeyPressed(KEY_P);
        if (IsGamepadAvailable(0)) {
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT)) {
                pausePressed = true;
            }
        }
        if (pausePressed) {
            audio_.PlayPause();
            pauseSelection_ = 0;
            state_ = State::Paused;
        } else {
            UpdatePlaying(dt);
        }
    } else if (state_ == State::Paused) {
        bool keyUp = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
        bool keyDown = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
        bool keyConfirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_P);
        bool keyBack = IsKeyPressed(KEY_ESCAPE);
        
        if (IsGamepadAvailable(0)) {
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) keyUp = true;
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) keyDown = true;
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) keyConfirm = true;
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT)) keyConfirm = true;
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) keyBack = true;
        }
        
        Vector2 mousePos = GetMousePosition();
        static Vector2 pauseLastMouse = { -1.0f, -1.0f };
        bool mouseMoved = (std::abs(mousePos.x - pauseLastMouse.x) > 0.5f || std::abs(mousePos.y - pauseLastMouse.y) > 0.5f);
        pauseLastMouse = mousePos;
        
        if (lastInputType_ == InputType::Mouse && mouseMoved) {
            for (int i = 0; i < 4; ++i) {
                int y = 290 + i * 36;
                if (mousePos.x >= 120 && mousePos.x <= 360 && mousePos.y >= y - 6 && mousePos.y <= y + 20) {
                    if (pauseSelection_ != i) {
                        pauseSelection_ = i;
                        audio_.PlayMenuMove();
                    }
                }
            }
        }
        
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            for (int i = 0; i < 4; ++i) {
                int y = 290 + i * 36;
                if (mousePos.x >= 120 && mousePos.x <= 360 && mousePos.y >= y - 6 && mousePos.y <= y + 20) {
                    pauseSelection_ = i;
                    keyConfirm = true;
                }
            }
        }
        
        if (keyUp) {
            pauseSelection_ = (pauseSelection_ + 3) % 4;
            audio_.PlayMenuMove();
        }
        if (keyDown) {
            pauseSelection_ = (pauseSelection_ + 1) % 4;
            audio_.PlayMenuMove();
        }
        
        if (keyConfirm) {
            if (pauseSelection_ == 0) audio_.PlayResume();
            else audio_.PlayMenuConfirm();
            if (pauseSelection_ == 0) {
                state_ = State::Playing;
            } else if (pauseSelection_ == 1) {
                returnFromHowTo_ = State::Paused;
                StartTransition(State::HowTo);
            } else if (pauseSelection_ == 2) {
                returnFromSettings_ = State::Paused;
                isFullscreen_ = IsWindowFullscreen();
                StartTransition(State::Settings);
            } else {
                exitConfirmSelection_ = 0;
                returnFromExitConfirm_ = State::Paused;
                StartTransition(State::ExitConfirm);
            }
        } else if (keyBack) {
            audio_.PlayResume();
            state_ = State::Playing;
        }
    } else if (state_ == State::StageClear) {
        clearTimer_ += dt;
        if (clearTimer_ > 0.92f && clearTimer_ < 3.45f && stageClearTicksPlayed_ < 22) {
            stageClearTickTimer_ -= dt;
            if (stageClearTickTimer_ <= 0.0f) {
                stageClearTickTimer_ += 0.115f;
                audio_.PlayBonusTick(stageClearTicksPlayed_);
                ++stageClearTicksPlayed_;
            }
        }
        bool skipPressed = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        if (IsGamepadAvailable(0)) {
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) skipPressed = true;
        }
        if (clearTimer_ > 8.0f || skipPressed) {
            audio_.PlayMenuConfirm();
            NextLoop();
        }
    } else if (state_ == State::GameOver) {
        bool keyUp = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
        bool keyDown = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
        bool keyConfirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
        
        if (IsGamepadAvailable(0)) {
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) keyUp = true;
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) keyDown = true;
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) keyConfirm = true;
        }
        
        Vector2 mousePos = GetMousePosition();
        static Vector2 overLastMouse = { -1.0f, -1.0f };
        bool mouseMoved = (std::abs(mousePos.x - overLastMouse.x) > 0.5f || std::abs(mousePos.y - overLastMouse.y) > 0.5f);
        overLastMouse = mousePos;
        
        if (lastInputType_ == InputType::Mouse && mouseMoved) {
            for (int i = 0; i < 3; ++i) {
                int y = 290 + i * 36;
                if (mousePos.x >= 120 && mousePos.x <= 360 && mousePos.y >= y - 6 && mousePos.y <= y + 20) {
                    if (gameOverSelection_ != i) {
                        gameOverSelection_ = i;
                        audio_.PlayMenuMove();
                    }
                }
            }
        }
        
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            for (int i = 0; i < 3; ++i) {
                int y = 290 + i * 36;
                if (mousePos.x >= 120 && mousePos.x <= 360 && mousePos.y >= y - 6 && mousePos.y <= y + 20) {
                    gameOverSelection_ = i;
                    keyConfirm = true;
                }
            }
        }
        
        if (keyUp) {
            gameOverSelection_ = (gameOverSelection_ + 2) % 3;
            audio_.PlayMenuMove();
        }
        if (keyDown) {
            gameOverSelection_ = (gameOverSelection_ + 1) % 3;
            audio_.PlayMenuMove();
        }
        
        if (keyConfirm) {
            if (gameOverSelection_ == 0) {
                if (credits_ > 0 || demoMode_) {
                    audio_.PlayPressStart();
                    if (!demoMode_) credits_--;
                    StartTransition(State::Playing);
                } else {
                    audio_.PlayDenied();
                    effects_.Shake(4.0f, 0.15f);
                    effects_.AddText({ 240, 215 }, "INSERT COIN!", RED);
                }
            } else if (gameOverSelection_ == 1) {
                audio_.PlayMenuConfirm();
                returnFromSettings_ = State::GameOver;
                isFullscreen_ = IsWindowFullscreen();
                StartTransition(State::Settings);
            } else {
                audio_.PlayMenuConfirm();
                exitConfirmSelection_ = 0;
                returnFromExitConfirm_ = State::GameOver;
                StartTransition(State::ExitConfirm);
            }
        }
        
        if (demoMode_) {
            gameOverTimer_ += dt;
            if (gameOverTimer_ > 4.0f) {
                StartTransition(State::Title);
            }
        }
    }
}

void Game::UpdatePlaying(float dt) {
    if (bossClearDelayTimer_ > 0.0f) {
        bossClearDelayTimer_ -= dt;
        enemyBullets_.clear();
        playerBullets_.clear();
        player_.Update(dt, effects_, enemies_, enemyBullets_);
        if (bossClearDelayTimer_ <= 0.0f) {
            audio_.SetMusicDucked(false);
            audio_.PlayStageClear();
            state_ = State::StageClear;
            clearTimer_ = 0.0f;
            stageClearTickTimer_ = 0.0f;
            stageClearTicksPlayed_ = 0;
            int baseBonus = 2000;
            int livesBonus = player_.lives * 500;
            int bombsBonus = player_.bombs * 200;
            int stageBonus = (baseBonus + livesBonus + bombsBonus) * loop_;
            if (difficulty_ == 1) stageBonus = (stageBonus * 3) / 2;
            stageClearBonus_ = stageBonus;
            player_.score += stageBonus;
            CheckScoreMilestones();
        }
        return;
    }

    if (bossDeathTimer_ > 0.0f) {
        bossDeathTimer_ -= dt;
        bossDeathExplosionTimer_ += dt;
        
        // Slowly drift the boss position down during its death sequence
        bossDeathPos_.y += 15.0f * dt;
        
        // Phase 1: wing pods rupture with sparks and shrapnel.
        if (bossDeathTimer_ > 1.22f) {
            if (bossDeathExplosionTimer_ > 0.07f) {
                bossDeathExplosionTimer_ = 0.0f;
                Vector2 wingOffset = (GetRandomValue(0, 1) == 0) ? Vector2{-35.0f, 0.0f} : Vector2{35.0f, 0.0f};
                Vector2 spawnPos = { bossDeathPos_.x + wingOffset.x + GetRandomValue(-6, 6), bossDeathPos_.y + wingOffset.y + GetRandomValue(-6, 6) };
                effects_.Spark(spawnPos, GOLD, {wingOffset.x * 2.0f, 30.0f});
                effects_.Explosion(spawnPos, ORANGE, 12, SpriteId::DebrisEnemyWing);
                effects_.DebrisShower(spawnPos, Color{145, 40, 200, 255}, 5);
                effects_.DebrisShower(spawnPos, DARKGRAY, 4);
                effects_.Shake(4.5f, 0.12f);
                audio_.PlayExplosionAt(ExplosionSize::Small, spawnPos.x, (float)ScreenW);
            }
        }
        // Phase 2: engines blow out, smoke vents stay behind the core.
        else if (bossDeathTimer_ > 0.70f) {
            if (bossDeathExplosionTimer_ > 0.085f) {
                bossDeathExplosionTimer_ = 0.0f;
                Vector2 engineOffset = (GetRandomValue(0, 1) == 0) ? Vector2{-20.0f, 26.0f} : Vector2{20.0f, 26.0f};
                Vector2 spawnPos = { bossDeathPos_.x + engineOffset.x + GetRandomValue(-5, 5), bossDeathPos_.y + engineOffset.y + GetRandomValue(-5, 5) };
                effects_.Smoke(spawnPos, Color{55, 55, 60, 170});
                effects_.Explosion(spawnPos, GRAY, 15, SpriteId::DebrisEnemyThruster);
                effects_.DebrisShower(spawnPos, Color{80, 80, 85, 255}, 6);
                effects_.Shake(6.5f, 0.15f);
                audio_.PlayExplosionAt(ExplosionSize::Small, spawnPos.x, (float)ScreenW);
            }
        }
        // Phase 3: the exposed core collapses inward before the final blast.
        else if (bossDeathTimer_ > 0.30f) {
            if (bossDeathExplosionTimer_ > 0.085f) {
                bossDeathExplosionTimer_ = 0.0f;
                Vector2 randOffset = { (float)GetRandomValue(-25, 25), (float)GetRandomValue(-25, 25) };
                Vector2 spawnPos = { bossDeathPos_.x + randOffset.x, bossDeathPos_.y + randOffset.y };
                effects_.Explosion(spawnPos, ORANGE, 18, SpriteId::DebrisBossCore);
                effects_.Spark(bossDeathPos_, Color{255, 80, 180, 255}, {0.0f, -25.0f});
                effects_.Shake(9.0f, 0.15f);
                audio_.PlayExplosionAt(ExplosionSize::Small, spawnPos.x, (float)ScreenW);
            }
        }
        // Phase 4: final implosion hold before catastrophic release.
        else {
            if (bossDeathExplosionTimer_ > 0.055f) {
                bossDeathExplosionTimer_ = 0.0f;
                effects_.Spark(bossDeathPos_, WHITE, {0.0f, -35.0f});
                effects_.Smoke(bossDeathPos_, Color{35, 28, 45, 150});
                effects_.Shake(3.0f + (0.30f - std::max(0.0f, bossDeathTimer_)) * 16.0f, 0.08f);
            }
        }

        if (bossDeathTimer_ <= 0.0f) {
            effects_.AddText(bossDeathPos_, "CORE COLLAPSE", WHITE);
            effects_.Explosion(bossDeathPos_, VIOLET, 112, SpriteId::DebrisBossCore);
            effects_.DebrisShower(bossDeathPos_, Color{235, 190, 15, 255}, 18); // Gold core shards
            effects_.DebrisShower(bossDeathPos_, DARKGRAY, 22); // Hull shards
            effects_.DebrisShower(bossDeathPos_, RED, 16); // Hot core slag
            effects_.Spark(bossDeathPos_, WHITE, {0.0f, -160.0f});
            effects_.Shake(26.0f, 0.72f);
            
            // Trigger screen white flash
            screenFlashTimer_ = 0.16f;

            audio_.PlayExplosionAt(ExplosionSize::Large, bossDeathPos_.x, (float)ScreenW);
            bossClearDelayTimer_ = 1.05f;
        }
        
        enemyBullets_.clear();
        return;
    }

    stageTime_ += dt;
    missionBriefing_.Update(dt);
    bossAttackName_.Update(dt);
    UpdateCommsEvents(dt);
    if (tutorialTimer_ > 0.0f) tutorialTimer_ -= dt;
    if (medalChainTimer_ > 0.0f) {
        medalChainTimer_ -= dt;
        if (medalChainTimer_ <= 0.0f) medalChain_ = 0;
    }
    if (playerShotShakeTimer_ > 0.0f) playerShotShakeTimer_ -= dt;

    bool recoveryWindow = !bossSpawned_ && stageDirector_.IsRecoveryWindow(stageTime_);
    if (recoveryWindow && !stageRecoveryActive_) {
        int bullets = ActiveEnemyBulletCount();
        int enemies = ActiveEnemyCount();
        if (bullets > DiagRecoveryBulletThreshold || enemies > DiagRecoveryEnemyThreshold) {
            TraceLog(LOG_WARNING, TextFormat("[STAGE DIAG] recovery pollution: block=%s bullets=%d enemies=%d t=%.2f",
                                             stageDirector_.CurrentBlockName(stageTime_), bullets, enemies,
                                             stageDirector_.CurrentBlockElapsed(stageTime_)));
        }
        enemyBullets_.clear();
        enemyShotAudioTimer_ = 0.0f;
    }
    stageRecoveryActive_ = recoveryWindow;

    bool bossRunway = !bossSpawned_ && stageDirector_.IsBossRunway(stageTime_);
    if (bossRunway && !stageBossRunwayActive_) {
        int bullets = ActiveEnemyBulletCount();
        int nonBossEnemies = ActiveNonBossEnemyCount();
        if (bullets > 0 || nonBossEnemies > 0) {
            TraceLog(LOG_WARNING, TextFormat("[STAGE DIAG] runway cleanup: staleBullets=%d staleNonBossEnemies=%d t=%.2f",
                                             bullets, nonBossEnemies, stageDirector_.CurrentBlockElapsed(stageTime_)));
        }
        enemyBullets_.clear();
        enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(),
                                      [](const Enemy& e) { return !e.IsBoss(); }),
                       enemies_.end());
        for (int i = 0; i < 32; ++i) formationCount_[i] = 0;
        enemyShotAudioTimer_ = 0.0f;
    }
    stageBossRunwayActive_ = bossRunway;

    player_.Update(dt, effects_, enemies_, enemyBullets_);
    UpdateBossStory(dt);

    bool bossStoryHold = bossStoryState_ == BossStoryState::Entrance || bossDialogue_.IsActive();
    if (bossStoryHold) {
        playerBullets_.clear();
        enemyBullets_.clear();
    }
    if (bossDialogue_.IsActive()) {
        return;
    }

    size_t before = playerBullets_.size();
    if (!bossStoryHold) player_.TryShoot(playerBullets_);
    if (playerBullets_.size() > before) {
        audio_.PlayPlayerShotAt(player_.weapon, player_.weaponLevel, player_.pos.x, (float)ScreenW);
        if (playerShotShakeTimer_ <= 0.0f) {
            if (player_.weapon == WeaponType::Vulcan) {
                effects_.Shake(0.55f, 0.035f);
                playerShotShakeTimer_ = 0.09f;
            } else if (player_.weapon == WeaponType::Plasma) {
                effects_.Shake(0.95f, 0.045f);
                playerShotShakeTimer_ = 0.13f;
            } else {
                effects_.Shake(0.75f, 0.04f);
                playerShotShakeTimer_ = 0.16f;
            }
        }
    }

    bool bombRequested = !bossStoryHold && player_.BombPressed();
    if (!bossStoryHold && player_.TryBomb()) {
        enemyBullets_.clear();
        for (auto& e : enemies_) e.Hit(e.IsBoss() ? 28 : 99, effects_);
        effects_.Explosion(player_.pos, SKYBLUE, 58);
        effects_.Shake(10.0f, 0.35f);
        audio_.PlayBomb();
        if (!commsBombUsed_) {
            commsBombUsed_ = true;
            QueueComms(CommsSpeaker::Shade, "That bought you three seconds.", "Use them.", 2, 2.4f);
        }
    } else if (bombRequested && player_.bombs <= 0 && !demoMode_) {
        audio_.PlayDenied();
        effects_.AddText(player_.pos, "NO BOMBS!", RED);
    }

    // Boss warning klaxon timer loop
    if (bossWarningTimer_ > 0.0f) {
        bossWarningTimer_ -= dt;
        bossWarningKlaxonTimer_ += dt;
        if (bossWarningKlaxonTimer_ > 0.8f) {
            bossWarningKlaxonTimer_ = 0.0f;
            if (bossWarningTimer_ > 0.5f) audio_.PlayBossWarning();
        }
    }

    if (!bossSpawned_) {
        stageDirector_.Update(stageTime_, loop_ + (difficulty_ == 1 ? 1 : 0), enemies_);
    }
    if (!bossSpawned_ && stageDirector_.ShouldSpawnBoss(stageTime_, BossAlive())) {
        int bullets = ActiveEnemyBulletCount();
        int nonBossEnemies = ActiveNonBossEnemyCount();
        if (bullets > 0 || nonBossEnemies > 0) {
            TraceLog(LOG_WARNING, TextFormat("[STAGE DIAG] boss spawn cleanup: staleBullets=%d staleNonBossEnemies=%d block=%s",
                                             bullets, nonBossEnemies, stageDirector_.CurrentBlockName(stageTime_)));
        }
        enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(),
                                      [](const Enemy& e) { return !e.IsBoss(); }),
                       enemies_.end());
        for (int i = 0; i < 32; ++i) formationCount_[i] = 0;
        enemies_.emplace_back(EnemyType::Miniboss, Vector2{240, -70}, loop_ + (difficulty_ == 1 ? 1 : 0), 0);
        bossSpawned_ = true;
        bossStoryState_ = BossStoryState::Entrance;
        lastBossAttackPhase_ = -1;
        enemyBullets_.clear();
        playerBullets_.clear();
        
        bossWarningTimer_ = 3.0f;
        bossWarningKlaxonTimer_ = 0.8f;
        audio_.PlayMusic(AudioSystem::MusicTrack::Boss);
        audio_.PlayBossEntrance();
        audio_.PlayBossWarning();
        if (!commsBossEntrance_) {
            commsBossEntrance_ = true;
            QueueComms(CommsSpeaker::Control, "Gate signature ahead.", "That is not a drone.", 4, 2.7f);
        }
    }

    // Auto-detect newly spawned formations
    for (const auto& e : enemies_) {
        if (e.active && e.formationId > 0 && e.formationId < 32) {
            if (formationCount_[e.formationId] == 0) {
                int count = 0;
                for (const auto& other : enemies_) {
                    if (other.active && other.formationId == e.formationId) {
                        count++;
                    }
                }
                formationCount_[e.formationId] = count;
            }
        }
    }

    for (auto& b : playerBullets_) {
        b.Update(dt, enemies_, effects_);
    }
    for (auto& b : enemyBullets_) b.Update(dt, {}, effects_);
    size_t beforeBullets = enemyBullets_.size();
    for (auto& e : enemies_) e.Update(dt, player_.pos, enemyBullets_, loop_ + (difficulty_ == 1 ? 1 : 0));
    UpdateBossStory(dt);
    if (enemyBullets_.size() > beforeBullets) {
        enemyShotAudioTimer_ -= dt;
        if (enemyShotAudioTimer_ <= 0.0f) {
            float shotX = 240.0f;
            int shotCount = 0;
            for (size_t i = beforeBullets; i < enemyBullets_.size(); ++i) {
                shotX += enemyBullets_[i].pos.x;
                ++shotCount;
            }
            if (shotCount > 0) shotX /= (float)(shotCount + 1);
            audio_.PlayEnemyShotAt(BossAlive() ? EnemyShotType::Strong : EnemyShotType::Basic, shotX, (float)ScreenW);
            enemyShotAudioTimer_ = 0.07f;
        }
    }
    for (auto& p : powerups_) p.Update(dt);

    UpdateStageDiagnostics();
    HandleCollisions();
    CheckScoreMilestones();
    Cleanup();

    for (const auto& e : enemies_) {
        if (e.active && e.IsBoss() && !bossPhaseChanged_ && e.hp <= e.maxHp / 2) {
            bossPhaseChanged_ = true;
            audio_.PlayBossPhaseChange();
            if (!commsBossBreach_) {
                commsBossBreach_ = true;
                QueueComms(CommsSpeaker::Wrench, "Armor breach!", "I see the core!", 5, 2.6f);
            }
            break;
        }
    }
    ShowBossAttackTitleIfChanged();

    // Spawn damage smoke and spark venting from damaged boss core and wings
    for (const auto& e : enemies_) {
        if (e.active && e.IsBoss() && e.hp < e.maxHp / 2) {
            float damageRatio = 1.0f - (float)e.hp / (float)e.maxHp;
            if (GetRandomValue(0, 100) < (int)(damageRatio * 40.0f)) {
                Vector2 ventPos = { e.pos.x + (float)GetRandomValue(-48, 48), e.pos.y + (float)GetRandomValue(-25, 20) };
                effects_.Smoke(ventPos, Color{58, 58, 64, 155});
                if (GetRandomValue(0, 100) < 44) {
                    effects_.Spark(ventPos, ORANGE, {0.0f, 60.0f});
                }
            }
        }
    }


    if (bossSpawned_ && !BossAlive() && state_ == State::Playing) {
        bossDeathTimer_ = BossDeathDuration;
        bossStoryState_ = BossStoryState::None;
        bossDeathExplosionTimer_ = 0.0f;
        bossDeathPos_ = { 240, 110 };
        for (const auto& e : enemies_) {
            if (e.IsBoss()) {
                bossDeathPos_ = e.pos;
                break;
            }
        }
        enemyBullets_.clear();
        audio_.PlayBossDefeat();
        effects_.AddText(bossDeathPos_, "CORE COLLAPSE", GOLD);
    }

    // Low-life ticking audio warning while the player is in danger.
    if (player_.lives <= 1 && !demoMode_ && state_ == State::Playing) {
        lowLifeAudioTimer_ += dt;
        if (lowLifeAudioTimer_ > 1.0f) {
            lowLifeAudioTimer_ = 0.0f;
            audio_.PlayLowLifeWarning();
        }
    } else {
        lowLifeAudioTimer_ = 0.0f;
    }
}

void Game::UpdateSettings() {
    bool keyUp = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
    bool keyDown = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
    bool keyLeft = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
    bool keyRight = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
    bool keyConfirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    
    if (IsGamepadAvailable(0)) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) keyUp = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) keyDown = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) keyLeft = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) keyRight = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) keyConfirm = true;
    }
    
    Vector2 mousePos = GetMousePosition();
    static Vector2 settingsLastMouse = { -1.0f, -1.0f };
    bool mouseMoved = (std::abs(mousePos.x - settingsLastMouse.x) > 0.5f || std::abs(mousePos.y - settingsLastMouse.y) > 0.5f);
    settingsLastMouse = mousePos;
    int startIdx = settingsSelection_ >= VisibleSettingsRows ? settingsSelection_ - (VisibleSettingsRows - 1) : 0;
    int endIdx = std::min(SettingsCount, startIdx + VisibleSettingsRows);
    
    if (lastInputType_ == InputType::Mouse && mouseMoved) {
        for (int i = startIdx; i < endIdx; ++i) {
            int y = 125 + (i - startIdx) * 25;
            if (mousePos.x >= 50 && mousePos.x <= 430 && mousePos.y >= y - 5 && mousePos.y <= y + 18) {
                if (settingsSelection_ != i) {
                    settingsSelection_ = i;
                    audio_.PlayMenuMove();
                }
            }
        }
    }
    
    // Mouse dragging sliders
    bool mouseDragging = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    if (mouseDragging) {
        for (int i = startIdx; i < endIdx; ++i) {
            if (!((i >= 0 && i <= 2) || (i >= 7 && i <= 12))) continue;
            int y = 125 + (i - startIdx) * 25;
            if (mousePos.x >= 270 && mousePos.x <= 390 && mousePos.y >= y - 8 && mousePos.y <= y + 18) {
                settingsSelection_ = i;
                int vol = (int)((mousePos.x - 280.0f + 5.0f) / 10.0f);
                vol = std::clamp(vol, 0, 10);
                if (i == 0 && masterVolume_ != vol) {
                    masterVolume_ = vol;
                    SaveSettings();
                    ApplyVolumeSettings();
                    audio_.PlaySettingsTick();
                } else if (i == 1 && sfxVolume_ != vol) {
                    sfxVolume_ = vol;
                    SaveSettings();
                    ApplyVolumeSettings();
                    audio_.PlaySettingsTick();
                } else if (i == 2 && bgmVolume_ != vol) {
                    bgmVolume_ = vol;
                    SaveSettings();
                    ApplyVolumeSettings();
                    audio_.PlaySettingsTick();
                } else if (i == 7 && crtCurvature_ != vol) {
                    crtCurvature_ = vol;
                    SaveSettings();
                    audio_.PlaySettingsTick();
                } else if (i == 8 && crtScanline_ != vol) {
                    crtScanline_ = vol;
                    SaveSettings();
                    audio_.PlaySettingsTick();
                } else if (i == 9 && crtMask_ != vol) {
                    crtMask_ = vol;
                    SaveSettings();
                    audio_.PlaySettingsTick();
                } else if (i == 10 && crtBloom_ != vol) {
                    crtBloom_ = vol;
                    SaveSettings();
                    audio_.PlaySettingsTick();
                } else if (i == 11 && crtVignette_ != vol) {
                    crtVignette_ = vol;
                    SaveSettings();
                    audio_.PlaySettingsTick();
                } else if (i == 12 && crtGlare_ != vol) {
                    crtGlare_ = vol;
                    SaveSettings();
                    audio_.PlaySettingsTick();
                }
            }
        }
    }
    
    // Mouse clicks
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (int i = startIdx; i < endIdx; ++i) {
            int y = 125 + (i - startIdx) * 25;
            if (mousePos.x >= 50 && mousePos.x <= 430 && mousePos.y >= y - 5 && mousePos.y <= y + 18) {
                settingsSelection_ = i;
                keyConfirm = true;
            }
        }
    }
    
    if (keyUp) {
        settingsSelection_ = (settingsSelection_ + SettingsCount - 1) % SettingsCount;
        audio_.PlayMenuMove();
    }
    if (keyDown) {
        settingsSelection_ = (settingsSelection_ + 1) % SettingsCount;
        audio_.PlayMenuMove();
    }
    
    bool changed = false;
    bool triggerBeep = false;
    if (settingsSelection_ == 0) {
        if (keyLeft && masterVolume_ > 0) {
            masterVolume_--;
            changed = true;
            triggerBeep = true;
        }
        if (keyRight && masterVolume_ < 10) {
            masterVolume_++;
            changed = true;
            triggerBeep = true;
        }
    } else if (settingsSelection_ == 1) {
        if (keyLeft && sfxVolume_ > 0) {
            sfxVolume_--;
            changed = true;
            triggerBeep = true;
        }
        if (keyRight && sfxVolume_ < 10) {
            sfxVolume_++;
            changed = true;
            triggerBeep = true;
        }
    } else if (settingsSelection_ == 2) {
        if (keyLeft && bgmVolume_ > 0) {
            bgmVolume_--;
            changed = true;
            triggerBeep = true;
        }
        if (keyRight && bgmVolume_ < 10) {
            bgmVolume_++;
            changed = true;
            triggerBeep = true;
        }
    } else if (settingsSelection_ == 3) {
        if (keyLeft || keyRight || keyConfirm) {
            screenShakeEnabled_ = !screenShakeEnabled_;
            changed = true;
            audio_.PlayMenuConfirm();
            if (screenShakeEnabled_) {
                effects_.Shake(8.0f, 0.25f);
            }
        }
    } else if (settingsSelection_ == 4) {
        if (keyLeft || keyRight || keyConfirm) {
            hitboxOverlayEnabled_ = !hitboxOverlayEnabled_;
            debug_ = hitboxOverlayEnabled_;
            changed = true;
            audio_.PlayMenuConfirm();
        }
    } else if (settingsSelection_ == 5) {
        if (keyLeft || keyRight || keyConfirm) {
            crtShaderEnabled_ = !crtShaderEnabled_;
            changed = true;
            audio_.PlayMenuConfirm();
        }
    } else if (settingsSelection_ == 6) {
        if (keyLeft || keyRight || keyConfirm) {
            cleanPixelMode_ = !cleanPixelMode_;
            changed = true;
            audio_.PlayMenuConfirm();
        }
    } else if (settingsSelection_ == 7) {
        if (keyLeft && crtCurvature_ > 0) { crtCurvature_--; changed = true; triggerBeep = true; }
        if (keyRight && crtCurvature_ < 10) { crtCurvature_++; changed = true; triggerBeep = true; }
    } else if (settingsSelection_ == 8) {
        if (keyLeft && crtScanline_ > 0) { crtScanline_--; changed = true; triggerBeep = true; }
        if (keyRight && crtScanline_ < 10) { crtScanline_++; changed = true; triggerBeep = true; }
    } else if (settingsSelection_ == 9) {
        if (keyLeft && crtMask_ > 0) { crtMask_--; changed = true; triggerBeep = true; }
        if (keyRight && crtMask_ < 10) { crtMask_++; changed = true; triggerBeep = true; }
    } else if (settingsSelection_ == 10) {
        if (keyLeft && crtBloom_ > 0) { crtBloom_--; changed = true; triggerBeep = true; }
        if (keyRight && crtBloom_ < 10) { crtBloom_++; changed = true; triggerBeep = true; }
    } else if (settingsSelection_ == 11) {
        if (keyLeft && crtVignette_ > 0) { crtVignette_--; changed = true; triggerBeep = true; }
        if (keyRight && crtVignette_ < 10) { crtVignette_++; changed = true; triggerBeep = true; }
    } else if (settingsSelection_ == 12) {
        if (keyLeft && crtGlare_ > 0) { crtGlare_--; changed = true; triggerBeep = true; }
        if (keyRight && crtGlare_ < 10) { crtGlare_++; changed = true; triggerBeep = true; }
    } else if (settingsSelection_ == 13) {
        if (keyLeft || keyConfirm) {
            aspectMode_ = (aspectMode_ + 2) % 3;
            changed = true;
            audio_.PlayMenuConfirm();
        } else if (keyRight) {
            aspectMode_ = (aspectMode_ + 1) % 3;
            changed = true;
            audio_.PlayMenuConfirm();
        }
    } else if (settingsSelection_ == 14) {
        if (keyLeft || keyRight || keyConfirm) {
            drawBezel_ = !drawBezel_;
            changed = true;
            audio_.PlayMenuConfirm();
        }
    } else if (settingsSelection_ == 15) {
        if (keyLeft || keyRight || keyConfirm) {
            isFullscreen_ = !isFullscreen_;
            ToggleFullscreen();
            changed = true;
            audio_.PlayMenuConfirm();
        }
    } else if (settingsSelection_ == 16) {
        if (keyLeft || keyRight || keyConfirm) {
            controlLayout_ = (controlLayout_ + 1) % 2;
            player_.controlLayout = controlLayout_;
            changed = true;
            audio_.PlayMenuConfirm();
        }
    } else if (settingsSelection_ == 17) {
        if (keyConfirm) {
            ResetSettingsToDefault();
            audio_.PlayMenuConfirm();
            settingsFeedbackTimer_ = 2.0f;
            settingsFeedbackText_ = "SETTINGS RESET!";
            settingsFeedbackColor_ = SKYBLUE;
        }
    } else if (settingsSelection_ == 18) {
        if (keyConfirm) {
            clearScoresSelection_ = 0;
            audio_.PlayMenuConfirm();
            StartTransition(State::ClearScoresConfirm);
        }
    } else if (settingsSelection_ == 19) {
        if (keyConfirm || IsKeyPressed(KEY_ESCAPE)) {
            SaveSettings();
            audio_.PlayMenuConfirm();
            StartTransition(returnFromSettings_);
        }
    }
    
    if (changed) {
        SaveSettings();
        ApplyVolumeSettings();
        if (triggerBeep) audio_.PlaySettingsTick();
    }
    
    if (IsKeyPressed(KEY_ESCAPE)) {
        SaveSettings();
        audio_.PlayMenuConfirm();
        StartTransition(returnFromSettings_);
    }
}

void Game::ResetSettingsToDefault() {
    masterVolume_ = 9;
    sfxVolume_ = 8;
    bgmVolume_ = 6;
    screenShakeEnabled_ = true;
    hitboxOverlayEnabled_ = false;
    isFullscreen_ = false;
    controlLayout_ = 0;
    player_.controlLayout = controlLayout_;
    crtShaderEnabled_ = true;
    cleanPixelMode_ = false;
    crtCurvature_ = 3;
    crtScanline_ = 3;
    crtMask_ = 4;
    crtBloom_ = 2;
    crtVignette_ = 3;
    crtGlare_ = 2;
    aspectMode_ = 0;
    drawBezel_ = true;
    
    debug_ = hitboxOverlayEnabled_;
    if (IsWindowFullscreen()) {
        ToggleFullscreen();
    }
    
    ApplyVolumeSettings();
    SaveSettings();
}


void Game::ClearScores() {
    highScores_.clear();
    HighScoreEntry defaults[5] = {
        { "SKY", 25000, 3 },
        { "ACE", 18000, 2 },
        { "PIL", 12000, 2 },
        { "BOX", 8000, 1 },
        { "AAA", 5000, 1 }
    };
    for (int i = 0; i < 5; ++i) {
        highScores_.push_back(defaults[i]);
    }
    SaveHighScores();
    settingsFeedbackTimer_ = 2.0f;
    settingsFeedbackText_ = "HIGH SCORES WIPED!";
    settingsFeedbackColor_ = RED;
}

void Game::UpdateNameEntry() {
    bool keyLeft = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
    bool keyRight = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
    bool keyUp = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
    bool keyDown = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
    bool keyConfirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    bool keyBack = IsKeyPressed(KEY_BACKSPACE);
    
    if (IsGamepadAvailable(0)) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) keyLeft = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) keyRight = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) keyUp = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) keyDown = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) keyConfirm = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) keyBack = true;
    }
    
    Vector2 mousePos = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (int i = 0; i < 3; ++i) {
            int x = ScreenW / 2 - 75 + i * 56;
            int y = 300;
            
            // Hover slot box
            if (mousePos.x >= x && mousePos.x <= x + 40 && mousePos.y >= y && mousePos.y <= y + 50) {
                nameEntryIndex_ = i;
                audio_.PlayHighScoreTick();
            }
            
            // Hover Up Arrow
            if (mousePos.x >= x && mousePos.x <= x + 40 && mousePos.y >= y - 18 && mousePos.y <= y) {
                nameEntryIndex_ = i;
                nameEntryBuf_[i]++;
                if (nameEntryBuf_[i] > 'Z') nameEntryBuf_[i] = 'A';
                audio_.PlayHighScoreTick();
            }
            
            // Hover Down Arrow
            if (mousePos.x >= x && mousePos.x <= x + 40 && mousePos.y >= y + 50 && mousePos.y <= y + 68) {
                nameEntryIndex_ = i;
                nameEntryBuf_[i]--;
                if (nameEntryBuf_[i] < 'A') nameEntryBuf_[i] = 'Z';
                audio_.PlayHighScoreTick();
            }
        }
        
        // Click Confirm initials button
        if (mousePos.x >= 140 && mousePos.x <= 340 && mousePos.y >= 440 && mousePos.y <= 476) {
            nameEntryIndex_ = 3;
            keyConfirm = true;
        }
    }
    
    static Vector2 entryLastMouse = { -1.0f, -1.0f };
    bool mouseMoved = (std::abs(mousePos.x - entryLastMouse.x) > 0.5f || std::abs(mousePos.y - entryLastMouse.y) > 0.5f);
    entryLastMouse = mousePos;
    if (lastInputType_ == InputType::Mouse && mouseMoved) {
        if (mousePos.x >= 140 && mousePos.x <= 340 && mousePos.y >= 440 && mousePos.y <= 476) {
            if (nameEntryIndex_ != 3) {
                nameEntryIndex_ = 3;
                audio_.PlayHighScoreTick();
            }
        } else {
            for (int i = 0; i < 3; ++i) {
                int x = ScreenW / 2 - 75 + i * 56;
                int y = 300;
                if (mousePos.x >= x && mousePos.x <= x + 40 && mousePos.y >= y - 18 && mousePos.y <= y + 68) {
                    if (nameEntryIndex_ != i) {
                        nameEntryIndex_ = i;
                        audio_.PlayHighScoreTick();
                    }
                }
            }
        }
    }
    
    if (keyBack) {
        if (nameEntryIndex_ > 0) {
            nameEntryIndex_--;
            audio_.PlayHighScoreTick();
        }
    } else {
        if (keyLeft && nameEntryIndex_ > 0) {
            nameEntryIndex_--;
            audio_.PlayHighScoreTick();
        }
        if (keyRight && nameEntryIndex_ < 3) {
            nameEntryIndex_++;
            audio_.PlayHighScoreTick();
        }
    }
    
    if (nameEntryIndex_ >= 0 && nameEntryIndex_ <= 2) {
        char curChar = nameEntryBuf_[nameEntryIndex_];
        if (keyUp) {
            curChar++;
            if (curChar > 'Z') curChar = 'A';
            nameEntryBuf_[nameEntryIndex_] = curChar;
            audio_.PlayHighScoreTick();
        }
        if (keyDown) {
            curChar--;
            if (curChar < 'A') curChar = 'Z';
            nameEntryBuf_[nameEntryIndex_] = curChar;
            audio_.PlayHighScoreTick();
        }
        
        // Direct characters
        for (int key = KEY_A; key <= KEY_Z; ++key) {
            if (IsKeyPressed(key)) {
                nameEntryBuf_[nameEntryIndex_] = (char)('A' + (key - KEY_A));
                audio_.PlayHighScoreTick();
                nameEntryIndex_++; // Auto advance to next slot/confirm button
                break;
            }
        }
    }
    
    if (keyConfirm) {
        if (nameEntryIndex_ < 3) {
            nameEntryIndex_++;
            audio_.PlayHighScoreTick();
        } else {
            nameEntryBuf_[3] = '\0';
            InsertHighScore(nameEntryBuf_, player_.score, loop_);
            SaveHighScores();
            audio_.PlayMenuConfirm();
            StartTransition(State::HighScores);
        }
    }
}

void Game::UpdateExitConfirm() {
    bool keyLeft = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
    bool keyRight = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) || IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
    bool keyConfirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    bool keyBack = IsKeyPressed(KEY_ESCAPE);
    
    if (IsGamepadAvailable(0)) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) keyLeft = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) keyRight = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) keyConfirm = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) keyBack = true;
    }
    
    Vector2 mousePos = GetMousePosition();
    static Vector2 ecLastMouse = { -1.0f, -1.0f };
    bool mouseMoved = (std::abs(mousePos.x - ecLastMouse.x) > 0.5f || std::abs(mousePos.y - ecLastMouse.y) > 0.5f);
    ecLastMouse = mousePos;
    
    if (lastInputType_ == InputType::Mouse && mouseMoved) {
        if (mousePos.y >= 320 && mousePos.y <= 356) {
            if (mousePos.x >= 100 && mousePos.x <= 220) {
                if (exitConfirmSelection_ != 0) {
                    exitConfirmSelection_ = 0;
                    audio_.PlayMenuMove();
                }
            } else if (mousePos.x >= 260 && mousePos.x <= 380) {
                if (exitConfirmSelection_ != 1) {
                    exitConfirmSelection_ = 1;
                    audio_.PlayMenuMove();
                }
            }
        }
    }
    
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (mousePos.y >= 320 && mousePos.y <= 356) {
            if (mousePos.x >= 100 && mousePos.x <= 220) {
                exitConfirmSelection_ = 0;
                keyConfirm = true;
            } else if (mousePos.x >= 260 && mousePos.x <= 380) {
                exitConfirmSelection_ = 1;
                keyConfirm = true;
            }
        }
    }
    
    if (keyLeft || keyRight) {
        exitConfirmSelection_ = 1 - exitConfirmSelection_;
        audio_.PlayMenuMove();
    }
    
    if (keyConfirm) {
        if (exitConfirmSelection_ == 0) {
            audio_.PlayMenuCancel();
            StartTransition(returnFromExitConfirm_);
        } else {
            audio_.PlayMenuConfirm();
            if (returnFromExitConfirm_ == State::Title) {
                shouldExit_ = true;
            } else {
                demoMode_ = false;
                StartTransition(State::Title);
            }
        }
    } else if (keyBack) {
        audio_.PlayMenuCancel();
        StartTransition(returnFromExitConfirm_);
    }
}

void Game::UpdateClearScoresConfirm() {
    bool keyLeft = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
    bool keyRight = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) || IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
    bool keyConfirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    bool keyBack = IsKeyPressed(KEY_ESCAPE);
    
    if (IsGamepadAvailable(0)) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) keyLeft = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) keyRight = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) keyConfirm = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) keyBack = true;
    }
    
    Vector2 mousePos = GetMousePosition();
    static Vector2 cscLastMouse = { -1.0f, -1.0f };
    bool mouseMoved = (std::abs(mousePos.x - cscLastMouse.x) > 0.5f || std::abs(mousePos.y - cscLastMouse.y) > 0.5f);
    cscLastMouse = mousePos;
    
    if (lastInputType_ == InputType::Mouse && mouseMoved) {
        if (mousePos.y >= 320 && mousePos.y <= 356) {
            if (mousePos.x >= 100 && mousePos.x <= 220) {
                if (clearScoresSelection_ != 0) {
                    clearScoresSelection_ = 0;
                    audio_.PlayMenuMove();
                }
            } else if (mousePos.x >= 260 && mousePos.x <= 380) {
                if (clearScoresSelection_ != 1) {
                    clearScoresSelection_ = 1;
                    audio_.PlayMenuMove();
                }
            }
        }
    }
    
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (mousePos.y >= 320 && mousePos.y <= 356) {
            if (mousePos.x >= 100 && mousePos.x <= 220) {
                clearScoresSelection_ = 0;
                keyConfirm = true;
            } else if (mousePos.x >= 260 && mousePos.x <= 380) {
                clearScoresSelection_ = 1;
                keyConfirm = true;
            }
        }
    }
    
    if (keyLeft || keyRight) {
        clearScoresSelection_ = 1 - clearScoresSelection_;
        audio_.PlayMenuMove();
    }
    
    if (keyConfirm) {
        if (clearScoresSelection_ == 0) {
            audio_.PlayMenuCancel();
            StartTransition(State::Settings);
        } else {
            audio_.PlayMenuConfirm();
            ClearScores();
            StartTransition(State::Settings);
        }
    } else if (keyBack) {
        audio_.PlayMenuCancel();
        StartTransition(State::Settings);
    }
}

int Game::ActiveEnemyCount() const {
    return (int)std::count_if(enemies_.begin(), enemies_.end(), [](const Enemy& e) {
        return e.active;
    });
}

int Game::ActiveEnemyBulletCount() const {
    return (int)std::count_if(enemyBullets_.begin(), enemyBullets_.end(), [](const Bullet& b) {
        return b.active;
    });
}

int Game::ActiveNonBossEnemyCount() const {
    return (int)std::count_if(enemies_.begin(), enemies_.end(), [](const Enemy& e) {
        return e.active && !e.IsBoss();
    });
}

static const char* EnemyTypeName(EnemyType type) {
    switch (type) {
        case EnemyType::Popcorn: return "Popcorn";
        case EnemyType::Turret: return "Turret";
        case EnemyType::Miniboss: return "Boss";
    }
    return "Unknown";
}

const char* Game::NearestActiveEnemyTypeName() const {
    const Enemy* nearest = nullptr;
    float bestDistSq = 0.0f;
    for (const auto& e : enemies_) {
        if (!e.active) continue;
        float dx = e.pos.x - player_.pos.x;
        float dy = e.pos.y - player_.pos.y;
        float distSq = dx * dx + dy * dy;
        if (!nearest || distSq < bestDistSq) {
            nearest = &e;
            bestDistSq = distSq;
        }
    }
    return nearest ? EnemyTypeName(nearest->type) : "None";
}

void Game::LogPlayerDeath(const char* cause) {
    diagLastDeathBlock_ = stageDirector_.CurrentBlockName(stageTime_);
    diagLastDeathCause_ = cause;
    TraceLog(LOG_WARNING, TextFormat(
        "[STAGE DEATH] cause=%s block=%s section=%s encounter=%s transition=%s blockT=%.2f enemies=%d bullets=%d intensity=%.2f nearest=%s recovery=%s runway=%s lives=%d",
        cause,
        stageDirector_.CurrentBlockName(stageTime_),
        stageDirector_.CurrentSectionName(stageTime_),
        stageDirector_.CurrentEncounterName(stageTime_),
        stageDirector_.CurrentTransitionName(stageTime_),
        stageDirector_.CurrentBlockElapsed(stageTime_),
        ActiveEnemyCount(),
        ActiveEnemyBulletCount(),
        stageDirector_.CurrentIntensity(stageTime_),
        NearestActiveEnemyTypeName(),
        stageDirector_.IsRecoveryWindow(stageTime_) ? "yes" : "no",
        stageDirector_.IsBossRunway(stageTime_) ? "yes" : "no",
        player_.lives));
}

void Game::UpdateStageDiagnostics() {
    if (state_ != State::Playing || demoMode_) return;

    int block = stageDirector_.CurrentBlockIndex(stageTime_);
    if (block != diagLastBlock_) {
        diagLastBlock_ = block;
        diagSludgeWarned_ = false;
    }

    int enemies = ActiveEnemyCount();
    int bullets = ActiveEnemyBulletCount();
    bool recovery = stageDirector_.IsRecoveryWindow(stageTime_);
    bool runway = stageDirector_.IsBossRunway(stageTime_);
    bool climax = std::string(stageDirector_.CurrentTransitionName(stageTime_)) == "CLIMAX" || BossAlive();

    bool bulletSludge = bullets > DiagBulletWarnThreshold;
    bool enemySludge = enemies > DiagEnemyWarnThreshold;
    bool combinedSludge = !climax && bullets >= DiagCombinedBulletThreshold && enemies >= DiagCombinedEnemyThreshold;
    bool calmPollution = (recovery || runway) && bullets > DiagRecoveryBulletThreshold;

    if (!diagSludgeWarned_ && (bulletSludge || enemySludge || combinedSludge || calmPollution)) {
        diagSludgeWarned_ = true;
        TraceLog(LOG_WARNING, TextFormat(
            "[STAGE DIAG] pressure warning: block=%s section=%s encounter=%s transition=%s blockT=%.2f enemies=%d bullets=%d intensity=%.2f recovery=%s runway=%s",
            stageDirector_.CurrentBlockName(stageTime_),
            stageDirector_.CurrentSectionName(stageTime_),
            stageDirector_.CurrentEncounterName(stageTime_),
            stageDirector_.CurrentTransitionName(stageTime_),
            stageDirector_.CurrentBlockElapsed(stageTime_),
            enemies,
            bullets,
            stageDirector_.CurrentIntensity(stageTime_),
            recovery ? "yes" : "no",
            runway ? "yes" : "no"));
    }
}

void Game::HandleCollisions() {
    for (auto& b : playerBullets_) {
        if (!b.active) continue;
        for (auto& e : enemies_) {
            if (!e.active || !CircleHit(b.pos, b.radius, e.pos, e.radius)) continue;
            b.active = false;
            e.Hit(b.damage, effects_);
            effects_.Spark(b.pos, YELLOW);
            if (!e.active) {
                int scoreAdded = e.scoreValue;
                if (difficulty_ == 1) scoreAdded = (scoreAdded * 3) / 2;
                player_.score += scoreAdded;
                
                char scoreText[16];
                std::snprintf(scoreText, sizeof(scoreText), "+%d", scoreAdded);
                effects_.AddText(e.pos, scoreText, YELLOW);

                SpriteId debrisSprite = SpriteId::AsteroidChunk;
                if (e.type == EnemyType::Popcorn) debrisSprite = SpriteId::DebrisEnemyWing;
                else if (e.type == EnemyType::Turret) debrisSprite = SpriteId::DebrisEnemyThruster;
                effects_.Explosion(e.pos, e.IsBoss() ? VIOLET : ORANGE, e.IsBoss() ? 80 : 22, debrisSprite);
                effects_.Shake(e.IsBoss() ? 8.0f : 3.0f, e.IsBoss() ? 0.45f : 0.16f);
                if (e.IsBoss()) bossDeathPos_ = e.pos;
                SpawnDrop(e.pos, e.type);

                if (e.IsBoss()) {
                    audio_.PlayExplosionAt(ExplosionSize::Large, e.pos.x, (float)ScreenW);
                } else if (e.type == EnemyType::Turret) {
                    audio_.PlayExplosionAt(ExplosionSize::Medium, e.pos.x, (float)ScreenW);
                } else {
                    audio_.PlayExplosionAt(ExplosionSize::Small, e.pos.x, (float)ScreenW);
                }

                if (e.formationId > 0 && e.formationId < 32) {
                    if (formationCount_[e.formationId] > 0) {
                        formationCount_[e.formationId]--;
                        if (formationCount_[e.formationId] == 0) {
                            int bonus = 1000;
                            if (difficulty_ == 1) bonus = (bonus * 3) / 2;
                            player_.score += bonus;
                            effects_.AddText(e.pos, TextFormat("FORMATION BONUS! +%d", bonus), GOLD);
                            powerups_.emplace_back(PowerupType::Medal, e.pos);
                            audio_.PlayFormationBonusAt(e.pos.x, (float)ScreenW);
                        }
                    }
                }
            } else if (e.IsBoss()) {
                audio_.PlayBossDamageAt(e.pos.x, (float)ScreenW);
            } else if (e.type == EnemyType::Turret) {
                audio_.PlayBossDamageAt(e.pos.x, (float)ScreenW);
            }
            break;
        }
    }

    if (!player_.invulnerable) {
        for (auto& b : enemyBullets_) {
            if (b.active && CircleHit(b.pos, b.radius, player_.pos, player_.hitRadius)) {
                b.active = false;
                --player_.lives;
                LogPlayerDeath("enemy bullet");
                effects_.Explosion(player_.pos, RED, 38, SpriteId::DebrisPlayerWingLeft);
                effects_.Shake(8.0f, 0.35f);
                if (player_.lives <= 0) {
                    audio_.PlayPlayerDeath();
                    if (demoMode_) {
                        audio_.PlayGameOver();
                        StartTransition(State::GameOver);
                    } else {
                        continueTimer_ = 9.9f;
                        StartTransition(State::Continue);
                    }
                } else {
                    audio_.PlayPlayerHit();
                    player_.ResetAfterHit();
                    audio_.PlayRespawn();
                }
                break;
            }
        }
    }

    for (auto& e : enemies_) {
        if (e.active && !player_.invulnerable && CircleHit(e.pos, e.radius, player_.pos, player_.hitRadius)) {
            e.active = false;
            --player_.lives;
            LogPlayerDeath("enemy collision");
            effects_.Explosion(player_.pos, RED, 38, SpriteId::DebrisPlayerWingRight);
            if (player_.lives <= 0) {
                audio_.PlayPlayerDeath();
                if (demoMode_) {
                    audio_.PlayGameOver();
                    StartTransition(State::GameOver);
                } else {
                    continueTimer_ = 9.9f;
                    StartTransition(State::Continue);
                }
            } else {
                audio_.PlayPlayerHit();
                player_.ResetAfterHit();
                audio_.PlayRespawn();
            }
        }
    }

    for (auto& p : powerups_) {
        if (!p.active || !CircleHit(p.pos, p.radius, player_.pos, player_.spriteRadius)) continue;
        p.active = false;
        bool isMedal = false;
        if (p.type == PowerupType::WeaponChange) {
            player_.NextWeapon();
            effects_.AddText(player_.pos, "WEAPON SWAP!", SKYBLUE);
        }
        else if (p.type == PowerupType::Upgrade) {
            player_.UpgradeWeapon();
            effects_.AddText(player_.pos, "POWER UP!", ORANGE);
        }
        else if (p.type == PowerupType::Bomb) {
            player_.bombs = std::min(6, player_.bombs + 1);
            effects_.AddText(player_.pos, "BOMB +1", PURPLE);
        }
        else {
            int scoreGain = (difficulty_ == 1) ? 750 : 500;
            player_.score += scoreGain;
            medalChain_ = medalChainTimer_ > 0.0f ? medalChain_ + 1 : 1;
            medalChainTimer_ = 1.35f;
            effects_.AddText(p.pos, medalChain_ > 1 ? TextFormat("+%d  x%d", scoreGain, medalChain_) : TextFormat("+%d", scoreGain), GOLD);
            isMedal = true;
        }
        if (isMedal) audio_.PlayMedalAt(medalChain_, p.pos.x, (float)ScreenW);
        else audio_.PlayPickupAt(p.type, p.pos.x, (float)ScreenW);
        if (!commsPickup_ && !isMedal) {
            commsPickup_ = true;
            if (p.type == PowerupType::Upgrade) {
                QueueComms(CommsSpeaker::Wrench, "Output’s cleaner.", "SOLACE still answers.", 2, 2.5f);
            } else if (p.type == PowerupType::Bomb) {
                QueueComms(CommsSpeaker::Control, "Bomb capsule loaded.", "Save it for the ugly part.", 2, 2.5f);
            } else {
                QueueComms(CommsSpeaker::Shade, "New teeth.", "Same stubborn pilot.", 2, 2.5f);
            }
        }
    }
}

void Game::SpawnDrop(Vector2 pos, EnemyType source) {
    if (source == EnemyType::Miniboss) {
        powerups_.emplace_back(PowerupType::Upgrade, pos);
        powerups_.emplace_back(PowerupType::Bomb, Vector2{pos.x - 30, pos.y});
        return;
    }
    int r = GetRandomValue(0, 99);
    if (r < 12) powerups_.emplace_back(PowerupType::WeaponChange, pos);
    else if (r < 24) powerups_.emplace_back(PowerupType::Upgrade, pos);
    else if (r < 33) powerups_.emplace_back(PowerupType::Bomb, pos);
    else if (r < 58) powerups_.emplace_back(PowerupType::Medal, pos);
}

bool Game::BossAlive() const {
    for (const auto& e : enemies_) if (e.active && e.IsBoss()) return true;
    return false;
}

Enemy* Game::ActiveBoss() {
    for (auto& e : enemies_) {
        if (e.active && e.IsBoss()) return &e;
    }
    return nullptr;
}

const Enemy* Game::ActiveBoss() const {
    for (const auto& e : enemies_) {
        if (e.active && e.IsBoss()) return &e;
    }
    return nullptr;
}

void Game::ResetStorySystems() {
    bossStoryState_ = BossStoryState::None;
    lastBossAttackPhase_ = -1;
    for (bool& shown : bossAttackShown_) shown = false;
    bossDialogueStarted_ = false;
    commsFirstWave_ = false;
    commsUpperLane_ = false;
    commsBreakThrough_ = false;
    commsEncirclement_ = false;
    commsRecovery_ = false;
    commsGateApproach_ = false;
    commsLowHealth_ = false;
    commsBombUsed_ = false;
    commsPickup_ = false;
    commsBossEntrance_ = false;
    commsBossBreach_ = false;
    commsBossFinal_ = false;
    bossDialogue_.Reset();
    commsQueue_.Reset();
    bossAttackName_.Reset();
    missionBriefing_.Reset();
}

void Game::StartBossDialogue() {
    if (bossDialogueStarted_) return;
    bossDialogueStarted_ = true;
    bossStoryState_ = BossStoryState::Dialogue;
    enemyBullets_.clear();
    playerBullets_.clear();
    bossWarningTimer_ = 0.0f;
    bossWarningKlaxonTimer_ = 0.0f;
    audio_.PlayBossWarning();
    bossDialogue_.Begin({
        {"ACE", "Route’s closed. I’ll cut one.", CommsSpeaker::Ace},
        {"VANTAGE-9", "UNREGISTERED ACE DETECTED. SOLACE SIGNATURE: OBSOLETE.", CommsSpeaker::Boss},
        {"SHADE", "Good news: it hates you specifically.", CommsSpeaker::Shade},
        {"VANTAGE-9", "SKY CIRCUIT ACCESS DENIED. MEMORY OF FLIGHT: REVOKED.", CommsSpeaker::Boss}
    });
}

void Game::UpdateBossStory(float dt) {
    if (bossStoryState_ == BossStoryState::Entrance) {
        const Enemy* boss = ActiveBoss();
        if (boss && boss->pos.y >= 107.0f && bossWarningTimer_ <= 0.15f) {
            StartBossDialogue();
        }
    }

    if (bossDialogue_.IsActive()) {
        bossDialogue_.Update(dt);
        if (!bossDialogue_.IsActive()) {
            if (Enemy* boss = ActiveBoss()) {
                boss->BeginBossCombat();
                bossStoryState_ = BossStoryState::Combat;
                lastBossAttackPhase_ = -1;
                ShowBossAttackTitleIfChanged();
                effects_.AddText({boss->pos.x, boss->pos.y + 55.0f}, "DUEL START", GOLD);
            }
        }
    }
}

void Game::ShowBossAttackTitleIfChanged() {
    const Enemy* boss = ActiveBoss();
    if (!boss || bossStoryState_ != BossStoryState::Combat) return;
    int phase = static_cast<int>(boss->BossPhase());
    if (phase <= 0 || phase >= 5 || phase == lastBossAttackPhase_) return;
    lastBossAttackPhase_ = phase;
    if (bossAttackShown_[phase]) return;
    bossAttackShown_[phase] = true;
    bossAttackName_.Show(boss->BossAttackTitle());
    audio_.PlayBossPhaseChange();

    if (boss->BossPhase() == BossAttackPhase::AzureNeedle) {
        QueueComms(CommsSpeaker::Boss, "ACCESS DENIED.", "GATE LAW: DESCEND.", 5, 2.4f);
    } else if (boss->BossPhase() == BossAttackPhase::FallingStar) {
        QueueComms(CommsSpeaker::Control, "Gate lattice forming.", "Read the gaps, not the glow.", 4, 2.7f);
    } else if (boss->BossPhase() == BossAttackPhase::BrokenHalo) {
        QueueComms(CommsSpeaker::Boss, "PILOT MEMORY INVALID.", "WHITE HALO DENIAL.", 5, 2.5f);
    } else if (boss->BossPhase() == BossAttackPhase::Overload && !commsBossFinal_) {
        commsBossFinal_ = true;
        audio_.SetMusicDucked(true);
        QueueComms(CommsSpeaker::Boss, "FINAL GATE LAW:", "COLLAPSE.", 5, 2.5f);
    }
}

void Game::QueueComms(CommsSpeaker speaker, const char* line1, const char* line2, int priority, float duration) {
    if (bossDialogue_.IsActive()) return;
    if (commsQueue_.Push(CommsMessage{speaker, line1, line2, priority, duration})) {
        if (priority >= 4) audio_.PlayBossWarning();
        else audio_.PlaySettingsTick();
    }
}

void Game::UpdateCommsEvents(float dt) {
    commsQueue_.Update(dt);
    if (demoMode_ || state_ != State::Playing || bossDialogue_.IsActive()) return;

    if (!commsFirstWave_ && stageTime_ > 0.75f) {
        commsFirstWave_ = true;
        QueueComms(CommsSpeaker::Ace, "Keep the channel open.", "I’m going in.", 2, 2.7f);
    }
    if (!commsUpperLane_ && stageTime_ > 16.4f && !bossSpawned_) {
        commsUpperLane_ = true;
        QueueComms(CommsSpeaker::Control, "Upper lane is hostile.", "Keep low.", 2, 2.4f);
    }
    if (!commsBreakThrough_ && stageTime_ > 32.8f && !bossSpawned_) {
        commsBreakThrough_ = true;
        QueueComms(CommsSpeaker::Shade, "You always pick the locked door.", "Don’t drift admiring it.", 2, 2.7f);
    }
    if (!commsEncirclement_ && stageTime_ > 48.8f && !bossSpawned_) {
        commsEncirclement_ = true;
        QueueComms(CommsSpeaker::Control, "Pincer vectors crossing.", "Find the gap, then push.", 3, 2.5f);
    }
    if (!commsRecovery_ && stageTime_ > 59.7f && !bossSpawned_) {
        commsRecovery_ = true;
        QueueComms(CommsSpeaker::Wrench, "Lane is open for a second.", "Take the metal, breathe.", 2, 2.5f);
    }
    if (!commsGateApproach_ && stageTime_ > 96.8f && !bossSpawned_) {
        commsGateApproach_ = true;
        QueueComms(CommsSpeaker::Ace, "Comms are getting thin.", "Something is listening.", 4, 2.8f);
    }
    if (!commsLowHealth_ && player_.lives <= 1) {
        commsLowHealth_ = true;
        QueueComms(CommsSpeaker::Wrench, "SOLACE is heating fast.", "Ease the frame, Liora.", 4, 3.0f);
    }
}

void Game::CheckScoreMilestones() {
    if (demoMode_) return;
    while (player_.score >= nextScoreMilestone_) {
        audio_.PlayScoreMilestone();
        nextScoreMilestone_ += 10000;
    }
    while (player_.score >= nextExtraLifeScore_) {
        player_.lives = std::min(player_.lives + 1, 6);
        effects_.AddText(player_.pos, "EXTRA LIFE!", LIME);
        audio_.PlayExtraLife();
        nextExtraLifeScore_ += 50000;
    }
}

void Game::Cleanup() {
    // Disqualify formation score if any member escapes offscreen
    for (const auto& e : enemies_) {
        if (e.active && e.Offscreen(ScreenH) && e.formationId > 0 && e.formationId < 32) {
            formationCount_[e.formationId] = -1;
        }
    }

    auto eraseBullets = [](auto& v, int w, int h) {
        v.erase(std::remove_if(v.begin(), v.end(), [w, h](const Bullet& b){ return !b.active || b.Offscreen(w, h); }), v.end());
    };
    eraseBullets(playerBullets_, ScreenW, ScreenH);
    eraseBullets(enemyBullets_, ScreenW, ScreenH);
    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(), [](const Enemy& e){ return !e.active || e.Offscreen(ScreenH); }), enemies_.end());
    powerups_.erase(std::remove_if(powerups_.begin(), powerups_.end(), [](const Powerup& p){ return !p.active || p.Offscreen(ScreenH); }), powerups_.end());
}

void Game::UpdateDifficultySelect() {
    bool keyUp = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
    bool keyDown = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
    bool keyConfirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    bool keyBack = IsKeyPressed(KEY_ESCAPE);
    
    if (IsGamepadAvailable(0)) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) keyUp = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) keyDown = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) keyConfirm = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) keyBack = true;
    }
    
    Vector2 mousePos = GetMousePosition();
    static Vector2 dsLastMouse = { -1.0f, -1.0f };
    bool mouseMoved = (std::abs(mousePos.x - dsLastMouse.x) > 0.5f || std::abs(mousePos.y - dsLastMouse.y) > 0.5f);
    dsLastMouse = mousePos;
    
    if (lastInputType_ == InputType::Mouse && mouseMoved) {
        for (int i = 0; i < 2; ++i) {
            int y = 220 + i * 130;
            if (mousePos.x >= 50 && mousePos.x <= 430 && mousePos.y >= y - 10 && mousePos.y <= y + 100) {
                if (difficultySelection_ != i) {
                    difficultySelection_ = i;
                    audio_.PlayMenuMove();
                }
            }
        }
    }
    
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (int i = 0; i < 2; ++i) {
            int y = 220 + i * 130;
            if (mousePos.x >= 50 && mousePos.x <= 430 && mousePos.y >= y - 10 && mousePos.y <= y + 100) {
                difficultySelection_ = i;
                keyConfirm = true;
            }
        }
    }
    
    if (keyUp || keyDown) {
        difficultySelection_ = 1 - difficultySelection_;
        audio_.PlayMenuMove();
    }
    
    if (keyConfirm) {
        audio_.PlayPressStart();
        difficulty_ = difficultySelection_;
        if (credits_ > 0) credits_--;
        demoMode_ = false;
        StartTransition(State::Playing);
    } else if (keyBack) {
        audio_.PlayMenuConfirm();
        StartTransition(State::Title);
    }
}

void Game::DrawDifficultySelect() const {
    DrawRectangle(30, 100, ScreenW - 60, ScreenH - 200, Fade(BLACK, 0.85f));
    DrawRectangleLines(30, 100, ScreenW - 60, ScreenH - 200, Fade(SKYBLUE, 0.8f));
    
    int titleW = MeasureText("SELECT DIFFICULTY", 24);
    DrawText("SELECT DIFFICULTY", ScreenW / 2 - titleW / 2, 130, 24, GOLD);
    
    DrawLine(50, 175, ScreenW - 50, 175, Fade(SKYBLUE, 0.4f));
    
    const char* diffTitles[2] = { "NORMAL MODE", "ACE MODE (HARD)" };
    const char* diffDescs[2] = { 
        "Standard difficulty loop. Highly recommended for newcomers.", 
        "Hyper-aggressive enemies and faster bullets. 1.5x score multiplier!" 
    };
    const char* pilotClasses[2] = { "CLASS: CADET PATROLLER", "CLASS: ACE INTERCEPTOR" };
    Color diffColors[2] = { LIME, RED };

    auto drawStatBar = [](int x, int y, const char* label, int filled, Color col) {
        DrawText(label, x, y, 9, GRAY);
        for (int b = 0; b < 5; ++b) {
            Color barCol = (b < filled) ? col : DARKGRAY;
            DrawRectangle(x + 28 + b * 9, y + 1, 7, 7, barCol);
        }
    };

    auto drawSelectShip = [](float x, float y, Color color, bool selected) {
        float time = (float)GetTime();
        if (selected) {
            float flameH = 6.0f + std::sin(time * 35.0f) * 2.5f;
            DrawTriangle({x - 3, y + 6}, {x - 4, y + 6 + flameH}, {x - 2, y + 6}, ORANGE);
            DrawTriangle({x + 3, y + 6}, {x + 2, y + 6}, {x + 4, y + 6 + flameH}, ORANGE);
        }
        DrawTriangle({x, y - 5}, {x - 10, y + 3}, {x - 2, y + 3}, DARKGRAY);
        DrawTriangle({x, y - 5}, {x + 2, y + 3}, {x + 10, y + 3}, DARKGRAY);
        DrawTriangle({x, y - 10}, {x - 5, y + 6}, {x + 5, y + 6}, color);
        DrawCircleV({x, y - 1}, 2.0f, SKYBLUE);
    };
    
    for (int i = 0; i < 2; ++i) {
        int y = 220 + i * 130;
        bool selected = (difficultySelection_ == i);
        
        if (selected) {
            DrawRectangle(50, y - 10, ScreenW - 100, 110, Fade(BLUE, 0.35f));
            DrawRectangleLines(50, y - 10, ScreenW - 100, 110, Fade(GOLD, 0.7f));
        } else {
            DrawRectangle(50, y - 10, ScreenW - 100, 110, Fade(DARKGRAY, 0.12f));
            DrawRectangleLines(50, y - 10, ScreenW - 100, 110, Fade(GRAY, 0.25f));
        }
        
        // Draw Fighter ship preview on the left inside the card
        drawSelectShip(85.0f, (float)y + 35.0f, diffColors[i], selected);

        // Text labels
        DrawText(diffTitles[i], 125, y, 16, selected ? GOLD : diffColors[i]);
        DrawText(diffDescs[i], 125, y + 22, 10, RAYWHITE);
        DrawText(pilotClasses[i], 125, y + 38, 10, selected ? GOLD : GRAY);

        // Stat meters
        drawStatBar(125, y + 54, "SPD", i == 0 ? 3 : 5, SKYBLUE);
        drawStatBar(220, y + 54, "ARM", i == 0 ? 5 : 2, LIME);
        drawStatBar(315, y + 54, "THT", i == 0 ? 2 : 5, RED);
    }
    
    DrawText("UP/DOWN navigate  |  ENTER/CLICK select", ScreenW / 2 - MeasureText("UP/DOWN navigate  |  ENTER/CLICK select", 12) / 2, 480, 12, GRAY);
}

void Game::UpdateContinue(float dt) {
    float oldTimer = continueTimer_;
    continueTimer_ -= dt;
    if (std::ceil(oldTimer) != std::ceil(continueTimer_)) {
        if (continueTimer_ > 0.0f) audio_.PlayContinueTick();
    }
    
    bool startPressed = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    if (IsGamepadAvailable(0)) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
            startPressed = true;
        }
    }
    
    if (startPressed) {
        if (credits_ > 0) {
            credits_--;
            audio_.PlayPressStart();
            
            player_.lives = (difficulty_ == 0) ? 3 : 2;
            player_.bombs = 3;
            player_.invulnerable = true;
            player_.invulnTimer = 3.0f;
            player_.pos = {240, 560};
            audio_.PlayRespawn();
            
            enemyBullets_.clear();
            
            // Go straight back to playing
            state_ = State::Playing;
            return;
        } else {
            audio_.PlayDenied();
            effects_.Shake(4.0f, 0.15f);
            effects_.AddText({ 240, 380 }, "INSERT COIN!", RED);
        }
    }
    
    if (continueTimer_ <= 0.0f) {
        if (CheckHighScore(player_.score)) {
            nameEntryBuf_[0] = 'A'; nameEntryBuf_[1] = 'A'; nameEntryBuf_[2] = 'A'; nameEntryBuf_[3] = '\0';
            nameEntryIndex_ = 0;
            audio_.PlayHighScoreEntry();
            StartTransition(State::NameEntry);
        } else {
            audio_.PlayGameOver();
            StartTransition(State::GameOver);
        }
    }
}

void Game::DrawContinue() const {
    DrawRectangle(0, 0, ScreenW, ScreenH, Fade(BLACK, 0.75f));
    
    int cy = ScreenH / 2 - 130;
    
    int textW = MeasureText("CONTINUE?", 32);
    Color pulseCol = (int)(GetTime() * 4.0f) % 2 == 0 ? GOLD : RAYWHITE;
    DrawText("CONTINUE?", ScreenW / 2 - textW / 2, cy, 32, pulseCol);
    
    int sec = (int)std::ceil(continueTimer_);
    sec = std::max(0, std::min(9, sec));
    const char* timerStr = TextFormat("%d", sec);
    int timerW = MeasureText(timerStr, 80);
    DrawText(timerStr, ScreenW / 2 - timerW / 2, cy + 45, 80, RED);
    
    int scW = MeasureText(TextFormat("CURRENT SCORE: %08d", player_.score), 15);
    DrawText(TextFormat("CURRENT SCORE: %08d", player_.score), ScreenW / 2 - scW / 2, cy + 145, 15, SKYBLUE);
    
    float blink = std::sin((float)GetTime() * 10.0f);
    if (credits_ > 0) {
        if (blink > 0.0f) {
            int prW = MeasureText("PRESS START TO CONTINUE", 16);
            DrawText("PRESS START TO CONTINUE", ScreenW / 2 - prW / 2, cy + 185, 16, LIME);
        }
        int crW = MeasureText(TextFormat("CREDITS: %02d (1 USED)", credits_), 14);
        DrawText(TextFormat("CREDITS: %02d (1 USED)", credits_), ScreenW / 2 - crW / 2, cy + 212, 14, SKYBLUE);
    } else {
        if (blink > 0.0f) {
            int prW = MeasureText("INSERT COIN [C]", 16);
            DrawText("INSERT COIN [C]", ScreenW / 2 - prW / 2, cy + 185, 16, RED);
        }
        int crW = MeasureText("NO CREDITS AVAILABLE", 13);
        DrawText("NO CREDITS AVAILABLE", ScreenW / 2 - crW / 2, cy + 212, 13, GRAY);
    }

    // Glowing Coin Door graphic on Continue screen
    int doorX = ScreenW / 2 - 40;
    int doorY = cy + 242;
    DrawRectangle(doorX, doorY, 80, 50, BLACK);
    DrawRectangleLines(doorX, doorY, 80, 50, GRAY);
    DrawRectangle(doorX + 32, doorY + 10, 16, 22, DARKGRAY);
    DrawRectangle(doorX + 38, doorY + 12, 4, 18, (credits_ == 0 && (int)(GetTime() * 3.0f) % 2 == 0) ? RED : ORANGE);
    DrawText("COIN", doorX + 26, doorY + 36, 9, GRAY);
}

void Game::DrawBackground() const {
    // 0. Stage Color Scripting Palette Selection
    Color bgColor = {6, 8, 20, 255};
    Color nebulaColor1 = Fade(PURPLE, 0.18f);
    Color nebulaColor2 = Fade(BLUE, 0.12f);
    Color nebulaColor3 = Fade(VIOLET, 0.10f);

    float routeTime = std::min(stageTime_, stageDirector_.RouteDuration());
    int beat = (int)routeTime;
    if (bossWarningTimer_ > 0.0f) {
        // Red Alert Alert/Warning State
        float alertPulse = 0.5f + 0.5f * std::sin((float)GetTime() * 6.0f);
        bgColor = Color{ (unsigned char)(20 + 8 * alertPulse), 5, 8, 255 };
        nebulaColor1 = Fade(RED, 0.22f);
        nebulaColor2 = Fade(MAROON, 0.15f);
        nebulaColor3 = Fade(ORANGE, 0.10f);
    } else if (routeTime < 32.0f) {
        // Launch/sweep: cool orbit lanes with clear silhouettes.
        bgColor = Color{4, 11, 18, 255};
        nebulaColor1 = Color{0, 145, 118, 30};
        nebulaColor2 = Color{0, 70, 105, 22};
        nebulaColor3 = Color{15, 120, 150, 18};
    } else if (routeTime < 48.0f) {
        // Intercept: colder blue pressure before the route clamps down.
        bgColor = Color{5, 7, 22, 255};
        nebulaColor1 = Color{40, 90, 180, 24};
        nebulaColor2 = Color{0, 120, 135, 18};
        nebulaColor3 = Color{70, 40, 160, 15};
    } else if (routeTime < 59.0f) {
        // Encirclement: warmer magenta pressure while vectors close in.
        bgColor = Color{11, 6, 21, 255};
        nebulaColor1 = Color{112, 0, 145, 27};
        nebulaColor2 = Color{70, 0, 105, 18};
        nebulaColor3 = Color{145, 10, 110, 17};
    } else if (routeTime < 70.0f) {
        // Recovery: desaturated calm before the route climbs again.
        bgColor = Color{5, 10, 16, 255};
        nebulaColor1 = Color{0, 120, 150, 18};
        nebulaColor2 = Color{60, 70, 110, 14};
        nebulaColor3 = Color{20, 150, 120, 12};
    } else if (routeTime < 96.0f) {
        // Escalation: industrial amber searchlight space.
        bgColor = Color{16, 7, 8, 255};
        nebulaColor1 = Color{145, 24, 0, 28};
        nebulaColor2 = Color{100, 15, 0, 18};
        nebulaColor3 = Color{170, 70, 0, 15};
    } else {
        // Gate warning: colder, emptier, with the lock shape coming forward.
        bgColor = Color{8, 6, 13, 255};
        nebulaColor1 = Color{90, 210, 255, 22};
        nebulaColor2 = Color{145, 20, 60, 16};
        nebulaColor3 = Color{255, 120, 30, 12};
    }

    ClearBackground(bgColor);
    DrawRectangleGradientV(0, 0, ScreenW, ScreenH, Fade(Color{80, 120, 160, 255}, 0.035f), Fade(Color{120, 40, 60, 255}, 0.045f));
    
    // 1. Layer 1: Distant Nebulae (rendered via soft circular gradients)
    float nY1 = nebulaScroll_ - ScreenH / 2.0f;
    float nY2 = nebulaScroll_ + ScreenH / 2.0f;
    DrawCircleGradient(120, (int)nY1, 260, nebulaColor1, Fade(BLACK, 0.0f));
    DrawCircleGradient(360, (int)nY2, 340, nebulaColor2, Fade(BLACK, 0.0f));
    DrawCircleGradient(200, (int)(nY2 - ScreenH), 290, nebulaColor3, Fade(BLACK, 0.0f));

    // 1a. Faint orbital gate silhouette: the stage's recurring denial icon.
    if (routeTime > 96.5f || bossWarningTimer_ > 0.0f || BossAlive() || bossDeathTimer_ > 0.0f || bossClearDelayTimer_ > 0.0f) {
        float gateAlpha = BossAlive() ? 0.18f : (routeTime > 96.5f ? 0.07f + std::min(0.06f, (routeTime - 96.5f) * 0.008f) : 0.11f);
        float gatePulse = 0.5f + 0.5f * std::sin((float)GetTime() * 1.8f);
        Vector2 gateCenter = {240.0f, 128.0f};
        BeginBlendMode(BLEND_ADDITIVE);
        DrawRing(gateCenter, 124.0f, 128.0f, 0.0f, 360.0f, 64, Fade(Color{90, 210, 255, 255}, gateAlpha));
        DrawRing(gateCenter, 92.0f, 94.0f, 0.0f, 360.0f, 64, Fade(Color{255, 86, 120, 255}, gateAlpha * 0.42f));
        for (int i = 0; i < 8; ++i) {
            float a = ((float)i / 8.0f) * PI * 2.0f + (float)GetTime() * 0.14f;
            Vector2 p1 = {gateCenter.x + std::cos(a) * 95.0f, gateCenter.y + std::sin(a) * 95.0f};
            Vector2 p2 = {gateCenter.x + std::cos(a) * 126.0f, gateCenter.y + std::sin(a) * 126.0f};
            DrawLineEx(p1, p2, 2.0f, Fade(Color{90, 210, 255, 255}, gateAlpha * (0.55f + gatePulse * 0.45f)));
        }
        EndBlendMode();
    }

    // 1b. Layer 1b: Sweeping Background Industrial Searchlights (Stage Phase 3 / Boss Alert)
    if (beat >= 82 || bossWarningTimer_ > 0.0f || BossAlive()) {
        float time = (float)GetTime();
        float angle1 = std::sin(time * 0.45f) * 35.0f - 90.0f;
        float angle2 = std::cos(time * 0.35f) * 25.0f - 90.0f;
        
        BeginBlendMode(BLEND_ADDITIVE);
        Vector2 sl1 = { 40.0f, (float)ScreenH };
        DrawTriangle(
            sl1,
            { sl1.x + std::cos((angle1 - 7.0f) * DEG2RAD) * 580.0f, sl1.y + std::sin((angle1 - 7.0f) * DEG2RAD) * 580.0f },
            { sl1.x + std::cos((angle1 + 7.0f) * DEG2RAD) * 580.0f, sl1.y + std::sin((angle1 + 7.0f) * DEG2RAD) * 580.0f },
            Fade(bgColor.r > bgColor.b ? RED : GOLD, 0.05f)
        );
        Vector2 sl2 = { (float)ScreenW - 40.0f, (float)ScreenH };
        DrawTriangle(
            sl2,
            { sl2.x + std::cos((angle2 - 6.0f) * DEG2RAD) * 580.0f, sl2.y + std::sin((angle2 - 6.0f) * DEG2RAD) * 580.0f },
            { sl2.x + std::cos((angle2 + 6.0f) * DEG2RAD) * 580.0f, sl2.y + std::sin((angle2 + 6.0f) * DEG2RAD) * 580.0f },
            Fade(bgColor.r > bgColor.b ? ORANGE : SKYBLUE, 0.04f)
        );
        EndBlendMode();
    }
    
    // 2. Layer 2: Twinkling Parallax Stars with Micro Cross Flares
    for (int i = 0; i < NumStars; ++i) {
        float sparkle = 0.35f + 0.45f * std::sin((float)GetTime() * 3.4f + stars_[i].pos.x * 0.1f);
        float size = (stars_[i].speed > 100.0f) ? 1.55f : ((stars_[i].speed > 45.0f) ? 1.05f : 0.75f);
        DrawCircleV(stars_[i].pos, size, Fade(stars_[i].color, sparkle * 0.82f));
        
        // Additive micro flare crosses for foreground stars
        if (stars_[i].speed > 120.0f && sparkle > 0.78f) {
            float flareAlpha = (sparkle - 0.78f) * 2.1f;
            DrawLineV({stars_[i].pos.x - 2, stars_[i].pos.y}, {stars_[i].pos.x + 2, stars_[i].pos.y}, Fade(WHITE, flareAlpha));
            DrawLineV({stars_[i].pos.x, stars_[i].pos.y - 2}, {stars_[i].pos.x, stars_[i].pos.y + 2}, Fade(WHITE, flareAlpha));
        }
    }

    // 2b. Layer 2b: Distant Atmospheric Battles (faint lasers and micro-sparks deep in the background)
    BeginBlendMode(BLEND_ADDITIVE);
    for (const auto& l : bgLasers_) {
        float alpha = l.life / l.maxLife;
        DrawLineEx(l.start, l.end, 1.5f, Fade(l.color, alpha * 0.18f));
    }
    for (const auto& s : bgSparks_) {
        float alpha = s.life / s.maxLife;
        DrawCircleV(s.pos, 1.0f, Fade(s.color, alpha * 0.28f));
    }
    EndBlendMode();
    
    // 3. Layer 3: Scrolling asteroid debris chunks (Mid-ground)
    for (const auto& ast : backgroundAsteroids_) {
        SpriteManager::Instance().Draw(SpriteId::AsteroidChunk, ast.pos, ast.rotation, ast.scale, Fade(GRAY, 0.32f));
    }

    // 3aa. Layer 3aa: Scrolling playfield base girder lines (very low-contrast mechanical base plates)
    float gridStartY = spaceStationScrollY_ - 64.0f;
    for (float y = gridStartY; y < ScreenH + 64.0f; y += 64.0f) {
        DrawLine(32, (int)y, ScreenW - 32, (int)y, Color{ 14, 16, 26, 255 });
        DrawLineEx({32.0f, y}, {64.0f, y + 32.0f}, 1.5f, Color{ 14, 16, 26, 255 });
        DrawLineEx({ScreenW - 32.0f, y}, {ScreenW - 64.0f, y + 32.0f}, 1.5f, Color{ 14, 16, 26, 255 });
    }

    // 3b. Layer 3b: Space-station terrain superstructures (Left and Right margins)
    float startY = spaceStationScrollY_ - 32.0f;
    for (float y = startY; y < ScreenH + 32.0f; y += 32.0f) {
        int rowIdx = (int)((y - startY) / 32.0f);
        
        // Left side tile
        SpriteId leftSprite = (rowIdx % 4 == 1) ? SpriteId::SpaceStationLeftCore : SpriteId::SpaceStationLeft;
        Vector2 leftPos = { 16.0f, y };
        SpriteManager::Instance().Draw(leftSprite, leftPos, 0.0f, 1.0f, Color{ 200, 200, 220, 255 });
        
        // Flashing beacon / core for left side core
        if (leftSprite == SpriteId::SpaceStationLeftCore) {
            float pulse = 0.5f + 0.5f * sinf(GetTime() * 10.0f);
            float pulseCyan = 0.5f + 0.5f * sinf(GetTime() * 8.0f);
            
            BeginBlendMode(BLEND_ADDITIVE);
            // Red warning beacon at (x: 10, y: 2) from left edge (left edge is leftPos.x - 16 = 0)
            DrawCircleV({ leftPos.x - 16.0f + 10.0f, leftPos.y - 16.0f + 2.0f }, 3.0f + 2.0f * pulse, Fade(RED, 0.4f + 0.4f * pulse));
            // Cyan cores at (x: 15, y: 8) and (x: 15, y: 22) from left edge
            DrawCircleV({ leftPos.x - 16.0f + 15.0f, leftPos.y - 16.0f + 8.0f }, 4.0f + 2.0f * pulseCyan, Fade(SKYBLUE, 0.3f + 0.2f * pulseCyan));
            DrawCircleV({ leftPos.x - 16.0f + 15.0f, leftPos.y - 16.0f + 22.0f }, 4.0f + 2.0f * pulseCyan, Fade(SKYBLUE, 0.3f + 0.2f * pulseCyan));
            EndBlendMode();
        }
        
        // Right side tile
        SpriteId rightSprite = (rowIdx % 4 == 3) ? SpriteId::SpaceStationRightCore : SpriteId::SpaceStationRight;
        Vector2 rightPos = { (float)(ScreenW - 16), y };
        SpriteManager::Instance().Draw(rightSprite, rightPos, 0.0f, 1.0f, Color{ 200, 200, 220, 255 });
        
        // Flashing core for right side core
        if (rightSprite == SpriteId::SpaceStationRightCore) {
            float pulseCyan = 0.5f + 0.5f * sinf(GetTime() * 8.0f);
            
            BeginBlendMode(BLEND_ADDITIVE);
            // Cyan cores at (x: 15, y: 8) and (x: 15, y: 22) from right edge of sprite (sprite left edge is rightPos.x - 16)
            DrawCircleV({ rightPos.x - 16.0f + 15.0f, rightPos.y - 16.0f + 8.0f }, 4.0f + 2.0f * pulseCyan, Fade(SKYBLUE, 0.3f + 0.2f * pulseCyan));
            DrawCircleV({ rightPos.x - 16.0f + 15.0f, rightPos.y - 16.0f + 22.0f }, 4.0f + 2.0f * pulseCyan, Fade(SKYBLUE, 0.3f + 0.2f * pulseCyan));
            EndBlendMode();
        }
    }

    // 4. Layer 4: Foreground scrolling atmospheric clouds
    for (const auto& cld : backgroundClouds_) {
        SpriteManager::Instance().Draw(SpriteId::CloudForeground, cld.pos, 0.0f, cld.scale, Fade(WHITE, 0.075f));
    }

    DrawRectangleLines(8, 8, ScreenW - 16, ScreenH - 16, Fade(SKYBLUE, 0.35f));
}

void Game::DrawCabinetBezel(float rx, float ry, float rw, float rh) const {
    (void)ry;
    (void)rh;
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    
    // 1. Draw Left Bezel panel space
    if (rx > 0.0f) {
        DrawRectangle(0, 0, (int)rx, screenH, Color{24, 25, 30, 255});
        // Draw separation wood/metal seams
        DrawLineEx({rx - 5.0f, 0.0f}, {rx - 5.0f, (float)screenH}, 4.0f, Color{45, 45, 52, 255});
        DrawLineEx({rx - 1.0f, 0.0f}, {rx - 1.0f, (float)screenH}, 1.0f, Color{82, 85, 92, 255});
        
        // Draw instructions stickers if bezel width allows
        if (rx > 105.0f) {
            int stickerX = 15;
            int stickerW = (int)rx - 30;
            DrawRectangle(stickerX, 60, stickerW, 140, Color{15, 15, 18, 255});
            DrawRectangleLines(stickerX, 60, stickerW, 140, Color{250, 195, 15, 255});
            
            DrawText("SKY CIRCUIT", stickerX + 12, 70, 12, GOLD);
            DrawText("ARCADE SYSTEM v1.5", stickerX + 12, 86, 8, GRAY);
            DrawText("COIN: PRESS C", stickerX + 12, 105, 9, RAYWHITE);
            DrawText("START: ENTER", stickerX + 12, 120, 9, RAYWHITE);
            DrawText("SHOOT: Z or SPACE", stickerX + 12, 135, 9, GOLD);
            DrawText("BOMB: X KEY", stickerX + 12, 150, 9, PURPLE);
            DrawText("SLOW: SHIFT", stickerX + 12, 165, 9, SKYBLUE);

            // Draw miniature player ship graphics preview
            SpriteManager::Instance().Draw(SpriteId::PlayerIdle, Vector2{rx / 2.0f, 260.0f}, 0.0f, 2.0f);
        }
    }
    
    // 2. Draw Right Bezel panel space
    if (rx > 0.0f) {
        float rStart = rx + rw;
        float rWidth = screenW - rStart;
        DrawRectangle((int)rStart, 0, (int)rWidth, screenH, Color{24, 25, 30, 255});
        DrawLineEx({rStart + 1.0f, 0.0f}, {rStart + 1.0f, (float)screenH}, 4.0f, Color{45, 45, 52, 255});
        DrawLineEx({rStart + 4.0f, 0.0f}, {rStart + 4.0f, (float)screenH}, 1.0f, Color{82, 85, 92, 255});
        
        if (rWidth > 105.0f) {
            int stickerX = (int)rStart + 15;
            int stickerW = (int)rWidth - 30;
            
            // Warnings panel
            DrawRectangle(stickerX, 60, stickerW, 110, Color{15, 15, 18, 255});
            DrawRectangleLines(stickerX, 60, stickerW, 110, RED);
            DrawText("WARNING!", stickerX + 12, 70, 11, RED);
            DrawText("DO NOT COIN IN", stickerX + 12, 90, 8, RAYWHITE);
            DrawText("WHILE ATTRACT MODE", stickerX + 12, 105, 8, RAYWHITE);
            DrawText("IS DEMONSTRATING", stickerX + 12, 120, 8, RAYWHITE);
            DrawText("FLIGHT SYSTEMS", stickerX + 12, 135, 8, RAYWHITE);

            // High scores cabinet teaser
            DrawText("TOP PILOTS", stickerX + 12, 210, 11, GOLD);
            for (size_t i = 0; i < std::min((size_t)5, highScores_.size()); ++i) {
                int yPos = 230 + (int)i * 20;
                DrawText(TextFormat("%06d  %s", highScores_[i].score, highScores_[i].initials), stickerX + 12, yPos, 9, SKYBLUE);
            }
        }
    }
}

void Game::DrawHud() const {

    DrawRectangle(0, 0, ScreenW, 36, Fade(BLACK, 0.85f));
    DrawRectangleLines(0, 0, ScreenW, 36, Fade(SKYBLUE, 0.4f));
    
    // SCORE
    DrawText("1P", 12, 4, 10, LIME);
    DrawText(TextFormat("%08d", player_.score), 12, 16, 13, RAYWHITE);
    
    // HI-SCORE
    int hiScore = 25000;
    if (!highScores_.empty()) {
        hiScore = highScores_[0].score;
    }
    DrawText("HI-SCORE", ScreenW / 2 - MeasureText("HI-SCORE", 10) / 2, 4, 10, RED);
    DrawText(TextFormat("%08d", std::max(hiScore, player_.score)), ScreenW / 2 - MeasureText(TextFormat("%08d", std::max(hiScore, player_.score)), 13) / 2, 16, 13, GOLD);
    
    // WEAPON
    DrawText("WEAPON", 280, 4, 10, SKYBLUE);
    DrawText(TextFormat("%s LV%d", player_.WeaponName(), player_.weaponLevel), 280, 16, 13, RAYWHITE);
    
    // LIVES (Icons)
    int maxDrawLives = std::min(5, player_.lives);
    for (int i = 0; i < maxDrawLives; ++i) {
        float lx = 153.0f + i * 16.0f;
        float ly = 22.0f;
        SpriteManager::Instance().Draw(SpriteId::PlayerIdle, {lx, ly}, 0.0f, 0.65f);
    }
    DrawText("LIVES", 145, 4, 10, SKYBLUE);
    
    // BOMBS (Icons)
    int maxDrawBombs = std::min(5, player_.bombs);
    for (int i = 0; i < maxDrawBombs; ++i) {
        float bx = 221.0f + i * 12.0f;
        float by = 22.0f;
        SpriteManager::Instance().Draw(SpriteId::MiniBombCapsule, {bx, by}, 0.0f, 1.0f);
    }
    DrawText("BOMBS", 215, 4, 10, PURPLE);

    // Flashing Combo Decay Bar
    if (medalChain_ > 0) {
        float ratio = medalChainTimer_ / 1.35f;
        ratio = std::clamp(ratio, 0.0f, 1.0f);
        
        bool flash = (medalChainTimer_ > 0.4f) || ((int)(GetTime() * 12.0f) % 2 == 0);
        Color chainColor = flash ? GOLD : RED;
        
        int comboY = 44;
        DrawRectangle(12, comboY, 80, 16, Fade(BLACK, 0.7f));
        DrawRectangleLines(12, comboY, 80, 16, Fade(GOLD, 0.4f));
        
        DrawText(TextFormat("CHAIN x%d", medalChain_), 16, comboY + 3, 10, flash ? YELLOW : ORANGE);
        DrawRectangle(16, comboY + 12, 72, 2, Fade(GRAY, 0.5f));
        DrawRectangle(16, comboY + 12, (int)(72 * ratio), 2, chainColor);
    }

    // Dynamic stage act / WARNING
    float routeDuration = stageDirector_.RouteDuration();
    if (stageTime_ < routeDuration) {
        int remaining = std::max(0, (int)(routeDuration - stageTime_));
        DrawText(TextFormat("B%02d %s", stageDirector_.CurrentBlockIndex(stageTime_), stageDirector_.CurrentEncounterName(stageTime_)), 334, 4, 10, GRAY);
        DrawText(TextFormat("%s %03d  L%d", stageDirector_.CurrentTransitionName(stageTime_), remaining, loop_), 334, 16, 10, SKYBLUE);
    } else {
        DrawText("WARNING", 395, 4, 10, RED);
        DrawText("BOSS ALIVE", 395, 16, 12, RED);
    }
    
    if (state_ == State::Playing) {
        DrawRectangle(0, ScreenH - 24, ScreenW, 24, Fade(BLACK, 0.75f));
        DrawText(TextFormat("CREDIT %02d", credits_), 12, ScreenH - 18, 12, SKYBLUE);
        if (difficulty_ == 1) {
            DrawText("ACE DIFFICULTY (1.5x)", ScreenW - MeasureText("ACE DIFFICULTY (1.5x)", 12) - 12, ScreenH - 18, 12, RED);
        } else {
            DrawText("NORMAL DIFFICULTY", ScreenW - MeasureText("NORMAL DIFFICULTY", 12) - 12, ScreenH - 18, 12, LIME);
        }
    }

    if (demoMode_) {
        Color demoColor = (int)(GetTime() * 2) % 2 == 0 ? Fade(RED, 0.8f) : Fade(YELLOW, 0.8f);
        int demoW = MeasureText("DEMO PLAY - PRESS ANY KEY", 16);
        DrawRectangle(0, ScreenH - 46, ScreenW, 46, Fade(BLACK, 0.85f));
        DrawText("DEMO PLAY - PRESS ANY KEY", ScreenW / 2 - demoW / 2, ScreenH - 30, 16, demoColor);
    }
}

void Game::DrawStageDiagnostics() const {
    int enemies = ActiveEnemyCount();
    int bullets = ActiveEnemyBulletCount();
    bool recovery = stageDirector_.IsRecoveryWindow(stageTime_);
    bool runway = stageDirector_.IsBossRunway(stageTime_);
    bool climax = std::string(stageDirector_.CurrentTransitionName(stageTime_)) == "CLIMAX" || BossAlive();
    bool pressureWarn = bullets > DiagBulletWarnThreshold ||
                        enemies > DiagEnemyWarnThreshold ||
                        (bullets >= DiagCombinedBulletThreshold && enemies >= DiagCombinedEnemyThreshold) ||
                        ((recovery || runway) && bullets > DiagRecoveryBulletThreshold);

    int x = 12;
    int y = 72;
    int w = 286;
    int h = 126;
    DrawRectangle(x, y, w, h, Fade(BLACK, 0.82f));
    DrawRectangleLines(x, y, w, h, pressureWarn ? RED : Fade(SKYBLUE, 0.8f));
    DrawText("STAGE DIRECTOR DIAG", x + 8, y + 7, 10, pressureWarn ? RED : SKYBLUE);
    DrawText(TextFormat("Block %02d  %.1fs  %s",
                        stageDirector_.CurrentBlockIndex(stageTime_),
                        stageDirector_.CurrentBlockElapsed(stageTime_),
                        stageDirector_.CurrentBlockName(stageTime_)),
             x + 8, y + 22, 10, RAYWHITE);
    DrawText(TextFormat("Section %-9s Encounter %s",
                        stageDirector_.CurrentSectionName(stageTime_),
                        stageDirector_.CurrentEncounterName(stageTime_)),
             x + 8, y + 36, 10, GRAY);
    DrawText(TextFormat("Transition %-10s Intensity %.2f",
                        stageDirector_.CurrentTransitionName(stageTime_),
                        stageDirector_.CurrentIntensity(stageTime_)),
             x + 8, y + 50, 10, GRAY);
    DrawText(TextFormat("Enemies %02d/%02d  Bullets %02d/%02d",
                        enemies, DiagEnemyWarnThreshold, bullets, DiagBulletWarnThreshold),
             x + 8, y + 64, 10, pressureWarn ? ORANGE : RAYWHITE);
    DrawText(TextFormat("Recovery %s  Runway %s  Climax %s",
                        recovery ? "YES" : "no",
                        runway ? "YES" : "no",
                        climax ? "YES" : "no"),
             x + 8, y + 78, 10, (recovery || runway) ? LIME : GRAY);
    DrawText(TextFormat("Last death: %s", diagLastDeathBlock_.c_str()), x + 8, y + 92, 10, GOLD);
    DrawText(TextFormat("Cause: %s", diagLastDeathCause_.c_str()), x + 8, y + 106, 10, GOLD);
}

void Game::DrawCenteredText(const char* title, const char* subtitle) const {
    int tw = MeasureText(title, 34);
    DrawText(title, ScreenW / 2 - tw / 2, 238, 34, RAYWHITE);
    int sw = MeasureText(subtitle, 16);
    DrawText(subtitle, ScreenW / 2 - sw / 2, 290, 16, SKYBLUE);
}

void Game::DrawTitleMenu() const {
    DrawRectangle(42, 120, ScreenW - 84, 305, Fade(BLACK, 0.85f));
    DrawRectangleLines(42, 120, ScreenW - 84, 305, Fade(SKYBLUE, 0.8f));
    
    int titleW = MeasureText("SKY CIRCUIT", 38);
    Color titleCol = (int)(GetTime() * 3.0f) % 2 == 0 ? GOLD : YELLOW;
    DrawText("SKY CIRCUIT", ScreenW / 2 - titleW / 2, 140, 38, titleCol);
    DrawText("REAL ARCADE CABINET EDITION", ScreenW / 2 - MeasureText("REAL ARCADE CABINET EDITION", 11) / 2, 185, 11, SKYBLUE);

    const char* items[5] = {"1 PLAYER START", "HOW TO PLAY", "SETTINGS", "LEADERBOARD", "SHUT DOWN CABINET"};
    for (int i = 0; i < 5; ++i) {
        int y = 220 + i * 32;
        bool selected = titleSelection_ == i;
        int xOffset = selected ? (int)(5.0f + 4.0f * std::sin((float)GetTime() * 14.0f)) : 0;
        
        if (selected) DrawRectangle(120, y - 6, 240, 26, Fade(BLUE, 0.45f));
        DrawText(selected ? ">" : " ", 132 + xOffset, y, 16, selected ? GOLD : GRAY);
        
        Color textCol = RAYWHITE;
        if (i == 0) {
            if (credits_ == 0) {
                textCol = selected ? RED : GRAY;
            } else {
                textCol = selected ? GOLD : LIME;
            }
        } else {
            textCol = selected ? GOLD : RAYWHITE;
        }
        DrawText(items[i], 158 + xOffset, y, 16, textCol);
    }

    float blink = std::sin((float)GetTime() * 10.0f);
    if (blink > 0.0f) {
        if (credits_ == 0) {
            int insW = MeasureText("INSERT COIN TO PLAY", 16);
            DrawText("INSERT COIN TO PLAY", ScreenW / 2 - insW / 2, 382, 16, RED);
        } else {
            int stW = MeasureText("PRESS START (1P ACTIVE)", 16);
            DrawText("PRESS START (1P ACTIVE)", ScreenW / 2 - stW / 2, 382, 16, LIME);
        }
    }

    // Glowing Coin Door Slot Graphic
    int slotX = ScreenW / 2 - 60;
    int slotY = 440;
    DrawRectangle(slotX, slotY, 120, 40, BLACK);
    DrawRectangleLines(slotX, slotY, 120, 40, (credits_ == 0 && (int)(GetTime() * 2) % 2 == 0) ? RED : DARKGRAY);
    
    // Coin insert entry path
    DrawRectangle(slotX + 52, slotY + 8, 16, 24, DARKGRAY);
    DrawRectangle(slotX + 58, slotY + 10, 4, 20, (credits_ == 0 && (int)(GetTime() * 2) % 2 == 0) ? RED : ORANGE);
    
    DrawText("COIN", slotX + 12, slotY + 15, 9, GRAY);
    DrawText("25c", slotX + 85, slotY + 15, 9, GOLD);

    // Large LED Credits Panel
    DrawRectangle(ScreenW / 2 - 65, 498, 130, 28, Fade(DARKGRAY, 0.4f));
    DrawRectangleLines(ScreenW / 2 - 65, 498, 130, 28, Fade(SKYBLUE, 0.4f));
    
    Color ledColor = credits_ > 0 ? LIME : RED;
    const char* creditsStr = TextFormat("CREDITS: %02d", credits_);
    int credW = MeasureText(creditsStr, 16);
    DrawText(creditsStr, ScreenW / 2 - credW / 2, 504, 16, ledColor);

    // Large readable cabinet guidelines
    const char* instructStr = "PRESS [C] TO INSERT COIN  |  PRESS [ENTER] TO START";
    int instW = MeasureText(instructStr, 11);
    DrawText(instructStr, ScreenW / 2 - instW / 2, 545, 11, GRAY);
}

void Game::DrawHowTo() const {
    DrawRectangle(30, 56, ScreenW - 60, ScreenH - 112, Fade(BLACK, 0.85f));
    DrawRectangleLines(30, 56, ScreenW - 60, ScreenH - 112, Fade(SKYBLUE, 0.8f));
    DrawText("HOW TO PLAY", ScreenW / 2 - MeasureText("HOW TO PLAY", 26) / 2, 78, 26, RAYWHITE);
    
    // Mini ship with hitbox preview (top right)
    Vector2 shipPos = { 395.0f, 122.0f };
    float time = (float)GetTime();
    float flameH = 6.0f + std::sin(time * 45.0f) * 2.0f;
    DrawTriangle({shipPos.x - 3, shipPos.y + 6}, {shipPos.x - 4, shipPos.y + 6 + flameH}, {shipPos.x - 2, shipPos.y + 6}, SKYBLUE);
    DrawTriangle({shipPos.x + 3, shipPos.y + 6}, {shipPos.x + 2, shipPos.y + 6}, {shipPos.x + 4, shipPos.y + 6 + flameH}, SKYBLUE);
    DrawTriangle({shipPos.x, shipPos.y - 5}, {shipPos.x - 8, shipPos.y + 3}, {shipPos.x - 3, shipPos.y + 3}, GRAY);
    DrawTriangle({shipPos.x, shipPos.y - 5}, {shipPos.x + 3, shipPos.y + 3}, {shipPos.x + 8, shipPos.y + 3}, GRAY);
    DrawTriangle({shipPos.x, shipPos.y - 10}, {shipPos.x - 5, shipPos.y + 6}, {shipPos.x + 5, shipPos.y + 6}, RAYWHITE);
    float corePulse = 2.0f + 1.2f * std::sin(time * 15.0f);
    DrawCircleV(shipPos, corePulse, RED);
    DrawCircleLines((int)shipPos.x, (int)shipPos.y, (int)(corePulse + 2.0f), Fade(RED, 0.6f));
    DrawText("HITBOX", shipPos.x - 16, shipPos.y + 12, 8, RED);

    // Objective Section
    DrawText("MISSION OBJECTIVE", 50, 114, 15, GOLD);
    DrawText("Defeat the enemy Carrier boss before the 90s timer runs out.", 50, 134, 12, RAYWHITE);
    DrawText("Only your fighter's red core is vulnerable to bullet impacts.", 50, 150, 12, RAYWHITE);

    // Powerups Grid Section
    DrawText("POWERUP COLLECTIBLES", 50, 178, 15, GOLD);
    
    auto drawPowerupPreview = [](Vector2 pos, Color c, const char* label, float radius) {
        float time = (float)GetTime();
        float pulse = 1.0f + 1.0f * std::sin(time * 12.0f);
        DrawCircleLines((int)pos.x, (int)pos.y, (int)(radius + 2.0f + pulse), Fade(c, 0.45f));
        DrawCircleLines((int)pos.x, (int)pos.y, (int)radius, Fade(WHITE, 0.7f));
        float angle = time * 2.8f;
        float dRadius = radius - 1.0f;
        Vector2 p1 = { pos.x + std::cos(angle) * dRadius, pos.y + std::sin(angle) * dRadius };
        Vector2 p2 = { pos.x + std::cos(angle + 1.57079f) * dRadius, pos.y + std::sin(angle + 1.57079f) * dRadius };
        Vector2 p3 = { pos.x + std::cos(angle + 3.14159f) * dRadius, pos.y + std::sin(angle + 3.14159f) * dRadius };
        Vector2 p4 = { pos.x + std::cos(angle + 4.71239f) * dRadius, pos.y + std::sin(angle + 4.71239f) * dRadius };
        DrawLineEx(p1, p2, 1.0f, c);
        DrawLineEx(p2, p3, 1.0f, c);
        DrawLineEx(p3, p4, 1.0f, c);
        DrawLineEx(p4, p1, 1.0f, c);
        int w = MeasureText(label, 10);
        DrawText(label, (int)(pos.x - w / 2), (int)(pos.y - 5), 10, RAYWHITE);
    };

    drawPowerupPreview({65, 212}, SKYBLUE, "W", 10.0f);
    DrawText("Weapon Swap", 84, 206, 11, SKYBLUE);
    
    drawPowerupPreview({175, 212}, ORANGE, "P", 10.0f);
    DrawText("Upgrade", 194, 206, 11, ORANGE);
    
    drawPowerupPreview({270, 212}, PURPLE, "B", 10.0f);
    DrawText("Add Bomb", 289, 206, 11, PURPLE);
    
    drawPowerupPreview({365, 212}, GOLD, "$", 9.0f);
    DrawText("Medal (Pts)", 384, 206, 11, GOLD);

    // Cabinet Controls Panel Section
    DrawText("CABINET CONTROL PANEL", 50, 238, 15, GOLD);
    
    // Draw Metal Bezel Panel
    DrawRectangle(46, 260, ScreenW - 92, 116, DARKGRAY);
    DrawRectangleLines(46, 260, ScreenW - 92, 116, GRAY);
    DrawRectangle(50, 264, ScreenW - 100, 108, BLACK);
    
    // Draw Joystick
    int joyX = 100;
    int joyY = 322;
    DrawEllipse(joyX, joyY + 12, 18, 6, DARKGRAY);
    DrawEllipse(joyX, joyY + 12, 14, 4, BLACK);
    DrawLineEx({(float)joyX, (float)joyY + 12}, {(float)joyX - 6, (float)joyY - 14}, 4.0f, LIGHTGRAY);
    DrawCircle(joyX - 6, joyY - 14, 9, RED);
    DrawCircleLines(joyX - 6, joyY - 14, 9, MAROON);
    DrawCircle(joyX - 9, joyY - 17, 3, Fade(WHITE, 0.6f)); // specular highlight
    DrawText("JOYSTICK", joyX - MeasureText("JOYSTICK", 9) / 2, joyY + 22, 9, GRAY);
    DrawText("(MOVE)", joyX - MeasureText("(MOVE)", 8) / 2, joyY + 34, 8, GRAY);

    // Draw Buttons
    int btnY = 312;
    // Shoot Button (RED)
    DrawCircle(195, btnY, 13, RED);
    DrawCircleLines(195, btnY, 13, MAROON);
    DrawCircle(195, btnY, 11, Fade(BLACK, 0.2f));
    DrawText("FIRE", 195 - MeasureText("FIRE", 9) / 2, btnY - 4, 9, RAYWHITE);
    DrawText("[Z / Space]", 195 - MeasureText("[Z / Space]", 8) / 2, btnY + 18, 8, GRAY);

    // Bomb Button (BLUE)
    DrawCircle(250, btnY, 13, BLUE);
    DrawCircleLines(250, btnY, 13, DARKBLUE);
    DrawCircle(250, btnY, 11, Fade(BLACK, 0.2f));
    DrawText("BOMB", 250 - MeasureText("BOMB", 9) / 2, btnY - 4, 9, RAYWHITE);
    DrawText("[X]", 250 - MeasureText("[X]", 8) / 2, btnY + 18, 8, GRAY);

    // 1P Start Button (WHITE)
    DrawCircle(305, btnY - 10, 10, RAYWHITE);
    DrawCircleLines(305, btnY - 10, 10, GRAY);
    DrawText("START", 305 - MeasureText("START", 8) / 2, btnY - 14, 8, BLACK);
    DrawText("[ENTER]", 305 - MeasureText("[ENTER]", 8) / 2, btnY + 4, 8, GRAY);

    // Coin Button (ORANGE)
    DrawCircle(355, btnY - 10, 10, ORANGE);
    DrawCircleLines(355, btnY - 10, 10, MAROON);
    DrawText("COIN", 355 - MeasureText("COIN", 8) / 2, btnY - 14, 8, BLACK);
    DrawText("[C]", 355 - MeasureText("[C]", 8) / 2, btnY + 4, 8, GRAY);

    // Cabinet Guidelines & Info
    DrawText("TACTICAL GUIDELINES", 50, 390, 15, GOLD);
    DrawText("- Hold FIRE button to continuous shoot.", 50, 410, 12, RAYWHITE);
    DrawText("- Deploy BOMB to clear bullets and deal screen-wide damage.", 50, 426, 12, RAYWHITE);
    DrawText("- Gamepad equivalents: Left Stick moves | Button Down shoots.", 50, 442, 12, GRAY);
    DrawText("- ACE Mode features scaled up enemy speeds and 1.5x points.", 50, 458, 12, RED);

    // Back to menu action
    Vector2 mousePos = GetMousePosition();
    bool hovered = (mousePos.x >= 140 && mousePos.x <= 340 && mousePos.y >= 520 && mousePos.y <= 556);
    DrawRectangleRounded({ 140, 520, 200, 36 }, 0.25f, 4, hovered ? Fade(BLUE, 0.45f) : Fade(DARKGRAY, 0.3f));
    DrawRectangleRoundedLines({ 140, 520, 200, 36 }, 0.25f, 4, hovered ? GOLD : GRAY);
    int backW = MeasureText("BACK", 15);
    DrawText("BACK", 240 - backW / 2, 531, 15, hovered ? GOLD : RAYWHITE);
}

void Game::DrawHighScores() const {
    DrawRectangle(30, 56, ScreenW - 60, ScreenH - 112, Fade(BLACK, 0.85f));
    DrawRectangleLines(30, 56, ScreenW - 60, ScreenH - 112, Fade(SKYBLUE, 0.8f));
    
    int titleW = MeasureText("HIGH SCORE HALL OF FAME", 22);
    DrawText("HIGH SCORE HALL OF FAME", ScreenW / 2 - titleW / 2, 85, 22, GOLD);
    
    DrawText("RANK  NAME     SCORE    LOOP", 68, 150, 16, SKYBLUE);
    DrawLine(68, 175, ScreenW - 68, 175, Fade(SKYBLUE, 0.5f));
    
    for (size_t i = 0; i < highScores_.size(); ++i) {
        int y = 195 + i * 46;
        Color c = RAYWHITE;
        if (i == 0) c = GOLD;
        else if (i == 1) c = LIGHTGRAY;
        else if (i == 2) c = ORANGE;
        
        DrawText(TextFormat("%d.", i + 1), 74, y, 18, c);
        DrawText(highScores_[i].initials, 135, y, 18, c);
        DrawText(TextFormat("%08d", highScores_[i].score), 215, y, 18, c);
        DrawText(TextFormat("L%02d", highScores_[i].loop), 340, y, 18, c);
    }
    
    Vector2 mousePos = GetMousePosition();
    bool hovered = (mousePos.x >= 140 && mousePos.x <= 340 && mousePos.y >= 520 && mousePos.y <= 556);
    DrawRectangleRounded({ 140, 520, 200, 36 }, 0.25f, 4, hovered ? Fade(BLUE, 0.45f) : Fade(DARKGRAY, 0.3f));
    DrawRectangleRoundedLines({ 140, 520, 200, 36 }, 0.25f, 4, hovered ? GOLD : GRAY);
    int backW = MeasureText("BACK TO TITLE", 15);
    DrawText("BACK TO TITLE", 240 - backW / 2, 531, 15, hovered ? GOLD : RAYWHITE);
}

void Game::DrawSettings() const {
    DrawRectangle(30, 56, ScreenW - 60, ScreenH - 112, Fade(BLACK, 0.85f));
    DrawRectangleLines(30, 56, ScreenW - 60, ScreenH - 112, Fade(SKYBLUE, 0.8f));
    
    int titleW = MeasureText("USER SETTINGS", 24);
    DrawText("USER SETTINGS", ScreenW / 2 - titleW / 2, 85, 24, GOLD);

    const char* options[SettingsCount] = {
        "Master Volume",
        "SFX Volume",
        "Music Volume",
        "Screen Shake",
        "Hitboxes",
        "CRT Filter",
        "Clean Pixel Mode",
        "CRT Curvature",
        "CRT Scanlines",
        "CRT Shadow Mask",
        "CRT Bloom Bleed",
        "CRT Vignette",
        "CRT Screen Glare",
        "Aspect Scaling",
        "Cabinet Bezel",
        "Fullscreen",
        "Control Layout",
        "Reset to Defaults",
        "Clear High Scores",
        "Save and Return"
    };

    int startIdx = 0;
    if (settingsSelection_ >= VisibleSettingsRows) {
        startIdx = settingsSelection_ - (VisibleSettingsRows - 1);
    }

    // Scroll indicators
    if (startIdx > 0) {
        DrawTriangle({ScreenW / 2 - 8, 114}, {ScreenW / 2, 108}, {ScreenW / 2 + 8, 114}, GOLD);
    }
    if (startIdx + VisibleSettingsRows < SettingsCount) {
        DrawTriangle({ScreenW / 2 - 8, 396}, {ScreenW / 2 + 8, 396}, {ScreenW / 2, 402}, GOLD);
    }

    for (int i = startIdx; i < startIdx + VisibleSettingsRows && i < SettingsCount; ++i) {
        int y = 125 + (i - startIdx) * 25;
        bool selected = settingsSelection_ == i;
        
        if (selected) {
            DrawRectangle(50, y - 4, ScreenW - 100, 22, Fade(BLUE, 0.35f));
            DrawText(">", 58, y, 14, GOLD);
        }
        
        DrawText(options[i], 78, y, 14, selected ? GOLD : RAYWHITE);
        
        if (i == 0) {
            DrawLine(280, y + 7, 380, y + 7, GRAY);
            DrawLine(280, y + 7, 280 + masterVolume_ * 10, y + 7, SKYBLUE);
            DrawCircle(280 + masterVolume_ * 10, y + 7, 4, selected ? GOLD : WHITE);
            DrawText(TextFormat("%d", masterVolume_), 395, y, 14, selected ? GOLD : SKYBLUE);
        } else if (i == 1) {
            DrawLine(280, y + 7, 380, y + 7, GRAY);
            DrawLine(280, y + 7, 280 + sfxVolume_ * 10, y + 7, SKYBLUE);
            DrawCircle(280 + sfxVolume_ * 10, y + 7, 4, selected ? GOLD : WHITE);
            DrawText(TextFormat("%d", sfxVolume_), 395, y, 14, selected ? GOLD : SKYBLUE);
        } else if (i == 2) {
            DrawLine(280, y + 7, 380, y + 7, GRAY);
            DrawLine(280, y + 7, 280 + bgmVolume_ * 10, y + 7, SKYBLUE);
            DrawCircle(280 + bgmVolume_ * 10, y + 7, 4, selected ? GOLD : WHITE);
            DrawText(TextFormat("%d", bgmVolume_), 395, y, 14, selected ? GOLD : SKYBLUE);
        } else if (i == 3) {
            DrawText(screenShakeEnabled_ ? "ON" : "OFF", 280, y, 14, screenShakeEnabled_ ? LIME : RED);
        } else if (i == 4) {
            DrawText(hitboxOverlayEnabled_ ? "ON" : "OFF", 280, y, 14, hitboxOverlayEnabled_ ? LIME : RED);
            
            Vector2 shipPos = { 355.0f, (float)y + 6.0f };
            float time = (float)GetTime();
            float flameH = 4.0f + std::sin(time * 45.0f) * 1.5f;
            DrawTriangle({shipPos.x - 2, shipPos.y + 4}, {shipPos.x - 3, shipPos.y + 4 + flameH}, {shipPos.x - 1, shipPos.y + 4}, SKYBLUE);
            DrawTriangle({shipPos.x + 2, shipPos.y + 4}, {shipPos.x + 1, shipPos.y + 4}, {shipPos.x + 3, shipPos.y + 4 + flameH}, SKYBLUE);
            DrawTriangle({shipPos.x, shipPos.y - 4}, {shipPos.x - 6, shipPos.y + 1}, {shipPos.x - 2, shipPos.y + 1}, GRAY);
            DrawTriangle({shipPos.x, shipPos.y - 4}, {shipPos.x + 2, shipPos.y + 1}, {shipPos.x + 6, shipPos.y + 1}, GRAY);
            DrawTriangle({shipPos.x, shipPos.y - 8}, {shipPos.x - 4, shipPos.y + 4}, {shipPos.x + 4, shipPos.y + 4}, RAYWHITE);
            
            if (hitboxOverlayEnabled_ || selected) {
                float corePulse = 1.5f + 1.0f * std::sin(time * 15.0f);
                DrawCircleV(shipPos, corePulse, RED);
            } else {
                DrawCircleV(shipPos, 1.5f, DARKGRAY);
            }
        } else if (i == 5) {
            DrawText(crtShaderEnabled_ ? "ON" : "OFF", 280, y, 14, crtShaderEnabled_ ? LIME : RED);
        } else if (i == 6) {
            DrawText(cleanPixelMode_ ? "ON" : "OFF", 280, y, 14, cleanPixelMode_ ? LIME : RED);
            DrawText(cleanPixelMode_ ? "NO CRT" : "CABINET", 335, y, 14, cleanPixelMode_ ? SKYBLUE : GRAY);
        } else if (i == 7) {
            DrawLine(280, y + 7, 380, y + 7, GRAY);
            DrawLine(280, y + 7, 280 + crtCurvature_ * 10, y + 7, SKYBLUE);
            DrawCircle(280 + crtCurvature_ * 10, y + 7, 4, selected ? GOLD : WHITE);
            DrawText(TextFormat("%d", crtCurvature_), 395, y, 14, selected ? GOLD : SKYBLUE);
        } else if (i == 8) {
            DrawLine(280, y + 7, 380, y + 7, GRAY);
            DrawLine(280, y + 7, 280 + crtScanline_ * 10, y + 7, SKYBLUE);
            DrawCircle(280 + crtScanline_ * 10, y + 7, 4, selected ? GOLD : WHITE);
            DrawText(TextFormat("%d", crtScanline_), 395, y, 14, selected ? GOLD : SKYBLUE);
        } else if (i == 9) {
            DrawLine(280, y + 7, 380, y + 7, GRAY);
            DrawLine(280, y + 7, 280 + crtMask_ * 10, y + 7, SKYBLUE);
            DrawCircle(280 + crtMask_ * 10, y + 7, 4, selected ? GOLD : WHITE);
            DrawText(TextFormat("%d", crtMask_), 395, y, 14, selected ? GOLD : SKYBLUE);
        } else if (i == 10) {
            DrawLine(280, y + 7, 380, y + 7, GRAY);
            DrawLine(280, y + 7, 280 + crtBloom_ * 10, y + 7, SKYBLUE);
            DrawCircle(280 + crtBloom_ * 10, y + 7, 4, selected ? GOLD : WHITE);
            DrawText(TextFormat("%d", crtBloom_), 395, y, 14, selected ? GOLD : SKYBLUE);
        } else if (i == 11) {
            DrawLine(280, y + 7, 380, y + 7, GRAY);
            DrawLine(280, y + 7, 280 + crtVignette_ * 10, y + 7, SKYBLUE);
            DrawCircle(280 + crtVignette_ * 10, y + 7, 4, selected ? GOLD : WHITE);
            DrawText(TextFormat("%d", crtVignette_), 395, y, 14, selected ? GOLD : SKYBLUE);
        } else if (i == 12) {
            DrawLine(280, y + 7, 380, y + 7, GRAY);
            DrawLine(280, y + 7, 280 + crtGlare_ * 10, y + 7, SKYBLUE);
            DrawCircle(280 + crtGlare_ * 10, y + 7, 4, selected ? GOLD : WHITE);
            DrawText(TextFormat("%d", crtGlare_), 395, y, 14, selected ? GOLD : SKYBLUE);
        } else if (i == 13) {
            const char* modeNames[3] = { "FIT SCREEN", "INTEGER 1X", "STRETCH" };
            DrawText(modeNames[aspectMode_], 280, y, 14, SKYBLUE);
        } else if (i == 14) {
            DrawText(drawBezel_ ? "ON" : "OFF", 280, y, 14, drawBezel_ ? LIME : RED);
        } else if (i == 15) {
            DrawText(isFullscreen_ ? "ON" : "OFF", 280, y, 14, isFullscreen_ ? LIME : RED);
        } else if (i == 16) {
            DrawText(controlLayout_ == 0 ? "ARROWS" : "WASD", 280, y, 14, controlLayout_ == 0 ? SKYBLUE : GOLD);
        }
    }
 
    // Visual Control Layout Schematic Panel
    int bx = ScreenW / 2 - 180;
    int by = 462;
    
    DrawRectangle(bx - 10, by - 24, 360, 100, Fade(DARKGRAY, 0.18f));
    DrawRectangleLines(bx - 10, by - 24, 360, 100, Fade(SKYBLUE, 0.38f));
    DrawText("SCHEMATIC CONFIGURATION MAP", bx + 15, by - 16, 11, SKYBLUE);
    
    auto drawKey = [](int x, int y, int w, int h, const char* txt, Color col) {
        DrawRectangle(x, y, w, h, Fade(col, 0.18f));
        DrawRectangleLines(x, y, w, h, col);
        int tw = MeasureText(txt, 10);
        DrawText(txt, x + w / 2 - tw / 2, y + h / 2 - 5, 10, col);
    };
    
    // Draw direction pad
    drawKey(bx + 54, by + 14, 24, 24, controlLayout_ == 0 ? "Up" : "W", RAYWHITE);
    drawKey(bx + 26, by + 40, 24, 24, controlLayout_ == 0 ? "L" : "A", RAYWHITE);
    drawKey(bx + 54, by + 40, 24, 24, controlLayout_ == 0 ? "Dn" : "S", RAYWHITE);
    drawKey(bx + 82, by + 40, 24, 24, controlLayout_ == 0 ? "R" : "D", RAYWHITE);
    DrawText(controlLayout_ == 0 ? "MOVE: Arrows" : "MOVE: WASD", bx + 24, by + 70, 9, GRAY);
    
    // Draw primary action
    DrawText("SHOOT", bx + 148, by + 23, 9, GOLD);
    drawKey(bx + 152, by + 40, 24, 24, controlLayout_ == 0 ? "Z" : "J", GOLD);
    
    // Draw secondary action
    DrawText("BOMB", bx + 182, by + 23, 9, PURPLE);
    drawKey(bx + 184, by + 40, 24, 24, controlLayout_ == 0 ? "X" : "K", PURPLE);
    
    // Draw alternate shoot
    DrawText("SHOOT (ALT)", bx + 230, by + 23, 9, GOLD);
    drawKey(bx + 218, by + 40, 116, 24, "Spacebar", GOLD);
    
    int promptW = MeasureText("UP/DOWN navigate  |  LEFT/RIGHT or DRAG adjust", 12);
    DrawText("UP/DOWN navigate  |  LEFT/RIGHT or DRAG adjust", ScreenW / 2 - promptW / 2, 548, 12, GRAY);
    
    if (settingsFeedbackTimer_ > 0.0f) {
        float alpha = std::min(1.0f, settingsFeedbackTimer_ / 0.4f);
        int tw = MeasureText(settingsFeedbackText_, 14);
        int bxBg = ScreenW / 2 - tw / 2 - 15;
        DrawRectangleRounded({ (float)bxBg, 510.0f, (float)tw + 30, 24.0f }, 0.4f, 4, Fade(BLACK, 0.9f * alpha));
        DrawRectangleRoundedLines({ (float)bxBg, 510.0f, (float)tw + 30, 24.0f }, 0.4f, 4, Fade(settingsFeedbackColor_, 0.6f * alpha));
        DrawText(settingsFeedbackText_, ScreenW / 2 - tw / 2, 515, 14, Fade(settingsFeedbackColor_, alpha));
    }
}

void Game::DrawNameEntry() const {
    DrawRectangle(30, 120, ScreenW - 60, ScreenH - 240, Fade(BLACK, 0.90f));
    DrawRectangleLines(30, 120, ScreenW - 60, ScreenH - 240, Fade(SKYBLUE, 0.85f));
    
    Color titleColor = (int)(GetTime() * 4) % 2 == 0 ? GOLD : YELLOW;
    int titleW = MeasureText("NEW HIGH SCORE!", 26);
    DrawText("NEW HIGH SCORE!", ScreenW / 2 - titleW / 2, 158, 26, titleColor);
    
    int scoreW = MeasureText(TextFormat("SCORE: %08d", player_.score), 18);
    DrawText(TextFormat("SCORE: %08d", player_.score), ScreenW / 2 - scoreW / 2, 206, 18, RAYWHITE);
    
    int subW = MeasureText("ENTER YOUR INITIALS", 15);
    DrawText("ENTER YOUR INITIALS", ScreenW / 2 - subW / 2, 245, 15, SKYBLUE);
    
    for (int i = 0; i < 3; ++i) {
        int x = ScreenW / 2 - 75 + i * 56;
        int y = 300;
        
        bool isCurrent = (nameEntryIndex_ == i);
        
        DrawRectangle(x, y, 40, 50, isCurrent ? Fade(BLUE, 0.5f) : Fade(DARKGRAY, 0.3f));
        DrawRectangleLines(x, y, 40, 50, isCurrent ? GOLD : GRAY);
        
        char charStr[2] = { nameEntryBuf_[i], '\0' };
        int lW = MeasureText(charStr, 30);
        DrawText(charStr, x + 20 - lW / 2, y + 10, 30, isCurrent ? GOLD : RAYWHITE);
        
        if (isCurrent && (int)(GetTime() * 3) % 2 == 0) {
            DrawRectangle(x + 5, y + 42, 30, 4, GOLD);
        }

        // Draw Up and Down Triangles for slot adjustments
        DrawTriangle({(float)x + 10, (float)y - 6}, {(float)x + 20, (float)y - 16}, {(float)x + 30, (float)y - 6}, isCurrent ? GOLD : GRAY);
        DrawTriangle({(float)x + 10, (float)y + 56}, {(float)x + 30, (float)y + 56}, {(float)x + 20, (float)y + 66}, isCurrent ? GOLD : GRAY);
    }
    
    bool confirmSelected = (nameEntryIndex_ == 3);
    DrawRectangleRounded({ 140, 440, 200, 36 }, 0.25f, 4, confirmSelected ? Fade(BLUE, 0.45f) : Fade(DARKGRAY, 0.3f));
    DrawRectangleRoundedLines({ 140, 440, 200, 36 }, 0.25f, 4, confirmSelected ? GOLD : GRAY);
    int confW = MeasureText("CONFIRM INITIALS", 15);
    DrawText("CONFIRM INITIALS", 240 - confW / 2, 451, 15, confirmSelected ? GOLD : RAYWHITE);

    int tip1W = MeasureText("UP/DOWN/Arrows choose char", 14);
    DrawText("UP/DOWN/Arrows choose char", ScreenW / 2 - tip1W / 2, 388, 14, GRAY);
    
    int tip2W = MeasureText("LEFT/RIGHT move cursor | ENTER confirm", 14);
    DrawText("LEFT/RIGHT move cursor | ENTER confirm", ScreenW / 2 - tip2W / 2, 412, 14, GRAY);
}

void Game::DrawExitConfirm() const {
    DrawRectangle(0, 0, ScreenW, ScreenH, Fade(BLACK, 0.75f));
    
    int cx = ScreenW / 2 - 160;
    int cy = ScreenH / 2 - 110;
    
    DrawRectangle(cx, cy, 320, 180, Fade(BLACK, 0.92f));
    DrawRectangleLines(cx, cy, 320, 180, Fade(SKYBLUE, 0.8f));
    
    bool isFromTitle = (returnFromExitConfirm_ == State::Title);
    const char* titleText = isFromTitle ? "EXIT GAME?" : "ABANDON MISSION?";
    const char* subText = isFromTitle ? "Are you sure you want to quit?" : "Return to Title screen?";
    
    int titleW = MeasureText(titleText, 20);
    DrawText(titleText, ScreenW / 2 - titleW / 2, cy + 30, 20, GOLD);
    
    int subW = MeasureText(subText, 14);
    DrawText(subText, ScreenW / 2 - subW / 2, cy + 68, 14, RAYWHITE);
    
    // Draw buttons
    // Button NO
    bool noSel = (exitConfirmSelection_ == 0);
    DrawRectangleRounded({ (float)cx + 20, (float)cy + 110, 120, 36 }, 0.2f, 4, noSel ? Fade(BLUE, 0.5f) : Fade(DARKGRAY, 0.3f));
    DrawRectangleRoundedLines({ (float)cx + 20, (float)cy + 110, 120, 36 }, 0.2f, 4, noSel ? GOLD : GRAY);
    int noTextW = MeasureText("NO, RETURN", 13);
    DrawText("NO, RETURN", cx + 80 - noTextW / 2, cy + 122, 13, noSel ? GOLD : RAYWHITE);
    
    // Button YES
    bool yesSel = (exitConfirmSelection_ == 1);
    DrawRectangleRounded({ (float)cx + 180, (float)cy + 110, 120, 36 }, 0.2f, 4, yesSel ? Fade(RED, 0.4f) : Fade(DARKGRAY, 0.3f));
    DrawRectangleRoundedLines({ (float)cx + 180, (float)cy + 110, 120, 36 }, 0.2f, 4, yesSel ? RED : GRAY);
    
    const char* yesLabel = isFromTitle ? "YES, QUIT" : "YES, ABANDON";
    int yesTextW = MeasureText(yesLabel, 13);
    DrawText(yesLabel, cx + 240 - yesTextW / 2, cy + 122, 13, yesSel ? RED : RAYWHITE);
}

void Game::DrawClearScoresConfirm() const {
    DrawRectangle(0, 0, ScreenW, ScreenH, Fade(BLACK, 0.75f));
    
    int cx = ScreenW / 2 - 160;
    int cy = ScreenH / 2 - 110;
    
    DrawRectangle(cx, cy, 320, 180, Fade(BLACK, 0.92f));
    DrawRectangleLines(cx, cy, 320, 180, Fade(SKYBLUE, 0.8f));
    
    int titleW = MeasureText("CLEAR HALL OF FAME?", 18);
    DrawText("CLEAR HALL OF FAME?", ScreenW / 2 - titleW / 2, cy + 30, 18, RED);
    
    int subW = MeasureText("This will permanently wipe scores!", 13);
    DrawText("This will permanently wipe scores!", ScreenW / 2 - subW / 2, cy + 68, 13, RAYWHITE);
    
    // Draw buttons
    // Button NO
    bool noSel = (clearScoresSelection_ == 0);
    DrawRectangleRounded({ (float)cx + 20, (float)cy + 110, 120, 36 }, 0.2f, 4, noSel ? Fade(BLUE, 0.5f) : Fade(DARKGRAY, 0.3f));
    DrawRectangleRoundedLines({ (float)cx + 20, (float)cy + 110, 120, 36 }, 0.2f, 4, noSel ? GOLD : GRAY);
    int noTextW = MeasureText("NO, RETURN", 13);
    DrawText("NO, RETURN", cx + 80 - noTextW / 2, cy + 122, 13, noSel ? GOLD : RAYWHITE);
    
    // Button YES
    bool yesSel = (clearScoresSelection_ == 1);
    DrawRectangleRounded({ (float)cx + 180, (float)cy + 110, 120, 36 }, 0.2f, 4, yesSel ? Fade(RED, 0.4f) : Fade(DARKGRAY, 0.3f));
    DrawRectangleRoundedLines({ (float)cx + 180, (float)cy + 110, 120, 36 }, 0.2f, 4, yesSel ? RED : GRAY);
    int yesTextW = MeasureText("YES, WIPE", 13);
    DrawText("YES, WIPE", cx + 240 - yesTextW / 2, cy + 122, 13, yesSel ? RED : RAYWHITE);
}

void Game::DrawTransition() const {
    DrawRectangle(0, 0, ScreenW, ScreenH, Fade(BLACK, transitionAlpha_));
}

void Game::Draw() {
    Vector2 shake = screenShakeEnabled_ ? effects_.ShakeOffset() : Vector2{0, 0};
    BeginMode2D(Camera2D{shake, {0, 0}, 0.0f, 1.0f});
    DrawBackground();
    for (const auto& p : powerups_) p.Draw();
    for (const auto& b : playerBullets_) b.Draw(debug_);
    for (const auto& e : enemies_) e.Draw(debug_);
    
    if (bossDeathTimer_ > 0.0f) {
        float age = BossDeathDuration - bossDeathTimer_;
        float wobbleScale = 1.0f + age * 1.8f;
        float wobbleX = std::sin(age * 55.0f) * 2.5f * wobbleScale;
        float wobbleY = std::cos(age * 48.0f) * 1.5f * wobbleScale;
        Vector2 drawPos = { bossDeathPos_.x + wobbleX, bossDeathPos_.y + wobbleY };
        
        if (GetRandomValue(0, 100) < 55) {
            float plumeH = 10.0f + std::sin(bossDeathTimer_ * 60.0f) * 4.0f;
            DrawTriangle({drawPos.x - 24, drawPos.y - 48}, {drawPos.x - 28, drawPos.y - 48 - plumeH}, {drawPos.x - 20, drawPos.y - 48}, ORANGE);
            DrawTriangle({drawPos.x + 24, drawPos.y - 48}, {drawPos.x + 20, drawPos.y - 48}, {drawPos.x + 28, drawPos.y - 48 - plumeH}, ORANGE);
        }
        
        Color tint = WHITE;
        if (bossDeathTimer_ < 0.25f) {
            float whiteFactor = (0.25f - bossDeathTimer_) / 0.25f;
            tint = Fade(WHITE, 1.0f - whiteFactor);
            SpriteManager::Instance().Draw(SpriteId::BossDamaged, drawPos, 180.0f, 1.8f, tint);
            BeginBlendMode(BLEND_ADDITIVE);
            float collapse = 1.0f - bossDeathTimer_ / 0.25f;
            DrawCircleV(drawPos, 32.0f * collapse, Fade(Color{255, 72, 180, 255}, 0.55f));
            DrawCircleLines((int)drawPos.x, (int)drawPos.y, 48.0f * collapse, Fade(WHITE, 0.75f));
            SpriteManager::Instance().Draw(SpriteId::BossDamaged, drawPos, 180.0f, 1.8f, Fade(WHITE, whiteFactor * 0.95f));
            EndBlendMode();
        } else {
            if ((int)(bossDeathTimer_ * 14.0f) % 2 == 0) {
                tint = RED;
            }
            SpriteManager::Instance().Draw(SpriteId::BossDamaged, drawPos, 180.0f, 1.8f, tint);
        }
    }

    player_.Draw(debug_ && state_ != State::Title);
    effects_.Draw();
    EndMode2D();

    if (screenFlashTimer_ > 0.0f) {
        float alpha = (screenFlashTimer_ / 0.10f) * 0.42f;
        if (alpha > 1.0f) alpha = 1.0f;
        DrawRectangle(0, 0, ScreenW, ScreenH, Fade(WHITE, alpha));
    }

    BeginMode2D(Camera2D{shake, {0, 0}, 0.0f, 1.0f});
    for (const auto& b : enemyBullets_) b.Draw(debug_);
    EndMode2D();

    DrawHud();
    if (debug_ && state_ == State::Playing) {
        DrawStageDiagnostics();
    }

    if (state_ == State::Playing && bossWarningTimer_ > 0.0f) {
        if ((int)(bossWarningTimer_ * 8) % 2 == 0) {
            DrawRectangleLines(10, 10, ScreenW - 20, ScreenH - 20, RED);
            DrawRectangle(10, 10, ScreenW - 20, ScreenH - 20, Fade(RED, 0.06f));
        }
        
        // 1. Black backing panel
        DrawRectangle(0, 180, ScreenW, 60, Fade(BLACK, 0.85f));
        
        // 2. Caustics (sweeping light sheen)
        float sheenX = fmodf(GetTime() * 180.0f, (float)(ScreenW + 120)) - 60.0f;
        BeginBlendMode(BLEND_ADDITIVE);
        DrawRectangleGradientH((int)sheenX - 30, 186, 30, 48, Fade(RED, 0.0f), Fade(RED, 0.28f));
        DrawRectangleGradientH((int)sheenX, 186, 30, 48, Fade(RED, 0.28f), Fade(RED, 0.0f));
        EndBlendMode();
        
        // 3. Scrolling Caution Stripes
        int stripeOffset = (int)(GetTime() * 50.0f) % 20;
        for (int sx = -20; sx < ScreenW + 20; sx += 20) {
            // Top caution stripe band (y: 180 to 186)
            Vector2 p1 = { (float)(sx + stripeOffset), 180.0f };
            Vector2 p2 = { (float)(sx + stripeOffset + 10), 180.0f };
            Vector2 p3 = { (float)(sx + stripeOffset + 5), 186.0f };
            Vector2 p4 = { (float)(sx + stripeOffset - 5), 186.0f };
            DrawTriangle(p1, p4, p3, ORANGE);
            DrawTriangle(p1, p3, p2, ORANGE);
            
            // Bottom caution stripe band (y: 234 to 240)
            Vector2 b1 = { (float)(sx - stripeOffset), 234.0f };
            Vector2 b2 = { (float)(sx - stripeOffset + 10), 234.0f };
            Vector2 b3 = { (float)(sx - stripeOffset + 15), 240.0f };
            Vector2 b4 = { (float)(sx - stripeOffset + 5), 240.0f };
            DrawTriangle(b1, b4, b3, ORANGE);
            DrawTriangle(b1, b3, b2, ORANGE);
        }
        
        // Borders
        DrawLine(0, 180, ScreenW, 180, RED);
        DrawLine(0, 186, ScreenW, 186, RED);
        DrawLine(0, 234, ScreenW, 234, RED);
        DrawLine(0, 240, ScreenW, 240, RED);
        
        Color warnColor = (int)(bossWarningTimer_ * 5) % 2 == 0 ? RED : YELLOW;
        int warnW = MeasureText("ORBITAL GATEKEEPER DETECTED: VANTAGE-9", 16);
        DrawText("ORBITAL GATEKEEPER DETECTED: VANTAGE-9", ScreenW / 2 - warnW / 2, 202, 16, warnColor);
    }

    if (state_ == State::Playing) {
        missionBriefing_.Draw();
        commsQueue_.Draw(portrait_);
        bossAttackName_.Draw();
        bossDialogue_.Draw(dialogueBox_, portrait_);
    }

    if (state_ == State::Playing && tutorialTimer_ > 0.0f) {
        float alpha = std::min(1.0f, tutorialTimer_);
        DrawRectangle(30, ScreenH - 180, ScreenW - 60, 90, Fade(BLACK, alpha * 0.85f));
        DrawRectangleLines(30, ScreenH - 180, ScreenW - 60, 90, Fade(SKYBLUE, alpha * 0.75f));
        
        int textY = ScreenH - 165;
        if (controlLayout_ == 0) {
            int w1 = MeasureText("KEYBOARD CONTROLS (CLASSIC)", 15);
            DrawText("KEYBOARD CONTROLS (CLASSIC)", ScreenW / 2 - w1 / 2, textY, 15, Fade(GOLD, alpha));
            int w2 = MeasureText("MOVE: Arrow Keys  |  SHOOT: Z or SPACE  |  BOMB: X", 13);
            DrawText("MOVE: Arrow Keys  |  SHOOT: Z or SPACE  |  BOMB: X", ScreenW / 2 - w2 / 2, textY + 24, 13, Fade(WHITE, alpha));
        } else {
            int w1 = MeasureText("KEYBOARD CONTROLS (LEFT-HAND / WASD)", 15);
            DrawText("KEYBOARD CONTROLS (LEFT-HAND / WASD)", ScreenW / 2 - w1 / 2, textY, 15, Fade(GOLD, alpha));
            int w2 = MeasureText("MOVE: W / A / S / D  |  SHOOT: J or SPACE  |  BOMB: K", 13);
            DrawText("MOVE: W / A / S / D  |  SHOOT: J or SPACE  |  BOMB: K", ScreenW / 2 - w2 / 2, textY + 24, 13, Fade(WHITE, alpha));
        }
        int w3 = MeasureText("Gamepad: D-pad / Stick  |  Shoot: A/X  |  Bomb: B", 12);
        DrawText("Gamepad: D-pad / Stick  |  Shoot: A/X  |  Bomb: B", ScreenW / 2 - w3 / 2, textY + 44, 12, Fade(GRAY, alpha));
    }

    if (state_ == State::Title) {
        DrawTitleMenu();
    } else if (state_ == State::DifficultySelect) {
        DrawDifficultySelect();
    } else if (state_ == State::Continue) {
        DrawContinue();
    } else if (state_ == State::HowTo) {
        DrawHowTo();
    } else if (state_ == State::HighScores) {
        DrawHighScores();
    } else if (state_ == State::Settings) {
        DrawSettings();
    } else if (state_ == State::NameEntry) {
        DrawNameEntry();
    } else if (state_ == State::ExitConfirm) {
        DrawExitConfirm();
    } else if (state_ == State::ClearScoresConfirm) {
        DrawClearScoresConfirm();
    } else if (state_ == State::Paused) {
        DrawRectangle(0, 0, ScreenW, ScreenH, Fade(BLACK, 0.65f));
        DrawRectangle(42, 190, ScreenW - 84, 270, Fade(BLACK, 0.8f));
        DrawRectangleLines(42, 190, ScreenW - 84, 270, Fade(SKYBLUE, 0.75f));
        
        // Caution stripes (diagonal yellow/black) at top and bottom
        int stripY = 190;
        DrawRectangle(42, stripY, ScreenW - 84, 8, GOLD);
        for (int sx = 42; sx < ScreenW - 42; sx += 16) {
            DrawLineEx({(float)sx, (float)stripY}, {(float)sx + 8, (float)stripY + 8}, 3.0f, BLACK);
        }
        int stripY2 = 452;
        DrawRectangle(42, stripY2, ScreenW - 84, 8, GOLD);
        for (int sx = 42; sx < ScreenW - 42; sx += 16) {
            DrawLineEx({(float)sx, (float)stripY2}, {(float)sx + 8, (float)stripY2 + 8}, 3.0f, BLACK);
        }

        int pTitleW = MeasureText("PAUSED", 28);
        DrawText("PAUSED", ScreenW / 2 - pTitleW / 2, 215, 28, RAYWHITE);
        DrawLine(60, 255, ScreenW - 60, 255, Fade(SKYBLUE, 0.4f));
        
        const char* pItems[4] = { "Resume Mission", "How To Play", "Settings", "Quit to Title" };
        for (int i = 0; i < 4; ++i) {
            int y = 290 + i * 36;
            bool selected = (pauseSelection_ == i);
            int xOffset = selected ? (int)(5.0f + 4.0f * std::sin((float)GetTime() * 14.0f)) : 0;
            
            if (selected) DrawRectangle(120, y - 6, 240, 26, Fade(BLUE, 0.45f));
            DrawText(selected ? ">" : " ", 132 + xOffset, y, 16, selected ? GOLD : GRAY);
            DrawText(pItems[i], 158 + xOffset, y, 16, selected ? GOLD : RAYWHITE);
        }
        
        int tipW = MeasureText("UP/DOWN navigate  |  ENTER/CLICK confirm", 12);
        DrawText("UP/DOWN navigate  |  ENTER/CLICK confirm", ScreenW / 2 - tipW / 2, 442, 12, GRAY);
    } else if (state_ == State::StageClear) {
        stageClearDebrief_.Draw(clearTimer_, loop_, player_.lives, player_.bombs, stageClearBonus_);
    } else if (state_ == State::GameOver) {
        DrawRectangle(0, 0, ScreenW, ScreenH, Fade(BLACK, 0.65f));
        DrawRectangle(42, 190, ScreenW - 84, 250, Fade(BLACK, 0.8f));
        DrawRectangleLines(42, 190, ScreenW - 84, 250, Fade(RED, 0.75f));
        
        // Warning stripes (diagonal red/black) at top and bottom
        int stripY = 190;
        DrawRectangle(42, stripY, ScreenW - 84, 8, RED);
        for (int sx = 42; sx < ScreenW - 42; sx += 16) {
            DrawLineEx({(float)sx, (float)stripY}, {(float)sx + 8, (float)stripY + 8}, 3.0f, BLACK);
        }
        int stripY2 = 432;
        DrawRectangle(42, stripY2, ScreenW - 84, 8, RED);
        for (int sx = 42; sx < ScreenW - 42; sx += 16) {
            DrawLineEx({(float)sx, (float)stripY2}, {(float)sx + 8, (float)stripY2 + 8}, 3.0f, BLACK);
        }

        int goTitleW = MeasureText("GAME OVER", 28);
        DrawText("GAME OVER", ScreenW / 2 - goTitleW / 2, 215, 28, RED);
        DrawLine(60, 255, ScreenW - 60, 255, Fade(RED, 0.4f));
        
        const char* goItems[3] = { "Restart Mission", "Settings", "Quit to Title" };
        for (int i = 0; i < 3; ++i) {
            int y = 290 + i * 36;
            bool selected = (gameOverSelection_ == i);
            int xOffset = selected ? (int)(5.0f + 4.0f * std::sin((float)GetTime() * 14.0f)) : 0;
            
            if (selected) DrawRectangle(120, y - 6, 240, 26, Fade(RED, 0.25f));
            DrawText(selected ? ">" : " ", 132 + xOffset, y, 16, selected ? GOLD : GRAY);
            
            Color textCol = RAYWHITE;
            const char* label = goItems[i];
            if (i == 0) {
                if (demoMode_) {
                    textCol = selected ? GOLD : RAYWHITE;
                } else {
                    if (credits_ == 0) {
                        textCol = selected ? RED : GRAY;
                        label = "Insert Coin to Restart";
                    } else {
                        textCol = selected ? GOLD : LIME;
                        label = "Restart Mission (-1)";
                    }
                }
            } else {
                textCol = selected ? GOLD : RAYWHITE;
            }
            DrawText(label, 158 + xOffset, y, 16, textCol);
        }
        
        int tipW = MeasureText("UP/DOWN navigate  |  ENTER/CLICK confirm", 12);
        DrawText("UP/DOWN navigate  |  ENTER/CLICK confirm", ScreenW / 2 - tipW / 2, 412, 12, GRAY);
    }
    
    if (transitioning_) {
        DrawTransition();
    }

    if (crtShaderEnabled_ && !cleanPixelMode_) {
        float scanAlpha = 0.012f + (float)crtScanline_ * 0.0045f;
        for (int y = 0; y < ScreenH; y += 2) {
            DrawLine(0, y, ScreenW, y, Fade(BLACK, scanAlpha));
        }

        float vignetteAlpha = 0.10f + (float)crtVignette_ * 0.020f;
        DrawCircleGradient(ScreenW / 2, ScreenH / 2, ScreenH * 0.8f, Fade(WHITE, 0.0f), Fade(BLACK, vignetteAlpha));

        float glareAlpha = 0.012f + (float)crtGlare_ * 0.007f;
        DrawLine(8, 8, ScreenW - 8, 8, Fade(WHITE, glareAlpha));
        DrawLine(8, 8, 8, ScreenH - 8, Fade(WHITE, glareAlpha));

        DrawRectangleLinesEx(Rectangle{ 0.0f, 0.0f, (float)ScreenW, (float)ScreenH }, 8.0f, DARKGRAY);
        DrawRectangleLinesEx(Rectangle{ 8.0f, 8.0f, (float)ScreenW - 16.0f, (float)ScreenH - 16.0f }, 2.0f, BLACK);
    }
}

void Game::LoadHighScores() {
    highScores_.clear();
    HighScoreEntry defaults[5] = {
        { "SKY", 25000, 3 },
        { "ACE", 18000, 2 },
        { "PIL", 12000, 2 },
        { "BOX", 8000, 1 },
        { "AAA", 5000, 1 }
    };
    
    bool loadedFromFile = false;
    std::ifstream file("highscores.txt");
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            line = Trim(line);
            if (line.empty()) continue;
            std::stringstream ss(line);
            HighScoreEntry entry;
            std::string initials;
            int score = 0, loop = 0;
            if (ss >> initials >> score >> loop) {
                std::snprintf(entry.initials, sizeof(entry.initials), "%s", initials.substr(0, 3).c_str());
                entry.score = score;
                entry.loop = loop;
                highScores_.push_back(entry);
                loadedFromFile = true;
            }
        }
        file.close();
    }
    
    if (!loadedFromFile) {
        highScores_.clear();
        for (int i = 0; i < 5; ++i) {
            highScores_.push_back(defaults[i]);
        }
        SaveHighScores();
    } else {
        while (highScores_.size() < 5) {
            highScores_.push_back(defaults[highScores_.size()]);
        }
    }
    
    std::sort(highScores_.begin(), highScores_.end(), [](const HighScoreEntry& a, const HighScoreEntry& b) {
        return a.score > b.score;
    });
    
    if (highScores_.size() > 5) {
        highScores_.resize(5);
    }
}

void Game::SaveHighScores() {
    std::ofstream file("highscores.txt");
    if (file.is_open()) {
        for (const auto& entry : highScores_) {
            file << entry.initials << " " << entry.score << " " << entry.loop << "\n";
        }
        file.close();
    }
}

bool Game::CheckHighScore(int score) const {
    if (highScores_.size() < 5) return true;
    return score > highScores_.back().score;
}

void Game::InsertHighScore(const char* initials, int score, int loop) {
    HighScoreEntry entry;
    std::snprintf(entry.initials, sizeof(entry.initials), "%s", initials);
    entry.score = score;
    entry.loop = loop;
    
    highScores_.push_back(entry);
    std::sort(highScores_.begin(), highScores_.end(), [](const HighScoreEntry& a, const HighScoreEntry& b) {
        return a.score > b.score;
    });
    
    if (highScores_.size() > 5) {
        highScores_.pop_back();
    }
}

void Game::ApplyVolumeSettings() {
    audio_.SetMasterVolume((float)masterVolume_ / 10.0f);
    audio_.SetVolumes(sfxVolume_, bgmVolume_);
}

void Game::LoadSettings() {
    masterVolume_ = 9;
    sfxVolume_ = 8;
    bgmVolume_ = 6;
    screenShakeEnabled_ = true;
    hitboxOverlayEnabled_ = false;
    isFullscreen_ = false;
    controlLayout_ = 0;
    crtShaderEnabled_ = true;
    cleanPixelMode_ = false;
    crtCurvature_ = 3;
    crtScanline_ = 3;
    crtMask_ = 4;
    crtBloom_ = 2;
    crtVignette_ = 3;
    crtGlare_ = 2;
    aspectMode_ = 0;
    drawBezel_ = true;

    bool fileExists = false;
    std::ifstream file("settings.cfg");
    if (file.is_open()) {
        fileExists = true;
        std::string line;
        while (std::getline(file, line)) {
            line = Trim(line);
            if (line.empty() || line[0] == '#') continue;
            
            size_t eqPos = line.find('=');
            if (eqPos == std::string::npos) continue;
            
            std::string key = Trim(line.substr(0, eqPos));
            std::string valStr = Trim(line.substr(eqPos + 1));
            
            try {
                int val = std::stoi(valStr);
                if (key == "masterVol") masterVolume_ = std::clamp(val, 0, 10);
                else if (key == "sfxVol") sfxVolume_ = std::clamp(val, 0, 10);
                else if (key == "bgmVol") bgmVolume_ = std::clamp(val, 0, 10);
                else if (key == "shake") screenShakeEnabled_ = (val != 0);
                else if (key == "hitbox") hitboxOverlayEnabled_ = (val != 0);
                else if (key == "fullscreen") isFullscreen_ = (val != 0);
                else if (key == "layout") controlLayout_ = std::clamp(val, 0, 1);
                else if (key == "crt") crtShaderEnabled_ = (val != 0);
                else if (key == "cleanPixel") cleanPixelMode_ = (val != 0);
                else if (key == "crtCurvature") crtCurvature_ = std::clamp(val, 0, 10);
                else if (key == "crtScanline") crtScanline_ = std::clamp(val, 0, 10);
                else if (key == "crtMask") crtMask_ = std::clamp(val, 0, 10);
                else if (key == "crtBloom") crtBloom_ = std::clamp(val, 0, 10);
                else if (key == "crtVignette") crtVignette_ = std::clamp(val, 0, 10);
                else if (key == "crtGlare") crtGlare_ = std::clamp(val, 0, 10);
                else if (key == "aspect") aspectMode_ = std::clamp(val, 0, 2);
                else if (key == "bezel") drawBezel_ = (val != 0);
            } catch (...) {
                // Ignore malformed values
            }
        }
        file.close();
    }
    
    if (!fileExists) {
        SaveSettings();
    }
    
    debug_ = hitboxOverlayEnabled_;
    
    if (isFullscreen_ && !IsWindowFullscreen()) {
        ToggleFullscreen();
    }
    
    ApplyVolumeSettings();
    player_.controlLayout = controlLayout_;
}

void Game::SaveSettings() {
    std::ofstream file("settings.cfg");
    if (file.is_open()) {
        file << "masterVol=" << masterVolume_ << "\n";
        file << "sfxVol=" << sfxVolume_ << "\n";
        file << "bgmVol=" << bgmVolume_ << "\n";
        file << "shake=" << (screenShakeEnabled_ ? 1 : 0) << "\n";
        file << "hitbox=" << (hitboxOverlayEnabled_ ? 1 : 0) << "\n";
        file << "fullscreen=" << (isFullscreen_ ? 1 : 0) << "\n";
        file << "layout=" << controlLayout_ << "\n";
        file << "crt=" << (crtShaderEnabled_ ? 1 : 0) << "\n";
        file << "cleanPixel=" << (cleanPixelMode_ ? 1 : 0) << "\n";
        file << "crtCurvature=" << crtCurvature_ << "\n";
        file << "crtScanline=" << crtScanline_ << "\n";
        file << "crtMask=" << crtMask_ << "\n";
        file << "crtBloom=" << crtBloom_ << "\n";
        file << "crtVignette=" << crtVignette_ << "\n";
        file << "crtGlare=" << crtGlare_ << "\n";
        file << "aspect=" << aspectMode_ << "\n";
        file << "bezel=" << (drawBezel_ ? 1 : 0) << "\n";
        file.close();
    }
}


void Game::StartTransition(State target) {
    if (transitioning_) return;
    transitioning_ = true;
    transitionTimer_ = 0.0f;
    transitionTarget_ = target;
    transitionAlpha_ = 0.0f;
    if (target == State::GameOver) {
        gameOverTimer_ = 0.0f;
        gameOverSelection_ = 0;
    } else if (target == State::Continue) {
        audio_.PlayContinueTick();
    }
}

void Game::UpdateTransition(float dt) {
    if (!transitioning_) return;
    transitionTimer_ += dt;
    if (transitionTimer_ < 0.25f) {
        transitionAlpha_ = transitionTimer_ / 0.25f;
    } else if (transitionTimer_ < 0.35f) {
        if (state_ != transitionTarget_) {
            state_ = transitionTarget_;
            if (state_ == State::Title) {
                ReturnToTitle();
            } else if (state_ == State::Playing) {
                StartGame();
            }
        }
        transitionAlpha_ = 1.0f;
    } else if (transitionTimer_ < 0.60f) {
        transitionAlpha_ = 1.0f - (transitionTimer_ - 0.35f) / 0.25f;
    } else {
        transitioning_ = false;
        transitionAlpha_ = 0.0f;
    }
}
