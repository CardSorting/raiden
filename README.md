# Sky Circuit

Sky Circuit is a complete local C++17/raylib vertical arcade shooter. It captures the immediate feel of 1990s cabinet shmups with original procedural vector art, generated arcade tones, readable enemy patterns, powerups, bombs, score pressure, and a looping stage built for quick play sessions.

## Onboarding & How to Start the Game

### 1. Compile the Project
Before running, compile the codebase using CMake. It will automatically fetch Raylib 5.5 in memory.
Run the following commands in your project root directory:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### 2. Launch the Executable
Once the build is complete, launch the game binary:
- **macOS / Linux**:
  ```bash
  ./build/sky_circuit
  ```
- **Windows**:
  ```powershell
  .\\build\\Release\\sky_circuit.exe
  ```

### 3. Navigate the Title Screen
When the game starts up, select **Start Mission** to play:
- **Mouse Navigation**: Hover your cursor over **Start Mission** and **left-click** to select it.
- **Keyboard Navigation**: Press **Up/Down** or **W/S** to move the cursor pointer `>` and press **Enter** or **Space** to confirm.
- **Gamepad Navigation**: Press **D-Pad Up/Down** and confirm with the **Down Face Button (A / Cross)**.

### 4. First Flight Controls
Once the mission starts, a helper banner will appear at the bottom of the playfield showing the active control configuration:
- **Arrows Layout (Default)**: Move with Arrow Keys, Shoot with Z or Space, Bomb with X.
- **WASD Layout**: Move with W/A/S/D, Shoot with J or Space, Bomb with K.
*Note: You can switch control presets, adjust volume sliders, or toggle fullscreen modes inside the **Settings** menu at any time.*

## Features

- 480x640 vertical playfield.
- Familiar title menu with **Start Mission**, **High Scores**, **Settings**, **How To Play**, and **Exit**.
- **Interactive Settings**: Sliders for SFX and Music volume, Screen Shake toggle, Screen Mode (Windowed/Fullscreen), and Hitbox display toggle.
- **Persistent High Scores Leaderboard**: Hall of Fame saving to local `highscores.txt` with Name Entry UI for player initials.
- **Procedural Audio & Chiptune BGM**: Fully synthesized retro chiptune loop, sound effects, explosions, and UI tones generated entirely in code.
- **Attract Mode (Gameplay Demo)**: Automatically starts an auto-play demonstration controlled by a dodging/shooting AI when left idle for 15 seconds.
- **Gamepad / Controller Support**: Full controller integration for gameplay and menus.
- In-game help screen with objective, controls, pickups, and survival tips.
- Pause, stage-clear, and game-over states.
- Smooth 8-direction movement plus focused slow movement.
- Deliberately small visible red hitbox for fair bullet reading.
- Hold-to-fire weapons:
  - **Vulcan**: fast straight shot with upgraded spread.
  - **Plasma**: slower wide coverage.
  - **Missile**: secondary homing projectiles.
- Bomb button clears enemy bullets, damages enemies, grants brief invulnerability, and shakes the screen.
- Powerups: weapon change, weapon upgrade, bomb, and score medal.
- Scripted popcorn and turret waves on a 90-second route.
- Miniboss with aimed, radial, and mixed attack phases.
- Stage clear restarts the loop harder.
- Particle explosions, HUD, scrolling layered background, and F1 debug hitboxes.

## Controls

| Action | Keyboard | Gamepad | Mouse |
| --- | --- | --- | --- |
| Menu selection | Up/Down or W/S | D-Pad Up/Down | Hover cursor over option |
| Confirm selection | Enter or Space | Face Button Down (A / Cross) | Left-click option |
| Adjust volume sliders | Left/Right or A/D | D-Pad Left/Right | Drag slider handle |
| Back from submenus | Enter, Escape, or Backspace | Face Button Down / Right (A/B) | Left-click BACK button |
| Move | Arrow keys or WASD | Left Analog Stick or D-Pad | - |
| Focus / Slow movement | Left Shift | Shoulder Triggers (L1 / R1) | - |
| Shoot | Hold Z or Space | Face Button Down / Left (A / X) | - |
| Bomb | X | Face Button Right (B / Circle) | - |
| Pause / Resume | P | Menu/Start Button | Navigate pause menu choices |
| Toggle hitboxes | F1 (in gameplay) | Configurable in Settings | - |


## Build

This project uses CMake. It first tries `find_package(raylib)`. If raylib is not installed, CMake fetches raylib 5.5 from GitHub as the only third-party dependency.

### macOS / Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/sky_circuit
```

### Windows

From a Developer PowerShell or terminal with CMake and a C++ compiler installed:

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
.\\build\\Release\\sky_circuit.exe
```

If using MinGW or Ninja single-config generators, the executable may be at `build/sky_circuit.exe`.

## Project Structure

```text
.
├── CMakeLists.txt          # CMake build and raylib dependency resolution
├── main.cpp                # Entry point
├── Game.h/.cpp             # State machine, orchestration, collisions, rendering, sounds
├── Player.h/.cpp           # Movement, weapons, bombs, player rendering
├── Enemy.h/.cpp            # Popcorn, turret, and miniboss behavior
├── Bullet.h/.cpp           # Projectile movement, collision radius, homing missiles
├── Powerup.h/.cpp          # Collectible item behavior
├── WaveManager.h/.cpp      # Deterministic stage wave schedule
└── Effects.h/.cpp          # Particles and screen shake
```

## Design Notes

- The game uses simple vectors and circle collisions for clarity and fast iteration.
- Procedural vector art and tones are intentional: the game has no copyrighted sprites, logos, music, or level layouts.
- The gameplay loop is deterministic-feeling and compact: survive the route, defeat the carrier, clear the stage, then repeat at a higher difficulty.
- The red center dot is the actual player hitbox; the ship body may overlap bullets without counting as a hit.

## Verification

The current source has been verified with:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Honest Limitations

- There is no external art or music pipeline; presentation is intentionally procedural.
- The stage is a compact arcade loop rather than a multi-level campaign.
- High scores are saved persistently to `highscores.txt` in the working directory.
