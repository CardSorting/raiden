#include "AudioSystem.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <string>
#include <thread>

namespace {
constexpr int SampleRate = 44100;
constexpr float Pi2 = 6.28318530718f;

struct ExportCueSpec {
    AudioSystem::Cue cue;
    const char* name;
    int variants;
};

struct ExportMusicSpec {
    AudioSystem::MusicTrack track;
    const char* name;
};

constexpr ExportCueSpec ExportCues[] = {
    { AudioSystem::Cue::VulcanShot, "player_vulcan_shot", 8 },
    { AudioSystem::Cue::PlasmaShot, "player_plasma_shot", 5 },
    { AudioSystem::Cue::MissileLaunch, "player_missile_launch", 4 },
    { AudioSystem::Cue::EnemyBullet, "enemy_bullet", 6 },
    { AudioSystem::Cue::EnemyStrongShot, "enemy_strong_shot", 4 },
    { AudioSystem::Cue::ExplosionSmall, "explosion_small", 8 },
    { AudioSystem::Cue::ExplosionMedium, "explosion_medium", 5 },
    { AudioSystem::Cue::ExplosionLarge, "explosion_large", 3 },
    { AudioSystem::Cue::BossDamage, "boss_damage", 5 },
    { AudioSystem::Cue::BossPhaseChange, "boss_phase_change", 1 },
    { AudioSystem::Cue::BossDefeat, "boss_defeat", 1 },
    { AudioSystem::Cue::PlayerHit, "player_hit", 1 },
    { AudioSystem::Cue::PlayerDeath, "player_death", 1 },
    { AudioSystem::Cue::Respawn, "player_respawn", 1 },
    { AudioSystem::Cue::Bomb, "bomb", 1 },
    { AudioSystem::Cue::BombClear, "bomb_clear", 1 },
    { AudioSystem::Cue::PickupPowerup, "pickup_powerup", 3 },
    { AudioSystem::Cue::PickupWeaponSwitch, "pickup_weapon_switch", 2 },
    { AudioSystem::Cue::PickupWeaponUpgrade, "pickup_weapon_upgrade", 1 },
    { AudioSystem::Cue::PickupBomb, "pickup_bomb", 1 },
    { AudioSystem::Cue::PickupMedal, "pickup_medal", 4 },
    { AudioSystem::Cue::ScoreMilestone, "score_milestone", 1 },
    { AudioSystem::Cue::FormationBonus, "formation_bonus", 1 },
    { AudioSystem::Cue::ExtraLife, "extra_life", 1 },
    { AudioSystem::Cue::MenuMove, "menu_move", 4 },
    { AudioSystem::Cue::MenuConfirm, "menu_confirm", 2 },
    { AudioSystem::Cue::MenuCancel, "menu_cancel", 2 },
    { AudioSystem::Cue::InsertCoin, "insert_coin", 1 },
    { AudioSystem::Cue::PressStart, "press_start", 1 },
    { AudioSystem::Cue::Pause, "pause", 1 },
    { AudioSystem::Cue::Resume, "resume", 1 },
    { AudioSystem::Cue::ContinueTick, "continue_tick", 1 },
    { AudioSystem::Cue::BonusTick, "bonus_tick", 4 },
    { AudioSystem::Cue::HighScoreTick, "high_score_tick", 2 },
    { AudioSystem::Cue::HighScoreEntry, "high_score_entry", 1 },
    { AudioSystem::Cue::StageStart, "stage_start", 1 },
    { AudioSystem::Cue::StageClear, "stage_clear", 1 },
    { AudioSystem::Cue::GameOver, "game_over", 1 },
    { AudioSystem::Cue::BossWarning, "boss_warning", 2 },
    { AudioSystem::Cue::BossEntrance, "boss_entrance", 1 },
    { AudioSystem::Cue::LowLife, "low_life_warning", 2 },
    { AudioSystem::Cue::Victory, "victory", 1 },
    { AudioSystem::Cue::AttractShimmer, "attract_shimmer", 1 },
    { AudioSystem::Cue::Denied, "denied", 2 },
};

constexpr ExportMusicSpec ExportMusic[] = {
    { AudioSystem::MusicTrack::Title, "music_title_loop" },
    { AudioSystem::MusicTrack::Stage, "music_stage_loop" },
    { AudioSystem::MusicTrack::Boss, "music_boss_loop" },
};

double AudioClockSeconds() {
    using Clock = std::chrono::steady_clock;
    static const Clock::time_point start = Clock::now();
    std::chrono::duration<double> elapsed = Clock::now() - start;
    return elapsed.count();
}

float OscSine(float phase) {
    return std::sin(phase * Pi2);
}

float OscSquare(float phase, float duty = 0.5f) {
    float p = phase - std::floor(phase);
    return p < duty ? 1.0f : -1.0f;
}

float OscTriangle(float phase) {
    float p = phase - std::floor(phase);
    return 1.0f - 4.0f * std::abs(p - 0.5f);
}

float OscSaw(float phase) {
    float p = phase - std::floor(phase);
    return 2.0f * p - 1.0f;
}

float Noise(unsigned int& seed) {
    seed = seed * 1664525u + 1013904223u;
    return ((float)((seed >> 8) & 0xFFFFu) / 32768.0f) - 1.0f;
}

// Pink-ish filtered noise for heavier explosions and electrical texture.
float PinkNoise(unsigned int& seed, float& low, float& mid) {
    float white = Noise(seed);
    low += (white - low) * 0.025f;
    mid += (white - mid) * 0.16f;
    return low * 0.58f + mid * 0.30f + white * 0.12f;
}

float MidiNote(int note) {
    return 440.0f * std::pow(2.0f, (float)(note - 69) / 12.0f);
}

float Adsr(float t, float duration, float attack, float decay, float sustain, float release) {
    attack = std::max(attack, 0.0001f);
    decay = std::max(decay, 0.0001f);
    release = std::max(release, 0.0001f);
    if (t < attack) return t / attack;
    if (t < attack + decay) {
        float k = (t - attack) / decay;
        return 1.0f + (sustain - 1.0f) * k;
    }
    float releaseStart = std::max(attack + decay, duration - release);
    if (t < releaseStart) return sustain;
    return std::max(0.0f, sustain * (1.0f - (t - releaseStart) / release));
}

float PercEnv(float t, float duration, float attack, float curve) {
    if (t < attack) return t / std::max(attack, 0.0001f);
    float k = std::clamp((t - attack) / std::max(duration - attack, 0.0001f), 0.0f, 1.0f);
    return std::pow(1.0f - k, curve);
}

float SoftLimit(float x) {
    x *= 1.35f;
    return x / (1.0f + std::abs(x) * 0.55f);
}

float MixLayers(std::initializer_list<float> layers) {
    float sum = 0.0f;
    for (float layer : layers) sum += layer;
    return sum;
}

float PitchSweep(float startHz, float endHz, float k, float curve = 1.0f) {
    return startHz + (endHz - startHz) * std::pow(std::clamp(k, 0.0f, 1.0f), curve);
}

float Vibrato(float hz, float t, float depth, float rate) {
    return hz * (1.0f + depth * std::sin(t * Pi2 * rate));
}

float Tremolo(float t, float depth, float rate) {
    return 1.0f - depth * (0.5f + 0.5f * std::sin(t * Pi2 * rate));
}

float Bitcrush(float s, float levels) {
    return std::round(s * levels) / levels;
}

float CabinetDrive(float x, float drive, float crushMix = 0.0f) {
    float driven = std::tanh(x * drive);
    if (crushMix > 0.0f) {
        driven = driven * (1.0f - crushMix) + Bitcrush(driven, 28.0f) * crushMix;
    }
    return driven;
}

struct OnePoleLowPass {
    float z = 0.0f;
    float Process(float input, float cutoffHz) {
        float a = std::clamp(Pi2 * cutoffHz / (Pi2 * cutoffHz + (float)SampleRate), 0.0f, 1.0f);
        z += (input - z) * a;
        return z;
    }
};

struct OnePoleHighPass {
    OnePoleLowPass lp;
    float Process(float input, float cutoffHz) {
        return input - lp.Process(input, cutoffHz);
    }
};

void AddEcho(std::vector<float>& samples, float delaySeconds, float feedback, float wet) {
    int delay = std::max(1, (int)(delaySeconds * (float)SampleRate));
    for (int i = delay; i < (int)samples.size(); ++i) {
        samples[(size_t)i] += samples[(size_t)(i - delay)] * wet;
        samples[(size_t)i] = SoftLimit(samples[(size_t)i] + samples[(size_t)(i - delay)] * feedback * 0.12f);
    }
}

void Normalize(std::vector<float>& samples, float ceiling = 0.92f) {
    float peak = 0.0001f;
    for (float s : samples) peak = std::max(peak, std::abs(s));
    float gain = std::min(ceiling / peak, 1.6f);
    for (float& s : samples) s *= gain;
}

void FadeLoopBoundary(std::vector<float>& samples, int fadeFrames) {
    if ((int)samples.size() < fadeFrames * 2) return;
    for (int i = 0; i < fadeFrames; ++i) {
        float k = (float)i / (float)std::max(1, fadeFrames - 1);
        float in = k * k * (3.0f - 2.0f * k);
        float out = 1.0f - in;
        samples[(size_t)i] *= in;
        samples[samples.size() - 1u - (size_t)i] *= out;
    }
}

Wave MakeWave(float duration, const std::function<float(float, int)>& sampleFn) {
    int frames = std::max(1, (int)(duration * (float)SampleRate));
    int16_t* samples = (int16_t*)std::malloc(sizeof(int16_t) * frames);
    if (!samples) return {};

    std::vector<float> mix((size_t)frames);
    float dc = 0.0f;
    for (int i = 0; i < frames; ++i) {
        float t = (float)i / (float)SampleRate;
        float s = sampleFn(t, i);
        dc += (s - dc) * 0.0025f;
        s -= dc;
        mix[(size_t)i] = s;
    }
    Normalize(mix);
    for (int i = 0; i < frames; ++i) {
        float s = SoftLimit(mix[(size_t)i]);
        samples[i] = (int16_t)(std::clamp(s, -1.0f, 1.0f) * 30000.0f);
    }

    Wave w{};
    w.frameCount = (unsigned int)frames;
    w.sampleRate = SampleRate;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = samples;
    return w;
}

Wave MakeVulcanShot(int variant) {
    unsigned int seed = 0x1200u + (unsigned int)variant * 97u;
    float phaseA = 0.0f;
    float phaseB = 0.0f;
    float detune = 1.0f + ((float)variant - 2.0f) * 0.007f;
    return MakeWave(0.068f, [=](float t, int) mutable {
        float k = t / 0.068f;
        float env = PercEnv(t, 0.068f, 0.002f, 2.5f);
        float hz = (1850.0f + (650.0f - 1850.0f) * std::pow(k, 0.7f)) * detune;
        phaseA += hz / (float)SampleRate;
        phaseB += hz * 1.997f / (float)SampleRate;
        float click = (k < 0.12f ? (1.0f - k / 0.12f) : 0.0f) * Noise(seed);
        float body = MixLayers({OscSquare(phaseA, 0.38f) * 0.46f, OscSine(phaseB) * 0.18f, click * 0.42f});
        return CabinetDrive(body, 1.55f, 0.08f) * env * 0.66f;
    });
}

Wave MakePlasmaShot(int variant) {
    unsigned int seed = 0x2200u + (unsigned int)variant * 131u;
    float phaseA = 0.0f;
    float phaseB = 0.23f;
    float phaseC = 0.51f;
    float detune = 1.0f + ((float)variant - 1.5f) * 0.012f;
    return MakeWave(0.135f, [=](float t, int) mutable {
        float k = t / 0.135f;
        float env = Adsr(t, 0.135f, 0.006f, 0.032f, 0.64f, 0.065f);
        float hz = (420.0f + (250.0f - 420.0f) * k) * detune;
        phaseA += hz / (float)SampleRate;
        phaseB += hz * 1.51f / (float)SampleRate;
        phaseC += hz * 0.51f / (float)SampleRate;
        float breath = Noise(seed) * std::pow(1.0f - k, 2.2f) * 0.16f;
        float body = MixLayers({OscTriangle(phaseA) * 0.36f, OscSine(phaseB) * 0.22f, OscSaw(phaseC) * 0.14f, breath});
        return CabinetDrive(body, 1.35f, 0.03f) * env * 0.76f;
    });
}

Wave MakeMissileLaunch(int variant) {
    unsigned int seed = 0x3300u + (unsigned int)variant * 173u;
    float thump = 0.0f;
    float sweep = 0.0f;
    return MakeWave(0.24f, [=](float t, int) mutable {
        float k = t / 0.24f;
        float hitEnv = std::exp(-t * 18.0f);
        float sweepEnv = Adsr(t, 0.24f, 0.01f, 0.035f, 0.72f, 0.07f);
        thump += (92.0f + (44.0f - 92.0f) * std::min(k * 3.0f, 1.0f)) / (float)SampleRate;
        sweep += (180.0f + (980.0f - 180.0f) * std::pow(k, 0.75f)) / (float)SampleRate;
        float exhaust = Noise(seed) * std::pow(1.0f - k, 1.4f) * 0.25f;
        float motor = CabinetDrive(OscSaw(sweep) + exhaust * 0.65f, 1.45f, 0.10f) * sweepEnv * 0.34f;
        return OscSine(thump) * hitEnv * 0.88f + motor;
    });
}

Wave MakeEnemyBullet(int variant) {
    float phase = 0.0f;
    float detune = 1.0f + ((float)variant - 2.0f) * 0.01f;
    return MakeWave(0.062f, [=](float t, int) mutable {
        float k = t / 0.062f;
        float env = PercEnv(t, 0.062f, 0.002f, 1.8f);
        float hz = (940.0f + (520.0f - 940.0f) * k) * detune;
        phase += hz / (float)SampleRate;
        return OscTriangle(phase) * env * 0.34f;
    });
}

Wave MakeEnemyStrongShot(int variant) {
    // Sharper threat cue for boss/turret-heavy fire: high-mid edge, short body, no bass mud.
    unsigned int seed = 0x3900u + (unsigned int)variant * 181u;
    float phase = 0.1f;
    float buzz = 0.0f;
    OnePoleHighPass hp;
    return MakeWave(0.105f, [=](float t, int) mutable {
        float k = t / 0.105f;
        float env = PercEnv(t, 0.105f, 0.002f, 1.35f);
        phase += PitchSweep(1320.0f, 740.0f, k, 0.75f) / (float)SampleRate;
        buzz += PitchSweep(2100.0f, 1200.0f, k, 1.2f) / (float)SampleRate;
        float edge = hp.Process(Noise(seed), 1500.0f) * std::pow(1.0f - k, 2.0f) * 0.12f;
        return (OscSquare(phase, 0.28f) * 0.34f + OscSaw(buzz) * 0.16f + edge) * env;
    });
}

Wave MakeExplosionSmall(int variant) {
    unsigned int seed = 0x4400u + (unsigned int)variant * 211u;
    float boom = 0.0f;
    float metal = 0.2f;
    return MakeWave(0.30f, [=](float t, int) mutable {
        float k = t / 0.30f;
        float thumpEnv = std::exp(-t * 12.0f);
        float noiseEnv = std::exp(-t * 9.0f);
        boom += (155.0f + (48.0f - 155.0f) * k) / (float)SampleRate;
        metal += (1900.0f + 420.0f * std::sin(t * 81.0f)) / (float)SampleRate;
        float transient = k < 0.045f ? (1.0f - k / 0.045f) * Noise(seed) : 0.0f;
        float grit = Noise(seed) * noiseEnv * 0.58f;
        float ring = OscSquare(metal, 0.22f) * std::exp(-t * 19.0f) * 0.18f;
        return OscSine(boom) * thumpEnv * 0.56f + CabinetDrive(grit + ring + transient * 0.5f, 1.7f, 0.16f);
    });
}

Wave MakeExplosionMedium(int variant) {
    unsigned int seed = 0x5500u + (unsigned int)variant * 251u;
    float boom = 0.0f;
    float sub = 0.3f;
    float crack = 0.0f;
    return MakeWave(0.54f, [=](float t, int) mutable {
        float k = t / 0.54f;
        boom += (125.0f + (31.0f - 125.0f) * std::pow(k, 0.75f)) / (float)SampleRate;
        sub += (64.0f + (36.0f - 64.0f) * k) / (float)SampleRate;
        crack += (2600.0f + 800.0f * std::sin(t * 53.0f)) / (float)SampleRate;
        float low = OscSine(sub) * std::exp(-t * 4.3f) * 0.62f;
        float mid = OscTriangle(boom) * std::exp(-t * 6.4f) * 0.46f;
        float noise = Noise(seed) * std::exp(-t * 5.8f) * 0.52f;
        float sparks = OscSquare(crack, 0.14f) * std::exp(-t * 13.0f) * 0.18f;
        float hit = k < 0.05f ? (1.0f - k / 0.05f) * Noise(seed) * 0.7f : 0.0f;
        return low + mid + CabinetDrive(noise + sparks + hit, 1.65f, 0.18f);
    });
}

Wave MakeExplosionLarge(int variant) {
    // Boss-scale explosion: bass impact, pink noise body, then delayed metal debris.
    unsigned int seed = 0x5A00u + (unsigned int)variant * 283u;
    float pinkLow = 0.0f;
    float pinkMid = 0.0f;
    float sub = 0.0f;
    float debris = 0.0f;
    OnePoleLowPass lp;
    OnePoleHighPass hp;
    return MakeWave(0.92f, [=](float t, int) mutable {
        float k = t / 0.92f;
        sub += PitchSweep(72.0f, 26.0f, k, 0.55f) / (float)SampleRate;
        debris += (1600.0f + 1300.0f * std::sin(t * 41.0f)) / (float)SampleRate;
        float bass = OscSine(sub) * std::exp(-t * 3.2f) * 0.95f;
        float body = lp.Process(PinkNoise(seed, pinkLow, pinkMid), 1550.0f) * std::exp(-t * 3.5f) * 0.55f;
        float snap = hp.Process(Noise(seed), 2400.0f) * (k < 0.06f ? 1.0f - k / 0.06f : 0.0f) * 0.9f;
        float debrisGate = (t > 0.16f && std::sin(t * Pi2 * 17.0f) > 0.5f) ? 1.0f : 0.0f;
        float metal = OscSquare(debris, 0.12f) * debrisGate * std::exp(-(t - 0.16f) * 4.8f) * 0.22f;
        return bass + CabinetDrive(body + snap + metal, 1.55f, 0.14f);
    });
}

Wave MakeBossDamage(int variant) {
    // Restrained armored hit: low-mid knock plus filtered grit so boss damage reads without flooding the mix.
    unsigned int seed = 0x5B00u + (unsigned int)variant * 313u;
    float tone = 0.0f;
    OnePoleLowPass lp;
    return MakeWave(0.16f, [=](float t, int) mutable {
        float k = t / 0.16f;
        tone += PitchSweep(190.0f, 96.0f, k, 0.9f) / (float)SampleRate;
        float grit = lp.Process(Noise(seed), 900.0f) * std::pow(1.0f - k, 2.2f) * 0.22f;
        return (OscTriangle(tone) * 0.44f + grit) * PercEnv(t, 0.16f, 0.002f, 1.45f);
    });
}

Wave MakeBossPhaseChange() {
    // Phase shift: warning swell into mechanical downshift, designed to cut through music.
    unsigned int seed = 0x5C00u;
    float alarm = 0.0f;
    float servo = 0.4f;
    return MakeWave(1.05f, [=](float t, int) mutable {
        float k = t / 1.05f;
        alarm += Vibrato(PitchSweep(360.0f, 980.0f, k, 1.35f), t, 0.018f, 7.0f) / (float)SampleRate;
        servo += PitchSweep(420.0f, 90.0f, k, 0.65f) / (float)SampleRate;
        float swell = Adsr(t, 1.05f, 0.08f, 0.12f, 0.78f, 0.28f);
        float grind = Bitcrush(OscSaw(servo) + Noise(seed) * 0.16f, 18.0f) * std::exp(-t * 1.4f) * 0.28f;
        return OscSquare(alarm, 0.42f) * swell * 0.32f + grind;
    });
}

Wave MakeBossDefeat() {
    // Multi-stage collapse: failing core, hull break, final low cabinet-rattling hit.
    unsigned int seed = 0x5D00u;
    float core = 0.0f;
    float sub = 0.25f;
    float pinkLow = 0.0f;
    float pinkMid = 0.0f;
    OnePoleHighPass hp;
    return MakeWave(1.95f, [=](float t, int) mutable {
        float k = t / 1.95f;
        core += Vibrato(PitchSweep(680.0f, 72.0f, k, 0.62f), t, 0.035f, 11.0f) / (float)SampleRate;
        sub += PitchSweep(62.0f, 24.0f, k, 0.85f) / (float)SampleRate;
        float gate = std::sin(t * Pi2 * (11.0f + k * 16.0f)) > -0.1f ? 1.0f : 0.28f;
        float coreBreak = OscSquare(core, 0.46f) * gate * Adsr(t, 1.95f, 0.02f, 0.35f, 0.78f, 0.7f) * 0.34f;
        float collapse = PinkNoise(seed, pinkLow, pinkMid) * std::exp(-std::max(0.0f, t - 0.35f) * 2.2f) * (t > 0.25f ? 0.48f : 0.0f);
        float finalHit = OscSine(sub) * std::exp(-std::max(0.0f, t - 1.05f) * 3.4f) * (t > 1.05f ? 0.9f : 0.0f);
        float debris = hp.Process(Noise(seed), 2200.0f) * (t > 0.55f ? std::exp(-(t - 0.55f) * 2.8f) : 0.0f) * 0.2f;
        return coreBreak + collapse + finalHit + debris;
    });
}

Wave MakePlayerDeath() {
    unsigned int seed = 0x6600u;
    float tone = 0.0f;
    float crack = 0.0f;
    return MakeWave(1.18f, [=](float t, int) mutable {
        float k = t / 1.18f;
        float hz = 720.0f + (55.0f - 720.0f) * std::pow(k, 0.62f);
        tone += hz / (float)SampleRate;
        crack += (1800.0f + 900.0f * std::sin(t * 25.0f)) / (float)SampleRate;
        float breakup = (std::sin(t * 95.0f) > -0.2f ? 1.0f : 0.35f);
        float env = Adsr(t, 1.18f, 0.012f, 0.18f, 0.86f, 0.52f);
        float noise = Noise(seed) * (0.25f + k * 0.45f) * env;
        return (OscSquare(tone, 0.44f) * 0.43f + OscSaw(crack) * 0.15f) * env * breakup + noise * 0.48f;
    });
}

Wave MakePlayerHit() {
    // Immediate harsh player damage impact: short, low-mid punch plus clipped static.
    unsigned int seed = 0x6650u;
    float body = 0.0f;
    OnePoleHighPass hp;
    return MakeWave(0.24f, [=](float t, int) mutable {
        float k = t / 0.24f;
        body += PitchSweep(260.0f, 90.0f, k, 0.7f) / (float)SampleRate;
        float staticRip = hp.Process(Noise(seed), 1400.0f) * std::pow(1.0f - k, 2.0f) * 0.35f;
        return (OscSquare(body, 0.36f) * 0.46f + staticRip) * PercEnv(t, 0.24f, 0.001f, 1.5f);
    });
}

Wave MakeRespawn() {
    // Re-entry tone: clean rising lock-on sweep with soft sparkle.
    float tone = 0.0f;
    float shine = 0.2f;
    return MakeWave(0.62f, [=](float t, int) mutable {
        float k = t / 0.62f;
        tone += PitchSweep(260.0f, 880.0f, k, 1.25f) / (float)SampleRate;
        shine += PitchSweep(1040.0f, 1760.0f, k, 1.0f) / (float)SampleRate;
        float env = Adsr(t, 0.62f, 0.035f, 0.08f, 0.66f, 0.2f);
        return OscTriangle(tone) * env * 0.36f + OscSine(shine) * env * 0.14f * Tremolo(t, 0.35f, 8.0f);
    });
}

Wave MakeBomb() {
    unsigned int seed = 0x7700u;
    float bass = 0.0f;
    float riser = 0.0f;
    return MakeWave(1.55f, [=](float t, int) mutable {
        float k = t / 1.55f;
        bass += (58.0f + (30.0f - 58.0f) * std::min(k * 1.7f, 1.0f)) / (float)SampleRate;
        riser += (180.0f + (1600.0f - 180.0f) * std::pow(k, 1.65f)) / (float)SampleRate;
        float pulse = OscSine(bass) * std::exp(-t * 1.8f) * 0.92f;
        float warning = OscSaw(riser) * Adsr(t, 1.55f, 0.04f, 0.18f, 0.6f, 0.35f) * 0.28f;
        float blastStart = std::clamp((k - 0.56f) / (0.72f - 0.56f), 0.0f, 1.0f);
        blastStart = blastStart * blastStart * (3.0f - 2.0f * blastStart);
        float blast = Noise(seed) * blastStart * std::exp(-(t - 0.86f) * 2.1f) * 0.62f;
        return pulse + warning + blast;
    });
}

Wave MakeBombClear() {
    // Screen-clear sweep: wide-feeling rising wash that says bullets were erased.
    unsigned int seed = 0x7750u;
    float sweep = 0.0f;
    float air = 0.0f;
    OnePoleHighPass hp;
    return MakeWave(0.58f, [=](float t, int) mutable {
        float k = t / 0.58f;
        sweep += PitchSweep(160.0f, 1900.0f, k, 0.82f) / (float)SampleRate;
        air += PitchSweep(480.0f, 2600.0f, k, 1.2f) / (float)SampleRate;
        float noise = hp.Process(Noise(seed), 900.0f) * std::pow(1.0f - k, 1.3f) * 0.24f;
        return (OscSaw(sweep) * 0.26f + OscTriangle(air) * 0.18f + noise) * Adsr(t, 0.58f, 0.015f, 0.08f, 0.72f, 0.18f);
    });
}

Wave MakePickupPowerup() {
    float phase = 0.0f;
    constexpr int notes[5] = { 76, 79, 83, 88, 91 };
    return MakeWave(0.36f, [&](float t, int) mutable {
        int idx = std::clamp((int)(t / 0.055f), 0, 4);
        float local = std::fmod(t, 0.055f);
        float env = PercEnv(local, 0.055f, 0.002f, 1.4f);
        phase += MidiNote(notes[idx]) / (float)SampleRate;
        float shimmer = OscSine(phase * 2.01f) * 0.18f;
        return (OscTriangle(phase) * 0.42f + shimmer) * env * 0.72f;
    });
}

Wave MakePickupWeaponSwitch() {
    // Weapon switch: two-tone confirmation, more deliberate than a generic pickup.
    float a = 0.0f;
    return MakeWave(0.22f, [&](float t, int) mutable {
        int note = t < 0.085f ? 67 : 74;
        float local = t < 0.085f ? t : t - 0.085f;
        a += MidiNote(note) / (float)SampleRate;
        return (OscTriangle(a) * 0.42f + OscSquare(a * 0.5f, 0.42f) * 0.1f) * PercEnv(local, 0.13f, 0.003f, 1.1f);
    });
}

Wave MakePickupWeaponUpgrade() {
    // Power gained phrase: ascending arcade arpeggio with a high shimmer tail.
    float phase = 0.0f;
    float glint = 0.3f;
    constexpr int notes[6] = { 72, 76, 79, 84, 88, 91 };
    return MakeWave(0.48f, [&](float t, int) mutable {
        int idx = std::clamp((int)(t / 0.065f), 0, 5);
        float local = std::fmod(t, 0.065f);
        phase += MidiNote(notes[idx]) / (float)SampleRate;
        glint += MidiNote(notes[idx] + 12) / (float)SampleRate;
        return OscSquare(phase, 0.45f) * PercEnv(local, 0.065f, 0.003f, 1.0f) * 0.28f +
               OscSine(glint) * Adsr(t, 0.48f, 0.02f, 0.08f, 0.35f, 0.16f) * 0.16f;
    });
}

Wave MakePickupBomb() {
    // Bomb pickup: reward chime with lower support so it feels heavier than medals.
    float low = 0.0f;
    float high = 0.25f;
    return MakeWave(0.38f, [&](float t, int) mutable {
        int note = t < 0.14f ? 55 : 67;
        low += MidiNote(note) / (float)SampleRate;
        high += MidiNote(note + 12) / (float)SampleRate;
        float env = PercEnv(std::fmod(t, 0.14f), 0.14f, 0.004f, 1.15f) * Adsr(t, 0.38f, 0.004f, 0.06f, 0.75f, 0.16f);
        return OscTriangle(low) * env * 0.34f + OscSine(high) * env * 0.20f;
    });
}

Wave MakePickupMedal() {
    float a = 0.0f;
    float b = 0.17f;
    float c = 0.41f;
    return MakeWave(0.34f, [&](float t, int) mutable {
        float env = PercEnv(t, 0.34f, 0.001f, 1.9f);
        a += 1320.0f / (float)SampleRate;
        b += 1980.0f / (float)SampleRate;
        c += 2640.0f / (float)SampleRate;
        float strike = t < 0.018f ? (1.0f - t / 0.018f) * 0.4f : 0.0f;
        return (OscSine(a) * 0.42f + OscSine(b) * 0.25f + OscTriangle(c) * 0.14f + strike) * env;
    });
}

Wave MakeScoreMilestone() {
    // Short flourish for score thresholds; bright but very brief to avoid stealing combat focus.
    float p = 0.0f;
    constexpr int notes[4] = { 79, 83, 86, 91 };
    return MakeWave(0.31f, [&](float t, int) mutable {
        int idx = std::clamp((int)(t / 0.065f), 0, 3);
        p += MidiNote(notes[idx]) / (float)SampleRate;
        return OscSquare(p, 0.42f) * PercEnv(std::fmod(t, 0.065f), 0.065f, 0.002f, 0.9f) * 0.27f;
    });
}

Wave MakeFormationBonus() {
    // Group-clear reward: mechanical payout plus bright flourish, bigger than a medal but shorter than extra life.
    unsigned int seed = 0xF0B0u;
    float lead = 0.0f;
    float relay = 0.25f;
    constexpr int notes[6] = { 67, 72, 76, 79, 84, 91 };
    return MakeWave(0.56f, [&](float t, int) mutable {
        int idx = std::clamp((int)(t / 0.075f), 0, 5);
        float local = std::fmod(t, 0.075f);
        lead += MidiNote(notes[idx]) / (float)SampleRate;
        relay += PitchSweep(420.0f, 1180.0f, t / 0.56f, 1.2f) / (float)SampleRate;
        float clickGate = (std::sin(t * Pi2 * 23.0f) > 0.45f) ? 1.0f : 0.0f;
        float click = (OscSquare(relay, 0.18f) + Noise(seed) * 0.35f) * clickGate * std::exp(-t * 4.0f) * 0.15f;
        return OscSquare(lead, 0.43f) * PercEnv(local, 0.075f, 0.003f, 0.85f) * 0.30f + click;
    });
}

Wave MakeExtraLife() {
    // Rare extra-life signature: unmistakable, longer major arpeggio with echo baked into the sample.
    float p = 0.0f;
    constexpr int notes[8] = { 72, 76, 79, 84, 88, 91, 96, 103 };
    std::vector<float> raw((size_t)(0.9f * (float)SampleRate));
    for (int i = 0; i < (int)raw.size(); ++i) {
        float t = (float)i / (float)SampleRate;
        int idx = std::clamp((int)(t / 0.085f), 0, 7);
        p += MidiNote(notes[idx]) / (float)SampleRate;
        raw[(size_t)i] = OscSquare(p, 0.45f) * PercEnv(std::fmod(t, 0.085f), 0.085f, 0.003f, 0.75f) * 0.34f;
    }
    AddEcho(raw, 0.115f, 0.35f, 0.32f);
    Normalize(raw);
    int16_t* samples = (int16_t*)std::malloc(sizeof(int16_t) * raw.size());
    if (!samples) return {};
    for (int i = 0; i < (int)raw.size(); ++i) samples[i] = (int16_t)(SoftLimit(raw[(size_t)i]) * 30000.0f);
    Wave w{};
    w.frameCount = (unsigned int)raw.size();
    w.sampleRate = SampleRate;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = samples;
    return w;
}

Wave MakeMenuMove() {
    float phase = 0.0f;
    return MakeWave(0.042f, [&](float t, int) mutable {
        float k = t / 0.042f;
        phase += (980.0f + 260.0f * k) / (float)SampleRate;
        return OscTriangle(phase) * PercEnv(t, 0.042f, 0.001f, 1.7f) * 0.48f;
    });
}

Wave MakeMenuConfirm() {
    float phase = 0.0f;
    return MakeWave(0.17f, [&](float t, int) mutable {
        int note = t < 0.055f ? 72 : (t < 0.105f ? 76 : 79);
        phase += MidiNote(note) / (float)SampleRate;
        return (OscSquare(phase, 0.42f) * 0.26f + OscSine(phase * 2.0f) * 0.14f) * PercEnv(t, 0.17f, 0.003f, 1.25f);
    });
}

Wave MakeMenuCancel() {
    float phase = 0.0f;
    return MakeWave(0.15f, [&](float t, int) mutable {
        float k = t / 0.15f;
        phase += (520.0f + (260.0f - 520.0f) * k) / (float)SampleRate;
        return OscTriangle(phase) * PercEnv(t, 0.15f, 0.004f, 1.6f) * 0.44f;
    });
}

Wave MakeInsertCoin() {
    // Original cabinet credit sound: coin strike, two-note acknowledge, tiny metallic tail.
    float a = 0.0f;
    float ring = 0.5f;
    return MakeWave(0.42f, [&](float t, int) mutable {
        int note = t < 0.08f ? 84 : (t < 0.19f ? 91 : 96);
        a += MidiNote(note) / (float)SampleRate;
        ring += 2100.0f / (float)SampleRate;
        float strike = t < 0.025f ? (1.0f - t / 0.025f) * 0.42f : 0.0f;
        return OscSquare(a, 0.38f) * PercEnv(std::fmod(t, 0.11f), 0.11f, 0.002f, 1.1f) * 0.32f +
               OscSine(ring) * std::exp(-t * 9.0f) * 0.18f + strike;
    });
}

Wave MakePressStart() {
    // Energetic start hit: compact fanfare fragment with cabinet punch.
    float p = 0.0f;
    constexpr int notes[5] = { 67, 72, 76, 79, 84 };
    return MakeWave(0.40f, [&](float t, int) mutable {
        int idx = std::clamp((int)(t / 0.07f), 0, 4);
        p += MidiNote(notes[idx]) / (float)SampleRate;
        return (OscSquare(p, 0.43f) * 0.34f + OscTriangle(p * 0.5f) * 0.16f) *
               PercEnv(std::fmod(t, 0.07f), 0.07f, 0.003f, 0.95f);
    });
}

Wave MakePause() {
    // Muted stop tone: downshift and short damping click.
    float p = 0.0f;
    return MakeWave(0.18f, [&](float t, int) mutable {
        p += PitchSweep(420.0f, 180.0f, t / 0.18f, 0.9f) / (float)SampleRate;
        return OscTriangle(p) * PercEnv(t, 0.18f, 0.002f, 1.2f) * 0.36f;
    });
}

Wave MakeResume() {
    // Quick restart tone: small upward blip that reopens the action.
    float p = 0.0f;
    return MakeWave(0.16f, [&](float t, int) mutable {
        p += PitchSweep(300.0f, 760.0f, t / 0.16f, 1.15f) / (float)SampleRate;
        return OscSquare(p, 0.45f) * PercEnv(t, 0.16f, 0.003f, 1.0f) * 0.30f;
    });
}

Wave MakeContinueTick() {
    // Continue countdown tick: tense, lower than menu ticks, deliberately dry.
    float p = 0.0f;
    return MakeWave(0.075f, [&](float t, int) mutable {
        p += 440.0f / (float)SampleRate;
        return OscSquare(p, 0.30f) * PercEnv(t, 0.075f, 0.001f, 1.4f) * 0.36f;
    });
}

Wave MakeBonusTick(int variant) {
    // Stage-clear payout tick: bright counter relay with a little metal, built to stack quickly.
    unsigned int seed = 0xB07Au + (unsigned int)variant * 73u;
    float tone = 0.0f;
    float bell = 0.23f;
    OnePoleHighPass hp;
    return MakeWave(0.082f, [=](float t, int) mutable {
        float k = t / 0.082f;
        float base = 760.0f + (float)(variant % 4) * 46.0f;
        tone += PitchSweep(base, base * 1.38f, k, 0.7f) / (float)SampleRate;
        bell += (base * 2.01f) / (float)SampleRate;
        float strike = hp.Process(Noise(seed), 1700.0f) * (k < 0.20f ? 1.0f - k / 0.20f : 0.0f) * 0.18f;
        return (OscTriangle(tone) * 0.34f + OscSine(bell) * 0.15f + strike) *
               PercEnv(t, 0.082f, 0.001f, 1.15f);
    });
}

Wave MakeHighScoreTick() {
    // Character entry tick: tiny glassy selector sound.
    float p = 0.0f;
    return MakeWave(0.045f, [&](float t, int) mutable {
        p += 1480.0f / (float)SampleRate;
        return OscSine(p) * PercEnv(t, 0.045f, 0.001f, 1.5f) * 0.34f;
    });
}

Wave MakeHighScoreEntry() {
    // High-score entry sting: a cabinet-proud fanfare that leaves room for initials ticks afterward.
    unsigned int seed = 0x4853u;
    float lead = 0.0f;
    float bass = 0.25f;
    float bell = 0.51f;
    constexpr int notes[9] = { 67, 72, 76, 79, 84, 88, 91, 96, 103 };
    return MakeWave(1.08f, [&](float t, int) mutable {
        int idx = std::clamp((int)(t / 0.095f), 0, 8);
        float local = std::fmod(t, 0.095f);
        float env = PercEnv(local, 0.095f, 0.003f, idx >= 7 ? 0.55f : 0.9f);
        lead += MidiNote(notes[idx]) / (float)SampleRate;
        bass += MidiNote(notes[idx] - 24) / (float)SampleRate;
        bell += MidiNote(notes[idx] + 12) / (float)SampleRate;
        float sparkle = Noise(seed) * std::pow(1.0f - t / 1.08f, 2.0f) * 0.06f;
        return OscSquare(lead, 0.42f) * env * 0.28f +
               OscTriangle(bass) * env * 0.18f +
               OscSine(bell) * Adsr(t, 1.08f, 0.02f, 0.15f, 0.45f, 0.35f) * 0.11f +
               sparkle;
    });
}

Wave MakeStageStart() {
    float phase = 0.0f;
    constexpr int notes[6] = { 60, 64, 67, 72, 76, 79 };
    return MakeWave(0.72f, [&](float t, int) mutable {
        int idx = std::clamp((int)(t / 0.095f), 0, 5);
        float local = std::fmod(t, 0.095f);
        phase += MidiNote(notes[idx]) / (float)SampleRate;
        return (OscSquare(phase, 0.48f) * 0.36f + OscTriangle(phase * 0.5f) * 0.18f) *
               PercEnv(local, 0.095f, 0.004f, 1.1f);
    });
}

Wave MakeStageClear() {
    float lead = 0.0f;
    float harmony = 0.27f;
    constexpr int notes[12] = { 72, 76, 79, 84, 83, 84, 86, 88, 91, 88, 91, 96 };
    return MakeWave(1.42f, [&](float t, int) mutable {
        int idx = std::clamp((int)(t / 0.105f), 0, 11);
        float local = std::fmod(t, 0.105f);
        float env = PercEnv(local, 0.105f, 0.004f, idx >= 9 ? 0.55f : 1.0f);
        lead += MidiNote(notes[idx]) / (float)SampleRate;
        harmony += MidiNote(notes[idx] - 12) / (float)SampleRate;
        return OscSquare(lead, 0.45f) * env * 0.3f + OscTriangle(harmony) * env * 0.18f;
    });
}

Wave MakeVictory() {
    // Bright end-state flourish, distinct from stage clear and meant for rare wins.
    float lead = 0.0f;
    constexpr int notes[10] = { 76, 79, 84, 88, 91, 96, 91, 96, 100, 103 };
    return MakeWave(1.05f, [&](float t, int) mutable {
        int idx = std::clamp((int)(t / 0.095f), 0, 9);
        lead += MidiNote(notes[idx]) / (float)SampleRate;
        return OscSquare(lead, 0.45f) * PercEnv(std::fmod(t, 0.095f), 0.095f, 0.003f, idx > 7 ? 0.55f : 0.9f) * 0.32f;
    });
}

Wave MakeGameOver() {
    float phase = 0.0f;
    constexpr int notes[5] = { 72, 68, 65, 60, 48 };
    return MakeWave(1.55f, [&](float t, int) mutable {
        int idx = std::clamp((int)(t / 0.26f), 0, 4);
        float local = std::fmod(t, 0.26f);
        float env = idx == 4 ? Adsr(local, 0.51f, 0.01f, 0.08f, 0.76f, 0.33f) : PercEnv(local, 0.26f, 0.004f, 0.8f);
        phase += MidiNote(notes[idx]) / (float)SampleRate;
        float wobble = 1.0f + 0.03f * std::sin(t * 32.0f);
        return (OscSquare(phase * wobble, 0.5f) * 0.32f + OscSine(phase * 0.5f) * 0.22f) * env;
    });
}

Wave MakeBossWarning() {
    unsigned int seed = 0x8800u;
    float siren = 0.0f;
    float tension = 0.0f;
    return MakeWave(1.08f, [=](float t, int) mutable {
        float pulse = 0.55f + 0.45f * std::sin(t * Pi2 * 3.0f);
        siren += (390.0f + 220.0f * pulse) / (float)SampleRate;
        tension += (180.0f + 520.0f * (t / 1.08f)) / (float)SampleRate;
        float grit = Noise(seed) * 0.07f * pulse;
        return (OscSaw(siren) * 0.36f + OscSquare(tension, 0.36f) * 0.16f + grit) *
               Adsr(t, 1.08f, 0.025f, 0.08f, 0.82f, 0.14f);
    });
}

Wave MakeBossEntrance() {
    unsigned int seed = 0x9900u;
    float impact = 0.0f;
    float mech = 0.0f;
    return MakeWave(0.92f, [=](float t, int) mutable {
        float k = t / 0.92f;
        impact += (76.0f + (28.0f - 76.0f) * k) / (float)SampleRate;
        mech += (280.0f + (58.0f - 280.0f) * std::pow(k, 0.8f)) / (float)SampleRate;
        float clankGate = (std::sin(t * Pi2 * 11.0f) > 0.35f) ? 1.0f : 0.0f;
        float clank = (OscSquare(mech, 0.18f) + Noise(seed) * 0.55f) * clankGate * std::exp(-t * 2.2f) * 0.24f;
        return OscSine(impact) * std::exp(-t * 4.0f) * 0.9f + OscSaw(mech) * std::exp(-t * 1.5f) * 0.25f + clank;
    });
}

Wave MakeLowLife() {
    float phase = 0.0f;
    return MakeWave(0.09f, [&](float t, int) mutable {
        phase += 650.0f / (float)SampleRate;
        return OscSine(phase) * PercEnv(t, 0.09f, 0.006f, 1.8f) * 0.32f;
    });
}

Wave MakeAttractShimmer() {
    // Subtle arcade attract shimmer: decorative, low-priority, never louder than UI.
    float a = 0.0f;
    float b = 0.31f;
    return MakeWave(0.72f, [&](float t, int) mutable {
        a += Vibrato(880.0f, t, 0.012f, 5.0f) / (float)SampleRate;
        b += 1320.0f / (float)SampleRate;
        float env = Adsr(t, 0.72f, 0.08f, 0.14f, 0.42f, 0.28f);
        return (OscSine(a) * 0.19f + OscTriangle(b) * 0.12f) * env * Tremolo(t, 0.45f, 6.0f);
    });
}

Wave MakeDenied() {
    float phase = 0.0f;
    return MakeWave(0.20f, [&](float t, int) mutable {
        float hz = t < 0.09f ? 180.0f : 145.0f;
        phase += hz / (float)SampleRate;
        return OscSquare(phase, 0.34f) * PercEnv(t, 0.20f, 0.003f, 1.2f) * 0.38f;
    });
}

Wave MakeCueWave(AudioSystem::Cue cue, int variant) {
    switch (cue) {
        case AudioSystem::Cue::VulcanShot: return MakeVulcanShot(variant);
        case AudioSystem::Cue::PlasmaShot: return MakePlasmaShot(variant);
        case AudioSystem::Cue::MissileLaunch: return MakeMissileLaunch(variant);
        case AudioSystem::Cue::EnemyBullet: return MakeEnemyBullet(variant);
        case AudioSystem::Cue::EnemyStrongShot: return MakeEnemyStrongShot(variant);
        case AudioSystem::Cue::ExplosionSmall: return MakeExplosionSmall(variant);
        case AudioSystem::Cue::ExplosionMedium: return MakeExplosionMedium(variant);
        case AudioSystem::Cue::ExplosionLarge: return MakeExplosionLarge(variant);
        case AudioSystem::Cue::BossDamage: return MakeBossDamage(variant);
        case AudioSystem::Cue::BossPhaseChange: return MakeBossPhaseChange();
        case AudioSystem::Cue::BossDefeat: return MakeBossDefeat();
        case AudioSystem::Cue::PlayerHit: return MakePlayerHit();
        case AudioSystem::Cue::PlayerDeath: return MakePlayerDeath();
        case AudioSystem::Cue::Respawn: return MakeRespawn();
        case AudioSystem::Cue::Bomb: return MakeBomb();
        case AudioSystem::Cue::BombClear: return MakeBombClear();
        case AudioSystem::Cue::PickupPowerup: return MakePickupPowerup();
        case AudioSystem::Cue::PickupWeaponSwitch: return MakePickupWeaponSwitch();
        case AudioSystem::Cue::PickupWeaponUpgrade: return MakePickupWeaponUpgrade();
        case AudioSystem::Cue::PickupBomb: return MakePickupBomb();
        case AudioSystem::Cue::PickupMedal: return MakePickupMedal();
        case AudioSystem::Cue::ScoreMilestone: return MakeScoreMilestone();
        case AudioSystem::Cue::FormationBonus: return MakeFormationBonus();
        case AudioSystem::Cue::ExtraLife: return MakeExtraLife();
        case AudioSystem::Cue::MenuMove: return MakeMenuMove();
        case AudioSystem::Cue::MenuConfirm: return MakeMenuConfirm();
        case AudioSystem::Cue::MenuCancel: return MakeMenuCancel();
        case AudioSystem::Cue::InsertCoin: return MakeInsertCoin();
        case AudioSystem::Cue::PressStart: return MakePressStart();
        case AudioSystem::Cue::Pause: return MakePause();
        case AudioSystem::Cue::Resume: return MakeResume();
        case AudioSystem::Cue::ContinueTick: return MakeContinueTick();
        case AudioSystem::Cue::BonusTick: return MakeBonusTick(variant);
        case AudioSystem::Cue::HighScoreTick: return MakeHighScoreTick();
        case AudioSystem::Cue::HighScoreEntry: return MakeHighScoreEntry();
        case AudioSystem::Cue::StageStart: return MakeStageStart();
        case AudioSystem::Cue::StageClear: return MakeStageClear();
        case AudioSystem::Cue::GameOver: return MakeGameOver();
        case AudioSystem::Cue::BossWarning: return MakeBossWarning();
        case AudioSystem::Cue::BossEntrance: return MakeBossEntrance();
        case AudioSystem::Cue::LowLife: return MakeLowLife();
        case AudioSystem::Cue::Victory: return MakeVictory();
        case AudioSystem::Cue::AttractShimmer: return MakeAttractShimmer();
        case AudioSystem::Cue::Denied: return MakeDenied();
        case AudioSystem::Cue::Count: break;
    }
    return {};
}

Wave MakeMusic(AudioSystem::MusicTrack track) {
    const int steps = track == AudioSystem::MusicTrack::Title ? 96 : 128;
    const float stepTime = track == AudioSystem::MusicTrack::Boss ? 0.105f : 0.12f;
    const int framesPerStep = (int)(stepTime * (float)SampleRate);
    const int frames = framesPerStep * steps;
    std::vector<float> mixFrames((size_t)frames);
    int16_t* samples = (int16_t*)std::malloc(sizeof(int16_t) * frames);
    if (!samples) return {};

    unsigned int seed = track == AudioSystem::MusicTrack::Boss ? 0xB055u : 0x57A6Eu;
    float bass = 0.0f;
    float lead = 0.11f;
    float arp = 0.37f;
    float kick = 0.0f;
    const int stageBass[4] = { 45, 41, 48, 43 };
    const int bossBass[4] = { 38, 39, 36, 43 };
    const int titleBass[4] = { 48, 43, 45, 40 };
    const int melody[32] = {
        72, 0, 76, 79, 83, 0, 79, 76, 72, 0, 74, 76, 79, 0, 76, 74,
        71, 0, 74, 78, 81, 0, 78, 74, 72, 0, 76, 79, 84, 83, 79, 76
    };
    const int bossLead[16] = { 50, 0, 50, 55, 0, 50, 56, 55, 50, 0, 62, 61, 56, 55, 53, 50 };

    for (int i = 0; i < frames; ++i) {
        int step = i / framesPerStep;
        int offset = i % framesPerStep;
        float sp = (float)offset / (float)framesPerStep;
        int bar = (step / 16) % 4;
        int beat = step % 8;
        const int* roots = stageBass;
        if (track == AudioSystem::MusicTrack::Title) roots = titleBass;
        if (track == AudioSystem::MusicTrack::Boss) roots = bossBass;

        int root = roots[bar];
        int bassPattern = (step % 4 == 3) ? 7 : ((step % 2 == 0) ? 0 : 12);
        bass += MidiNote(root + bassPattern) / (float)SampleRate;
        float bassEnv = std::pow(1.0f - sp, 0.7f);
        float bassVal = (track == AudioSystem::MusicTrack::Boss ? OscSaw(bass) : OscSquare(bass, 0.42f)) * bassEnv * 0.17f;

        float leadVal = 0.0f;
        int note = track == AudioSystem::MusicTrack::Boss ? bossLead[step % 16] : melody[step % 32];
        if (note > 0 && (track != AudioSystem::MusicTrack::Title || step % 2 == 0)) {
            lead += MidiNote(note) / (float)SampleRate;
            leadVal = OscSquare(lead, 0.38f) * std::pow(1.0f - sp, 1.15f) * 0.13f;
        }

        int arpNote = root + ((step % 3 == 0) ? 12 : (step % 3 == 1 ? 15 : 19));
        arp += MidiNote(arpNote + (track == AudioSystem::MusicTrack::Boss ? -12 : 12)) / (float)SampleRate;
        float arpVal = OscTriangle(arp) * std::pow(1.0f - sp, 1.8f) * (track == AudioSystem::MusicTrack::Title ? 0.10f : 0.08f);

        float drum = 0.0f;
        if (beat == 0 || beat == 4 || (track == AudioSystem::MusicTrack::Boss && beat == 6)) {
            kick += (135.0f + (45.0f - 135.0f) * std::min(sp * 1.8f, 1.0f)) / (float)SampleRate;
            drum += OscSine(kick) * std::pow(1.0f - sp, 2.4f) * 0.28f;
        }
        if (beat == 2 || beat == 6) {
            drum += Noise(seed) * std::pow(1.0f - sp, 2.0f) * 0.10f;
        }
        if ((step % 2) == 1) {
            drum += Noise(seed) * std::pow(1.0f - sp, 4.2f) * 0.045f;
        }

        mixFrames[(size_t)i] = bassVal + leadVal + arpVal + drum;
    }

    FadeLoopBoundary(mixFrames, 384);
    Normalize(mixFrames, track == AudioSystem::MusicTrack::Boss ? 0.72f : 0.68f);
    for (int i = 0; i < frames; ++i) {
        float mix = SoftLimit(mixFrames[(size_t)i]);
        samples[i] = (int16_t)(std::clamp(mix, -1.0f, 1.0f) * 30000.0f);
    }

    Wave w{};
    w.frameCount = (unsigned int)frames;
    w.sampleRate = SampleRate;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = samples;
    return w;
}

float WavePeak(const Wave& wave) {
    if (!wave.data || wave.sampleSize != 16) return 0.0f;
    const int16_t* samples = static_cast<const int16_t*>(wave.data);
    unsigned int count = wave.frameCount * std::max(1u, wave.channels);
    int peak = 0;
    for (unsigned int i = 0; i < count; ++i) {
        peak = std::max(peak, (int)std::abs(samples[i]));
    }
    return (float)peak / 32767.0f;
}

float WaveDuration(const Wave& wave) {
    if (wave.sampleRate == 0) return 0.0f;
    return (float)wave.frameCount / (float)wave.sampleRate;
}

std::filesystem::path RuntimeMusicPath(AudioSystem::MusicTrack track) {
    const char* name = "none";
    if (track == AudioSystem::MusicTrack::Title) name = "title";
    if (track == AudioSystem::MusicTrack::Stage) name = "stage";
    if (track == AudioSystem::MusicTrack::Boss) name = "boss";
    return std::filesystem::temp_directory_path() / (std::string("sky_circuit_runtime_music_") + name + ".wav");
}
} // namespace

AudioSystem::~AudioSystem() {
    Shutdown();
}

bool AudioSystem::Init() {
    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
    }
    ready_ = IsAudioDeviceReady();
    if (!ready_) return false;
    LoadCues();
    return soundsReady_;
}

void AudioSystem::Shutdown() {
    if (soundsReady_) {
        StopMusic();
        UnloadCues();
        soundsReady_ = false;
    }
    if (ready_ && IsAudioDeviceReady()) {
        CloseAudioDevice();
    }
    ready_ = false;
}

void AudioSystem::LoadCues() {
    soundsReady_ = true;
    soundsReady_ &= AddCue(Cue::VulcanShot, Category::Player, Priority::Low, 0.50f, 0.025f, 8);
    soundsReady_ &= AddCue(Cue::PlasmaShot, Category::Player, Priority::Low, 0.62f, 0.055f, 5);
    soundsReady_ &= AddCue(Cue::MissileLaunch, Category::Player, Priority::Normal, 0.74f, 0.085f, 4);
    soundsReady_ &= AddCue(Cue::EnemyBullet, Category::Enemy, Priority::Low, 0.33f, 0.075f, 6);
    soundsReady_ &= AddCue(Cue::EnemyStrongShot, Category::Enemy, Priority::Normal, 0.46f, 0.12f, 4);
    soundsReady_ &= AddCue(Cue::ExplosionSmall, Category::Explosion, Priority::Normal, 0.78f, 0.035f, 8);
    soundsReady_ &= AddCue(Cue::ExplosionMedium, Category::Explosion, Priority::High, 0.92f, 0.075f, 5);
    soundsReady_ &= AddCue(Cue::ExplosionLarge, Category::Explosion, Priority::Critical, 0.96f, 0.25f, 3);
    soundsReady_ &= AddCue(Cue::BossDamage, Category::Explosion, Priority::Low, 0.54f, 0.09f, 5);
    soundsReady_ &= AddCue(Cue::BossPhaseChange, Category::Warning, Priority::Critical, 0.92f, 0.65f, 1);
    soundsReady_ &= AddCue(Cue::BossDefeat, Category::Explosion, Priority::Critical, 1.0f, 1.0f, 1);
    soundsReady_ &= AddCue(Cue::PlayerHit, Category::Explosion, Priority::Critical, 0.92f, 0.12f, 1);
    soundsReady_ &= AddCue(Cue::PlayerDeath, Category::Explosion, Priority::Critical, 1.0f, 0.25f, 1);
    soundsReady_ &= AddCue(Cue::Respawn, Category::Player, Priority::High, 0.72f, 0.25f, 1);
    soundsReady_ &= AddCue(Cue::Bomb, Category::Explosion, Priority::Critical, 1.0f, 0.5f, 1);
    soundsReady_ &= AddCue(Cue::BombClear, Category::Explosion, Priority::High, 0.78f, 0.35f, 1);
    soundsReady_ &= AddCue(Cue::PickupPowerup, Category::Pickup, Priority::Normal, 0.82f, 0.045f, 3);
    soundsReady_ &= AddCue(Cue::PickupWeaponSwitch, Category::Pickup, Priority::Normal, 0.74f, 0.055f, 2);
    soundsReady_ &= AddCue(Cue::PickupWeaponUpgrade, Category::Pickup, Priority::High, 0.86f, 0.08f, 1);
    soundsReady_ &= AddCue(Cue::PickupBomb, Category::Pickup, Priority::High, 0.82f, 0.08f, 1);
    soundsReady_ &= AddCue(Cue::PickupMedal, Category::Pickup, Priority::Normal, 0.76f, 0.035f, 4);
    soundsReady_ &= AddCue(Cue::ScoreMilestone, Category::Pickup, Priority::High, 0.72f, 0.3f, 1);
    soundsReady_ &= AddCue(Cue::FormationBonus, Category::Pickup, Priority::High, 0.78f, 0.18f, 1);
    soundsReady_ &= AddCue(Cue::ExtraLife, Category::Pickup, Priority::Critical, 0.94f, 1.0f, 1);
    soundsReady_ &= AddCue(Cue::MenuMove, Category::UI, Priority::Normal, 0.54f, 0.025f, 4);
    soundsReady_ &= AddCue(Cue::MenuConfirm, Category::UI, Priority::Normal, 0.74f, 0.04f, 2);
    soundsReady_ &= AddCue(Cue::MenuCancel, Category::UI, Priority::Normal, 0.62f, 0.04f, 2);
    soundsReady_ &= AddCue(Cue::InsertCoin, Category::UI, Priority::High, 0.86f, 0.12f, 1);
    soundsReady_ &= AddCue(Cue::PressStart, Category::UI, Priority::High, 0.84f, 0.18f, 1);
    soundsReady_ &= AddCue(Cue::Pause, Category::UI, Priority::High, 0.62f, 0.08f, 1);
    soundsReady_ &= AddCue(Cue::Resume, Category::UI, Priority::High, 0.68f, 0.08f, 1);
    soundsReady_ &= AddCue(Cue::ContinueTick, Category::UI, Priority::Normal, 0.62f, 0.08f, 1);
    soundsReady_ &= AddCue(Cue::BonusTick, Category::Pickup, Priority::Normal, 0.42f, 0.018f, 4);
    soundsReady_ &= AddCue(Cue::HighScoreTick, Category::UI, Priority::Normal, 0.52f, 0.02f, 2);
    soundsReady_ &= AddCue(Cue::HighScoreEntry, Category::UI, Priority::Critical, 0.92f, 0.8f, 1);
    soundsReady_ &= AddCue(Cue::StageStart, Category::UI, Priority::High, 0.86f, 0.2f, 1);
    soundsReady_ &= AddCue(Cue::StageClear, Category::UI, Priority::Critical, 0.95f, 0.5f, 1);
    soundsReady_ &= AddCue(Cue::GameOver, Category::UI, Priority::Critical, 0.94f, 0.5f, 1);
    soundsReady_ &= AddCue(Cue::BossWarning, Category::Warning, Priority::High, 0.78f, 0.62f, 2);
    soundsReady_ &= AddCue(Cue::BossEntrance, Category::Warning, Priority::Critical, 0.95f, 0.4f, 1);
    soundsReady_ &= AddCue(Cue::LowLife, Category::Warning, Priority::Low, 0.42f, 0.55f, 2);
    soundsReady_ &= AddCue(Cue::Victory, Category::UI, Priority::Critical, 0.95f, 0.8f, 1);
    soundsReady_ &= AddCue(Cue::AttractShimmer, Category::UI, Priority::Low, 0.35f, 1.8f, 1);
    soundsReady_ &= AddCue(Cue::Denied, Category::UI, Priority::Normal, 0.68f, 0.1f, 2);

    std::array<MusicTrack, 3> tracks = { MusicTrack::Title, MusicTrack::Stage, MusicTrack::Boss };
    for (int i = 0; i < 3; ++i) {
        Wave w = MakeMusic(tracks[(size_t)i]);
        if (!w.data) {
            soundsReady_ = false;
            continue;
        }
        std::filesystem::path musicPath = RuntimeMusicPath(tracks[(size_t)i]);
        musicPaths_[(size_t)i] = musicPath.string();
        bool exported = ExportWave(w, musicPaths_[(size_t)i].c_str());
        UnloadWave(w);
        if (!exported) {
            soundsReady_ = false;
            continue;
        }
        music_[(size_t)i] = LoadMusicStream(musicPaths_[(size_t)i].c_str());
        music_[(size_t)i].looping = true;
        if (!IsMusicValid(music_[(size_t)i])) {
            soundsReady_ = false;
        }
    }

    if (!soundsReady_) {
        UnloadCues();
        return;
    }

    ApplyVolumes();
}

void AudioSystem::UnloadCues() {
    for (CueBank& bank : cues_) {
        for (Sound& sound : bank.variants) {
            if (IsSoundValid(sound)) {
                UnloadSound(sound);
            }
        }
        bank.variants.clear();
    }
    for (size_t i = 0; i < music_.size(); ++i) {
        Music& music = music_[i];
        if (IsMusicValid(music)) {
            UnloadMusicStream(music);
        }
        music = {};
        if (!musicPaths_[i].empty()) {
            std::error_code ec;
            std::filesystem::remove(musicPaths_[i], ec);
            musicPaths_[i].clear();
        }
    }
}

bool AudioSystem::AddCue(Cue cue, Category category, Priority priority, float baseGain, float cooldown, int variants) {
    CueBank& bank = cues_[(size_t)cue];
    bank.category = category;
    bank.priority = priority;
    bank.baseGain = baseGain;
    bank.cooldown = cooldown;
    bank.lastPlayed = -1000.0;
    bank.variants.reserve((size_t)variants);

    for (int i = 0; i < variants; ++i) {
        Wave w = MakeCueWave(cue, i);
        if (!w.data) return false;
        Sound sound = LoadSoundFromWave(w);
        UnloadWave(w);
        if (!IsSoundValid(sound)) return false;
        bank.variants.push_back(sound);
    }
    return true;
}

void AudioSystem::Update() {
    if (!Ready() || currentMusic_ == MusicTrack::None) return;
    double now = AudioClockSeconds();
    float targetMusicGain = 1.0f;
    if (now < musicDuckUntil_) targetMusicGain = std::min(targetMusicGain, 0.52f);
    if (musicManuallyDucked_) targetMusicGain = std::min(targetMusicGain, 0.38f);
    currentMusicGain_ += (targetMusicGain - currentMusicGain_) * 0.12f;
    ApplyMusicVolume();

    Music& music = MusicStream(currentMusic_);
    UpdateMusicStream(music);
    if (!IsMusicStreamPlaying(music)) {
        PlayMusicStream(music);
    }
}

void AudioSystem::SetVolumes(int sfxVolume, int musicVolume) {
    float s = std::clamp((float)sfxVolume / 10.0f, 0.0f, 1.0f);
    float m = std::clamp((float)musicVolume / 10.0f, 0.0f, 1.0f);
    sfxMaster_ = s * s;
    musicMaster_ = m * m;
    ApplyVolumes();
}

void AudioSystem::SetMasterVolume(float volume) {
    masterVolume_ = std::clamp(volume, 0.0f, 1.0f);
    ApplyVolumes();
}

void AudioSystem::SetMusicDucked(bool ducked) {
    musicManuallyDucked_ = ducked;
}

void AudioSystem::ApplyVolumes() {
    if (!soundsReady_) return;
    for (CueBank& bank : cues_) {
        for (Sound& sound : bank.variants) {
            SetSoundVolume(sound, masterVolume_ * bank.baseGain * CategoryGain(bank.category));
        }
    }
    ApplyMusicVolume();
}

void AudioSystem::ApplyMusicVolume() {
    if (!soundsReady_) return;
    for (Music& music : music_) {
        if (IsMusicValid(music)) {
            SetMusicVolume(music, masterVolume_ * musicMaster_ * CategoryGain(Category::Music) * currentMusicGain_);
        }
    }
}

float AudioSystem::CategoryGain(Category category) const {
    switch (category) {
        case Category::Player: return sfxMaster_ * 0.88f;
        case Category::Enemy: return sfxMaster_ * 0.54f;
        case Category::Explosion: return sfxMaster_ * 0.82f;
        case Category::Pickup: return sfxMaster_ * 0.72f;
        case Category::UI: return sfxMaster_ * 0.70f;
        case Category::Warning: return sfxMaster_ * 0.76f;
        case Category::Music: return 0.44f;
    }
    return sfxMaster_;
}

float AudioSystem::NextRandom() {
    randomSeed_ = randomSeed_ * 1664525u + 1013904223u;
    return (float)((randomSeed_ >> 8) & 0xFFFFu) / 65535.0f;
}

float AudioSystem::RandomRange(float lo, float hi) {
    return lo + (hi - lo) * NextRandom();
}

int AudioSystem::CountPlaying() const {
    int count = 0;
    for (const CueBank& bank : cues_) {
        for (const Sound& sound : bank.variants) {
            if (IsSoundPlaying(sound)) ++count;
        }
    }
    return count;
}

int AudioSystem::CountPlaying(Category category) const {
    int count = 0;
    for (const CueBank& bank : cues_) {
        if (bank.category != category) continue;
        for (const Sound& sound : bank.variants) {
            if (IsSoundPlaying(sound)) ++count;
        }
    }
    return count;
}

int AudioSystem::VoiceBudget(Category category) const {
    switch (category) {
        case Category::Player: return 5;
        case Category::Enemy: return 3;
        case Category::Explosion: return 5;
        case Category::Pickup: return 3;
        case Category::UI: return 4;
        case Category::Warning: return 2;
        case Category::Music: return 1;
    }
    return 4;
}

int AudioSystem::PriorityRank(Priority priority) const {
    switch (priority) {
        case Priority::Low: return 0;
        case Priority::Normal: return 1;
        case Priority::High: return 2;
        case Priority::Critical: return 3;
    }
    return 1;
}

float AudioSystem::CongestionGain(int activeVoices, int activeInCategory, Priority priority) const {
    float globalTrim = 1.0f - std::max(0, activeVoices - 7) * 0.045f;
    float categoryTrim = 1.0f - std::max(0, activeInCategory - 2) * 0.06f;
    if (PriorityRank(priority) >= PriorityRank(Priority::High)) {
        globalTrim = std::max(globalTrim, 0.82f);
        categoryTrim = std::max(categoryTrim, 0.86f);
    }
    return std::clamp(globalTrim * categoryTrim, 0.56f, 1.0f);
}

float AudioSystem::CategoryHeadroom(Category category, Priority priority) const {
    float headroom = 1.0f;
    switch (category) {
        case Category::Player: headroom = 0.92f; break;
        case Category::Enemy: headroom = 0.72f; break;
        case Category::Explosion: headroom = 0.86f; break;
        case Category::Pickup: headroom = 0.78f; break;
        case Category::UI: headroom = 0.80f; break;
        case Category::Warning: headroom = 0.88f; break;
        case Category::Music: headroom = 1.0f; break;
    }
    if (priority == Priority::Critical) headroom *= 1.08f;
    return headroom;
}

float AudioSystem::RandomPan(Category category) {
    switch (category) {
        case Category::Player: return RandomRange(0.44f, 0.56f);
        case Category::Enemy: return RandomRange(0.24f, 0.76f);
        case Category::Explosion: return RandomRange(0.18f, 0.82f);
        case Category::Pickup: return RandomRange(0.36f, 0.64f);
        case Category::UI: return 0.5f;
        case Category::Warning: return 0.5f;
        case Category::Music: return 0.5f;
    }
    return 0.5f;
}

float AudioSystem::PositionPan(float x, float screenWidth, Category category) {
    if (screenWidth <= 1.0f) return RandomPan(category);
    float normalized = std::clamp(x / screenWidth, 0.0f, 1.0f);
    float centerBlend = 0.20f;
    if (category == Category::Pickup) centerBlend = 0.36f;
    if (category == Category::Enemy) centerBlend = 0.24f;
    if (category == Category::Explosion) centerBlend = 0.16f;
    float pan = normalized * (1.0f - centerBlend) + 0.5f * centerBlend;
    return std::clamp(pan + RandomRange(-0.025f, 0.025f), 0.08f, 0.92f);
}

bool AudioSystem::StealLowerPriorityVoice(Priority incomingPriority, Category incomingCategory) {
    int incomingRank = PriorityRank(incomingPriority);
    int bestRank = incomingRank;
    Sound* candidate = nullptr;

    for (CueBank& bank : cues_) {
        int rank = PriorityRank(bank.priority);
        if (rank >= bestRank) continue;
        if (incomingCategory == Category::Warning && bank.category == Category::UI) continue;
        for (Sound& sound : bank.variants) {
            if (!IsSoundPlaying(sound)) continue;
            bestRank = rank;
            candidate = &sound;
        }
    }

    if (!candidate) return false;
    StopSound(*candidate);
    ++runtimeStolenVoices_;
    return true;
}

void AudioSystem::StopVoicesForFocus(Priority minimumPriorityToKeep, bool keepUi, bool keepWarning) {
    int keepRank = PriorityRank(minimumPriorityToKeep);
    for (CueBank& bank : cues_) {
        if (keepUi && bank.category == Category::UI) continue;
        if (keepWarning && bank.category == Category::Warning) continue;
        if (PriorityRank(bank.priority) >= keepRank) continue;
        for (Sound& sound : bank.variants) {
            if (!IsSoundPlaying(sound)) continue;
            StopSound(sound);
            ++runtimeFocusStops_;
        }
    }
}

void AudioSystem::ResetRuntimeStats() {
    runtimePlayed_ = 0;
    runtimeCooldownDrops_ = 0;
    runtimePressureDrops_ = 0;
    runtimeStolenVoices_ = 0;
    runtimeFocusStops_ = 0;
}

void AudioSystem::PlayCue(Cue cue, float pitch, float gain, float pan) {
    if (!Ready()) return;
    CueBank& bank = cues_[(size_t)cue];
    if (bank.variants.empty()) return;

    double now = AudioClockSeconds();
    if (now - bank.lastPlayed < bank.cooldown && bank.priority != Priority::Critical) {
        ++runtimeCooldownDrops_;
        return;
    }

    int activeVoices = CountPlaying();
    int activeInCategory = CountPlaying(bank.category);
    if (bank.priority == Priority::Low && (activeVoices >= 12 || activeInCategory >= VoiceBudget(bank.category))) {
        ++runtimePressureDrops_;
        return;
    }
    if (bank.priority == Priority::Normal && (activeVoices >= 15 || activeInCategory > VoiceBudget(bank.category))) {
        ++runtimePressureDrops_;
        return;
    }
    if (activeVoices >= MaxSfxVoices && bank.priority != Priority::Low && bank.priority != Priority::Normal) {
        if (StealLowerPriorityVoice(bank.priority, bank.category)) {
            activeVoices = std::max(0, activeVoices - 1);
            activeInCategory = CountPlaying(bank.category);
        }
    }

    int choice = -1;
    int offset = (int)(NextRandom() * (float)bank.variants.size()) % (int)bank.variants.size();
    for (int i = 0; i < (int)bank.variants.size(); ++i) {
        int idx = (offset + i) % (int)bank.variants.size();
        if (!IsSoundPlaying(bank.variants[(size_t)idx])) {
            choice = idx;
            break;
        }
    }

    if (choice < 0) {
        if (bank.priority == Priority::Low || bank.priority == Priority::Normal) {
            ++runtimePressureDrops_;
            return;
        }
        choice = offset;
        StopSound(bank.variants[(size_t)choice]);
        ++runtimeStolenVoices_;
    }

    Sound& sound = bank.variants[(size_t)choice];
    float mixGain = CongestionGain(activeVoices, activeInCategory, bank.priority);
    SetSoundPitch(sound, std::clamp(pitch, 0.25f, 3.0f));
    SetSoundPan(sound, pan >= 0.0f ? std::clamp(pan, 0.0f, 1.0f) : RandomPan(bank.category));
    SetSoundVolume(sound, masterVolume_ * bank.baseGain * CategoryGain(bank.category) * CategoryHeadroom(bank.category, bank.priority) * mixGain * std::clamp(gain, 0.0f, 1.4f));
    PlaySound(sound);
    ++runtimePlayed_;
    bank.lastPlayed = now;

    if (bank.priority == Priority::Critical) {
        musicDuckUntil_ = std::max(musicDuckUntil_, now + 0.55);
    } else if (bank.priority == Priority::High && bank.category != Category::UI) {
        musicDuckUntil_ = std::max(musicDuckUntil_, now + 0.20);
    }
}

Music& AudioSystem::MusicStream(MusicTrack track) {
    switch (track) {
        case MusicTrack::Title: return music_[0];
        case MusicTrack::Stage: return music_[1];
        case MusicTrack::Boss: return music_[2];
        case MusicTrack::None: break;
    }
    return music_[0];
}

void AudioSystem::PlayMusic(MusicTrack track) {
    if (!Ready()) return;
    if (currentMusic_ == track && track != MusicTrack::None) return;
    StopMusic();
    currentMusic_ = track;
    musicManuallyDucked_ = false;
    if (track != MusicTrack::None) {
        Music& music = MusicStream(track);
        SetMusicPitch(music, 1.0f);
        currentMusicGain_ = 1.0f;
        ApplyMusicVolume();
        PlayMusicStream(music);
    }
}

void AudioSystem::StopMusic() {
    if (!soundsReady_) {
        currentMusic_ = MusicTrack::None;
        return;
    }
    for (Music& music : music_) {
        if (IsMusicValid(music)) {
            StopMusicStream(music);
        }
    }
    currentMusic_ = MusicTrack::None;
}

void AudioSystem::PlayPlayerShot(WeaponType weapon) {
    switch (weapon) {
        case WeaponType::Vulcan:
            PlayCue(Cue::VulcanShot, RandomRange(0.97f, 1.04f), RandomRange(0.86f, 1.0f));
            break;
        case WeaponType::Plasma:
            PlayCue(Cue::PlasmaShot, RandomRange(0.96f, 1.03f), RandomRange(0.9f, 1.04f));
            break;
        case WeaponType::Missile:
            PlayCue(Cue::MissileLaunch, RandomRange(0.95f, 1.04f), RandomRange(0.9f, 1.05f));
            break;
    }
}

void AudioSystem::PlayEnemyShot(EnemyShotType type) {
    if (type == EnemyShotType::Strong) {
        PlayCue(Cue::EnemyStrongShot, RandomRange(0.92f, 1.08f), RandomRange(0.78f, 1.0f));
    } else {
        PlayEnemyBullet();
    }
}

void AudioSystem::PlayEnemyShotAt(EnemyShotType type, float x, float screenWidth) {
    float pan = PositionPan(x, screenWidth, Category::Enemy);
    if (type == EnemyShotType::Strong) {
        PlayCue(Cue::EnemyStrongShot, RandomRange(0.92f, 1.08f), RandomRange(0.78f, 1.0f), pan);
    } else {
        PlayCue(Cue::EnemyBullet, RandomRange(0.93f, 1.1f), RandomRange(0.74f, 0.96f), pan);
    }
}

void AudioSystem::PlayExplosion(ExplosionSize size) {
    if (size == ExplosionSize::Small) {
        PlayCue(Cue::ExplosionSmall, RandomRange(0.91f, 1.13f), RandomRange(0.82f, 1.0f));
    } else if (size == ExplosionSize::Medium) {
        PlayCue(Cue::ExplosionMedium, RandomRange(0.88f, 1.04f), RandomRange(0.92f, 1.08f));
    } else {
        PlayCue(Cue::ExplosionLarge, RandomRange(0.92f, 1.02f), RandomRange(0.94f, 1.06f));
    }
}

void AudioSystem::PlayExplosionAt(ExplosionSize size, float x, float screenWidth) {
    float pan = PositionPan(x, screenWidth, Category::Explosion);
    if (size == ExplosionSize::Small) {
        PlayCue(Cue::ExplosionSmall, RandomRange(0.91f, 1.13f), RandomRange(0.82f, 1.0f), pan);
    } else if (size == ExplosionSize::Medium) {
        PlayCue(Cue::ExplosionMedium, RandomRange(0.88f, 1.04f), RandomRange(0.92f, 1.08f), pan);
    } else {
        PlayCue(Cue::ExplosionLarge, RandomRange(0.92f, 1.02f), RandomRange(0.94f, 1.06f), pan);
    }
}

void AudioSystem::PlayBossDamage() { PlayCue(Cue::BossDamage, RandomRange(0.92f, 1.06f), RandomRange(0.78f, 1.0f)); }
void AudioSystem::PlayBossDamageAt(float x, float screenWidth) {
    PlayCue(Cue::BossDamage, RandomRange(0.92f, 1.06f), RandomRange(0.78f, 1.0f), PositionPan(x, screenWidth, Category::Explosion));
}
void AudioSystem::PlayPlayerHit() { PlayCue(Cue::PlayerHit); }
void AudioSystem::PlayPlayerDeath() {
    StopVoicesForFocus(Priority::Critical, false, false);
    musicDuckUntil_ = std::max(musicDuckUntil_, AudioClockSeconds() + 1.2);
    PlayCue(Cue::PlayerDeath);
}
void AudioSystem::PlayRespawn() { PlayCue(Cue::Respawn); }
void AudioSystem::PlayBomb() {
    StopVoicesForFocus(Priority::High, true, true);
    musicDuckUntil_ = std::max(musicDuckUntil_, AudioClockSeconds() + 1.05);
    PlayCue(Cue::Bomb);
    PlayBombClear();
}
void AudioSystem::PlayBombClear() { PlayCue(Cue::BombClear); }
void AudioSystem::PlayEnemyBullet() { PlayCue(Cue::EnemyBullet, RandomRange(0.93f, 1.1f), RandomRange(0.74f, 0.96f)); }
void AudioSystem::PlayWeaponSwitch() { PlayCue(Cue::PickupWeaponSwitch, RandomRange(0.98f, 1.04f)); }
void AudioSystem::PlayWeaponUpgrade() { PlayCue(Cue::PickupWeaponUpgrade); }
void AudioSystem::PlayMedal(int chain) {
    int step = std::clamp(chain - 1, 0, 14);
    float pitch = 0.94f + (float)(step % 8) * 0.045f + (step >= 8 ? 0.08f : 0.0f);
    float gain = 0.82f + (float)std::min(step, 8) * 0.025f;
    PlayCue(Cue::PickupMedal, pitch, gain);
}

void AudioSystem::PlayMedalAt(int chain, float x, float screenWidth) {
    int step = std::clamp(chain - 1, 0, 14);
    float pitch = 0.94f + (float)(step % 8) * 0.045f + (step >= 8 ? 0.08f : 0.0f);
    float gain = 0.82f + (float)std::min(step, 8) * 0.025f;
    PlayCue(Cue::PickupMedal, pitch, gain, PositionPan(x, screenWidth, Category::Pickup));
}

void AudioSystem::PlayPickup(PowerupType type) {
    switch (type) {
        case PowerupType::WeaponChange: PlayPickup(PickupType::WeaponSwitch); break;
        case PowerupType::Upgrade: PlayPickup(PickupType::WeaponUpgrade); break;
        case PowerupType::Bomb: PlayPickup(PickupType::Bomb); break;
        case PowerupType::Medal: PlayMedal(1); break;
    }
}
void AudioSystem::PlayPickupAt(PowerupType type, float x, float screenWidth) {
    switch (type) {
        case PowerupType::WeaponChange: PlayPickupAt(PickupType::WeaponSwitch, x, screenWidth); break;
        case PowerupType::Upgrade: PlayPickupAt(PickupType::WeaponUpgrade, x, screenWidth); break;
        case PowerupType::Bomb: PlayPickupAt(PickupType::Bomb, x, screenWidth); break;
        case PowerupType::Medal: PlayMedalAt(1, x, screenWidth); break;
    }
}

void AudioSystem::PlayPickup(PickupType type) {
    switch (type) {
        case PickupType::Powerup: PlayCue(Cue::PickupPowerup, RandomRange(0.97f, 1.06f)); break;
        case PickupType::WeaponSwitch: PlayCue(Cue::PickupWeaponSwitch, RandomRange(0.98f, 1.05f)); break;
        case PickupType::WeaponUpgrade: PlayCue(Cue::PickupWeaponUpgrade); break;
        case PickupType::Bomb: PlayCue(Cue::PickupBomb); break;
        case PickupType::Medal: PlayMedal(1); break;
    }
}
void AudioSystem::PlayPickupAt(PickupType type, float x, float screenWidth) {
    float pan = PositionPan(x, screenWidth, Category::Pickup);
    switch (type) {
        case PickupType::Powerup: PlayCue(Cue::PickupPowerup, RandomRange(0.97f, 1.06f), 1.0f, pan); break;
        case PickupType::WeaponSwitch: PlayCue(Cue::PickupWeaponSwitch, RandomRange(0.98f, 1.05f), 1.0f, pan); break;
        case PickupType::WeaponUpgrade: PlayCue(Cue::PickupWeaponUpgrade, 1.0f, 1.0f, pan); break;
        case PickupType::Bomb: PlayCue(Cue::PickupBomb, 1.0f, 1.0f, pan); break;
        case PickupType::Medal: PlayMedalAt(1, x, screenWidth); break;
    }
}
void AudioSystem::PlayScoreMilestone() { PlayCue(Cue::ScoreMilestone); }
void AudioSystem::PlayFormationBonus() { PlayCue(Cue::FormationBonus); }
void AudioSystem::PlayFormationBonusAt(float x, float screenWidth) {
    PlayCue(Cue::FormationBonus, 1.0f, 1.0f, PositionPan(x, screenWidth, Category::Pickup));
}
void AudioSystem::PlayExtraLife() { PlayCue(Cue::ExtraLife); }
void AudioSystem::PlayMenuMove() { PlayCue(Cue::MenuMove, RandomRange(0.96f, 1.05f)); }
void AudioSystem::PlayMenuConfirm() { PlayCue(Cue::MenuConfirm); }
void AudioSystem::PlayMenuCancel() { PlayCue(Cue::MenuCancel); }
void AudioSystem::PlayInsertCoin() { PlayCue(Cue::InsertCoin); }
void AudioSystem::PlayPressStart() { PlayCue(Cue::PressStart); }
void AudioSystem::PlayPause() { SetMusicDucked(true); PlayCue(Cue::Pause); }
void AudioSystem::PlayResume() { SetMusicDucked(false); PlayCue(Cue::Resume); }
void AudioSystem::PlayContinueTick() { PlayCue(Cue::ContinueTick); }
void AudioSystem::PlayBonusTick(int step) { PlayCue(Cue::BonusTick, 0.92f + (float)(step % 18) * 0.018f, 0.82f); }
void AudioSystem::PlayHighScoreTick() { PlayCue(Cue::HighScoreTick, RandomRange(0.98f, 1.08f)); }
void AudioSystem::PlayHighScoreEntry() { StopMusic(); PlayCue(Cue::HighScoreEntry); }
void AudioSystem::PlayStageStart() { PlayCue(Cue::StageStart); }
void AudioSystem::PlayStageClear() { StopMusic(); PlayCue(Cue::StageClear); }
void AudioSystem::PlayVictory() { StopMusic(); PlayCue(Cue::Victory); }
void AudioSystem::PlayGameOver() {
    StopMusic();
    StopVoicesForFocus(Priority::Critical, false, false);
    PlayCue(Cue::GameOver);
}
void AudioSystem::PlayBossWarning() { PlayCue(Cue::BossWarning); }
void AudioSystem::PlayBossEntrance() { PlayCue(Cue::BossEntrance); }
void AudioSystem::PlayBossPhaseChange() { PlayCue(Cue::BossPhaseChange); }
void AudioSystem::PlayBossDefeat() {
    StopMusic();
    StopVoicesForFocus(Priority::Critical, false, false);
    PlayCue(Cue::BossDefeat);
}
void AudioSystem::PlayLowLifeWarning() { PlayCue(Cue::LowLife); }
void AudioSystem::PlayAttractShimmer() { PlayCue(Cue::AttractShimmer); }
void AudioSystem::PlayDenied() { PlayCue(Cue::Denied); }
void AudioSystem::PlaySettingsTick() { PlayCue(Cue::VulcanShot, 1.08f, 0.62f); }

void AudioSystem::RunChaosAudit(float seconds) {
    if (!Ready()) return;
    ResetRuntimeStats();
    SetMasterVolume(1.0f);
    SetVolumes(8, 6);
    PlayMusic(MusicTrack::Stage);

    double start = AudioClockSeconds();
    double nextPlayer = start;
    double nextEnemy = start;
    double nextExplosion = start + 0.18;
    double nextPickup = start + 0.33;
    double nextBoss = start + 1.1;
    double nextUi = start + 0.45;
    int weaponStep = 0;
    int pickupStep = 0;
    int explosionStep = 0;

    while (AudioClockSeconds() - start < (double)seconds) {
        double now = AudioClockSeconds();
        if (now >= nextPlayer) {
            WeaponType weapon = weaponStep % 3 == 0 ? WeaponType::Vulcan : (weaponStep % 3 == 1 ? WeaponType::Plasma : WeaponType::Missile);
            PlayPlayerShot(weapon);
            ++weaponStep;
            nextPlayer += 0.055;
        }
        if (now >= nextEnemy) {
            PlayEnemyShot((weaponStep % 5 == 0) ? EnemyShotType::Strong : EnemyShotType::Basic);
            nextEnemy += 0.072;
        }
        if (now >= nextExplosion) {
            ExplosionSize size = explosionStep % 5 == 0 ? ExplosionSize::Large : (explosionStep % 2 == 0 ? ExplosionSize::Medium : ExplosionSize::Small);
            PlayExplosion(size);
            ++explosionStep;
            nextExplosion += 0.23;
        }
        if (now >= nextPickup) {
            PowerupType type = pickupStep % 4 == 0 ? PowerupType::WeaponChange :
                               pickupStep % 4 == 1 ? PowerupType::Upgrade :
                               pickupStep % 4 == 2 ? PowerupType::Bomb : PowerupType::Medal;
            if (type == PowerupType::Medal) PlayMedal(pickupStep);
            else PlayPickup(type);
            ++pickupStep;
            nextPickup += 0.31;
        }
        if (now >= nextBoss) {
            PlayBossWarning();
            PlayBossPhaseChange();
            nextBoss += 1.45;
        }
        if (now >= nextUi) {
            PlayMenuMove();
            if (pickupStep % 3 == 0) PlayScoreMilestone();
            if (pickupStep % 5 == 0) PlayFormationBonus();
            PlayBonusTick(pickupStep);
            nextUi += 0.42;
        }
        Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    PlayBomb();
    PlayPlayerHit();
    PlayRespawn();
    PlayExtraLife();
    PlayHighScoreEntry();
    for (int i = 0; i < 90; ++i) {
        Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    TraceLog(LOG_INFO, "AUDIO AUDIT: played=%d cooldown_drops=%d pressure_drops=%d stolen_voices=%d focus_stops=%d max_voices=%d",
             runtimePlayed_, runtimeCooldownDrops_, runtimePressureDrops_, runtimeStolenVoices_, runtimeFocusStops_, MaxSfxVoices);
    StopMusic();
}

bool AudioSystem::ExportProceduralBank(const char* directory) {
    std::filesystem::path root = directory && directory[0] ? directory : "audio_export";
    std::error_code ec;
    std::filesystem::create_directories(root, ec);
    if (ec) {
        TraceLog(LOG_WARNING, "AUDIO EXPORT: could not create directory '%s'", root.string().c_str());
        return false;
    }

    std::ofstream manifest(root / "manifest.csv");
    if (!manifest) {
        TraceLog(LOG_WARNING, "AUDIO EXPORT: could not write manifest in '%s'", root.string().c_str());
        return false;
    }

    manifest << "kind,name,variant,file,duration_seconds,peak\n";
    int exported = 0;
    for (const ExportCueSpec& spec : ExportCues) {
        for (int variant = 0; variant < spec.variants; ++variant) {
            Wave wave = MakeCueWave(spec.cue, variant);
            if (!wave.data) return false;
            std::string fileName = std::string(spec.name) + "_v" + std::to_string(variant) + ".wav";
            std::filesystem::path path = root / fileName;
            float duration = WaveDuration(wave);
            float peak = WavePeak(wave);
            bool ok = ExportWave(wave, path.string().c_str());
            UnloadWave(wave);
            if (!ok) {
                TraceLog(LOG_WARNING, "AUDIO EXPORT: failed '%s'", path.string().c_str());
                return false;
            }
            manifest << "sfx," << spec.name << "," << variant << "," << fileName << "," << duration << "," << peak << "\n";
            ++exported;
        }
    }

    for (const ExportMusicSpec& spec : ExportMusic) {
        Wave wave = MakeMusic(spec.track);
        if (!wave.data) return false;
        std::string fileName = std::string(spec.name) + ".wav";
        std::filesystem::path path = root / fileName;
        float duration = WaveDuration(wave);
        float peak = WavePeak(wave);
        bool ok = ExportWave(wave, path.string().c_str());
        UnloadWave(wave);
        if (!ok) {
            TraceLog(LOG_WARNING, "AUDIO EXPORT: failed '%s'", path.string().c_str());
            return false;
        }
        manifest << "music," << spec.name << ",0," << fileName << "," << duration << "," << peak << "\n";
        ++exported;
    }

    TraceLog(LOG_INFO, "AUDIO EXPORT: wrote %d procedural WAV files to '%s'", exported, root.string().c_str());
    return true;
}
