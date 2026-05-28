# Sky Circuit

Sky Circuit is a complete local C++17/raylib vertical mobile-first arcade shooter. It keeps the theatre of 1990s cabinet shmups, but the primary play assumptions are handheld readability, thumb-driven steering, short sessions, compressed pacing, generous telegraphs, and stable performance.

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
When the game starts up, select **Start Run** to enter **Route Vantage**:
- **Touch / Mouse Navigation**: Tap or hover over **Start Run** and select it.
- **Keyboard Navigation**: Press **Up/Down** or **W/S** to move the cursor pointer `>` and press **Enter** or **Space** to confirm.
- **Gamepad Navigation**: Press **D-Pad Up/Down** and confirm with the **Down Face Button (A / Cross)**.

### 4. First Flight Controls
Once the mission starts, a helper banner appears near the bottom of the playfield:
- **Touch / Mouse Drag**: Drag below the ship to steer. The ship floats above the thumb/finger so it stays visible.
- **Auto-Fire**: Weapons fire continuously during play.
- **Bomb**: Two-finger tap on touch screens, right-click with mouse, or press the mapped bomb button.
- **Keyboard / Gamepad Compatibility**: Keyboard and controller inputs remain available for desktop play.
*Note: You can switch control presets, adjust volume sliders, or toggle fullscreen modes inside the **Tuning** menu at any time.*

## Features

- 480x640 vertical mobile playfield.
- Mobile-first title shell with **Start Run**, **Route Brief**, **Tuning**, **Scores**, and **Exit**.
- **Interactive Settings**: Sliders for SFX and Music volume, Screen Shake toggle, Screen Mode (Windowed/Fullscreen), and Hitbox display toggle.
- **Persistent High Scores Leaderboard**: Hall of Fame saving to local `highscores.txt` with Name Entry UI for player initials.
- **Procedural Audio & Chiptune BGM**: Fully synthesized retro chiptune loop, sound effects, explosions, and UI tones generated entirely in code.
- **Attract Mode (Gameplay Demo)**: Automatically starts an auto-play demonstration controlled by a dodging/shooting AI when left idle for 24 seconds.
- **Gamepad / Controller Support**: Full controller integration for gameplay and menus.
- In-game help screen with objective, controls, pickups, and survival tips.
- Pause, stage-clear, and game-over states.
- Touch/mouse drag steering with thumb offset, plus keyboard/gamepad compatibility.
- Auto-fire weapons tuned for movement-first handheld play.
- Deliberately small visible red hitbox for fair bullet reading.
- Weapons:
  - **Vulcan**: fast straight shot with upgraded spread.
  - **Plasma**: slower wide coverage.
  - **Missile**: secondary homing projectiles.
- Bomb button clears enemy bullets, damages enemies, grants brief invulnerability, and shakes the screen.
- Powerups: weapon change, weapon upgrade, bomb, and score medal.
- Authored mobile-first stage progression with named wave blocks, clear transition callouts, an earlier reward-heavy bonus formation, recovery windows, short pressure spikes, and an expanded boss runway.
- Miniboss with slower entrance, clearer weak point, wider safe lanes, and reduced projectile overlap.
- Stage clear restarts Route Vantage as a tighter score loop.
- Particle explosions, HUD, scrolling layered background, and F1 debug hitboxes.

## Controls

| Action | Keyboard | Gamepad | Mouse |
| --- | --- | --- | --- |
| Menu selection | Up/Down or W/S | D-Pad Up/Down | Hover cursor over option |
| Confirm selection | Enter or Space | Face Button Down (A / Cross) | Left-click option |
| Adjust volume sliders | Left/Right or A/D | D-Pad Left/Right | Drag slider handle |
| Back from submenus | Enter, Escape, or Backspace | Face Button Down / Right (A/B) | Left-click BACK button |
| Move | Arrow keys or WASD | Left Analog Stick or D-Pad | Drag below ship |
| Focus / Slow movement | Left Shift | Shoulder Triggers (L1 / R1) | - |
| Shoot | Auto-fire, Z, or Space | Auto-fire / Face Button Down or Left | Auto-fire |
| Bomb | X | Face Button Right (B / Circle) | Two-finger tap / right click |
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
├── StageDirector.h/.cpp    # Authored arcade stage, section, and wave block choreography
└── Effects.h/.cpp          # Particles and screen shake
```

## Design Notes

- The game uses simple vectors and circle collisions for clarity and fast iteration.
- Procedural vector art and tones are intentional: the game has no copyrighted sprites, logos, music, or level layouts.
- The gameplay loop is compact and mobile-first: quick orientation, early spectacle, short pressure bursts, a low-stress score parade, readable boss approach, boss combat, stage clear, then repeat as a tighter route loop.
- Stage pacing is directed through explicit wave blocks: opening orientation, sweep language, formation reassembly, aimed interceptors, false recovery pinch, breathing lane, debris drift, bonus formation, threat horizon, fortress corridor, fortress collapse, tactical silence, boss arrival, and boss combat.
- The red center dot is the actual player hitbox; the ship body may overlap bullets without counting as a hit.

## Identity Preservation Guardrails

Sky Circuit should stay a focused mobile arcade route, not a progression platform. Future changes should preserve:

- **Route identity**: Keep **Route Vantage**, **Gateline-01**, SOLACE, VANTAGE-9, the gate motif, and the bonus parade as recurring anchors. Add lore sparingly and only when it reinforces the ritual.
- **Mobile readability**: Prefer broad formation motion, strong silhouettes, clear safe lanes, short projectile bursts, and explicit phase color language. Do not increase density to create difficulty.
- **Replay flow**: Preserve free-play start, immediate retry, short menu depth, and fast emotional payoff. Avoid mandatory upgrades, reward-claim screens, nested menus, or unlock friction.
- **Bonus protection**: **BONUS PARADE** should remain low-stress, rhythmic, score-heavy, and celebratory. Do not add hidden threats or punishment mechanics to it.
- **Boss ceremony**: Bosses should remain introduced, readable, and theatrical. Telegraph posture before attacks and protect visible safe lanes.
- **Expansion philosophy**: If new content is added, prefer formation choreography, transition rituals, emotional palette shifts, and timing variation over new currencies, inventories, RPG stats, or enemy bloat.

## Verification

The current source has been verified with:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Honest Limitations

- There is no external art or music pipeline; presentation is intentionally procedural.
- The stage is a compact arcade route with named act progression rather than a multi-level campaign.
- High scores are saved persistently to `highscores.txt` in the working directory.
- There are no currencies, unlock trees, inventory systems, daily tasks, or live-service loops by design.
