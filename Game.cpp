#include "Game.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cstdio>

static bool CircleHit(Vector2 a, float ar, Vector2 b, float br) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float r = ar + br;
    return dx * dx + dy * dy <= r * r;
}

static Wave MakeToneSweep(float startHz, float endHz, float seconds) {
    const int rate = 22050;
    int frames = (int)(rate * seconds);
    int16_t* samples = (int16_t*)std::malloc(sizeof(int16_t) * frames);
    if (!samples) return {};
    float phase = 0.0f;
    for (int i = 0; i < frames; ++i) {
        float t = (float)i / (float)rate;
        float env = 1.0f - t / seconds;
        float hz = startHz + (endHz - startHz) * (t / seconds);
        phase += hz / (float)rate;
        samples[i] = (int16_t)(std::sin(phase * 6.2831853f) * 9000.0f * env);
    }
    Wave w{};
    w.frameCount = (unsigned int)frames;
    w.sampleRate = rate;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = samples;
    return w;
}

static Wave MakeExplosionTone(float seconds) {
    const int rate = 22050;
    int frames = (int)(rate * seconds);
    int16_t* samples = (int16_t*)std::malloc(sizeof(int16_t) * frames);
    if (!samples) return {};
    
    unsigned int randSeed = 0x12345;
    auto quickRand = [&randSeed]() {
        randSeed = randSeed * 1103515245 + 12345;
        return (float)(randSeed & 0x7FFF) / 32768.0f;
    };
    
    float phase = 0.0f;
    for (int i = 0; i < frames; ++i) {
        float t = (float)i / (float)rate;
        float env = 1.0f - t / seconds;
        float hz = 140.0f * (1.0f - t / seconds * 0.8f);
        phase += hz / (float)rate;
        float sineVal = std::sin(phase * 6.2831853f);
        float noiseVal = quickRand() * 2.0f - 1.0f;
        float mixed = sineVal * 0.4f + noiseVal * 0.6f;
        samples[i] = (int16_t)(mixed * 9500.0f * env * env);
    }
    Wave w{};
    w.frameCount = (unsigned int)frames;
    w.sampleRate = rate;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = samples;
    return w;
}

static Wave MakeChiptuneBGM() {
    const int sampleRate = 22050;
    const int totalSteps = 128;
    const int samplesPerStep = 2756;
    const int totalSamples = totalSteps * samplesPerStep;
    
    int16_t* samples = (int16_t*)std::malloc(sizeof(int16_t) * totalSamples);
    if (!samples) return {};
    
    float bassFreqs[4][4] = {
        { 110.0f, 130.8f, 164.8f, 220.0f }, // Am
        { 87.3f, 110.0f, 130.8f, 174.6f },  // F
        { 65.4f, 82.4f, 98.0f, 130.8f },    // C
        { 98.0f, 123.5f, 146.8f, 196.0f }   // G
    };
    
    int melNotes[128] = {
        69, 0, 72, 74, 76, 0, 76, 74,  72, 69, 72, 0,  74, 0, 71, 0,
        69, 0, 72, 74, 76, 0, 79, 76,  74, 72, 74, 0,  76, 0, 69, 0,
        65, 0, 69, 72, 72, 0, 72, 69,  65, 62, 65, 0,  69, 0, 67, 0,
        67, 0, 71, 74, 74, 0, 74, 76,  79, 76, 74, 71, 69, 72, 74, 76,
        69, 0, 72, 74, 76, 0, 76, 74,  72, 69, 72, 0,  74, 0, 71, 0,
        69, 0, 72, 74, 76, 0, 79, 76,  74, 72, 74, 0,  76, 0, 79, 0,
        81, 0, 79, 76, 76, 0, 74, 72,  74, 0, 76, 0,  79, 0, 81, 0,
        81, 79, 76, 74, 76, 74, 72, 69, 71, 0, 74, 0,  69, 0, 0, 0
    };
    
    float melPhase = 0.0f;
    float bassPhase = 0.0f;
    float kickPhase = 0.0f;
    
    unsigned int randSeed = 0x12345;
    auto quickRand = [&randSeed]() {
        randSeed = randSeed * 1103515245 + 12345;
        return (float)(randSeed & 0x7FFF) / 32768.0f;
    };
    
    for (int s = 0; s < totalSamples; ++s) {
        int stepIndex = s / samplesPerStep;
        int stepOffset = s % samplesPerStep;
        float stepProgress = (float)stepOffset / (float)samplesPerStep;
        
        int chordIdx = (stepIndex / 8) % 4;
        
        // 1. Bassline (sawtooth wave arpeggio)
        int bassPattern[8] = { 0, 2, 0, 3, 1, 2, 1, 3 };
        int bassNoteIdx = bassPattern[stepIndex % 8];
        float bassFreq = bassFreqs[chordIdx][bassNoteIdx];
        
        bassPhase += bassFreq / (float)sampleRate;
        if (bassPhase > 1.0f) bassPhase -= 1.0f;
        float bassVal = 2.0f * bassPhase - 1.0f;
        float bassEnv = 1.0f - stepProgress;
        bassVal *= bassEnv * 0.28f;
        
        // 2. Melody (Square wave with simple volume decay)
        float melVal = 0.0f;
        int note = melNotes[stepIndex];
        if (note > 0) {
            float melFreq = 440.0f * std::pow(2.0f, (float)(note - 69) / 12.0f);
            melPhase += melFreq / (float)sampleRate;
            if (melPhase > 1.0f) melPhase -= 1.0f;
            melVal = (melPhase < 0.5f) ? 1.0f : -1.0f;
            float melEnv = 1.0f - stepProgress;
            float vib = 1.0f + 0.02f * std::sin((float)stepOffset * 0.005f);
            melPhase += (melFreq * vib - melFreq) / (float)sampleRate;
            melVal *= melEnv * 0.20f;
        }
        
        // 3. Drums (Kick & Snare & Hi-hat)
        float drumVal = 0.0f;
        int beatStep = stepIndex % 8;
        
        if (beatStep == 0 || beatStep == 4) {
            float kickFreq = 150.0f * (1.0f - stepProgress * 0.85f);
            kickPhase += kickFreq / (float)sampleRate;
            float kickS = std::sin(kickPhase * 6.2831853f);
            float kickEnv = std::max(0.0f, 1.0f - stepProgress * 2.0f);
            drumVal += kickS * kickEnv * 0.35f;
        }
        
        if (beatStep == 2 || beatStep == 6) {
            float noise = quickRand() * 2.0f - 1.0f;
            float snareEnv = std::max(0.0f, 1.0f - stepProgress * 1.5f);
            drumVal += noise * snareEnv * 0.16f;
        }
        
        if (beatStep % 2 == 1) {
            float noise = quickRand() * 2.0f - 1.0f;
            float hatEnv = std::max(0.0f, 1.0f - stepProgress * 4.0f);
            drumVal += noise * hatEnv * 0.08f;
        }
        
        float mixed = bassVal + melVal + drumVal;
        mixed = std::clamp(mixed, -1.0f, 1.0f);
        samples[s] = (int16_t)(mixed * 8000.0f);
    }
    
    Wave w{};
    w.frameCount = (unsigned int)totalSamples;
    w.sampleRate = sampleRate;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = samples;
    return w;
}

Game::Game() {
    InitWindow(ScreenW, ScreenH, "Sky Circuit - raylib arcade shooter");
    SetTargetFPS(60);
    InitAudioDevice();
    audioReady_ = IsAudioDeviceReady();
    
    // Load high scores and settings
    LoadHighScores();
    
    if (audioReady_) {
        LoadGeneratedSounds();
    }
    
    LoadSettings(); // Loads & applies settings to the audio files

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
}

Game::~Game() {
    UnloadGeneratedSounds();
    if (audioReady_) CloseAudioDevice();
    CloseWindow();
}

void Game::LoadGeneratedSounds() {
    Wave shootW = MakeToneSweep(1000, 200, 0.08f);
    Wave boomW = MakeExplosionTone(0.45f);
    Wave pickupW = MakeToneSweep(600, 1600, 0.12f);
    Wave selectW = MakeToneSweep(620, 780, 0.05f);
    Wave confirmW = MakeToneSweep(520, 1040, 0.15f);
    Wave hitW = MakeToneSweep(400, 80, 0.40f);
    Wave overW = MakeToneSweep(300, 60, 0.95f);
    Wave clearW = MakeToneSweep(520, 1500, 0.60f);
    Wave bgmW = MakeChiptuneBGM();
    Wave klaxonW = MakeToneSweep(350, 480, 0.22f);
    
    if (!shootW.data || !boomW.data || !pickupW.data || !selectW.data || 
        !confirmW.data || !hitW.data || !overW.data || !clearW.data || !bgmW.data || !klaxonW.data) {
        
        if (shootW.data) UnloadWave(shootW);
        if (boomW.data) UnloadWave(boomW);
        if (pickupW.data) UnloadWave(pickupW);
        if (selectW.data) UnloadWave(selectW);
        if (confirmW.data) UnloadWave(confirmW);
        if (hitW.data) UnloadWave(hitW);
        if (overW.data) UnloadWave(overW);
        if (clearW.data) UnloadWave(clearW);
        if (bgmW.data) UnloadWave(bgmW);
        if (klaxonW.data) UnloadWave(klaxonW);
        soundsReady_ = false;
        return;
    }
    
    shootSound_ = LoadSoundFromWave(shootW);
    boomSound_ = LoadSoundFromWave(boomW);
    pickupSound_ = LoadSoundFromWave(pickupW);
    menuSelectSound_ = LoadSoundFromWave(selectW);
    menuConfirmSound_ = LoadSoundFromWave(confirmW);
    playerHitSound_ = LoadSoundFromWave(hitW);
    gameOverSound_ = LoadSoundFromWave(overW);
    stageClearSound_ = LoadSoundFromWave(clearW);
    bgmSound_ = LoadSoundFromWave(bgmW);
    bossKlaxonSound_ = LoadSoundFromWave(klaxonW);
    
    soundsReady_ = true;
    
    UnloadWave(shootW);
    UnloadWave(boomW);
    UnloadWave(pickupW);
    UnloadWave(selectW);
    UnloadWave(confirmW);
    UnloadWave(hitW);
    UnloadWave(overW);
    UnloadWave(clearW);
    UnloadWave(bgmW);
    UnloadWave(klaxonW);
}

void Game::UnloadGeneratedSounds() {
    if (!audioReady_) return;
    UnloadSound(shootSound_);
    UnloadSound(boomSound_);
    UnloadSound(pickupSound_);
    UnloadSound(menuSelectSound_);
    UnloadSound(menuConfirmSound_);
    UnloadSound(playerHitSound_);
    UnloadSound(gameOverSound_);
    UnloadSound(stageClearSound_);
    UnloadSound(bgmSound_);
    UnloadSound(bossKlaxonSound_);
}

void Game::Run() {
    while (!WindowShouldClose() && !shouldExit_) {
        Update(GetFrameTime());
        BeginDrawing();
        ClearBackground(BLACK);
        Draw();
        EndDrawing();
    }
}

void Game::StartGame() {
    player_.ResetForNewGame();
    player_.isDemo = demoMode_;
    playerBullets_.clear(); enemyBullets_.clear(); enemies_.clear(); powerups_.clear();
    effects_.Clear();
    waves_.Reset();
    stageTime_ = 0.0f;
    clearTimer_ = 0.0f;
    loop_ = 1;
    bossSpawned_ = false;
    bossWarningTimer_ = 0.0f;
    bossDeathTimer_ = 0.0f;
    for (int i = 0; i < 10; ++i) formationCount_[i] = 0;
    state_ = State::Playing;
    
    if (audioReady_) {
        StopSound(bgmSound_);
        PlaySound(bgmSound_);
    }
}

void Game::ReturnToTitle() {
    titleSelection_ = 0;
    returnFromHowTo_ = State::Title;
    player_.ResetForNewGame();
    playerBullets_.clear(); enemyBullets_.clear(); enemies_.clear(); powerups_.clear();
    effects_.Clear();
    waves_.Reset();
    stageTime_ = 0.0f;
    clearTimer_ = 0.0f;
    loop_ = 1;
    bossSpawned_ = false;
    demoMode_ = false;
    bossWarningTimer_ = 0.0f;
    bossDeathTimer_ = 0.0f;
    for (int i = 0; i < 10; ++i) formationCount_[i] = 0;
    state_ = State::Title;
    
    if (audioReady_) {
        StopSound(bgmSound_);
        PlaySound(bgmSound_);
    }
}

void Game::NextLoop() {
    ++loop_;
    stageTime_ = 0.0f;
    clearTimer_ = 0.0f;
    bossSpawned_ = false;
    playerBullets_.clear(); enemyBullets_.clear(); enemies_.clear(); powerups_.clear();
    waves_.Reset();
    player_.bombs = std::min(6, player_.bombs + 1);
    state_ = State::Playing;
}

void Game::Update(float dt) {
    if (IsKeyPressed(KEY_F1)) {
        debug_ = !debug_;
        hitboxOverlayEnabled_ = debug_;
        SaveSettings();
    }
    
    scrollA_ += (80.0f + loop_ * 6.0f) * dt;
    scrollB_ += (38.0f + loop_ * 3.0f) * dt;
    if (scrollA_ > ScreenH) scrollA_ -= ScreenH;
    if (scrollB_ > ScreenH) scrollB_ -= ScreenH;

    for (int i = 0; i < NumStars; ++i) {
        stars_[i].pos.y += stars_[i].speed * dt;
        if (stars_[i].pos.y > ScreenH) {
            stars_[i].pos.y = -10;
            stars_[i].pos.x = (float)GetRandomValue(0, ScreenW);
        }
    }
    
    effects_.Update(dt);
    
    // Play & loop BGM
    if (audioReady_ && !IsSoundPlaying(bgmSound_) && state_ != State::GameOver) {
        PlaySound(bgmSound_);
    }
    
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
            if (audioReady_) PlaySound(menuConfirmSound_);
            ReturnToTitle();
            return;
        }
    }
    
    if (state_ == State::Title) {
        titleTimer_ += dt;
        if (titleTimer_ > 15.0f) {
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
            
            if (keyUp) {
                titleSelection_ = (titleSelection_ + 4) % 5;
                titleTimer_ = 0.0f;
                if (audioReady_) PlaySound(menuSelectSound_);
            }
            if (keyDown) {
                titleSelection_ = (titleSelection_ + 1) % 5;
                titleTimer_ = 0.0f;
                if (audioReady_) PlaySound(menuSelectSound_);
            }
            
            if (keyConfirm) {
                titleTimer_ = 0.0f;
                if (audioReady_) PlaySound(menuConfirmSound_);
                if (titleSelection_ == 0) {
                    demoMode_ = false;
                    StartTransition(State::Playing);
                } else if (titleSelection_ == 1) {
                    StartTransition(State::HighScores);
                } else if (titleSelection_ == 2) {
                    StartTransition(State::Settings);
                } else if (titleSelection_ == 3) {
                    returnFromHowTo_ = State::Title;
                    StartTransition(State::HowTo);
                } else {
                    shouldExit_ = true;
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
        if (backPressed) {
            if (audioReady_) PlaySound(menuConfirmSound_);
            StartTransition(State::Title);
        }
    } else if (state_ == State::Settings) {
        UpdateSettings();
    } else if (state_ == State::NameEntry) {
        UpdateNameEntry();
    } else if (state_ == State::HowTo) {
        bool backPressed = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE);
        if (IsGamepadAvailable(0)) {
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
                backPressed = true;
            }
        }
        if (backPressed) {
            if (audioReady_) PlaySound(menuConfirmSound_);
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
            if (audioReady_) PlaySound(menuConfirmSound_);
            state_ = State::Paused;
        } else {
            UpdatePlaying(dt);
        }
    } else if (state_ == State::Paused) {
        bool resumePressed = IsKeyPressed(KEY_P);
        bool howtoPressed = IsKeyPressed(KEY_H);
        bool quitPressed = IsKeyPressed(KEY_Q);
        if (IsGamepadAvailable(0)) {
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                resumePressed = true;
            }
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP)) {
                howtoPressed = true;
            }
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
                quitPressed = true;
            }
        }
        
        if (resumePressed) {
            if (audioReady_) PlaySound(menuConfirmSound_);
            state_ = State::Playing;
        } else if (howtoPressed) {
            if (audioReady_) PlaySound(menuConfirmSound_);
            returnFromHowTo_ = State::Paused;
            StartTransition(State::HowTo);
        } else if (quitPressed) {
            if (audioReady_) PlaySound(menuConfirmSound_);
            demoMode_ = false;
            StartTransition(State::Title);
        }
    } else if (state_ == State::StageClear) {
        clearTimer_ += dt;
        bool skipPressed = IsKeyPressed(KEY_ENTER);
        if (IsGamepadAvailable(0)) {
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) skipPressed = true;
        }
        if (clearTimer_ > 3.0f || skipPressed) {
            NextLoop();
        }
    } else if (state_ == State::GameOver) {
        bool confirmPressed = IsKeyPressed(KEY_ENTER);
        if (IsGamepadAvailable(0)) {
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) confirmPressed = true;
        }
        if (confirmPressed) {
            if (audioReady_) PlaySound(menuConfirmSound_);
            StartTransition(State::Title);
        }
    }
}

void Game::UpdatePlaying(float dt) {
    if (bossDeathTimer_ > 0.0f) {
        bossDeathTimer_ -= dt;
        static float explosionDelay = 0.0f;
        explosionDelay += dt;
        if (explosionDelay > 0.15f) {
            explosionDelay = 0.0f;
            Vector2 offset = { (float)GetRandomValue(-40, 40), (float)GetRandomValue(-40, 40) };
            effects_.Explosion({ bossDeathPos_.x + offset.x, bossDeathPos_.y + offset.y }, ORANGE, 14);
            effects_.Shake(5.0f, 0.15f);
            if (audioReady_) PlaySound(boomSound_);
        }
        
        if (bossDeathTimer_ <= 0.0f) {
            effects_.Explosion(bossDeathPos_, VIOLET, 85);
            effects_.Shake(14.0f, 0.6f);
            if (audioReady_) PlaySound(boomSound_);
            if (audioReady_) PlaySound(stageClearSound_);
            
            state_ = State::StageClear;
            clearTimer_ = 0.0f;
            player_.score += 2000 * loop_;
        }
        
        enemyBullets_.clear();
        return;
    }

    stageTime_ += dt;
    player_.Update(dt, enemies_, enemyBullets_);
    size_t before = playerBullets_.size();
    player_.TryShoot(playerBullets_);
    if (audioReady_ && playerBullets_.size() > before) PlaySound(shootSound_);

    if (player_.TryBomb()) {
        enemyBullets_.clear();
        for (auto& e : enemies_) e.Hit(e.IsBoss() ? 28 : 99);
        effects_.Explosion(player_.pos, SKYBLUE, 58);
        effects_.Shake(10.0f, 0.35f);
        if (audioReady_) PlaySound(boomSound_);
    }

    // Boss warning klaxon timer loop
    if (bossWarningTimer_ > 0.0f) {
        bossWarningTimer_ -= dt;
        static float klaxonRepeatTimer = 0.0f;
        klaxonRepeatTimer += dt;
        if (klaxonRepeatTimer > 0.8f) {
            klaxonRepeatTimer = 0.0f;
            if (audioReady_ && bossWarningTimer_ > 0.5f) PlaySound(bossKlaxonSound_);
        }
    }

    waves_.Update(stageTime_, loop_, enemies_);
    if (!bossSpawned_ && waves_.ShouldSpawnBoss(stageTime_, BossAlive())) {
        enemies_.emplace_back(EnemyType::Miniboss, Vector2{240, -70}, loop_, 0);
        bossSpawned_ = true;
        enemyBullets_.clear();
        
        bossWarningTimer_ = 3.0f;
        if (audioReady_) PlaySound(bossKlaxonSound_);
    }

    // Auto-detect newly spawned formations
    for (const auto& e : enemies_) {
        if (e.active && e.formationId > 0 && e.formationId < 10) {
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

    for (auto& b : playerBullets_) b.Update(dt, enemies_);
    for (auto& b : enemyBullets_) b.Update(dt, enemies_);
    for (auto& e : enemies_) e.Update(dt, player_.pos, enemyBullets_, loop_);
    for (auto& p : powerups_) p.Update(dt);

    HandleCollisions();
    Cleanup();

    if (bossSpawned_ && !BossAlive() && state_ == State::Playing) {
        bossDeathTimer_ = 1.6f;
        bossDeathPos_ = { 240, 110 };
        for (const auto& e : enemies_) {
            if (e.IsBoss()) {
                bossDeathPos_ = e.pos;
                break;
            }
        }
        enemyBullets_.clear();
        effects_.AddText(bossDeathPos_, "BOSS DEFEATED!", GOLD);
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
    
    if (keyUp) {
        settingsSelection_ = (settingsSelection_ + 5) % 6;
        if (audioReady_) PlaySound(menuSelectSound_);
    }
    if (keyDown) {
        settingsSelection_ = (settingsSelection_ + 1) % 6;
        if (audioReady_) PlaySound(menuSelectSound_);
    }
    
    if (settingsSelection_ == 0) {
        if (keyLeft && sfxVolume_ > 0) {
            sfxVolume_--;
            LoadSettings();
            if (audioReady_) PlaySound(menuSelectSound_);
        }
        if (keyRight && sfxVolume_ < 10) {
            sfxVolume_++;
            LoadSettings();
            if (audioReady_) PlaySound(menuSelectSound_);
        }
    } else if (settingsSelection_ == 1) {
        if (keyLeft && bgmVolume_ > 0) {
            bgmVolume_--;
            LoadSettings();
            if (audioReady_) PlaySound(menuSelectSound_);
        }
        if (keyRight && bgmVolume_ < 10) {
            bgmVolume_++;
            LoadSettings();
            if (audioReady_) PlaySound(menuSelectSound_);
        }
    } else if (settingsSelection_ == 2) {
        if (keyLeft || keyRight || keyConfirm) {
            screenShakeEnabled_ = !screenShakeEnabled_;
            if (audioReady_) PlaySound(menuConfirmSound_);
        }
    } else if (settingsSelection_ == 3) {
        if (keyLeft || keyRight || keyConfirm) {
            hitboxOverlayEnabled_ = !hitboxOverlayEnabled_;
            debug_ = hitboxOverlayEnabled_;
            if (audioReady_) PlaySound(menuConfirmSound_);
        }
    } else if (settingsSelection_ == 4) {
        if (keyLeft || keyRight || keyConfirm) {
            isFullscreen_ = !isFullscreen_;
            ToggleFullscreen();
            if (audioReady_) PlaySound(menuConfirmSound_);
        }
    } else if (settingsSelection_ == 5) {
        if (keyConfirm || IsKeyPressed(KEY_ESCAPE)) {
            SaveSettings();
            if (audioReady_) PlaySound(menuConfirmSound_);
            StartTransition(State::Title);
        }
    }
    
    if (IsKeyPressed(KEY_ESCAPE)) {
        SaveSettings();
        if (audioReady_) PlaySound(menuConfirmSound_);
        StartTransition(State::Title);
    }
}

void Game::UpdateNameEntry() {
    bool keyLeft = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
    bool keyRight = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
    bool keyUp = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
    bool keyDown = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
    bool keyConfirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    
    if (IsGamepadAvailable(0)) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) keyLeft = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) keyRight = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) keyUp = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) keyDown = true;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) keyConfirm = true;
    }
    
    if (keyLeft && nameEntryIndex_ > 0) {
        nameEntryIndex_--;
        if (audioReady_) PlaySound(menuSelectSound_);
    }
    if (keyRight && nameEntryIndex_ < 2) {
        nameEntryIndex_++;
        if (audioReady_) PlaySound(menuSelectSound_);
    }
    
    char curChar = nameEntryBuf_[nameEntryIndex_];
    if (keyUp) {
        curChar++;
        if (curChar > 'Z') curChar = 'A';
        nameEntryBuf_[nameEntryIndex_] = curChar;
        if (audioReady_) PlaySound(menuSelectSound_);
    }
    if (keyDown) {
        curChar--;
        if (curChar < 'A') curChar = 'Z';
        nameEntryBuf_[nameEntryIndex_] = curChar;
        if (audioReady_) PlaySound(menuSelectSound_);
    }
    
    // Direct characters
    for (int key = KEY_A; key <= KEY_Z; ++key) {
        if (IsKeyPressed(key)) {
            nameEntryBuf_[nameEntryIndex_] = (char)('A' + (key - KEY_A));
            if (audioReady_) PlaySound(menuSelectSound_);
            if (nameEntryIndex_ < 2) nameEntryIndex_++;
        }
    }
    
    if (keyConfirm) {
        nameEntryBuf_[3] = '\0';
        InsertHighScore(nameEntryBuf_, player_.score, loop_);
        SaveHighScores();
        if (audioReady_) PlaySound(menuConfirmSound_);
        StartTransition(State::HighScores);
    }
}

void Game::HandleCollisions() {
    for (auto& b : playerBullets_) {
        if (!b.active) continue;
        for (auto& e : enemies_) {
            if (!e.active || !CircleHit(b.pos, b.radius, e.pos, e.radius)) continue;
            b.active = false;
            e.Hit(b.damage);
            effects_.Spark(b.pos, YELLOW);
            if (!e.active) {
                player_.score += e.scoreValue;
                
                char scoreText[16];
                std::snprintf(scoreText, sizeof(scoreText), "+%d", e.scoreValue);
                effects_.AddText(e.pos, scoreText, YELLOW);

                effects_.Explosion(e.pos, e.IsBoss() ? VIOLET : ORANGE, e.IsBoss() ? 80 : 22);
                effects_.Shake(e.IsBoss() ? 8.0f : 3.0f, e.IsBoss() ? 0.45f : 0.16f);
                SpawnDrop(e.pos, e.type);

                if (e.formationId > 0 && e.formationId < 10) {
                    if (formationCount_[e.formationId] > 0) {
                        formationCount_[e.formationId]--;
                        if (formationCount_[e.formationId] == 0) {
                            player_.score += 1000;
                            effects_.AddText(e.pos, "FORMATION BONUS! +1000", GOLD);
                            powerups_.emplace_back(PowerupType::Medal, e.pos);
                            if (audioReady_) PlaySound(pickupSound_);
                        }
                    }
                }
            }
            break;
        }
    }

    if (!player_.invulnerable) {
        for (auto& b : enemyBullets_) {
            if (b.active && CircleHit(b.pos, b.radius, player_.pos, player_.hitRadius)) {
                b.active = false;
                --player_.lives;
                effects_.Explosion(player_.pos, RED, 38);
                effects_.Shake(8.0f, 0.35f);
                if (player_.lives <= 0) {
                    if (audioReady_) PlaySound(gameOverSound_);
                    if (!demoMode_ && CheckHighScore(player_.score)) {
                        nameEntryBuf_[0] = 'A'; nameEntryBuf_[1] = 'A'; nameEntryBuf_[2] = 'A'; nameEntryBuf_[3] = '\0';
                        nameEntryIndex_ = 0;
                        StartTransition(State::NameEntry);
                    } else {
                        StartTransition(State::GameOver);
                    }
                } else {
                    if (audioReady_) PlaySound(playerHitSound_);
                    player_.ResetAfterHit();
                }
                break;
            }
        }
    }

    for (auto& e : enemies_) {
        if (e.active && !player_.invulnerable && CircleHit(e.pos, e.radius, player_.pos, player_.hitRadius)) {
            e.active = false;
            --player_.lives;
            effects_.Explosion(player_.pos, RED, 38);
            if (player_.lives <= 0) {
                if (audioReady_) PlaySound(gameOverSound_);
                if (!demoMode_ && CheckHighScore(player_.score)) {
                    nameEntryBuf_[0] = 'A'; nameEntryBuf_[1] = 'A'; nameEntryBuf_[2] = 'A'; nameEntryBuf_[3] = '\0';
                    nameEntryIndex_ = 0;
                    StartTransition(State::NameEntry);
                } else {
                    StartTransition(State::GameOver);
                }
            } else {
                if (audioReady_) PlaySound(playerHitSound_);
                player_.ResetAfterHit();
            }
        }
    }

    for (auto& p : powerups_) {
        if (!p.active || !CircleHit(p.pos, p.radius, player_.pos, player_.spriteRadius)) continue;
        p.active = false;
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
            player_.score += 500;
            effects_.AddText(p.pos, "+500", GOLD);
        }
        if (audioReady_) PlaySound(pickupSound_);
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

void Game::Cleanup() {
    // Disqualify formation score if any member escapes offscreen
    for (const auto& e : enemies_) {
        if (e.active && e.Offscreen(ScreenH) && e.formationId > 0 && e.formationId < 10) {
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

void Game::DrawBackground() const {
    ClearBackground({6, 8, 20, 255});
    
    // Layer 1: Distant Nebulae / Grids (drawn very faint)
    for (int y = -ScreenH; y < ScreenH * 2; y += 80) {
        int yy = (int)(y + scrollB_);
        DrawRectangle(72, yy, 38, 46, Fade(BLUE, 0.08f));
        DrawRectangle(352, yy + 28, 52, 32, Fade(SKYBLUE, 0.05f));
    }
    
    // Layer 2: Parallax Stars (dim & medium speed stars)
    for (int i = 0; i < NumStars; ++i) {
        DrawCircleV(stars_[i].pos, stars_[i].speed > 100 ? 1.5f : 1.0f, stars_[i].color);
    }
    
    // Layer 3: Foreground scrolling pipelines / borders (fast)
    for (int y = -ScreenH; y < ScreenH * 2; y += 48) {
        int yy = (int)(y + scrollA_);
        DrawLine(0, yy, ScreenW, yy + 34, Fade(DARKBLUE, 0.28f));
    }
    DrawRectangleLines(8, 8, ScreenW - 16, ScreenH - 16, Fade(SKYBLUE, 0.35f));
}

void Game::DrawHud() const {
    DrawRectangle(0, 0, ScreenW, 34, Fade(BLACK, 0.72f));
    DrawText(TextFormat("SCORE %08d", player_.score), 12, 10, 14, RAYWHITE);
    DrawText(TextFormat("LIVES %d", std::max(0, player_.lives)), 178, 10, 14, RAYWHITE);
    DrawText(TextFormat("BOMBS %d", player_.bombs), 258, 10, 14, RAYWHITE);
    DrawText(TextFormat("%s LV%d", player_.WeaponName(), player_.weaponLevel), 350, 10, 14, RAYWHITE);
    if (debug_) DrawText("F1 HITBOX", 12, 42, 12, LIME);
    if (stageTime_ < 90.0f) DrawText(TextFormat("STAGE %02d", 90 - (int)stageTime_), 390, 42, 12, GRAY);
    else DrawText("WARNING", 396, 42, 12, RED);
    
    if (demoMode_) {
        Color demoColor = (int)(GetTime() * 2) % 2 == 0 ? Fade(RED, 0.8f) : Fade(YELLOW, 0.8f);
        int demoW = MeasureText("DEMO PLAY - PRESS ANY KEY", 16);
        DrawRectangle(0, ScreenH - 42, ScreenW, 42, Fade(BLACK, 0.82f));
        DrawText("DEMO PLAY - PRESS ANY KEY", ScreenW / 2 - demoW / 2, ScreenH - 28, 16, demoColor);
    }
}

void Game::DrawCenteredText(const char* title, const char* subtitle) const {
    int tw = MeasureText(title, 34);
    DrawText(title, ScreenW / 2 - tw / 2, 238, 34, RAYWHITE);
    int sw = MeasureText(subtitle, 16);
    DrawText(subtitle, ScreenW / 2 - sw / 2, 290, 16, SKYBLUE);
}

void Game::DrawTitleMenu() const {
    DrawRectangle(42, 160, ScreenW - 84, 320, Fade(BLACK, 0.75f));
    DrawRectangleLines(42, 160, ScreenW - 84, 320, Fade(SKYBLUE, 0.75f));
    
    int titleW = MeasureText("SKY CIRCUIT", 36);
    DrawText("SKY CIRCUIT", ScreenW / 2 - titleW / 2, 185, 36, RAYWHITE);
    DrawText("A local vertical arcade shooter", ScreenW / 2 - MeasureText("A local vertical arcade shooter", 14) / 2, 228, 14, SKYBLUE);

    const char* items[5] = {"Start Mission", "High Scores", "Settings", "How To Play", "Exit"};
    for (int i = 0; i < 5; ++i) {
        int y = 265 + i * 34;
        bool selected = titleSelection_ == i;
        if (selected) DrawRectangle(120, y - 6, 240, 26, Fade(BLUE, 0.45f));
        DrawText(selected ? ">" : " ", 132, y, 16, selected ? GOLD : GRAY);
        DrawText(items[i], 158, y, 16, selected ? GOLD : RAYWHITE);
    }

    DrawText("UP/DOWN choose  ENTER confirm", ScreenW / 2 - MeasureText("UP/DOWN choose  ENTER confirm", 13) / 2, 442, 13, GRAY);
    DrawText("Procedural vector art & chiptunes", ScreenW / 2 - MeasureText("Procedural vector art & chiptunes", 13) / 2, 498, 13, DARKGRAY);
}

void Game::DrawHowTo() const {
    DrawRectangle(30, 56, ScreenW - 60, ScreenH - 112, Fade(BLACK, 0.85f));
    DrawRectangleLines(30, 56, ScreenW - 60, ScreenH - 112, Fade(SKYBLUE, 0.8f));
    DrawText("HOW TO PLAY", 148, 82, 28, RAYWHITE);
    DrawText("Objective", 58, 132, 18, GOLD);
    DrawText("Survive the 90-second route, defeat the carrier,", 58, 156, 14, RAYWHITE);
    DrawText("then loop into a faster, harder stage.", 58, 176, 14, RAYWHITE);

    DrawText("Controls", 58, 218, 18, GOLD);
    DrawText("Move: Arrow keys / WASD | Gamepad Left Stick", 58, 242, 14, RAYWHITE);
    DrawText("Shoot: Hold Z / Space | Gamepad Down Face Button", 58, 264, 14, RAYWHITE);
    DrawText("Bomb: X | Gamepad Right Face Button", 58, 286, 14, RAYWHITE);
    DrawText("Pause: P | Gamepad Menu button", 58, 308, 14, RAYWHITE);

    DrawText("Pickups", 58, 350, 18, GOLD);
    DrawText("W changes weapon, P upgrades, B adds bomb,", 58, 374, 14, RAYWHITE);
    DrawText("medals add score. Your red core is the hitbox.", 58, 394, 14, RAYWHITE);

    DrawText("Tips", 58, 436, 18, GOLD);
    DrawText("Hold fire, tap bombs before panic, and use focus", 58, 460, 14, RAYWHITE);
    DrawText("movement (Shift / Gamepad L1/R1) when lanes tighten.", 58, 480, 14, RAYWHITE);
    DrawText("ENTER / ESC / BACKSPACE returns", 124, 548, 15, SKYBLUE);
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
    
    int promptW = MeasureText("ENTER / ESC / BACKSPACE to return", 14);
    DrawText("ENTER / ESC / BACKSPACE to return", ScreenW / 2 - promptW / 2, 532, 14, SKYBLUE);
}

void Game::DrawSettings() const {
    DrawRectangle(30, 56, ScreenW - 60, ScreenH - 112, Fade(BLACK, 0.85f));
    DrawRectangleLines(30, 56, ScreenW - 60, ScreenH - 112, Fade(SKYBLUE, 0.8f));
    
    int titleW = MeasureText("USER SETTINGS", 24);
    DrawText("USER SETTINGS", ScreenW / 2 - titleW / 2, 85, 24, GOLD);

    const char* options[6] = {
        "SFX Volume",
        "Music Volume",
        "Screen Shake",
        "Hitboxes",
        "Fullscreen",
        "Save and Return"
    };

    for (int i = 0; i < 6; ++i) {
        int y = 160 + i * 48;
        bool selected = settingsSelection_ == i;
        
        if (selected) {
            DrawRectangle(50, y - 8, ScreenW - 100, 32, Fade(BLUE, 0.35f));
            DrawText(">", 58, y, 16, GOLD);
        }
        
        DrawText(options[i], 78, y, 16, selected ? GOLD : RAYWHITE);
        
        if (i == 0) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "[ %d / 10 ]", sfxVolume_);
            DrawText(buf, 280, y, 16, selected ? GOLD : SKYBLUE);
        } else if (i == 1) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "[ %d / 10 ]", bgmVolume_);
            DrawText(buf, 280, y, 16, selected ? GOLD : SKYBLUE);
        } else if (i == 2) {
            DrawText(screenShakeEnabled_ ? "ON" : "OFF", 280, y, 16, screenShakeEnabled_ ? LIME : RED);
        } else if (i == 3) {
            DrawText(hitboxOverlayEnabled_ ? "ON" : "OFF", 280, y, 16, hitboxOverlayEnabled_ ? LIME : RED);
        } else if (i == 4) {
            DrawText(isFullscreen_ ? "ON" : "OFF", 280, y, 16, isFullscreen_ ? LIME : RED);
        }
    }

    int promptW = MeasureText("UP/DOWN navigate  |  LEFT/RIGHT adjust", 13);
    DrawText("UP/DOWN navigate  |  LEFT/RIGHT adjust", ScreenW / 2 - promptW / 2, 532, 13, GRAY);
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
    }
    
    int tip1W = MeasureText("UP/DOWN/Direct Key choose char", 14);
    DrawText("UP/DOWN/Direct Key choose char", ScreenW / 2 - tip1W / 2, 388, 14, GRAY);
    
    int tip2W = MeasureText("LEFT/RIGHT move cursor | ENTER confirm", 14);
    DrawText("LEFT/RIGHT move cursor | ENTER confirm", ScreenW / 2 - tip2W / 2, 412, 14, GRAY);
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
    for (const auto& b : enemyBullets_) b.Draw(debug_);
    player_.Draw(debug_ && state_ != State::Title);
    effects_.Draw();
    EndMode2D();

    DrawHud();

    if (state_ == State::Playing && bossWarningTimer_ > 0.0f) {
        if ((int)(bossWarningTimer_ * 8) % 2 == 0) {
            DrawRectangleLines(10, 10, ScreenW - 20, ScreenH - 20, RED);
            DrawRectangle(10, 10, ScreenW - 20, ScreenH - 20, Fade(RED, 0.06f));
        }
        
        DrawRectangle(0, 180, ScreenW, 60, Fade(BLACK, 0.85f));
        DrawRectangleLines(0, 180, ScreenW, 60, RED);
        
        Color warnColor = (int)(bossWarningTimer_ * 5) % 2 == 0 ? RED : YELLOW;
        int warnW = MeasureText("WARNING: HOSTILE CARRIER DETECTED", 16);
        DrawText("WARNING: HOSTILE CARRIER DETECTED", ScreenW / 2 - warnW / 2, 202, 16, warnColor);
    }
    if (state_ == State::Title) {
        DrawTitleMenu();
    } else if (state_ == State::HowTo) {
        DrawHowTo();
    } else if (state_ == State::HighScores) {
        DrawHighScores();
    } else if (state_ == State::Settings) {
        DrawSettings();
    } else if (state_ == State::NameEntry) {
        DrawNameEntry();
    } else if (state_ == State::Paused) {
        DrawRectangle(0, 0, ScreenW, ScreenH, Fade(BLACK, 0.52f));
        DrawCenteredText("PAUSED", "P / Action A: resume  |  H / Action Y: controls  |  Q / Action B: exit");
    } else if (state_ == State::StageClear) {
        DrawRectangle(0, 0, ScreenW, ScreenH, Fade(BLACK, 0.35f));
        DrawCenteredText("STAGE CLEAR", TextFormat("Loop %d complete - next wave incoming", loop_));
    } else if (state_ == State::GameOver) {
        DrawRectangle(0, 0, ScreenW, ScreenH, Fade(BLACK, 0.62f));
        DrawCenteredText("GAME OVER", "Press ENTER / Action A for title");
    }
    
    if (transitioning_) {
        DrawTransition();
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
    
    FILE* f = std::fopen("highscores.txt", "r");
    if (f) {
        HighScoreEntry entry;
        while (std::fscanf(f, "%3s %d %d\n", entry.initials, &entry.score, &entry.loop) == 3) {
            highScores_.push_back(entry);
        }
        std::fclose(f);
    }
    
    while (highScores_.size() < 5) {
        highScores_.push_back(defaults[highScores_.size()]);
    }
    
    std::sort(highScores_.begin(), highScores_.end(), [](const HighScoreEntry& a, const HighScoreEntry& b) {
        return a.score > b.score;
    });
}

void Game::SaveHighScores() {
    FILE* f = std::fopen("highscores.txt", "w");
    if (f) {
        for (const auto& entry : highScores_) {
            std::fprintf(f, "%s %d %d\n", entry.initials, entry.score, entry.loop);
        }
        std::fclose(f);
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

void Game::LoadSettings() {
    FILE* f = std::fopen("settings.cfg", "r");
    if (f) {
        std::fscanf(f, "sfxVol=%d\n", &sfxVolume_);
        std::fscanf(f, "bgmVol=%d\n", &bgmVolume_);
        int shake = 1, hitbox = 0, full = 0;
        std::fscanf(f, "shake=%d\n", &shake);
        std::fscanf(f, "hitbox=%d\n", &hitbox);
        std::fscanf(f, "fullscreen=%d\n", &full);
        screenShakeEnabled_ = (shake != 0);
        hitboxOverlayEnabled_ = (hitbox != 0);
        isFullscreen_ = (full != 0);
        std::fclose(f);
    } else {
        sfxVolume_ = 8;
        bgmVolume_ = 6;
        screenShakeEnabled_ = true;
        hitboxOverlayEnabled_ = false;
        isFullscreen_ = false;
    }
    
    debug_ = hitboxOverlayEnabled_;
    
    if (isFullscreen_ && !IsWindowFullscreen()) {
        ToggleFullscreen();
    }
    
    float sVol = (float)sfxVolume_ / 10.0f;
    float mVol = (float)bgmVolume_ / 10.0f;
    
    if (soundsReady_) {
        SetSoundVolume(shootSound_, sVol);
        SetSoundVolume(boomSound_, sVol * 0.8f);
        SetSoundVolume(pickupSound_, sVol);
        SetSoundVolume(menuSelectSound_, sVol * 0.7f);
        SetSoundVolume(menuConfirmSound_, sVol * 0.8f);
        SetSoundVolume(playerHitSound_, sVol * 0.9f);
        SetSoundVolume(gameOverSound_, sVol * 0.9f);
        SetSoundVolume(stageClearSound_, sVol * 0.8f);
        SetSoundVolume(bgmSound_, mVol * 0.5f);
        SetSoundVolume(bossKlaxonSound_, sVol * 0.7f);
    }
}

void Game::SaveSettings() {
    FILE* f = std::fopen("settings.cfg", "w");
    if (f) {
        std::fprintf(f, "sfxVol=%d\n", sfxVolume_);
        std::fprintf(f, "bgmVol=%d\n", bgmVolume_);
        std::fprintf(f, "shake=%d\n", screenShakeEnabled_ ? 1 : 0);
        std::fprintf(f, "hitbox=%d\n", hitboxOverlayEnabled_ ? 1 : 0);
        std::fprintf(f, "fullscreen=%d\n", isFullscreen_ ? 1 : 0);
        std::fclose(f);
    }
}

void Game::StartTransition(State target) {
    if (transitioning_) return;
    transitioning_ = true;
    transitionTimer_ = 0.0f;
    transitionTarget_ = target;
    transitionAlpha_ = 0.0f;
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
