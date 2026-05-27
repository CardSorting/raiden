#include "Game.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

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

Game::Game() {
    InitWindow(ScreenW, ScreenH, "Sky Circuit - raylib arcade shooter");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    audio_.Init();
    lastMousePos_ = GetMousePosition();
    
    // Load high scores and settings
    LoadHighScores();
    
    LoadSettings(); // Loads & applies settings to the audio files
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
}

Game::~Game() {
    audio_.Shutdown();
    CloseWindow();
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
    player_.lives = (difficulty_ == 0) ? 3 : 2;
    player_.isDemo = demoMode_;
    player_.controlLayout = controlLayout_;
    playerBullets_.clear(); enemyBullets_.clear(); enemies_.clear(); powerups_.clear();
    effects_.Clear();
    waves_.Reset();
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
    lowLifeAudioTimer_ = 0.0f;
    enemyShotAudioTimer_ = 0.0f;
    medalChainTimer_ = 0.0f;
    medalChain_ = 0;
    nextScoreMilestone_ = 10000;
    nextExtraLifeScore_ = 50000;
    tutorialTimer_ = 4.0f;
    for (int i = 0; i < 10; ++i) formationCount_[i] = 0;
    state_ = State::Playing;
    
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
    waves_.Reset();
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
    lowLifeAudioTimer_ = 0.0f;
    enemyShotAudioTimer_ = 0.0f;
    medalChainTimer_ = 0.0f;
    medalChain_ = 0;
    for (int i = 0; i < 10; ++i) formationCount_[i] = 0;
    state_ = State::Title;
    
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
    lowLifeAudioTimer_ = 0.0f;
    enemyShotAudioTimer_ = 0.0f;
    medalChainTimer_ = 0.0f;
    medalChain_ = 0;
    playerBullets_.clear(); enemyBullets_.clear(); enemies_.clear(); powerups_.clear();
    waves_.Reset();
    player_.bombs = std::min(6, player_.bombs + 1);
    state_ = State::Playing;
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
    if (bossDeathTimer_ > 0.0f) {
        bossDeathTimer_ -= dt;
        bossDeathExplosionTimer_ += dt;
        if (bossDeathExplosionTimer_ > 0.15f) {
            bossDeathExplosionTimer_ = 0.0f;
            Vector2 offset = { (float)GetRandomValue(-40, 40), (float)GetRandomValue(-40, 40) };
            effects_.Explosion({ bossDeathPos_.x + offset.x, bossDeathPos_.y + offset.y }, ORANGE, 14);
            effects_.Shake(5.0f, 0.15f);
            audio_.PlayExplosionAt(ExplosionSize::Small, bossDeathPos_.x + offset.x, (float)ScreenW);
        }
        
        if (bossDeathTimer_ <= 0.0f) {
            effects_.Explosion(bossDeathPos_, VIOLET, 85);
            effects_.Shake(14.0f, 0.6f);
            audio_.PlayExplosionAt(ExplosionSize::Large, bossDeathPos_.x, (float)ScreenW);
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
        
        enemyBullets_.clear();
        return;
    }

    stageTime_ += dt;
    if (tutorialTimer_ > 0.0f) tutorialTimer_ -= dt;
    if (medalChainTimer_ > 0.0f) {
        medalChainTimer_ -= dt;
        if (medalChainTimer_ <= 0.0f) medalChain_ = 0;
    }
    player_.Update(dt, enemies_, enemyBullets_);
    size_t before = playerBullets_.size();
    player_.TryShoot(playerBullets_);
    if (playerBullets_.size() > before) {
        audio_.PlayPlayerShot(player_.weapon);
    }

    bool bombRequested = player_.BombPressed();
    if (player_.TryBomb()) {
        enemyBullets_.clear();
        for (auto& e : enemies_) e.Hit(e.IsBoss() ? 28 : 99);
        effects_.Explosion(player_.pos, SKYBLUE, 58);
        effects_.Shake(10.0f, 0.35f);
        audio_.PlayBomb();
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

    waves_.Update(stageTime_, loop_ + (difficulty_ == 1 ? 1 : 0), enemies_);
    if (!bossSpawned_ && waves_.ShouldSpawnBoss(stageTime_, BossAlive())) {
        enemies_.emplace_back(EnemyType::Miniboss, Vector2{240, -70}, loop_ + (difficulty_ == 1 ? 1 : 0), 0);
        bossSpawned_ = true;
        enemyBullets_.clear();
        
        bossWarningTimer_ = 3.0f;
        bossWarningKlaxonTimer_ = 0.8f;
        audio_.PlayMusic(AudioSystem::MusicTrack::Boss);
        audio_.PlayBossEntrance();
        audio_.PlayBossWarning();
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
    size_t beforeBullets = enemyBullets_.size();
    for (auto& e : enemies_) e.Update(dt, player_.pos, enemyBullets_, loop_ + (difficulty_ == 1 ? 1 : 0));
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

    HandleCollisions();
    CheckScoreMilestones();
    Cleanup();

    for (const auto& e : enemies_) {
        if (e.active && e.IsBoss() && !bossPhaseChanged_ && e.hp < e.maxHp / 2) {
            bossPhaseChanged_ = true;
            audio_.PlayBossPhaseChange();
            break;
        }
    }

    if (bossSpawned_ && !BossAlive() && state_ == State::Playing) {
        bossDeathTimer_ = 1.6f;
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
        effects_.AddText(bossDeathPos_, "BOSS DEFEATED!", GOLD);
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
    
    if (lastInputType_ == InputType::Mouse && mouseMoved) {
        for (int i = 0; i < 10; ++i) {
            int y = 145 + i * 30;
            if (mousePos.x >= 50 && mousePos.x <= 430 && mousePos.y >= y - 6 && mousePos.y <= y + 20) {
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
        for (int i = 0; i < 3; ++i) {
            int y = 145 + i * 30;
            if (mousePos.x >= 270 && mousePos.x <= 390 && mousePos.y >= y - 10 && mousePos.y <= y + 20) {
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
                }
            }
        }
    }
    
    // Mouse clicks
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (int i = 0; i < 10; ++i) {
            int y = 145 + i * 30;
            if (mousePos.x >= 50 && mousePos.x <= 430 && mousePos.y >= y - 6 && mousePos.y <= y + 20) {
                settingsSelection_ = i;
                keyConfirm = true;
            }
        }
    }
    
    if (keyUp) {
        settingsSelection_ = (settingsSelection_ + 9) % 10;
        audio_.PlayMenuMove();
    }
    if (keyDown) {
        settingsSelection_ = (settingsSelection_ + 1) % 10;
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
            isFullscreen_ = !isFullscreen_;
            ToggleFullscreen();
            changed = true;
            audio_.PlayMenuConfirm();
        }
    } else if (settingsSelection_ == 6) {
        if (keyLeft || keyRight || keyConfirm) {
            controlLayout_ = (controlLayout_ + 1) % 2;
            player_.controlLayout = controlLayout_;
            changed = true;
            audio_.PlayMenuConfirm();
        }
    } else if (settingsSelection_ == 7) {
        if (keyConfirm) {
            ResetSettingsToDefault();
            audio_.PlayMenuConfirm();
            settingsFeedbackTimer_ = 2.0f;
            settingsFeedbackText_ = "SETTINGS RESET!";
            settingsFeedbackColor_ = SKYBLUE;
        }
    } else if (settingsSelection_ == 8) {
        if (keyConfirm) {
            clearScoresSelection_ = 0;
            StartTransition(State::ClearScoresConfirm);
        }
    } else if (settingsSelection_ == 9) {
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

void Game::HandleCollisions() {
    for (auto& b : playerBullets_) {
        if (!b.active) continue;
        for (auto& e : enemies_) {
            if (!e.active || !CircleHit(b.pos, b.radius, e.pos, e.radius)) continue;
            b.active = false;
            e.Hit(b.damage);
            effects_.Spark(b.pos, YELLOW);
            if (!e.active) {
                int scoreAdded = e.scoreValue;
                if (difficulty_ == 1) scoreAdded = (scoreAdded * 3) / 2;
                player_.score += scoreAdded;
                
                char scoreText[16];
                std::snprintf(scoreText, sizeof(scoreText), "+%d", scoreAdded);
                effects_.AddText(e.pos, scoreText, YELLOW);

                effects_.Explosion(e.pos, e.IsBoss() ? VIOLET : ORANGE, e.IsBoss() ? 80 : 22);
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

                if (e.formationId > 0 && e.formationId < 10) {
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
                effects_.Explosion(player_.pos, RED, 38);
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
            effects_.Explosion(player_.pos, RED, 38);
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
        float lx = 145 + i * 16;
        float ly = 22;
        DrawTriangle({lx, ly - 6}, {lx - 5, ly + 4}, {lx + 5, ly + 4}, SKYBLUE);
        DrawCircleV({lx, ly + 1}, 2.0f, RED);
    }
    DrawText("LIVES", 145, 4, 10, SKYBLUE);
    
    // BOMBS (Icons)
    int maxDrawBombs = std::min(5, player_.bombs);
    for (int i = 0; i < maxDrawBombs; ++i) {
        float bx = 215 + i * 12;
        float by = 22;
        DrawCircleV({bx, by}, 4.0f, PURPLE);
        DrawCircleLines(bx, by, 4.0f, WHITE);
    }
    DrawText("BOMBS", 215, 4, 10, PURPLE);

    // Dynamic STAGE Timer / WARNING
    if (stageTime_ < 90.0f) {
        DrawText(TextFormat("TIME %02d", 90 - (int)stageTime_), 395, 4, 10, GRAY);
        DrawText(TextFormat("LOOP %d", loop_), 395, 16, 12, SKYBLUE);
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
        if (selected) DrawRectangle(120, y - 6, 240, 26, Fade(BLUE, 0.45f));
        DrawText(selected ? ">" : " ", 132, y, 16, selected ? GOLD : GRAY);
        
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
        DrawText(items[i], 158, y, 16, textCol);
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

    const char* options[10] = {
        "Master Volume",
        "SFX Volume",
        "Music Volume",
        "Screen Shake",
        "Hitboxes",
        "Fullscreen",
        "Control Layout",
        "Reset to Defaults",
        "Clear High Scores",
        "Save and Return"
    };

    for (int i = 0; i < 10; ++i) {
        int y = 145 + i * 30;
        bool selected = settingsSelection_ == i;
        
        if (selected) {
            DrawRectangle(50, y - 6, ScreenW - 100, 26, Fade(BLUE, 0.35f));
            DrawText(">", 58, y, 15, GOLD);
        }
        
        DrawText(options[i], 78, y, 15, selected ? GOLD : RAYWHITE);
        
        if (i == 0) {
            DrawLine(280, y + 7, 380, y + 7, GRAY);
            DrawLine(280, y + 7, 280 + masterVolume_ * 10, y + 7, SKYBLUE);
            DrawCircle(280 + masterVolume_ * 10, y + 7, 5, selected ? GOLD : WHITE);
            DrawText(TextFormat("%d", masterVolume_), 395, y, 15, selected ? GOLD : SKYBLUE);
        } else if (i == 1) {
            DrawLine(280, y + 7, 380, y + 7, GRAY);
            DrawLine(280, y + 7, 280 + sfxVolume_ * 10, y + 7, SKYBLUE);
            DrawCircle(280 + sfxVolume_ * 10, y + 7, 5, selected ? GOLD : WHITE);
            DrawText(TextFormat("%d", sfxVolume_), 395, y, 15, selected ? GOLD : SKYBLUE);
        } else if (i == 2) {
            DrawLine(280, y + 7, 380, y + 7, GRAY);
            DrawLine(280, y + 7, 280 + bgmVolume_ * 10, y + 7, SKYBLUE);
            DrawCircle(280 + bgmVolume_ * 10, y + 7, 5, selected ? GOLD : WHITE);
            DrawText(TextFormat("%d", bgmVolume_), 395, y, 15, selected ? GOLD : SKYBLUE);
        } else if (i == 3) {
            DrawText(screenShakeEnabled_ ? "ON" : "OFF", 280, y, 15, screenShakeEnabled_ ? LIME : RED);
        } else if (i == 4) {
            DrawText(hitboxOverlayEnabled_ ? "ON" : "OFF", 280, y, 15, hitboxOverlayEnabled_ ? LIME : RED);
            
            // Draw mini player ship preview next to the option (at x = 355)
            Vector2 shipPos = { 355.0f, (float)y + 7.0f };
            float time = (float)GetTime();
            float flameH = 4.0f + std::sin(time * 45.0f) * 1.5f;
            DrawTriangle({shipPos.x - 2, shipPos.y + 5}, {shipPos.x - 3, shipPos.y + 5 + flameH}, {shipPos.x - 1, shipPos.y + 5}, SKYBLUE);
            DrawTriangle({shipPos.x + 2, shipPos.y + 5}, {shipPos.x + 1, shipPos.y + 5}, {shipPos.x + 3, shipPos.y + 5 + flameH}, SKYBLUE);
            DrawTriangle({shipPos.x, shipPos.y - 4}, {shipPos.x - 6, shipPos.y + 2}, {shipPos.x - 2, shipPos.y + 2}, GRAY);
            DrawTriangle({shipPos.x, shipPos.y - 4}, {shipPos.x + 2, shipPos.y + 2}, {shipPos.x + 6, shipPos.y + 2}, GRAY);
            DrawTriangle({shipPos.x, shipPos.y - 8}, {shipPos.x - 4, shipPos.y + 5}, {shipPos.x + 4, shipPos.y + 5}, RAYWHITE);
            
            if (hitboxOverlayEnabled_ || selected) {
                float corePulse = 1.5f + 1.0f * std::sin(time * 15.0f);
                DrawCircleV(shipPos, corePulse, RED);
                DrawCircleLines((int)shipPos.x, (int)shipPos.y, (int)(corePulse + 1.5f), Fade(RED, 0.6f));
            } else {
                DrawCircleV(shipPos, 1.5f, DARKGRAY);
            }
        } else if (i == 5) {
            DrawText(isFullscreen_ ? "ON" : "OFF", 280, y, 15, isFullscreen_ ? LIME : RED);
        } else if (i == 6) {
            DrawText(controlLayout_ == 0 ? "ARROWS" : "WASD", 280, y, 15, controlLayout_ == 0 ? SKYBLUE : GOLD);
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
            if (selected) DrawRectangle(120, y - 6, 240, 26, Fade(BLUE, 0.45f));
            DrawText(selected ? ">" : " ", 132, y, 16, selected ? GOLD : GRAY);
            DrawText(pItems[i], 158, y, 16, selected ? GOLD : RAYWHITE);
        }
        
        int tipW = MeasureText("UP/DOWN navigate  |  ENTER/CLICK confirm", 12);
        DrawText("UP/DOWN navigate  |  ENTER/CLICK confirm", ScreenW / 2 - tipW / 2, 442, 12, GRAY);
    } else if (state_ == State::StageClear) {
        DrawRectangle(0, 0, ScreenW, ScreenH, Fade(BLACK, 0.65f));
        DrawRectangle(30, 100, ScreenW - 60, ScreenH - 200, Fade(BLACK, 0.85f));
        DrawRectangleLines(30, 100, ScreenW - 60, ScreenH - 200, Fade(GOLD, 0.8f));
        
        int scTitleW = MeasureText("STAGE CLEAR!", 32);
        DrawText("STAGE CLEAR!", ScreenW / 2 - scTitleW / 2, 140, 32, GOLD);
        DrawLine(50, 190, ScreenW - 50, 190, Fade(GOLD, 0.4f));
        
        int baseBonus = 2000;
        int livesBonus = player_.lives * 500;
        int bombsBonus = player_.bombs * 200;
        int totalBonus = stageClearBonus_ > 0 ? stageClearBonus_ : (baseBonus + livesBonus + bombsBonus) * loop_;
        int shownTotalBonus = totalBonus;
        if (clearTimer_ < 3.45f) {
            float reveal = std::clamp((clearTimer_ - 0.92f) / 2.53f, 0.0f, 1.0f);
            shownTotalBonus = (int)((float)totalBonus * reveal);
        }
        
        DrawText("MISSION PERFORMANCE RESULTS", 60, 220, 16, SKYBLUE);
        
        DrawText("Base Clear Bonus:", 60, 260, 16, RAYWHITE);
        DrawText(TextFormat("+%d", baseBonus), 320, 260, 16, LIME);
        
        DrawText(TextFormat("Survivor Lives (%d):", player_.lives), 60, 300, 16, RAYWHITE);
        DrawText(TextFormat("+%d", livesBonus), 320, 300, 16, LIME);
        
        DrawText(TextFormat("Remaining Bombs (%d):", player_.bombs), 60, 340, 16, RAYWHITE);
        DrawText(TextFormat("+%d", bombsBonus), 320, 340, 16, LIME);
        
        DrawLine(60, 385, ScreenW - 60, 385, Fade(GRAY, 0.5f));
        
        DrawText("TOTAL STAGE BONUS:", 60, 410, 18, GOLD);
        DrawText(TextFormat("+%d", shownTotalBonus), 320, 410, 18, GOLD);
        
        int loopW = MeasureText(TextFormat("ADVANCING TO LOOP %d", loop_ + 1), 16);
        DrawText(TextFormat("ADVANCING TO LOOP %d", loop_ + 1), ScreenW / 2 - loopW / 2, 470, 16, SKYBLUE);
        
        int promptW = MeasureText("Press ENTER / Action A to continue", 14);
        DrawText("Press ENTER / Action A to continue", ScreenW / 2 - promptW / 2, 505, 14, RAYWHITE);
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
            if (selected) DrawRectangle(120, y - 6, 240, 26, Fade(RED, 0.25f));
            DrawText(selected ? ">" : " ", 132, y, 16, selected ? GOLD : GRAY);
            
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
            DrawText(label, 158, y, 16, textCol);
        }
        
        int tipW = MeasureText("UP/DOWN navigate  |  ENTER/CLICK confirm", 12);
        DrawText("UP/DOWN navigate  |  ENTER/CLICK confirm", ScreenW / 2 - tipW / 2, 412, 12, GRAY);
    }
    
    if (transitioning_) {
        DrawTransition();
    }

    // --- Global CRT Arcade Monitor Filter Pass ---
    // 1. CRT Scanlines simulation
    for (int y = 0; y < ScreenH; y += 2) {
        DrawLine(0, y, ScreenW, y, Fade(BLACK, 0.08f));
    }
    
    // 2. Curved glass Vignette screen-space shading
    DrawCircleGradient(ScreenW / 2, ScreenH / 2, ScreenH * 0.8f, Fade(WHITE, 0.0f), Fade(BLACK, 0.38f));
    
    // 3. Screen tube subtle reflections (top and left glass edges)
    DrawLine(8, 8, ScreenW - 8, 8, Fade(WHITE, 0.05f));
    DrawLine(8, 8, 8, ScreenH - 8, Fade(WHITE, 0.05f));
    
    // 4. Cabinet Bezels / Vertical Monitor frame
    DrawRectangleLinesEx(Rectangle{ 0.0f, 0.0f, (float)ScreenW, (float)ScreenH }, 8.0f, DARKGRAY);
    DrawRectangleLinesEx(Rectangle{ 8.0f, 8.0f, (float)ScreenW - 16.0f, (float)ScreenH - 16.0f }, 2.0f, BLACK);
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
