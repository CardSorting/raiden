#include "Game.h"
#include "AudioSystem.h"
#include <cstring>

int main(int argc, char** argv) {
    if (argc > 1 && std::strcmp(argv[1], "--audio-chaos-test") == 0) {
        AudioSystem audio;
        if (!audio.Init()) return 1;
        audio.RunChaosAudit();
        audio.Shutdown();
        return 0;
    }
    if (argc > 1 && std::strcmp(argv[1], "--audio-export") == 0) {
        const char* directory = argc > 2 ? argv[2] : "audio_export";
        return AudioSystem::ExportProceduralBank(directory) ? 0 : 1;
    }

    Game game;
    game.Run();
    return 0;
}
